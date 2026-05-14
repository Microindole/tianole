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

check_expectations()
{
	local log_file=$1
	local expectation_file=$2
	local line

	while IFS= read -r line || [ -n "$line" ]; do
		if [ -z "$line" ] || [ "${line#\#}" != "$line" ]; then
			continue
		fi

		check_lines "$log_file" "$line"
	done < "$expectation_file"
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
