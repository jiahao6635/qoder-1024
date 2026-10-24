#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
result_dir="$project_dir/output/read_mouthpiece"
mkdir -p "$result_dir/firmware"
# Remove an old success marker first; a failed rebuild must not look verified.
rm -f "$result_dir/verification.json"
bash "$project_dir/tools/test-mouthpiece.sh" 2>&1 | tee "$result_dir/tests.log"
python3 "$project_dir/tools/preview-mouthpiece.py" 2>&1 | tee "$result_dir/layout.log"
arduino-cli compile --warnings more --jobs 4 \
  --build-path "$result_dir/build" --output-dir "$result_dir/firmware" \
  "$project_dir/read_mouthpiece" 2>&1 | tee "$result_dir/compile.log"
arduino-cli version > "$result_dir/environment.txt"
arduino-cli core list >> "$result_dir/environment.txt"
arduino-cli lib list >> "$result_dir/environment.txt"
arduino-cli board list --json > "$result_dir/ports.json"
python3 - "$project_dir" <<'PY'
import hashlib
import json
from pathlib import Path
import sys
from datetime import datetime
from zoneinfo import ZoneInfo

root = Path(sys.argv[1])
out = root / 'output/read_mouthpiece'
files = sorted((root / 'read_mouthpiece').glob('*')) + sorted((out / 'firmware').glob('*.bin'))
report = {
    'verified_at': datetime.now(ZoneInfo('Asia/Shanghai')).isoformat(),
    'compile_passed': True,
    'host_logic_tests_passed': True,
    'font_coverage_and_layout_passed': True,
    'fqbn': (root / 'read_mouthpiece/sketch.yaml').read_text().strip().split(': ', 1)[1],
    'uploaded': False,
    'physical_display_buttons_imu_battery_tested': False,
    'sha256': {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
               for p in files if p.is_file()},
}
(out / 'verification.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n')
print('已读，嘴替。：编译和离线检查通过；未烧录、未实机验证。')
PY
