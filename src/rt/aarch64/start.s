// src/rt/aarch64/start.asm

// Linux aarch64
// Exports: _start
// Calls: main
// Exits with main's return value

    .text
    .global _start
    .extern main

_start:
    bl    main            // w0 = return value
    mov   x8, #93         // __NR_exit
    svc   #0

