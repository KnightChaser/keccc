; src/rt/x86_64/printstring.asm

; Linux x86_64
; long printstring(const char *s)
; arg: rdi = s (NUL-terminated string)
; ret; rax = bytes written

global printstring

%define MAXLEN 65536

section .text
printstring:
    ; Treat printing NULL as doing nothing
    test    rdi, rdi
    jz      .ret0

    ; Save start pointer for write()
    mov     rsi, rdi

    ; Find NUL terminator
    cld                     ; make sure DF=0 (forward scan)
    mov     rcx, MAXLEN     ;
    xor     eax, eax        ; AL=0 (byte to search for)
    repne   scasb           ; while (rcx != 0 && *rdi != 0) rdi++, rcx--
                            ; (Scan Compare Al with byte at [rdi], then advance rdi)

    ; If ZF=1, we found NUL. Length = bytes scanned "including NUL" = MAXLEN - rcx
    ; Then exclude NUL, len = (MAXLEN - rcx) - 1
    ; If ZF=0, the string didn't end within MAXLEN bytes, so we just use MAXLEN as length
    jnz     .no_nul

    ; Calculate length excluding NUL
    mov     rdx, MAXLEN
    sub     rdx, rcx
    dec     rdx              ; exclude NUL
    jmp     .do_write

.no_nul:
    mov     rdx, MAXLEN      ; use MAXLEN as length

.do_write:
    ; write(stdout, rsi, rdx)
    mov     eax, 1           ; __NR_write
    mov     edi, 1           ; fd=stdout
                             ; buf=start pointer
    mov     rdx, rdx         ; count=length
    syscall

.ret0:
    xor     rax, rax         ; return 0 bytes written
    ret

