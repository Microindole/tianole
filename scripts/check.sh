#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

sources=$(find arch include kernel -name '*.c' -o -name '*.h')

clang-format --dry-run --Werror $sources

make clean
make

timeout 30s make run-headless || true

required_lines=(
  "Tianole x86 bootloader loaded."
  "jumping to kernel entry"
  "kernel_main entered"
  "boot_info.version ok"
  "boot services exited"
  "memory map descriptors="
  "conventional memory pages="
)

for line in "${required_lines[@]}"; do
	if ! grep -F "$line" build/debug.log >/dev/null; then
    echo "missing expected boot log line: $line" >&2
    echo "--- build/debug.log ---" >&2
    cat build/debug.log >&2 || true
    exit 1
	fi
done

serial_required_lines=(
  "kernel_main entered"
  "boot_info.version ok"
  "boot services exited"
  "memory map descriptors="
  "conventional memory pages="
)

for line in "${serial_required_lines[@]}"; do
  if ! grep -F "$line" build/serial.log >/dev/null; then
    echo "missing expected serial log line: $line" >&2
    echo "--- build/serial.log ---" >&2
    cat build/serial.log >&2 || true
    exit 1
  fi
done

cat build/debug.log
