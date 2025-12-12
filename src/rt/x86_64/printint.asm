; src/rt/x86_64/printint.asm

; Linux x86_64
; long printint(long x)
; arg:  rdi = x
; clobbers: rax, rcx, rdx, rsi, r8-r11
; preserves: rbx, rbp, r12-r15 (we don't touch callee-saved regs except rbp)

global printint

section .rodata
minstr: db "-9223372036854775808", 10
minlen: equ $-minstr
nl:     db 10

section .text
printint:
    push    rbp
    mov     rbp, rsp
    sub     rsp, 64             ; scratch buffer (keeps alignment)

    ; Special-case INT64_MIN (can't negate it)
    mov     rax, 0x8000000000000000
    cmp     rdi, rax
    jne     .not_min

    ; write(stdout, minstr, minlen)
    ; = write(1, "-9223372036854775808\n", 21)
    mov     eax, 1              ; __NR_write
    mov     edi, 1              ; fd=stdout
    lea     rsi, [rel minstr]
    mov     edx, minlen
    syscall
    leave
    ret

.not_min:
    ; sign flag in r8d: 0 or 1
    xor     r8d, r8d

    mov     rax, rdi            ; working value (signed)
    test    rax, rax
    jge     .abs_ready
    neg     rax
    mov     r8d, 1              ; negative

.abs_ready:

    ; Build digits backwards into [rbp-1 .. rbp-32]
    lea     r10, [rbp-1]        ; end ptr (one past last byte we use)
    mov     r11, r10            ; current ptr

    ; Handle 0
    test    rax, rax
    jne     .convert
    dec     r11
    mov     byte [r11], '0'
    jmp     .maybe_sign

.convert:
    mov     r9, 10              ; divisor

.loop:
    xor     rdx, rdx
    div     r9                  ; unsigned: rdx:rax / 10 -> rax=quot, rdx=rem
    add     dl, '0'             ; convert to ASCII (e.g. 3 -> '3')
    dec     r11
    mov     [r11], dl
    test    rax, rax
    jne     .loop

.maybe_sign:
    test    r8d, r8d
    jz      .write_num
    dec     r11
    mov     byte [r11], '-'

.write_num:
    ; write(stdout, start=r11, len=end-r11)
    mov     eax, 1              ; __NR_write
    mov     edi, 1              ; stdout
    mov     rsi, r11
    mov     rdx, r10
    sub     rdx, r11
    syscall

    ; write newline
    mov     eax, 1
    mov     edi, 1
    lea     rsi, [rel nl]
    mov     edx, 1
    syscall

    leave
    ret

