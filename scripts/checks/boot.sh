#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/../.."

. ./scripts/lib/check-common.sh

build_kernel
run_qemu

check_expectations build/debug.log scripts/checks/expectations/boot-debug.txt
check_expectations build/serial.log scripts/checks/expectations/boot-serial.txt

cat build/debug.log
