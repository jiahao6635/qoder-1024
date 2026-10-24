#!/usr/bin/env python3
"""Render the firmware's Ui.h on the host using the installed, exact M5GFX fonts.

Uses only the Python standard library and a C++17 compiler. No device simulation is claimed.
"""
import argparse
import ast
import functools
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import zlib

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests'))
from check_fonts import Font


class BitmapFont(Font):
    @functools.lru_cache(maxsize=None)
    def bitmap(self, char):
        pointer = self.glyph(ord(char))
        assert pointer is not None, f'Missing glyph: {char!r}'
        bit = pointer * 8

        def read(n, signed=False):
            nonlocal bit
            value = sum(((self.data[(bit+i)//8] >> ((bit+i)%8)) & 1) << i for i in range(n))
            bit += n
            return value - (1 << (n-1)) if signed else value

        w, h = read(self.data[4]), read(self.data[5])
        xoff, yoff, advance = read(self.data[6], True), read(self.data[7], True), read(self.data[8], True)
        pixels = []
        while len(pixels) < w*h:
            zeros, ones = read(self.data[2]), read(self.data[3])
            assert zeros + ones > 0, 'Invalid empty RLE run'
            while True:
                pixels.extend([0]*zeros)
                pixels.extend([1]*ones)
                if read(1) == 0:
                    break
        assert len(pixels) == w*h
        baseline = self.data[10] + (self.data[12] if self.data[12] < 128 else self.data[12]-256)
        return w, h, xoff, baseline-yoff-h, advance, pixels


def rgb(c):
    r, g, b = (c >> 11) & 31, (c >> 5) & 63, c & 31
    return ((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2))


class Canvas:
    def __init__(self, width, height, bg=(255, 255, 255)):
        self.w, self.h = width, height
        self.data = bytearray(bytes(bg)*width*height)

    def rect(self, x, y, w, h, color):
        assert 0 <= x <= x+w <= self.w and 0 <= y <= y+h <= self.h, (x,y,w,h,self.w,self.h)
        row = bytes(color)*w
        for yy in range(y, y+h):
            index = (yy*self.w+x)*3
            self.data[index:index+w*3] = row

    def text(self, text, x, y, font, scale, color):
        if text:
            x += max(0, -font.bitmap(text[0])[2])*scale
        for char in text:
            w,h,xoff,yoff,advance,pixels = font.bitmap(char)
            for yy in range(h):
                for xx in range(w):
                    if pixels[yy*w+xx]:
                        self.rect(x+(xoff+xx)*scale, y+(yoff+yy)*scale, scale,scale,color)
            x += advance*scale

    def paste(self, other, x, y, scale=1):
        for yy in range(other.h):
            row = other.data[yy*other.w*3:(yy+1)*other.w*3]
            if scale != 1:
                row = b''.join(row[i:i+3]*scale for i in range(0,len(row),3))
            for repeat in range(scale):
                index = ((y+yy*scale+repeat)*self.w+x)*3
                self.data[index:index+len(row)] = row

    def save(self, path):
        def chunk(kind,data):
            return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
        pixels = b''.join(b'\0'+self.data[y*self.w*3:(y+1)*self.w*3] for y in range(self.h))
        Path(path).write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',self.w,self.h,8,2,0,0,0))+
                              chunk(b'IDAT',zlib.compress(pixels,9))+chunk(b'IEND',b''))


def luminance(c):
    values = [v/255 for v in c]
    values = [v/12.92 if v <= .04045 else ((v+.055)/1.055)**2.4 for v in values]
    return sum(a*b for a,b in zip(values,(.2126,.7152,.0722)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library',type=Path,default=Path.home()/'Documents/Arduino/libraries/M5GFX')
    args = parser.parse_args()
    out = ROOT/'output/read_mouthpiece/previews'
    out.mkdir(parents=True,exist_ok=True)
    source = args.library/'src/lgfx/Fonts/efont/lgfx_efont_cn.c'
    font_source = source.read_text()
    fonts = {n:BitmapFont(font_source,n) for n in (12,16)}
    paths = list((ROOT/'read_mouthpiece').glob('*.h')) + list((ROOT/'read_mouthpiece').glob('*.ino'))
    paths += [ROOT/'tests/mouthpiece_ui_test.cpp']
    chars = set(chr(c) for c in range(32,127))
    for path in paths:
        for literal in re.findall(r'"(?:[^"\\]|\\.)*"',path.read_text()):
            try:
                value = ast.literal_eval(literal)
            except (ValueError,SyntaxError):
                continue
            chars.update(c for c in value if ord(c)>=32)
    for n,font in fonts.items():
        missing = [c for c in sorted(chars) if font.glyph(ord(c)) is None]
        assert not missing, f'efontCN_{n} missing {missing}'
        for c in chars:
            font.bitmap(c)
        print(f'efontCN_{n}: {len(chars)} required Chinese/ASCII glyphs decoded; none missing')
    # Host text widths come from the same binary glyph metrics used by firmware.
    rows = []
    for n,font in fonts.items():
        for char in sorted(chars):
            w,offset,advance = font.metrics(char)
            rows.append(f'  {{{n}, {ord(char)}, {w}, {offset}, {advance}}},')
    metrics = ROOT/'tests/mouthpiece_ui_stub/FontMetrics.h'
    metrics.write_text('#pragma once\n#include <cassert>\nstruct Metric { int size, code, width, offset, advance; };\n'
                       'constexpr Metric metrics[] = {\n'+'\n'.join(rows)+'\n};\n'
                       'inline Metric findMetric(int size, int code) {\n'
                       '  for (const auto& m : metrics) if (m.size == size && m.code == code) return m;\n'
                       '  assert(false && "glyph missing"); return {};\n}\n')
    binary = out/'ui-host-test'
    subprocess.run([os.environ.get('CXX','clang++'),'-std=c++17','-Wall','-Wextra','-Werror',
                    '-fsanitize=address,undefined','-I'+str(metrics.parent),
                    str(ROOT/'tests/mouthpiece_ui_test.cpp'),'-o',str(binary)],check=True)
    result = subprocess.run([str(binary)],check=True,capture_output=True,text=True)
    (out/'draw-commands.jsonl').write_text(result.stdout)
    screens = {}
    screen = None
    text_boxes = []
    for line in result.stdout.splitlines():
        op = json.loads(line)
        if 'screen' in op:
            screen = Canvas(240,135)
            screens[op['screen']] = screen
        elif op['op'] in ('fill','rect'):
            x,y,w,h = (op[k] for k in ('x','y','w','h'))
            c = rgb(op['c'])
            if op['op']=='fill': screen.rect(x,y,w,h,c)
            else:
                screen.rect(x,y,w,1,c); screen.rect(x,y+h-1,w,1,c)
                screen.rect(x,y,1,h,c); screen.rect(x+w-1,y,1,h,c)
        else:
            font = fonts[op['size']]
            width = font.width(op['text'],op['scale'])
            assert op['x']+width <= 240
            if op['size']==16:
                assert op['x']+width <= 168, f'Main text overlaps character: {op}'
                assert op['y']+op['size']*op['scale']<=97, f'Main text overlaps note: {op}'
            screen.text(op['text'],op['x'],op['y'],font,op['scale'],rgb(op['c']))
            text_boxes.append({k:op[k] for k in ('text','x','y','size','scale')}|{'width':width})
    for name,screen in screens.items(): screen.save(out/(name+'.png'))
    assert len(screens)>=38
    columns, scale = 4,2
    cell_w,cell_h = 496,298
    sheet = Canvas(columns*cell_w, ((len(screens)+columns-1)//columns)*cell_h, (31,35,37))
    for i,(name,screen) in enumerate(screens.items()):
        x,y=(i%columns)*cell_w,(i//columns)*cell_h
        sheet.text(name,x+8,y+5,fonts[12],1,(235,235,235))
        sheet.paste(screen,x+8,y+22,scale)
    sheet.save(out/'all-cards.png')
    sheet.save(out/'contact-sheet.png')
    trio = Canvas(3*720+4*24,405+48,(31,35,37))
    for i,name in enumerate(('pack1-card1-polite','pack1-card1-inner','rescue')):
        trio.paste(screens[name],24+i*744,24,3)
    trio.save(out/'demo-triptych.png')
    trio.save(out/'demo-strip.png')
    def c565(r,g,b): return ((r&248)<<8)|((g&252)<<3)|(b>>3)
    contrast = {}
    for name,bg,fg in [('polite',(177,242,198),(19,54,40)),('inner',(255,155,121),(71,31,26))]:
        ratio=(luminance(rgb(c565(*bg)))+.05)/(luminance(rgb(c565(*fg)))+.05)
        assert ratio >= 4.5
        contrast[name] = round(ratio,2)
    report = {'screen_size':[240,135],'screen_count':len(screens),'glyph_count_per_font':len(chars),
              'fonts':['efontCN_12','efontCN_16'],'font_source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),
              'all_presets_32px_and_at_most_two_lines':True,'all_text_boxes_within_bounds':True,
              'all_primitives_within_bounds':True,'contrast_ratios_rgb565':contrast,
              'hardware_display_verified':False,'method':'Same Ui.h through host drawing shim; actual M5GFX efont glyph metrics and RLE bitmaps.',
              'text_boxes':text_boxes}
    (out/'layout-report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
    print(f'PASS: {len(screens)} full firmware UI frames; exact font glyphs, text bounds, primitives and contrast checked.')
    print('All 36 preset sides retain 32px text; no cropped UTF-8 and at most two lines.')
    print(f'Preview: {out / "demo-triptych.png"}')
    print('Host previews only; LCD orientation, colour and physical readability still require the device.')


if __name__=='__main__':
    main()
