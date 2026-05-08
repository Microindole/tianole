#!/usr/bin/env bash

check_lines()
{
	local log_file=$1
	shift

	for line in "$@"; do
		if ! grep -F "$line" "$log_file" >/dev/null; then
			echo "missing expected log line: $line" >&2
			echo "--- $log_file ---" >&2
			cat "$log_file" >&2 || true
			exit 1
		fi
	done
}

build_kernel()
{
	make clean
	make "$@"
}

run_qemu()
{
	timeout 30s make run-headless "$@" || true
}
