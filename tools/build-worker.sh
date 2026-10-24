#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
result_dir="$project_dir/output/worker_powerbank"
mkdir -p "$result_dir/firmware"
bash "$project_dir/tools/test-worker.sh" 2>&1 | tee "$result_dir/tests.log"
python3 "$project_dir/tests/check_fonts.py" 2>&1 | tee "$result_dir/fonts.log"
arduino-cli compile --warnings more --jobs 4 \
  --build-path "$result_dir/build" --output-dir "$result_dir/firmware" \
  "$project_dir/worker_powerbank" 2>&1 | tee "$result_dir/compile.log"
arduino-cli version > "$result_dir/environment.txt"
arduino-cli core list >> "$result_dir/environment.txt"
arduino-cli lib list >> "$result_dir/environment.txt"
arduino-cli board list --json > "$result_dir/ports.json"
python3 - "$project_dir" <<'PY'
import hashlib
import json
import pathlib
import sys
from datetime import datetime
from zoneinfo import ZoneInfo

root = pathlib.Path(sys.argv[1])
out = root / 'output/worker_powerbank'
files = sorted((root / 'worker_powerbank').glob('*')) + sorted((out / 'firmware').glob('*.bin'))
hashes = {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
          for p in files if p.is_file()}
report = {
    'verified_at': datetime.now(ZoneInfo('Asia/Shanghai')).isoformat(),
    'compile_passed': True,
    'host_logic_tests_passed': True,
    'font_coverage_and_width_passed': True,
    'fqbn': (root / 'worker_powerbank/sketch.yaml').read_text().strip().split(': ', 1)[1],
    'hardware_uploaded_or_tested_by_this_build': False,
    'sha256': hashes,
}
(out / 'verification.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n')
print('编译与电脑端逻辑测试通过，固件已保存在 output/worker_powerbank/firmware。')
PY
