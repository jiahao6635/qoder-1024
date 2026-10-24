#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
# Enumerate afresh, never reuse a port saved by an earlier project.
arduino-cli board list
if [[ $# != 1 ]]; then
  echo '连接数据线后，请选择本次检测到的目标端口：'
  echo 'bash tools/upload-mouthpiece.sh /dev/cu.usbmodem实际端口'
  exit 2
fi
case "$1" in
  /dev/cu.usbmodem*|/dev/cu.usbserial*|/dev/ttyACM*|/dev/ttyUSB*|COM[0-9]*) ;;
  *) echo '请使用开发板 USB 串口，不能使用蓝牙或 debug-console 端口。'; exit 2 ;;
esac
if [[ "$1" == /dev/* && ! -c "$1" ]]; then
  echo '端口不存在，请重新检测。'
  exit 2
fi
bash "$project_dir/tools/build-mouthpiece.sh"
arduino-cli upload --port "$1" \
  --input-dir "$project_dir/output/read_mouthpiece/firmware" \
  "$project_dir/read_mouthpiece" 2>&1 | tee "$project_dir/output/read_mouthpiece/upload.log"
echo '上传命令完成。屏幕、按钮和摇动仍需按 README 的实机清单逐项检查。'
