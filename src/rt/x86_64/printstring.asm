; src/rt/x86_64/printstring.asm

; Linux x86_64
; long printstring(const char *s)
; arg: rdi = s (NUL-terminated string)
; ret; rax = bytes written

global printstring

section .text
printstring:
    ; Reserve 8 bytes so we can take address
    sub     rsp, 8

    ; Treat printing NULL as doing nothing
    test    rdi, rdi
    jz      .ret0

    ; Save start pointer for write()
    mov     rsi, rdi

    ; Find NUL terminator
    cld                     ; make sure DF=0 (forward scan)
    mov     rcx, -1         ; "infinite" count
    xor     eax, eax        ; AL=0 (byte to search for)
    repne   scasb           ; scan for AL (0) starting at [rdi]
                            ; (Scan Compare Al with byte at [rdi], then advance rdi)

    ; After sacn:
    ; - RCX = -1 - N, where N = bytes scanned including NUL
    not     rcx             ; RCX = N
    dec     rcx             ; RCX = N-1 (length of string)
    mov     rdx, rcx        ; rdx = count

    ; write(stdout, s, len)
    mov     eax, 1          ; __NR_write
    mov     edi, 1          ; fd = 1 (stdout)
                            ; rsi = (already) buf
                            ; rdx = (already) len
    syscall

    add     rsp, 8
    ret

.ret0:
    add     rsp, 8
    mov     rax, 0
    ret
