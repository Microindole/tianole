#!/usr/bin/env python3
import argparse
import re
import subprocess
import sys
from pathlib import Path


SOURCE_ROOTS = ("arch", "include", "kernel", "mm")
C_SOURCE_ROOTS = ("arch", "kernel", "mm")
MAKEFILE_SOURCE_VARS = (
	"ARCH_KERNEL_SRCS",
	"ARCH_MM_SRCS",
	"BOOT_SRCS",
	"KERNEL_SRCS",
	"MM_SRCS",
)


def repo_root() -> Path:
	return Path(__file__).resolve().parents[2]


def run_git(root: Path, args: list[str]) -> list[str]:
	result = subprocess.run(
		["git", *args],
		cwd=root,
		check=True,
		text=True,
		stdout=subprocess.PIPE,
	)
	return [line for line in result.stdout.splitlines() if line]


def changed_files(root: Path) -> set[Path]:
	files = set()

	for path in run_git(root, ["diff", "--name-only", "--diff-filter=ACMR"]):
		files.add(Path(path))

	for path in run_git(
		root, ["diff", "--cached", "--name-only", "--diff-filter=ACMR"]
	):
		files.add(Path(path))

	for path in run_git(root, ["ls-files", "--others", "--exclude-standard"]):
		files.add(Path(path))

	return files


def all_files(root: Path) -> set[Path]:
	files = set()

	for base in SOURCE_ROOTS:
		root_dir = root / base
		if not root_dir.exists():
			continue

		for path in root_dir.rglob("*"):
			if path.is_file():
				files.add(path.relative_to(root))

	for path in (root / "scripts").rglob("*"):
		if path.is_file():
			files.add(path.relative_to(root))

	return files


def source_files(files: set[Path]) -> list[Path]:
	return sorted(path for path in files if path.suffix in (".c", ".h"))


def public_headers(files: set[Path]) -> list[Path]:
	return sorted(
		path
		for path in files
		if path.suffix == ".h"
		and len(path.parts) == 3
		and path.parts[:2] == ("include", "tianole")
	)


def c_files(files: set[Path]) -> list[Path]:
	return sorted(
		path
		for path in files
		if path.suffix == ".c" and path.parts and path.parts[0] in C_SOURCE_ROOTS
	)


def read_text(root: Path, path: Path) -> str:
	return (root / path).read_text(encoding="utf-8")


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


def is_function_declaration_start(line: str) -> bool:
	stripped = line.strip()

	if stripped == "" or stripped.startswith("#"):
		return False

	if stripped.startswith(("typedef ", "struct ", "enum ", "union ")):
		return False

	if "(" not in stripped or stripped.startswith("*"):
		return False

	if "(*" in stripped or stripped.endswith("{"):
		return False

	return True


def collect_function_declarations(lines: list[str]) -> list[tuple[int, int, str]]:
	decls = []
	index = 0

	while index < len(lines):
		if not is_function_declaration_start(lines[index]):
			index += 1
			continue

		start = index
		text = lines[index].strip()
		while ";" not in lines[index]:
			index += 1
			if index >= len(lines):
				break
			text += " " + lines[index].strip()

		if index < len(lines):
			decls.append((start, index, text))

		index += 1

	return decls


def is_public_type_start(line: str) -> bool:
	stripped = line.strip()

	if stripped.startswith(("struct ", "enum ", "union ")):
		return stripped.endswith("{") or "{" in stripped

	if not stripped.startswith("typedef "):
		return False

	return "(" not in stripped


def collect_public_type_declarations(lines: list[str]) -> list[tuple[int, int, str]]:
	decls = []
	index = 0

	while index < len(lines):
		if not is_public_type_start(lines[index]):
			index += 1
			continue

		start = index
		text = lines[index].strip()
		while ";" not in lines[index]:
			index += 1
			if index >= len(lines):
				break
			text += " " + lines[index].strip()

		if index < len(lines):
			decls.append((start, index, text))

		index += 1

	return decls


def is_documented_public_define(lines: list[str], index: int, guard: str | None) -> bool:
	stripped = lines[index].strip()

	if not stripped.startswith("#define "):
		return False

	parts = stripped.split()
	if len(parts) < 2:
		return False

	name = parts[1].split("(", 1)[0]
	if name == guard:
		return False

	return True


def header_guard_name(lines: list[str]) -> str | None:
	for line in lines:
		stripped = line.strip()
		if stripped.startswith("#define "):
			parts = stripped.split()
			if len(parts) >= 2:
				return parts[1]

	return None


def has_kernel_doc_before(lines: list[str], start: int) -> bool:
	index = start - 1

	while index >= 0 and lines[index].strip() == "":
		index -= 1

	if index < 0 or lines[index].strip() != "*/":
		return False

	while index >= 0:
		if lines[index].strip().startswith("/**"):
			return True
		index -= 1

	return False


def has_blank_line_before_doc(lines: list[str], start: int) -> bool:
	index = start - 1

	while index >= 0 and lines[index].strip() == "":
		index -= 1

	if index < 0 or lines[index].strip() != "*/":
		return True

	while index >= 0 and not lines[index].strip().startswith("/**"):
		index -= 1

	if index <= 0:
		return True

	return lines[index - 1].strip() == ""


def symbol_name_from_type_declaration(text: str) -> str:
	if text.startswith("typedef "):
		return text.rsplit("}", 1)[-1].strip().rstrip(";").split()[-1]

	parts = text.split()
	if len(parts) >= 2:
		return parts[1]

	return "<unknown>"


def check_kernel_doc_block(
	errors: list[str], path: Path, lines: list[str], start: int, kind: str, name: str
) -> None:
	line_no = start + 1

	if not has_kernel_doc_before(lines, start):
		errors.append(
			f"{path}:{line_no}: public {kind} '{name}' needs "
			"a kernel-doc comment immediately before it"
		)
		return

	if not has_blank_line_before_doc(lines, start):
		errors.append(
			f"{path}:{line_no}: public {kind} '{name}' "
			"comment block must be separated by a blank line"
		)


def check_public_header_docs(root: Path, files: list[Path]) -> list[str]:
	errors = []

	for path in files:
		lines = read_text(root, path).splitlines()
		for start, _end, text in collect_function_declarations(lines):
			name = text.split("(", 1)[0].split()[-1].lstrip("*")
			check_kernel_doc_block(errors, path, lines, start, "function", name)

		for start, _end, text in collect_public_type_declarations(lines):
			name = symbol_name_from_type_declaration(text)
			check_kernel_doc_block(errors, path, lines, start, "type", name)

		guard = header_guard_name(lines)
		for index, line in enumerate(lines):
			if not is_documented_public_define(lines, index, guard):
				continue

			name = line.strip().split()[1].split("(", 1)[0]
			check_kernel_doc_block(errors, path, lines, index, "macro", name)

	return errors


def check_selftests_are_centralized(files: list[Path]) -> list[str]:
	errors = []

	for path in files:
		if "selftest" not in path.name:
			continue

		if len(path.parts) < 2 or path.parts[:2] != ("kernel", "selftest"):
			errors.append(f"{path}: kernel selftests belong under kernel/selftest/")

	return errors


def expand_make_var_token(token: str, makefile: Path) -> str:
	return token.replace("$(ARCH_DIR)", "arch/x86").replace(
		"$(BUILD_DIR)", "build"
	)


def parse_makefile_sources(root: Path, makefile: Path) -> set[Path]:
	text = read_text(root, makefile)
	lines = text.splitlines()
	sources = set()
	index = 0

	while index < len(lines):
		line = lines[index]
		match = re.match(r"^([A-Z0-9_]+)\s*:=", line)
		if match == None or match.group(1) not in MAKEFILE_SOURCE_VARS:
			index += 1
			continue

		remainder = line.split(":=", 1)[1].strip()
		while True:
			continued = remainder.endswith("\\")
			remainder = remainder[:-1].strip() if continued else remainder
			if remainder:
				for token in remainder.split():
					if token.endswith(".c"):
						sources.add(Path(expand_make_var_token(token, makefile)))

			if not continued:
				break

			index += 1
			if index >= len(lines):
				break
			remainder = lines[index].strip()

		index += 1

	return sources


def makefile_sources(root: Path) -> set[Path]:
	sources = set()

	for makefile in sorted(root.rglob("Makefile")):
		if "build" in makefile.parts:
			continue
		sources.update(parse_makefile_sources(root, makefile.relative_to(root)))

	return sources


def check_makefile_source_lists(root: Path, files: set[Path], all_mode: bool) -> list[str]:
	errors = []
	listed = makefile_sources(root)
	existing = set(c_files(all_files(root)))

	for path in sorted(listed):
		if not (root / path).exists():
			errors.append(f"{path}: listed in Makefile but file does not exist")

	if all_mode:
		for path in sorted(existing - listed):
			errors.append(f"{path}: C source is not listed in a Makefile source list")
	else:
		for path in sorted(c_files(files)):
			if path not in listed:
				errors.append(
					f"{path}: changed C source is not listed in a Makefile source list"
				)

	return errors


def main() -> int:
	parser = argparse.ArgumentParser(description="Check Tianole structure rules")
	mode = parser.add_mutually_exclusive_group()
	mode.add_argument("--all", action="store_true", help="check the whole tree")
	mode.add_argument(
		"--changed", action="store_true", help="check changed and untracked files"
	)
	args = parser.parse_args()

	root = repo_root()
	all_mode = not args.changed
	files = all_files(root) if all_mode else changed_files(root)

	errors = []
	errors.extend(check_no_relative_parent_includes(root, source_files(files)))
	errors.extend(check_no_bare_negative_errno(root, source_files(files)))
	errors.extend(check_public_header_docs(root, public_headers(files)))
	errors.extend(check_selftests_are_centralized(c_files(files)))
	errors.extend(check_makefile_source_lists(root, files, all_mode))

	if errors:
		for error in errors:
			print(error, file=sys.stderr)
		return 1

	return 0


if __name__ == "__main__":
	raise SystemExit(main())
