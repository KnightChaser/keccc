// src/rt/_ref/printint.c

/**
 * NOTE:
 * AARCH64 implementation: src/rt/aarch64/printint.S
 * x86_64 implementation: src/rt/x86_64/printint.asm
 *
 * NOTE:
 * The following code is a conceptual reference implementation in C.
 * It is not used in the actual runtime.
 */

#include <limits.h>
#include <unistd.h>

void printint(long x) {
    /*
     * NOTE:
     * Special case: INT64_MIN
     * Because -INT64_MIN overflows in two’s complement.
     */
    if (x == LONG_MIN) {
        static const char minstr[] = "-9223372036854775808\n";
        write(1, minstr, sizeof(minstr) - 1);
        return;
    }

    char buf[64];         // x86_64 code reserves 64B; uses ~32B
                          // In AAarch64, 96B are reserved; refer to the code
    char *end = buf + 32; // conceptually matches "rbp-1 ... rbp-32" region
    char *p = end;

    long n = x;
    int negative = 0;

    if (n < 0) {
        negative = 1;
        n = -n;
    }

    if (n == 0) {
        *--p = '0';
    } else {
        while (n != 0) {
            unsigned long un = (unsigned long)n;
            unsigned long q = un / 10;
            unsigned long r = un % 10;
            *--p = (char)('0' + r);
            n = (long)q;
        }
    }

    if (negative) {
        *--p = '-';
    }

    write(1, p, (size_t)(end - p));
    write(1, "\n", 1);
}
