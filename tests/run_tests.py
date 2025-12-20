#!/usr/bin/env python3
from __future__ import annotations

import argparse
import difflib
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Sequence, Tuple


@dataclass(frozen=True)
class TestCase:
    name: str
    source: Path
    expected: Path


def run_command(
    command: Sequence[str],
    cwd: Path,
    description: str,
    allow_nonzero_exit: bool = False,
) -> Tuple[bool, str, str]:
    """
    Runs a command in a subprocess, capturing stdout and stderr.
    If the command fails (non-zero exit code) and allow_nonzero_exit is False,
    prints an error message and returns False.
    Otherwise, returns True along with stdout and stderr.
    """
    process = subprocess.run(
        command,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    if process.returncode != 0 and not allow_nonzero_exit:
        print(f"[FAIL] {description}")
        print("  Command:", " ".join(command))
        print("  Exit code:", process.returncode)
        if process.stdout:
            print("  STDOUT:\n" + process.stdout)
        if process.stderr:
            print("  STDERR:\n" + process.stderr)
        return False, process.stdout, process.stderr

    return True, process.stdout, process.stderr


def find_required_executable(name: str) -> str:
    """
    Finds the full path of a required executable in the system PATH.
    If not found, prints a fatal error and exits.
    """
    path = shutil.which(name)
    if path is None:
        print(f"[FATAL] Required executable '{name}' not found in PATH")
        raise SystemExit(1)
    return path


def print_output_diff(expected_text: str, actual_text: str, test_name: str) -> None:
    """
    Prints a unified diff between expected and actual output texts.
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
    print()


def discover_tests(tests_dir: Path) -> list[TestCase]:
    """
    Discovers all test cases in the given tests directory.
    A test case consists of a .kc source file and a corresponding .expected file.
    Returns a list of TestCase objects.
    """
    test_cases: list[TestCase] = []
    for source_path in sorted(tests_dir.glob("*.c")):
        name = source_path.stem
        expected_path = source_path.with_suffix(".expected")
        test_cases.append(TestCase(name=name, source=source_path, expected=expected_path))
    return test_cases


def ensure_empty_dir(path: Path) -> None:
    """
    Ensures that the given directory path exists and is empty.
    """
    if path.exists():
        shutil.rmtree(path)
    path.mkdir(parents=True, exist_ok=True)


def run_single_test_nasm(
    test_case: TestCase,
    workdir: Path,
    keccc_path: Path,
    source_root: Path,
    nasm: str,
    ld: str,
) -> Tuple[bool, str]:
    """
    Runs a single test case targeting x86_64 using nasm and ld.
    Returns a tuple of (success: bool, stdout: str).
    """
    # Build artifacts in workdir
    out_asm = workdir / "out.asm"
    out_o = workdir / "out.o"
    start_o = workdir / "start.o"
    printint_o = workdir / "printint.o"
    printchar_o = workdir / "printchar.o"
    printstring_o = workdir / "printstring.o"
    out_bin = workdir / "out"

    rt_dir = source_root / "src" / "rt" / "x86_64"
    start_asm = rt_dir / "start.asm"
    printint_asm = rt_dir / "printint.asm"
    printchar_asm = rt_dir / "printchar.asm"
    printstring_asm = rt_dir / "printstring.asm"

    # 1) Compile to assembly (keccc writes out.s in cwd)
    ok, _, _ = run_command(
        [str(keccc_path), "--output", "out.asm", "--target", "nasm", str(test_case.source)],
        cwd=workdir,
        description=f"{test_case.name}: keccc(nasm)",
    )
    if not ok:
        return False, ""

    # keccc wrote "out.s" into workdir; keep it as out.s already
    if not (workdir / "out.asm").exists():
        print(f"[FAIL] {test_case.name}: keccc did not produce out.s in {workdir}")
        return False, ""

    # 2) Assemble program
    ok, _, _ = run_command(
        [nasm, "-felf64", str(out_asm), "-o", str(out_o)],
        cwd=workdir,
        description=f"{test_case.name}: nasm out.asm",
    )
    if not ok:
        return False, ""

    # 3) Assemble runtime objects
    ok, _, _ = run_command(
        [nasm, "-felf64", str(start_asm), "-o", str(start_o)],
        cwd=workdir,
        description=f"{test_case.name}: nasm start.asm",
    )
    if not ok:
        return False, ""

    ok, _, _ = run_command(
        [nasm, "-felf64", str(printint_asm), "-o", str(printint_o)],
        cwd=workdir,
        description=f"{test_case.name}: nasm printint.asm",
    )
    if not ok:
        return False, ""

    ok, _, _ = run_command(
        [nasm, "-felf64", str(printchar_asm), "-o", str(printchar_o)],
        cwd=workdir,
        description=f"{test_case.name}: nasm printchar.asm",
    )
    if not ok:
        return False, ""

    ok, _, _ = run_command(
        [nasm, "-felf64", str(printstring_asm), "-o", str(printstring_o)],
        cwd=workdir,
        description=f"{test_case.name}: nasm printstring.asm",
    )
    if not ok:
        return False, ""

    # 4) Link (no libc)
    ok, _, _ = run_command(
        [ld, "-o", str(out_bin), str(out_o), str(start_o), str(printint_o), str(printchar_o), str(printstring_o)],
        cwd=workdir,
        description=f"{test_case.name}: ld",
    )
    if not ok:
        return False, ""

    # 5) Run
    ok, stdout, _ = run_command(
        [str(out_bin)],
        cwd=workdir,
        description=f"{test_case.name}: run x86_64",
        allow_nonzero_exit=True,
    )
    return True, stdout


def run_single_test_aarch64(
    test_case: TestCase,
    workdir: Path,
    keccc_path: Path,
    source_root: Path,
    as_path: str,
    ld_path: str,
    qemu: str,
) -> Tuple[bool, str]:
    """
    Runs a single test case targeting aarch64 using GNU assembler and linker,
    and executes the resulting binary via qemu-aarch64.
    Returns a tuple of (success: bool, stdout: str).
    """
    out_s = workdir / "out.s"
    out_o = workdir / "out.o"
    start_o = workdir / "start.o"
    printint_o = workdir / "printint.o"
    printchar_o = workdir / "printchar.o"
    printstring_o = workdir / "printstring.o"
    out_bin = workdir / "out_aarch64"

    rt_dir = source_root / "src" / "rt" / "aarch64"
    start_s = rt_dir / "start.s"
    printint_s = rt_dir / "printint.s"
    printchar_s = rt_dir / "printchar.s"
    printstring_s = rt_dir / "printstring.s"

    # 1) Compile to assembly (keccc writes out.s in cwd)
    ok, _, _ = run_command(
        [str(keccc_path), "--output", "out.s", "--target", "aarch64", str(test_case.source)],
        cwd=workdir,
        description=f"{test_case.name}: keccc(aarch64)",
    )
    if not ok:
        return False, ""

    if not (workdir / "out.s").exists():
        print(f"[FAIL] {test_case.name}: keccc did not produce out.s in {workdir}")
        return False, ""

    # 2) Assemble program
    ok, _, _ = run_command(
        [as_path, str(out_s), "-o", str(out_o)],
        cwd=workdir,
        description=f"{test_case.name}: aarch64 as out.s",
    )
    if not ok:
        return False, ""

    # 3) Assemble runtime objects
    ok, _, _ = run_command(
        [as_path, str(start_s), "-o", str(start_o)],
        cwd=workdir,
        description=f"{test_case.name}: aarch64 as start.asm",
    )
    if not ok:
        return False, ""

    ok, _, _ = run_command(
        [as_path, str(printint_s), "-o", str(printint_o)],
        cwd=workdir,
        description=f"{test_case.name}: aarch64 as printint.s",
    )
    if not ok:
        return False, ""

    ok, _, _ = run_command(
        [as_path, str(printchar_s), "-o", str(printchar_o)],
        cwd=workdir,
        description=f"{test_case.name}: aarch64 as printchar.s",
    )
    if not ok:
        return False, ""

    ok, _, _ = run_command(
        [as_path, str(printstring_s), "-o", str(printstring_o)],
        cwd=workdir,
        description=f"{test_case.name}: aarch64 as printstring.s",
    )
    if not ok:
        return False, ""

    # 4) Link (no libc)
    ok, _, _ = run_command(
        [ld_path, "-o", str(out_bin), str(out_o), str(start_o), str(printint_o), str(printchar_o), str(printstring_o)],
        cwd=workdir,
        description=f"{test_case.name}: aarch64 ld",
    )
    if not ok:
        return False, ""

    # 5) Run via qemu-user
    ok, stdout, _ = run_command(
        [qemu, str(out_bin)],
        cwd=workdir,
        description=f"{test_case.name}: run aarch64 via qemu",
        allow_nonzero_exit=True,
    )
    return True, stdout


def main(argv: Sequence[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--target", required=True, choices=["nasm", "aarch64"])
    parser.add_argument("source_root")
    parser.add_argument("build_root")
    args = parser.parse_args(list(argv))

    source_root = Path(args.source_root).resolve()
    build_root = Path(args.build_root).resolve()

    tests_dir = source_root / "tests" / "testcases"
    keccc_path = build_root / "src" / "keccc"

    if not keccc_path.exists():
        print(f"[FATAL] keccc not found at {keccc_path}")
        return 1

    test_cases = discover_tests(tests_dir)
    if not test_cases:
        print(f"No *.kc tests found under {tests_dir}")
        return 1

    # Workspace per target to avoid filename clashes
    work_root = build_root / "tests-work" / args.target
    work_root.mkdir(parents=True, exist_ok=True)

    # Tool discovery
    if args.target == "nasm":
        nasm = find_required_executable("nasm")
        ld = find_required_executable("ld")
    else:
        as_path = find_required_executable("aarch64-linux-gnu-as")
        ld_path = find_required_executable("aarch64-linux-gnu-ld")
        qemu = find_required_executable("qemu-aarch64")

    all_ok = True

    """
    For each test case:
    1) Compile with keccc to assembly
    2) Assemble with nasm/as
    3) Link with ld
    4) Run the binary (directly or via qemu)
    5) Compare output to expected
    6) Report results
    """
    for tc in test_cases:
        print(f"== Running {args.target} test: {tc.name}")

        if not tc.expected.exists():
            print(f"[FATAL] Missing expected output file: {tc.expected}")
            return 1

        workdir = work_root / tc.name
        ensure_empty_dir(workdir)

        if args.target == "nasm":
            ok, stdout = run_single_test_nasm(
                test_case=tc,
                workdir=workdir,
                keccc_path=keccc_path,
                source_root=source_root,
                nasm=nasm, # type: ignore
                ld=ld, # type: ignore
            )
        else:
            ok, stdout = run_single_test_aarch64(
                test_case=tc,
                workdir=workdir,
                keccc_path=keccc_path,
                source_root=source_root,
                as_path=as_path, # type: ignore
                ld_path=ld_path, # type: ignore
                qemu=qemu, # type: ignore
            )

        if not ok:
            all_ok = False
            continue

        expected_text = tc.expected.read_text()
        if stdout.strip() != expected_text.strip():
            print_output_diff(expected_text, stdout, f"{tc.name} ({args.target})")
            all_ok = False
        else:
            print(f"[PASS] {tc.name} ({args.target})")

    return 0 if all_ok else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

