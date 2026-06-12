#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/../../.." && pwd)"
build_dir="$repo_root/.tmp/protocol-tests"
binary="$build_dir/test_yrm100_driver"
sdk_path="$(xcrun --show-sdk-path)"

mkdir -p "$build_dir"

"$(xcrun --find clang++)" -std=c++17 -Wall -Wextra -Werror \
  -isysroot "$sdk_path" \
  -isystem "$sdk_path/usr/include/c++/v1" \
  "$repo_root/tools/protocol/tests/test_yrm100_driver.cpp" \
  "$repo_root/firmware/esp32/yrm100_driver/Yrm100Driver.cpp" \
  -o "$binary"

"$binary"
