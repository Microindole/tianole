#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/../.."

. ./scripts/lib/check-common.sh

build_kernel
run_qemu

check_lines build/debug.log \
	"Tianole x86 bootloader loaded." \
	"jumping to kernel entry" \
	"kernel_main entered" \
	"traps initialized" \
	"boot_info.version ok" \
	"boot services exited" \
	"memory map descriptors=" \
	"conventional memory pages=" \
	"physical pages free=" \
	"physical page allocator selftest ok" \
	"kernel page table root active" \
	"page table selftest ok" \
	"kernel heap initialized" \
	"kernel heap selftest ok" \
	"scheduler initialized" \
	"kernel thread selftest ok" \
	"timer initialized" \
	"timer tick=1" \
	"timer tick=2" \
	"timer tick=3"

check_lines build/serial.log \
	"kernel_main entered" \
	"traps initialized" \
	"boot_info.version ok" \
	"boot services exited" \
	"memory map descriptors=" \
	"conventional memory pages=" \
	"physical pages free=" \
	"physical page allocator selftest ok" \
	"kernel page table root active" \
	"page table selftest ok" \
	"kernel heap initialized" \
	"kernel heap selftest ok" \
	"scheduler initialized" \
	"kernel thread selftest ok" \
	"timer initialized" \
	"timer tick=1" \
	"timer tick=2" \
	"timer tick=3"

cat build/debug.log
