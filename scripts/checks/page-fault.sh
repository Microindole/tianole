#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/../.."

. ./scripts/lib/check-common.sh

build_kernel KERNEL_TEST_PAGE_FAULT=1
run_qemu KERNEL_TEST_PAGE_FAULT=1

check_lines build/debug.log \
	"traps initialized" \
	"kernel page table root active" \
	"page table selftest ok" \
	"exception: page fault" \
	"vector=14 error=0x0000000000000002" \
	"page fault: address=0xffffff1000000000 error=0x0000000000000002" \
	"access=write mode=kernel reason=not-present" \
	"panic: unhandled CPU exception"

cat build/debug.log
