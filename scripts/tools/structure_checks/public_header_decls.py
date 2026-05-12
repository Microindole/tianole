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


def symbol_name_from_type_declaration(text: str) -> str:
	if text.startswith("typedef "):
		return text.rsplit("}", 1)[-1].strip().rstrip(";").split()[-1]

	parts = text.split()
	if len(parts) >= 2:
		return parts[1]

	return "<unknown>"
