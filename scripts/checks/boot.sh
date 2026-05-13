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
	"workqueue initialized" \
	"input initialized" \
	"ps2 keyboard initialized" \
	"input console initialized" \
	"kdb initialized" \
	"workqueue selftest ok" \
	"timer initialized" \
	"scheduler starting" \
	"preempt thread 1 step=1" \
	"preempt thread 2 step=1" \
	"waiter sleeping" \
	"waker sleeping" \
	"condition waiter sleeping" \
	"condition waker sleeping" \
	"timeout waiter sleeping" \
	"return exit thread returning" \
	"explicit exit thread exiting" \
	"thread reaped worker-a" \
	"thread reaped worker-b" \
	"thread reaped return-exit" \
	"thread reaped explicit-exit" \
	"preempt thread 1 step=2" \
	"preempt thread 2 step=2" \
	"timeout waiter timed out" \
	"preempt thread 1 step=3" \
	"preempt thread 2 step=3" \
	"waker wake_one" \
	"waiter woke" \
	"condition waker wake_all" \
	"condition waiter woke" \
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
	"workqueue initialized" \
	"input initialized" \
	"ps2 keyboard initialized" \
	"input console initialized" \
	"kdb initialized" \
	"workqueue selftest ok" \
	"timer initialized" \
	"scheduler starting" \
	"preempt thread 1 step=1" \
	"preempt thread 2 step=1" \
	"waiter sleeping" \
	"waker sleeping" \
	"condition waiter sleeping" \
	"condition waker sleeping" \
	"timeout waiter sleeping" \
	"return exit thread returning" \
	"explicit exit thread exiting" \
	"thread reaped worker-a" \
	"thread reaped worker-b" \
	"thread reaped return-exit" \
	"thread reaped explicit-exit" \
	"preempt thread 1 step=2" \
	"preempt thread 2 step=2" \
	"timeout waiter timed out" \
	"preempt thread 1 step=3" \
	"preempt thread 2 step=3" \
	"waker wake_one" \
	"waiter woke" \
	"condition waker wake_all" \
	"condition waiter woke" \
	"timer tick=1" \
	"timer tick=2" \
	"timer tick=3"

cat build/debug.log
