#!/usr/bin/env bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
result_dir="$project_dir/output/worker_powerbank/tests"
mkdir -p "$result_dir"
for name in game shake; do
  "${CXX:-clang++}" -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
    "$project_dir/tests/${name}_test.cpp" -o "$result_dir/${name}_test"
  "$result_dir/${name}_test"
done
