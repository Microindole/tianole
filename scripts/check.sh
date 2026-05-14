#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

SUITES_FILE=./scripts/checks/suites.list

list_check_suites()
{
	while read -r suite script; do
		if [ -z "${suite:-}" ] || [ "${suite#\#}" != "$suite" ]; then
			continue
		fi

		printf '%s\n' "$suite"
	done < "$SUITES_FILE"
}

check_suite_script()
{
	local requested=$1
	local suite
	local script

	while read -r suite script; do
		if [ -z "${suite:-}" ] || [ "${suite#\#}" != "$suite" ]; then
			continue
		fi

		if [ "$suite" = "$requested" ]; then
			printf '%s\n' "$script"
			return 0
		fi
	done < "$SUITES_FILE"

	return 1
}

if [ "${1:-}" = "--list" ]; then
	list_check_suites
	exit 0
fi

if [ "$#" -eq 0 ]; then
	set -- $(list_check_suites)
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
