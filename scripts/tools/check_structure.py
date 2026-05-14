#!/usr/bin/env python3
import argparse
import sys

from structure_checks.common import (
	all_files,
	c_files,
	changed_files,
	public_headers,
	repo_root,
	source_files,
)
from structure_checks.check_suites import check_validation_suites
from structure_checks.makefiles import check_makefile_source_lists
from structure_checks.public_headers import check_public_header_docs
from structure_checks.source_rules import (
	check_no_bare_negative_errno,
	check_no_relative_parent_includes,
	check_scheduler_state_writes,
	check_selftests_are_centralized,
)


def parse_args() -> argparse.Namespace:
	parser = argparse.ArgumentParser(description="Check Tianole structure rules")
	mode = parser.add_mutually_exclusive_group()
	mode.add_argument("--all", action="store_true", help="check the whole tree")
	mode.add_argument(
		"--changed", action="store_true", help="check changed and untracked files"
	)
	return parser.parse_args()


def main() -> int:
	args = parse_args()
	root = repo_root()
	all_mode = not args.changed
	files = all_files(root) if all_mode else changed_files(root)

	errors = []
	sources = source_files(files)
	errors.extend(check_no_relative_parent_includes(root, sources))
	errors.extend(check_no_bare_negative_errno(root, sources))
	errors.extend(check_scheduler_state_writes(root, sources))
	errors.extend(check_public_header_docs(root, public_headers(files)))
	errors.extend(check_selftests_are_centralized(c_files(files)))
	errors.extend(check_makefile_source_lists(root, files, all_mode))
	errors.extend(check_validation_suites(root))

	if errors:
		for error in errors:
			print(error, file=sys.stderr)
		return 1

	return 0


if __name__ == "__main__":
	raise SystemExit(main())
