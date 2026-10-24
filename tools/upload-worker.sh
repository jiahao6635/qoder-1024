#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
arduino-cli board list
if [[ $# != 1 ]]; then
  echo '先连接支持数据传输的 USB 线，再运行：'
  echo 'bash tools/upload-worker.sh /dev/cu.usbmodem实际端口'
  exit 2
fi
case "$1" in
  /dev/cu.usbmodem*|/dev/cu.usbserial*|/dev/ttyACM*|/dev/ttyUSB*|COM[0-9]*) ;;
  *) echo '请指定开发板的 USB 串口，不要使用蓝牙或 debug-console 端口。'; exit 2 ;;
esac
if [[ "$1" == /dev/* && ! -c "$1" ]]; then
  echo '指定端口不存在，请重新运行 arduino-cli board list。'
  exit 2
fi
# Rebuild before uploading so the retained binary always matches current sources.
bash "$project_dir/tools/build-worker.sh"
arduino-cli upload --port "$1" \
  --input-dir "$project_dir/output/worker_powerbank/firmware" \
  "$project_dir/worker_powerbank" 2>&1 | tee "$project_dir/output/worker_powerbank/upload.log"
