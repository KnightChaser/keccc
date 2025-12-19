// src/rt/_ref/printstring.c

/**
 * NOTE:
 * AARCH64 implementation: src/rt/aarch64/printstring.s
 * x86_64 implementation: src/rt/x86_64/printstring.asm
 *
 * NOTE:
 * The following code is a conceptual reference implementation in C.
 * It is not used in the actual runtime.
 */

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#define MAXLEN 65536u

ssize_t printstring(const char *s) {
    if (!s) {
        return 0;
    }

    size_t len = 0;
    while (len < MAXLEN && s[len] != '\0') {
        len++;
    }

    // If NUL terminator not found within MAXLEN, len == MAXLEN
    return write(STDOUT_FILENO, s, len);
}
