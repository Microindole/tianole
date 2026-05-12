import subprocess
from pathlib import Path


SOURCE_ROOTS = ("arch", "include", "kernel", "mm")
C_SOURCE_ROOTS = ("arch", "kernel", "mm")


def repo_root() -> Path:
	return Path(__file__).resolve().parents[3]


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
