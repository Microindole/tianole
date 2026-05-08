#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/../.."

. ./scripts/lib/check-common.sh

build_kernel KERNEL_TEST_TRAP=1
run_qemu KERNEL_TEST_TRAP=1

check_lines build/debug.log \
	"traps initialized" \
	"exception: invalid opcode" \
	"vector=6 error=0x0000000000000000" \
	"rip=0x" \
	"rsp=0x" \
	"panic: unhandled CPU exception"

cat build/debug.log
