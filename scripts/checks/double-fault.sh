#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/../.."

. ./scripts/lib/check-common.sh

build_kernel KERNEL_TEST_DOUBLE_FAULT=1
run_qemu KERNEL_TEST_DOUBLE_FAULT=1

check_lines build/debug.log \
	"traps initialized" \
	"exception: double fault" \
	"vector=8 error=0x0000000000000000" \
	"double fault: fatal exception" \
	"double fault: ist=1" \
	"panic: double fault"

cat build/debug.log
