// src/rt/aarch64/printint.asm

// Linux aarch64
// long printint(long x)
// arg:  x0 = x
// clobbers: x0-x15, x8
// preserves: x19-x28 (we don't touch), and uses standard frame save/restore

    .text
    .global printint

    .section .rodata

minstr:
    .ascii "-9223372036854775808\n"
minlen = . - minstr
newline:
    .ascii "\n"

    .text

printint:
    // 96 bytes stack frame (multiple of 16)
    stp x29, x30, [sp, -96]!
    mov x29, sp

    // Special-case INT64_MIN
    // x9 = 0x8000000000000000
    movz x9, #0x0000
    movk x9, #0x0000, lsl #16
    movk x9, #0x0000, lsl #32
    movk x9, #0x8000, lsl #48
    cmp  x0, x9
    b.ne 1f

    // write(1, minstr, minlen)
    mov x0, #1
    adrp x1, minstr
    add  x1, x1, :lo12:minstr
    mov x2, #minlen
    mov x8, #64
    svc #0

    ldp x29, x30, [sp], 96
    ret

1:
    // sign flag in w15: 0/1
    mov w15, #0
    mov x9, x0            // working value
    cmp x9, #0
    b.ge 2f
    neg x9, x9
    mov w15, #1
2:
    // buffer: use the top of our frame (end = sp+96)
    add x10, sp, #96      // end ptr
    mov x11, x10          // cur ptr
    mov x12, #10          // divisor

    // handle 0
    cbnz x9, 3f
    sub x11, x11, #1
    mov w14, #48          // '0'
    strb w14, [x11]
    b 5f

3:  // convert
    udiv x13, x9, x12
    msub x14, x13, x12, x9
    add x14, x14, #48
    sub x11, x11, #1
    strb w14, [x11]
    mov x9, x13
    cbnz x9, 3b

5:  // maybe sign
    cbz w15, 6f
    sub x11, x11, #1
    mov w14, #45          // '-'
    strb w14, [x11]

6:  // write number
    mov x0, #1            // stdout
    mov x1, x11           // start
    sub x2, x10, x11      // len
    mov x8, #64
    svc #0

    // write newline
    mov x0, #1
    adrp x1, newline
    add  x1, x1, :lo12:newline
    mov x2, #1
    mov x8, #64
    svc #0

    ldp x29, x30, [sp], 96
    ret

