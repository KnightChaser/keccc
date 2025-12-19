// src/rt/aarch64/printstring.s

// Linux aarch64
// long printstring(const char *s)
//   arg: x0 = s (NUL-terminated, but we cap scan/print)
//   ret: x0 = bytes written (or -errno from syscall)

    .global printstring
    .text

    .equ MAXLEN, 65536

printstring:
    // NULL => print nothing
    cbz    x0, .ret0

    // Keep original pointer in x1 for write()
    mov     x1, x0          // x1 = start
    mov     x4, x0          // x4 = cursor
    mov     x3, #MAXLEN     // x3 = remaining count

.scan_loop:
    ldrb    w5, [x4]        // load byte at cursor
    cbz     w5, .found_nul  // if byte == 0, done (cursor points at NUL)
    add     x4, x4, #1      // cursor++
    subs    x3, x3, #1      // remaining--
    b.ne    .scan_loop      // keep going if remaining != 0

    // No NUL found within MAXLEN => len = MAXLEN
    mov     x2, #MAXLEN
    b       .do_write

.found_nul:
    // len = cursor - start (excludes NUL, because cursor points at NUL)
    sub     x2, x4, x1

.do_write:
    // write(1, start, len)
    mov     x0, #1          // fd = stdout
    // x1 = buf
    // x2 = count
    mov     x8, #64         // __NR_write on Linux AArch64
    svc     #0              // syscall
    ret

.ret0:
    mov     x0, #0
    ret
