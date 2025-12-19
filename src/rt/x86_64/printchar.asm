; src/rt/x86_64/printchar.asm

; Linux x86_64
; long printchar(long x)
; arg:  rdi = x

global printchar

section .text
printchar:
    ; Reserve 8 bytes so we can take address
    sub     rsp, 8

    ; (char)(x & Ox7F)
    mov     rax, rdi
    and     rax, 0x7F
    mov     byte [rsp], al ; store the single output byte

    ; write(stdout, rsp, 1)
    mov     eax, 1              ; __NR_write
    mov     edi, 1              ; fd=stdout
    lea     rsi, [rsp]          ; buf=rsp
    mov     edx, 1              ; count=1
    syscall

    add     rsp, 8
    ret
