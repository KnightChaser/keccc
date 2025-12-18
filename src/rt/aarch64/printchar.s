// src/rt/aarch64/printchar.s

// Linux aarch64
// long printchar(long x)
// arg:  x0 = x

    .text
    .global printchar

printchar:
    // Make a tiny stack scratch
    sub sp, sp, #16

    // (char)(x & 0x7F)
    and x9, x0, #0x7F
    strb w9, [sp]

    // write(1, sp, 1)
    mov x0, #1                 // fd = stdout
    mov x1, sp                 // buf = &byte
    mov x2, #1                 // len = 1
    mov x8, #64                // SYS_write
    svc #0

    add sp, sp, #16
    ret
