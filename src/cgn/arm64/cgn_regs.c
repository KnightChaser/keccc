// src/cgn/arm64/cgn_regs.c

#include "cgn/arm64/cgn_regs.h"
#include "data.h"
#include "decl.h"
#include "defs.h"

// 4 general-purpose scratch registers
// Using caller-saved x9–x12 (AAPCS64: caller-saved, so we don't
// have to preserve them across calls; our codegen doesn’t assume
// anything survives a call except x0 return).
static bool arm64FreeRegisters[8];

char *arm64QwordRegisterList[8] = {"x9", // 64-bit GPR
                                   "x10", "x11", "x12", "x13",
                                   "x14", "x15", "x16"};

char *arm64DwordRegisterList[8] = {"w9", // low 32 bits of x9
                                   "w10", "w11", "w12", "w13",
                                   "w14", "w15", "w16"};

// WARNING:
// We don’t really have 8-bit registers on AArch64; we’ll treat
// “byte” registers as the 32-bit view (wN). Instructions like
// `ldrb`/`strb` take a w-register.
char *arm64ByteRegisterList[8] = {"w9",  "w10", "w11", "w12",
                                  "w13", "w14", "w15", "w16"};

/**
 * arm64ResetRegisterPool - Reset the ARM64 register pool, marking all registers
 * as free.
 */
void arm64ResetRegisterPool(void) {
    int n = sizeof(arm64FreeRegisters) / sizeof(arm64FreeRegisters[0]);
    for (int i = 0; i < n; i++) {
        arm64FreeRegisters[i] = true;
    }
}

/**
 * arm64AllocateRegister - Allocate a free ARM64 register from the pool.
 *
 * @return The index of the allocated register in the register list.
 */
int arm64AllocateRegister(void) {
    int n = sizeof(arm64FreeRegisters) / sizeof(arm64FreeRegisters[0]);
    for (int i = 0; i < n; i++) {
        if (arm64FreeRegisters[i]) {
            arm64FreeRegisters[i] = false;
            return i;
        }
    }

    fprintf(stderr, "Error: No free ARM64 registers available\n");
    exit(1);
}

/**
 * arm64FreeRegister - Free a previously allocated ARM64 register.
 *
 * @param r The index of the register to free.
 */
void arm64FreeRegister(int r) {
    if (arm64FreeRegisters[r]) {
        fprintf(stderr, "Error: ARM64 register %s is already free\n",
                arm64QwordRegisterList[r]);
        exit(1);
    }
    arm64FreeRegisters[r] = true;
}
