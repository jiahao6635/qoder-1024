#!/usr/bin/env python3
"""Check installed M5GFX font glyphs and this UI's text widths without a device."""

import argparse
import ast
from pathlib import Path
import re


class Font:
    def __init__(self, source, size):
        name = f"lgfx_efont_cn_{size}"
        match = re.search(rf'const uint8_t {name}\[(\d+)\]\s*=\s*((?:"(?:[^"\\]|\\.)*"\s*)+);', source)
        if not match:
            raise ValueError(f"Font array not found: {name}")
        strings = re.findall(r'"(?:[^"\\]|\\.)*"', match[2])
        self.data = b"".join(ast.literal_eval("b" + value) for value in strings) + b"\0"
        assert len(self.data) == int(match[1]), f"Incorrect array decoding: {name}"

    def word(self, offset):
        return int.from_bytes(self.data[offset:offset + 2], "big")

    def glyph(self, code):
        # Same index traversal as M5GFX U8g2font::getGlyph.
        offset = 23
        if code <= 255:
            if code >= ord("a"):
                offset += self.word(19)
            elif code >= ord("A"):
                offset += self.word(17)
            while self.data[offset + 1]:
                if self.data[offset] == code:
                    return offset + 2
                offset += self.data[offset + 1]
        else:
            offset += self.word(21)
            lookup = offset
            while True:
                offset += self.word(lookup)
                end = self.word(lookup + 2)
                lookup += 4
                if end >= code:
                    break
            while self.word(offset):
                if self.word(offset) == code:
                    return offset + 3
                step = self.data[offset + 2]
                assert step > 0, "Invalid font glyph size"
                offset += step
        return None

    def metrics(self, char):
        pointer = self.glyph(ord(char))
        assert pointer is not None, f"Missing glyph U+{ord(char):04X} {char!r}"
        bit = pointer * 8

        def read(count, signed=False):
            nonlocal bit
            result = sum(((self.data[(bit + i) // 8] >> ((bit + i) % 8)) & 1) << i
                         for i in range(count))
            bit += count
            return result - (1 << (count - 1)) if signed else result

        width = read(self.data[4])
        read(self.data[5])
        x_offset = read(self.data[6], True)
        read(self.data[7], True)
        advance = read(self.data[8], True)
        return width, x_offset, advance

    def width(self, text, scale=1):
        # Same metrics calculation as LGFXBase::text_width, at integer scale.
        left = right = 0
        for char in text:
            width, offset, advance = self.metrics(char)
            if left == 0 and right == 0 and offset < 0:
                left = right = -offset * scale
            right = left + max(advance, width + offset) * scale
            left += advance * scale
        return right


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library", type=Path,
                        default=Path.home() / "Documents/Arduino/libraries/M5GFX")
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[1]
    ui = (project / "worker_powerbank/Ui.h").read_text()
    font_path = args.library / "src/lgfx/Fonts/efont/lgfx_efont_cn.c"
    source = font_path.read_text()
    fonts = {size: Font(source, size) for size in (12, 16)}
    labels = []
    for line in ui.splitlines():
        for literal in re.findall(r'"(?:[^"\\]|\\.)*"', line):
            label = ast.literal_eval(literal)
            if any(ord(char) > 127 for char in label):
                labels.append((label, 12 if "hint =" in line else 16))
    chars = set("".join(label for label, _ in labels) + "0123456789:/ ABIMU")
    print(f"Font source: {font_path}")
    for size, font in fonts.items():
        missing = [char for char in sorted(chars) if font.glyph(ord(char)) is None]
        assert not missing, f"efontCN_{size} missing glyphs: {missing}"
        print(f"efontCN_{size}: {len(chars)}/{len(chars)} UI glyphs present; "
              f"{len(font.data)} font bytes decoded")

    for label, size in labels:
        width = fonts[size].width(label)
        assert width <= 240, f"Text exceeds screen: {label!r}: {width}px"
        print(f"efontCN_{size}: {width:3d}px / 240px  {label}")

    # Fixed-position regions from Ui.h, including maximum runtime values.
    fixed = [("title", "打工人充电宝", 16, 1, 6, 166),
             ("mode", "演示模式", 16, 1, 171, 240),
             ("energy", "100", 16, 2, 144, 198),
             ("denominator", "/100", 16, 1, 198, 240),
             ("timer", "25:00", 12, 1, 199, 240)]
    for name, label, size, scale, x, boundary in fixed:
        right = x + fonts[size].width(label, scale)
        assert right <= boundary, f"{name} overflows: {right} > {boundary}"
        print(f"{name}: right edge {right}px <= {boundary}px")
    print("PASS: all UI Chinese/ASCII glyphs exist, all text widths fit their checked regions.")
    print("Static font/layout check only; physical display appearance remains untested.")


if __name__ == "__main__":
    main()
