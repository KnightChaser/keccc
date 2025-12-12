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
    //
    // x29 for FP (Frame Pointer)
    // x30 for LR (Link Register)
    stp x29, x30, [sp, -96]!
    mov x29, sp

    // AArch64 can't load an arbitrary 64-bit constant in a single instruction...
    //
    // Special-case INT64_MIN
    // x9 = 0x8000000000000000
    movz x9, #0x0000          // movz; move 16-bit immediate, zeroing the rest
    movk x9, #0x0000, lsl #16 // movk; keep the other bits, overwrite 16-bit lane at a shift
    movk x9, #0x0000, lsl #32
    movk x9, #0x8000, lsl #48
    cmp  x0, x9
    b.ne 1f

    // write(1, minstr, minlen)
    // (The value is out of range, so we just print the minimum string)
    mov x0, #1                // x0 (1st argument): fd = stdout
    adrp x1, minstr           // x1 (2nd argument): buf (Pointer to string)
    add  x1, x1, :lo12:minstr
    mov x2, #minlen           // x2 (3rd argument): len
    mov x8, #64               // x8 (syscall number): SYS_WRITE
    svc #0                    // execute the system call! >_<

    ldp x29, x30, [sp], 96
    ret

1:
    // sign flag in w15: 0/1
    mov w15, #0               // x15: used as a sign 32-bit sign flag: 0 = positive, 1 = negative
    mov x9, x0                // working copy of input
    cmp x9, #0                // if x9 < 0, it means we need to print a '-'
    b.ge 2f
    neg x9, x9
    mov w15, #1               // set sign flag (negative)
2:
    // buffer: use the top of our frame (end = sp+96)
    add x10, sp, #96          // end of frame (sp + 96)
    mov x11, x10              // current write pointer, starts at end
    mov x12, #10              // divisor (10)

    // handle 0
    cbnz x9, 3f               // if x9 != 0 (not zero), jump to the convert loop
    sub x11, x11, #1          // make space for one byte
    mov w14, #48              // '0'
    strb w14, [x11]           // store byte '0'
    b 5f

3:  // convert loop: quotient + remainder without % instruction
    udiv x13, x9, x12         // x13 (quotient) = x9 / 10
    msub x14, x13, x12, x9    // x14 (remainder) = x9 - (x13(quotient) * 10)
    add x14, x14, #48         // convert to ASCII ('0' = 48)
    sub x11, x11, #1          // make space for one byte
    strb w14, [x11]           // store byte
    mov x9, x13               // x9 = quotient
    cbnz x9, 3b               // if x9 != 0, repeat (more digits)

5:  // maybe sign
    cbz w15, 6f               // If sign flag (w15) == 0, skip
    sub x11, x11, #1          // No? make space for '-'
    mov w14, #45              // '-'
    strb w14, [x11]

6:  // write number
    mov x0, #1                // x0 (1st argument): fd = stdout
    mov x1, x11               // x1 (2nd argument): buf = pointer to start of string
    sub x2, x10, x11          // x2 (3rd argument): len = end - start
    mov x8, #64               // x8 (syscall number): SYS_WRITE
    svc #0                    // execute the system call! >_<

    // write newline
    mov x0, #1
    adrp x1, newline
    add  x1, x1, :lo12:newline
    mov x2, #1
    mov x8, #64
    svc #0

    ldp x29, x30, [sp], 96
    ret

