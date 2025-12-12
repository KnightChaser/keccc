; src/rt/x86_64/start.asm

; Linux x86_64
; Exports: _start
; Calls: main
; Exits with main's return value

global _start
extern main

section .text
_start:
    sub   rsp, 8          ; Align stack to 16 bytes
    call  main           ; Call main function
    add   rsp, 8          ; Restore stack

    ; exit (status = eax)
    mov   rdi, rax        ; Move return value of main to rdi (first argument to exit)
    mov   rax, 60         ; syscall: exit
    syscall             ; Invoke kernel
