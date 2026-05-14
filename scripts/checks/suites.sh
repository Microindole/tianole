#!/usr/bin/env bash

TIANOLE_CHECK_SUITES=(
	format
	structure
	boot
	invalid-opcode
	double-fault
	general-protection
	page-fault
	default-build
)

check_suite_script()
{
	case "$1" in
	format)
		printf '%s\n' "./scripts/checks/format.sh"
		;;
	structure)
		printf '%s\n' "./scripts/checks/structure.sh"
		;;
	boot)
		printf '%s\n' "./scripts/checks/boot.sh"
		;;
	invalid-opcode)
		printf '%s\n' "./scripts/checks/trap.sh"
		;;
	double-fault)
		printf '%s\n' "./scripts/checks/double-fault.sh"
		;;
	general-protection)
		printf '%s\n' "./scripts/checks/general-protection.sh"
		;;
	page-fault)
		printf '%s\n' "./scripts/checks/page-fault.sh"
		;;
	default-build)
		printf '%s\n' "./scripts/checks/default-build.sh"
		;;
	*)
		return 1
		;;
	esac
}

list_check_suites()
{
	printf '%s\n' "${TIANOLE_CHECK_SUITES[@]}"
}
