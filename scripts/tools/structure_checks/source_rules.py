import re
from pathlib import Path

from .common import read_text


def check_no_relative_parent_includes(root: Path, files: list[Path]) -> list[str]:
	errors = []
	include_re = re.compile(r'^\s*#include\s+[<"]\.\./')

	for path in files:
		for line_no, line in enumerate(read_text(root, path).splitlines(), 1):
			if include_re.search(line):
				errors.append(
					f"{path}:{line_no}: do not use ../ includes; "
					"promote shared headers or add a private include root"
				)

	return errors


def check_no_bare_negative_errno(root: Path, files: list[Path]) -> list[str]:
	errors = []
	return_re = re.compile(r"\breturn\s+-[0-9]+\s*;")

	for path in files:
		for line_no, line in enumerate(read_text(root, path).splitlines(), 1):
			if return_re.search(line):
				errors.append(
					f"{path}:{line_no}: return a symbolic negative errno "
					"such as -EINVAL instead of a bare negative number"
				)

	return errors


def check_scheduler_state_writes(root: Path, files: list[Path]) -> list[str]:
	errors = []
	state_write_re = re.compile(r"->state\s*=")
	allowed_path = Path("kernel/sched/sched.h")

	for path in files:
		if path == allowed_path:
			continue

		for line_no, line in enumerate(read_text(root, path).splitlines(), 1):
			if state_write_re.search(line):
				errors.append(
					f"{path}:{line_no}: thread state writes must use "
					"kernel/sched/sched.h helpers"
				)

	return errors


def check_selftests_are_centralized(files: list[Path]) -> list[str]:
	errors = []

	for path in files:
		if "selftest" not in path.name:
			continue

		if len(path.parts) < 2 or path.parts[:2] != ("kernel", "selftest"):
			errors.append(f"{path}: kernel selftests belong under kernel/selftest/")

	return errors
