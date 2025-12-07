# tests/run_tests.py

import subprocess
import shutil
import sys
from pathlib import Path
from typing import Tuple

def run(cmd: str, cwd: Path, desc: str) -> Tuple[bool, str]:
    proc = subprocess.run(
        cmd,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )

    if proc.returncode != 0:
        print(f"[FAIL] {desc}")
        print("  Command:", " ".join(cmd))
        print("  Exit code:", proc.returncode)
        if proc.stdout:
            print("  STDOUT:")
            print(proc.stdout)
        if proc.stderr:
            print("  STDERR:")
            print(proc.stderr)
        return False, proc.stdout
    return True, proc.stdout

def run_single_test(test_name, src_path, expected_path, build_root, keccc_path):
    print(f"== Running test: {test_name}")

    # Sanity checks
    if not keccc_path.exists():
        print(f"[FATAL] keccc not found at {keccc_path}")
        return False
    if not expected_path.exists():
        print(f"[FATAL] expected file missing: {expected_path}")
        return False

    # We'll run everything in the build root so out.s/out.o/out land there
    cwd = build_root

    # Clean old artifacts
    for f in ["out.s", "out.o", "out"]:
        p = cwd / f
        if p.exists():
            p.unlink()

    # 1) Compile source with keccc
    ok, _ = run([str(keccc_path), str(src_path)], cwd, f"{test_name}: keccc")
    if not ok:
        return False

    # 2) Assemble out.s -> out.o
    nasm = shutil.which("nasm")
    if nasm is None:
        print("[FATAL] nasm not found in PATH")
        return False
    ok, _ = run(
        [nasm, "-f", "elf64", "out.s", "-o", "out.o"],
        cwd,
        f"{test_name}: nasm",
    )
    if not ok:
        return False

    # 3) Link out.o -> out
    gcc = shutil.which("gcc")
    if gcc is None:
        print("[FATAL] gcc not found in PATH")
        return False
    ok, _ = run(
        [gcc, "-no-pie", "out.o", "-o", "out"],
        cwd,
        f"{test_name}: gcc",
    )
    if not ok:
        return False

    # 4) Run ./out and capture stdout
    ok, stdout = run(["./out"], cwd, f"{test_name}: run out")
    if not ok:
        return False

    actual = stdout
    expected = expected_path.read_text()

    # Normalize trailing whitespace/newlines a bit
    actual_stripped = actual.strip()
    expected_stripped = expected.strip()

    if actual_stripped != expected_stripped:
        print(f"[FAIL] {test_name}: output mismatch")
        print("  Expected:")
        print(repr(expected_stripped))
        print("  Actual:")
        print(repr(actual_stripped))
        return False

    print(f"[PASS] {test_name}")
    return True

def main():
    if len(sys.argv) != 3:
        print("Usage: run_tests.py <source_root> <build_root>")
        return 1

    source_root = Path(sys.argv[1]).resolve()
    build_root = Path(sys.argv[2]).resolve()

    tests_dir = source_root / "tests"
    keccc_path = build_root / "src" / "keccc"

    test_files = sorted(tests_dir.glob("*.kc"))
    if not test_files:
        print(f"No *.kc tests found under {tests_dir}")
        return 1

    all_ok = True
    for src in test_files:
        name = src.stem          # while_1
        expected = src.with_suffix(".expected")
        ok = run_single_test(name, src, expected, build_root, keccc_path)
        if not ok:
            all_ok = False

    return 0 if all_ok else 1

if __name__ == "__main__":
    raise SystemExit(main())

