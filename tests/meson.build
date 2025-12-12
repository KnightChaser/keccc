# tests/meson.build

python = import('python').find_installation()

# NASM (x86_64) end-to-end test
test(
  'keccc-nasm-e2e',
  python,
  args: [
    files('run_tests.py'),
    '--target', 'nasm',
    meson.project_source_root(),
    meson.project_build_root(),
  ],
  is_parallel: true,  # safe now: per-testcase work dirs
)

# AARCH64 end-to-end
test(
  'keccc-aarch64-e2e',
  python,
  args: [
    files('run_tests.py'),
    '--target', 'aarch64',
    meson.project_source_root(),
    meson.project_build_root(),
  ],
  is_parallel: true,
)
