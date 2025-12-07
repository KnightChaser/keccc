# tests/meson.build

python = import('python').find_installation()

test(
  'keccc-test',
  python,
  args: [
    files('run_tests.py'),
    meson.project_source_root(),
    meson.project_build_root(),
  ],
  is_parallel: false, # NOTE: out.s/out.o/out are shared filenames
)
