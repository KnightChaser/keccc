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
./src/keccc input
bat ./out.s
nasm -f elf64 out.s -o out.o
gcc -no-pie out.o -o out
echo "Assembly and linking finished."
./out
```
