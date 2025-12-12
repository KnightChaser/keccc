// src/cgn/aarch64/cgn_regs.c

#include "cgn/aarch64/cgn_regs.h"
#include "data.h"
#include "decl.h"
#include "defs.h"

// 4 general-purpose scratch registers
// Using caller-saved x9–x12 (AAPCS64: caller-saved, so we don't
// have to preserve them across calls; our codegen doesn’t assume
// anything survives a call except x0 return).
static bool aarch64FreeRegisters[8];

char *aarch64QwordRegisterList[8] = {"x9", // 64-bit GPR
                                     "x10", "x11", "x12", "x13",
                                     "x14", "x15", "x16"};

char *aarch64DwordRegisterList[8] = {"w9", // low 32 bits of x9
                                     "w10", "w11", "w12", "w13",
                                     "w14", "w15", "w16"};

// WARNING:
// We don’t really have 8-bit registers on AArch64; we’ll treat
// “byte” registers as the 32-bit view (wN). Instructions like
// `ldrb`/`strb` take a w-register.
char *aarch64ByteRegisterList[8] = {"w9",  "w10", "w11", "w12",
                                    "w13", "w14", "w15", "w16"};

/**
 * aarch64ResetRegisterPool - Reset the aarch64 register pool, marking all
 * registers as free.
 */
void aarch64ResetRegisterPool(void) {
    int n = sizeof(aarch64FreeRegisters) / sizeof(aarch64FreeRegisters[0]);
    for (int i = 0; i < n; i++) {
        aarch64FreeRegisters[i] = true;
    }
}

/**
 * aarch64AllocateRegister - Allocate a free aarch64 register from the pool.
 *
 * @return The index of the allocated register in the register list.
 */
int aarch64AllocateRegister(void) {
    int n = sizeof(aarch64FreeRegisters) / sizeof(aarch64FreeRegisters[0]);
    for (int i = 0; i < n; i++) {
        if (aarch64FreeRegisters[i]) {
            aarch64FreeRegisters[i] = false;
            return i;
        }
    }

    fprintf(stderr, "Error: No free aarch64 registers available\n");
    exit(1);
}

/**
 * aarch64FreeRegister - Free a previously allocated aarch64 register.
 *
 * @param r The index of the register to free.
 */
void aarch64FreeRegister(int r) {
    if (aarch64FreeRegisters[r]) {
        fprintf(stderr, "Error: aarch64 register %s is already free\n",
                aarch64QwordRegisterList[r]);
        exit(1);
    }
    aarch64FreeRegisters[r] = true;
}
