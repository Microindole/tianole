from pathlib import Path

from .common import read_text
from .public_header_decls import (
	collect_function_declarations,
	collect_public_type_declarations,
	symbol_name_from_type_declaration,
)


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
