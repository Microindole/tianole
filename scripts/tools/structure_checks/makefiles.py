import re
from pathlib import Path

from .common import all_files, c_files, read_text


MAKEFILE_SOURCE_VARS = (
	"ARCH_KERNEL_SRCS",
	"ARCH_MM_SRCS",
	"BOOT_SRCS",
	"DRIVER_SRCS",
	"KERNEL_SRCS",
	"MM_SRCS",
)


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
