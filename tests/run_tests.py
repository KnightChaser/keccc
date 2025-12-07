#!/usr/bin/env python3
"""End-to-end test runner for the keccc compiler.

This script is intended to be invoked by Meson as a single test target.
It:

1. Finds all `*.kc` source files under `tests/`.
2. For each source file `<name>.kc`, expects a matching `<name>.expected`.
3. Runs the full pipeline:
   - `keccc <source>`
   - `nasm -f elf64 out.s -o out.o`
   - `gcc -no-pie out.o -o out`
   - `./out`
4. Compares the program's stdout to the expected output (after `.strip()`).
"""

from __future__ import annotations

import difflib
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence, Tuple


@dataclass(frozen=True)
class TestCase:
    """Represents a single end-to-end test case."""

    name: str
    source: Path
    expected: Path


def run_command(
    command: Sequence[str],
    cwd: Path,
    description: str,
) -> Tuple[bool, str, str]:
    """Run a command in a given working directory.

    Args:
        command: The command and its arguments as a sequence of strings.
        cwd: Directory in which to execute the command.
        description: Human-readable label for error messages.

    Returns:
        A tuple of (success, stdout, stderr) where:
        - success is True if returncode == 0, False otherwise.
        - stdout and stderr are the decoded text output from the process.
    """
    process = subprocess.run(
        command,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    if process.returncode != 0:
        print(f"[FAIL] {description}")
        print("  Command:", " ".join(command))
        print("  Exit code:", process.returncode)

        if process.stdout:
            print("  STDOUT:")
            print(process.stdout)

        if process.stderr:
            print("  STDERR:")
            print(process.stderr)

        return False, process.stdout, process.stderr

    return True, process.stdout, process.stderr


def clean_artifacts(build_root: Path, artifacts: Iterable[str]) -> None:
    """Remove temporary build artifacts if they exist.

    Args:
        build_root: Directory under which artifacts are created.
        artifacts: Iterable of filenames relative to build_root.
    """
    for name in artifacts:
        artifact_path = build_root / name
        if artifact_path.exists():
            artifact_path.unlink()


def find_required_executable(name: str) -> str:
    """Locate a required executable in PATH or abort.

    Args:
        name: Name of the executable to locate (e.g., 'nasm', 'gcc').

    Returns:
        The absolute path to the executable.

    Raises:
        SystemExit: If the executable cannot be found.
    """
    path = shutil.which(name)
    if path is None:
        print(f"[FATAL] Required executable '{name}' not found in PATH")
        raise SystemExit(1)
    return path


def print_output_diff(expected_text: str, actual_text: str, test_name: str) -> None:
    """Print expected vs actual output and a unified diff.

    Args:
        expected_text: The expected full output text.
        actual_text: The actual full output text.
        test_name: Name of the test case, used for labelling.
    """
    print(f"[FAIL] {test_name}: output mismatch")

    print("\n=== Expected output ===")
    print(expected_text if expected_text else "<empty>")

    print("\n=== Actual output ===")
    print(actual_text if actual_text else "<empty>")

    expected_lines = expected_text.splitlines(keepends=False)
    actual_lines = actual_text.splitlines(keepends=False)

    print("\n=== Unified diff (expected vs actual) ===")
    diff = difflib.unified_diff(
        expected_lines,
        actual_lines,
        fromfile="expected",
        tofile="actual",
        lineterm="",
    )
    for line in diff:
        print(line)
    print()  # trailing newline for cleanliness


def run_single_test(
    test_case: TestCase,
    build_root: Path,
    keccc_path: Path,
    nasm_path: str,
    gcc_path: str,
) -> bool:
    """Run the full compiler → assembler → linker → program pipeline.

    Args:
        test_case: The test case to execute.
        build_root: Build directory where out.s/out.o/out will be generated.
        keccc_path: Path to the compiled keccc executable.
        nasm_path: Path to the nasm executable.
        gcc_path: Path to the gcc executable.

    Returns:
        True if the test passes, False otherwise.
    """
    print(f"== Running test: {test_case.name}")

    if not keccc_path.exists():
        print(f"[FATAL] keccc not found at {keccc_path}")
        return False

    if not test_case.expected.exists():
        print(f"[FATAL] Missing expected output file: {test_case.expected}")
        return False

    # All artifacts are generated in the build directory
    cwd = build_root

    # Clean previous run's artifacts
    clean_artifacts(cwd, ["out.s", "out.o", "out"])

    # 1) Compile to assembly with keccc
    ok, _, _ = run_command(
        [str(keccc_path), str(test_case.source)],
        cwd=cwd,
        description=f"{test_case.name}: keccc",
    )
    if not ok:
        # Compilation failure: no diff to show, compiler error already printed
        return False

    # 2) Assemble out.s -> out.o
    ok, _, _ = run_command(
        [nasm_path, "-f", "elf64", "out.s", "-o", "out.o"],
        cwd=cwd,
        description=f"{test_case.name}: nasm",
    )
    if not ok:
        return False

    # 3) Link out.o -> out
    ok, _, _ = run_command(
        [gcc_path, "-no-pie", "out.o", "-o", "out"],
        cwd=cwd,
        description=f"{test_case.name}: gcc",
    )
    if not ok:
        return False

    # 4) Run the resulting binary
    ok, stdout, _ = run_command(
        ["./out"],
        cwd=cwd,
        description=f"{test_case.name}: run out",
    )
    if not ok:
        return False

    # Keep full originals for printing/diff
    actual_text = stdout
    expected_text = test_case.expected.read_text()

    # Normalized versions for equality check (ignore leading/trailing whitespace)
    actual_norm = actual_text.strip()
    expected_norm = expected_text.strip()

    if actual_norm != expected_norm:
        print_output_diff(expected_text, actual_text, test_case.name)
        return False

    print(f"[PASS] {test_case.name}")
    return True


def discover_tests(tests_dir: Path) -> list[TestCase]:
    """Discover all .kc tests in the given directory.

    Args:
        tests_dir: Directory containing test source files and expected outputs.

    Returns:
        A list of TestCase objects, one per `<name>.kc` file.

    Notes:
        For each `<name>.kc`, this expects a matching `<name>.expected`
        file in the same directory.
    """
    test_cases: list[TestCase] = []

    for source_path in sorted(tests_dir.glob("*.kc")):
        name = source_path.stem
        expected_path = source_path.with_suffix(".expected")
        test_cases.append(
            TestCase(
                name=name,
                source=source_path,
                expected=expected_path,
            )
        )

    return test_cases


def main(argv: Sequence[str]) -> int:
    """Entry point for the test runner.

    Args:
        argv: Command-line arguments, excluding the interpreter name.

    Returns:
        Process exit code (0 = all tests passed, 1 = failure).
    """
    if len(argv) != 2:
        print("Usage: run_tests.py <source_root> <build_root>")
        return 1

    source_root = Path(argv[0]).resolve()
    build_root = Path(argv[1]).resolve()

    tests_dir = source_root / "tests"
    keccc_path = build_root / "src" / "keccc"

    test_cases = discover_tests(tests_dir)
    if not test_cases:
        print(f"No *.kc tests found under {tests_dir}")
        return 1

    nasm_path = find_required_executable("nasm")
    gcc_path = find_required_executable("gcc")

    all_ok = True
    for test_case in test_cases:
        if not run_single_test(
            test_case=test_case,
            build_root=build_root,
            keccc_path=keccc_path,
            nasm_path=nasm_path,
            gcc_path=gcc_path,
        ):
            all_ok = False

    return 0 if all_ok else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
