from pathlib import Path


def parse_suite_registry(root: Path) -> tuple[set[Path], list[str]]:
	registry = root / "scripts/checks/suites.list"
	scripts = set()
	errors = []

	if not registry.exists():
		return scripts, ["scripts/checks/suites.list: missing validation suite registry"]

	for line_no, line in enumerate(registry.read_text(encoding="utf-8").splitlines(), 1):
		stripped = line.strip()
		if stripped == "" or stripped.startswith("#"):
			continue

		parts = stripped.split()
		if len(parts) != 2:
			errors.append(
				f"scripts/checks/suites.list:{line_no}: expected '<suite> <script>'"
			)
			continue

		script = Path(parts[1])
		scripts.add(script)
		if not (root / script).exists():
			errors.append(f"scripts/checks/suites.list:{line_no}: {script} does not exist")

	return scripts, errors


def check_validation_suites(root: Path) -> list[str]:
	registered, errors = parse_suite_registry(root)
	check_dir = root / "scripts/checks"

	for path in sorted(check_dir.glob("*.sh")):
		rel = path.relative_to(root)
		if rel not in registered:
			errors.append(f"{rel}: validation script is not listed in suites.list")

	return errors
