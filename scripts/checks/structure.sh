#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/../.."

fail()
{
	echo "structure check failed: $*" >&2
	exit 1
}

check_no_relative_parent_includes()
{
	local matches

	matches=$(find arch include kernel mm -name '*.c' -o -name '*.h' |
		xargs grep -nE '^[[:space:]]*#include[[:space:]]+[<"]\.\./' || true)

	if [ -n "$matches" ]; then
		echo "$matches" >&2
		fail "do not use ../ includes; promote shared headers or add a private include root"
	fi
}

check_selftests_are_centralized()
{
	local matches

	matches=$(find arch kernel mm -name '*selftest*.c' |
		grep -v '^kernel/selftest/' || true)

	if [ -n "$matches" ]; then
		echo "$matches" >&2
		fail "kernel selftests belong under kernel/selftest/"
	fi
}

check_no_relative_parent_includes
check_selftests_are_centralized
