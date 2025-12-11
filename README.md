# keccc

Knight's Experimental Compact C Compiler (supposedly).

NOTE: It uses Meson as the build system.

## Build & run

```sh
# Set up the build directory

# Option1: with AddressSanitizer and UndefinedBehaviorSanitizer
meson setup builddir-asan --buildtype=debug -Db_sanitize=address,undefined
# Option2: without sanitizers (normal)
meson setup builddir

# Compile the project
meson compile -C builddir

# Run the tests (/tests)
meson test -C builddir --print-errorlogs
```

## Individual test during development

Move to the project root and run:

```sh
./src/keccc --target nasm input
bat ./out.s
nasm -f elf64 out.s -o out.o
gcc -no-pie out.o -o out
echo "Assembly and linking finished."
./out
```

## Editor setup (clangd/Neovim)

- A `compile_commands.json` is generated under `builddir/` by Meson. Many language servers (clangd) need to know where this file lives.
- This repo includes a `.clangd` that points clangd to `builddir` and adds `-Isrc` as a fallback. If your build dir differs (e.g. `builddir-asan`), either:
  - change `.clangd` `CompilationDatabase: builddir-asan`, or
  - symlink the compile database into the repo root:

```sh
ln -sf builddir/compile_commands.json compile_commands.json
```

- For tools that look for `compile_flags.txt`, one is provided with `-Isrc`.

## Targets and layout

- `--target`: Selects the backend code generation target. Currently only `nasm` (x86_64) is supported and is the default.
  - Example: `./src/keccc --target nasm tests/input01.kc`
- Machine-dependent codegen is organized under `src/cgn/*/`:
  - `cgn_regs.c|h`: register pool and register names
  - `cgn_expr.c`: loads/stores, arithmetic, comparisons
  - `cgn_stmt.c`: labels, jumps, preamble/postamble, calls/returns

Generic/target-agnostic lowering lives in `src/gen.c` and dispatches to NASM routines.
