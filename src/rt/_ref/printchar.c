// src/rt/_ref/printchar.c

/**
 * NOTE:
 * AARCH64 implementation: src/rt/aarch64/printchar.s
 * x86_64 implementation: src/rt/x86_64/printchar.asm
 *
 * NOTE:
 * The following code is a conceptual reference implementation in C.
 * It is not used in the actual runtime.
 */

#include <stdio.h>

void printchar(long x) { putc((char)(x & 0x7f), stdout); }
