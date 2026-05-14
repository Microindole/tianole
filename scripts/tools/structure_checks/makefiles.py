from pathlib import Path

from .common import all_files, c_files, read_text


def is_object_list_variable(name: str) -> bool:
	return name.endswith("-y")


def source_for_object(root: Path, makefile: Path, token: str) -> Path | None:
	if not token.endswith(".o") or "$(" in token:
		return None

	source = makefile.parent / token[:-2]
	c_source = source.with_suffix(".c")

	if (root / c_source).exists():
		return c_source

	return None


def parse_makefile_sources(root: Path, makefile: Path) -> set[Path]:
	text = read_text(root, makefile)
	lines = text.splitlines()
	sources = set()
	index = 0

	while index < len(lines):
		line = lines[index]
		if ":=" not in line:
			index += 1
			continue

		name, remainder = line.split(":=", 1)
		if not is_object_list_variable(name.strip()):
			index += 1
			continue

		remainder = remainder.strip()
		while True:
			continued = remainder.endswith("\\")
			remainder = remainder[:-1].strip() if continued else remainder
			if remainder:
				for token in remainder.split():
					source = source_for_object(root, makefile, token)
					if source != None:
						sources.add(source)

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
