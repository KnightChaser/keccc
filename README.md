# kncc

## Build & run

```bash
meson setup builddir --native-file=clang.ini
meson compile -C builddir
./builddir/kncc
```

Install:

```bash
meson install -C builddir
```
