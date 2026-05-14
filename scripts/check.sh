#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

. ./scripts/checks/suites.sh

if [ "${1:-}" = "--list" ]; then
	list_check_suites
	exit 0
fi

if [ "$#" -eq 0 ]; then
	set -- "${TIANOLE_CHECK_SUITES[@]}"
fi

for suite in "$@"; do
	script=$(check_suite_script "$suite") || {
		echo "unknown check suite: $suite" >&2
		echo "available suites:" >&2
		list_check_suites >&2
		exit 1
	}

	echo "==> $suite"
	"$script"
done
