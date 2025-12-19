
# keccc

Knight's Experimental Compact C Compiler (supposedly).

NOTE: It uses Meson as the build system.

## Build & run

```sh
# Set up the build directory
meson setup builddir --native-file clang.ini

# Compile the project
meson compile -C builddir

# Run the tests (/tests)
meson test -C builddir --print-errorlogs
```

## Individual test during development

Suppose you're at the project root, and the input file is located at `./builddir`, after you successfully build the project with `meson compile -C ./builddir` command.

- **x86_64** NASM target

```sh
./builddir/src/keccc --output ./builddir/out.asm --target nasm ./builddir/input
nasm -felf64 ./builddir/out.asm -o ./builddir/out.o
nasm -felf64 src/rt/x86_64/start.asm -o ./builddir/start.o
nasm -felf64 src/rt/x86_64/printint.asm -o ./builddir/printint.o
nasm -felf64 src/rt/x86_64/printchar.asm -o ./builddir/printchar.o
nasm -felf64 src/rt/x86_64/printstring.asm -o ./builddir/printstring.o
ld -o ./builddir/out ./builddir/out.o ./builddir/start.o ./builddir/printint.o ./builddir/printchar.o ./builddir/printstring.o 
./builddir/out
```

- **AArch64** (ARM64) target

```sh
./builddir/src/keccc --output ./builddir/out.s --target aarch64 ./builddir/input
aarch64-linux-gnu-as ./builddir/out.s -o ./builddir/out.o
aarch64-linux-gnu-as src/rt/aarch64/start.s -o ./builddir/start.o
aarch64-linux-gnu-as src/rt/aarch64/printint.s -o ./builddir/printint.o
aarch64-linux-gnu-as src/rt/aarch64/printchar.s -o ./builddir/printchar.o
aarch64-linux-gnu-as src/rt/aarch64/printstring.s -o ./builddir/printstring.o
aarch64-linux-gnu-ld -o ./builddir/out_arm64 ./builddir/out.o ./builddir/start.o ./builddir/printint.o ./builddir/printchar.o ./builddir/printstring.o
qemu-aarch64 ./builddir/out_arm64
```

## Targets and layout

- `--target`: Selects the backend code generation target. `nasm` (Intel x86_64, NASM flavored) and `aarch64`(ARM64) is supported. Note that `aarch64` is tested via `qemu-aarch64`.
  - Example: `./src/keccc --target nasm tests/input01.kc`
- Machine-dependent codegen is organized under `src/cgn/*/`:
  - `cgn_regs.c|h`: register pool and register names
  - `cgn_expr.c`: loads/stores, arithmetic, comparisons
  - `cgn_stmt.c`: labels, jumps, preamble/postamble, calls/returns
  - `cgn_ops.c`: An abstraction layer between backend-agnostic code generation part to backend-dependent ASM code generation functions

Generic/target-agnostic lowering lives in `src/gen.c` and dispatches to NASM routines.

## Editor setup (clangd/Neovim)

- A `compile_commands.json` is generated under `builddir/` by Meson. Many language servers (clangd) need to know where this file lives.
- This repo includes a `.clangd` that points clangd to `builddir` and adds `-Isrc` as a fallback. If your build dir differs (e.g. `builddir-asan`), either:
  - change `.clangd` `CompilationDatabase: builddir-asan`, or
  - symlink the compile database into the repo root:

```sh
ln -sf builddir/compile_commands.json compile_commands.json
```

- For tools that look for `compile_flags.txt`, one is provided with `-Isrc`.
