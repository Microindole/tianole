#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/../.."

. ./scripts/lib/check-common.sh

build_kernel KERNEL_TEST_TRAP=1
run_qemu KERNEL_TEST_TRAP=1

check_expectations build/debug.log scripts/checks/expectations/invalid-opcode.txt

cat build/debug.log
