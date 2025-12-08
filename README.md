# keccc

Knight's Experimental Compact C Compiler (supposedly).

NOTE: It uses Meson as the build system.

## Build & run

```sh
# Set up the build directory
meson setup builddir-asan --buildtype=debug -Db_sanitize=address,undefined

# Compile the project
meson compile -C builddir-asan

# Run the tests (/tests)
meson test -C builddir-asan --print-errorlogs
```

## Individual test during development

Move to the project root and run:

```sh
./src/keccc input
bat ./out.s
nasm -f elf64 out.s -o out.o
gcc -no-pie out.o -o out
echo "Assembly and linking finished."
./out
```
