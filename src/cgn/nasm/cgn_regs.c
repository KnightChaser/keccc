// src/cgn/nasm/cgn_regs.c

#include "cgn/nasm/cgn_regs.h"
#include "data.h"
#include "decl.h"

/**
 * NOTE:
 * Code generation in NASM x86-64 assembly
 * (Target-specific layer)
 * Register management and register name tables
 *
 * NOTE: Use the following command to earn executable from generated asm code
 * $ nasm -f elf64 (output assembly path) -o out.o
 * $ gcc -no-pie out.o -o out
 * $ ./out
 */

// TODO:
// Better name for this variable is required,
// such as "Xth register is currently not used"
#define NUMFREEREGISTERS 4
static bool freeRegisters[NUMFREEREGISTERS] = {true, true, true, true};

char *qwordRegisterList[] = {
    "r8",  // x64 general-purpose register #1
    "r9",  // x64 general-purpose register #2
    "r10", // x64 general-purpose register #3
    "r11"  // x64 general-purpose register #4
};
char *dwordRegisterList[] = {
    "r8d",  // lower 32 bits of r8
    "r9d",  // lower 32 bits of r9
    "r10d", // lower 32 bits of r10
    "r11d"  // lower 32 bits of r11
};
char *byteRegisterList[] = {
    "r8b",  // lower 8 bits of r8
    "r9b",  // lower 8 bits of r9
    "r10b", // lower 8 bits of r10
    "r11b"  // lower 8 bits of r11
};

/**
 * nasmResetRegisterPool - Marks all registers as free for allocation.
 */
void nasmResetRegisterPool(void) {
    for (int i = 0; i < NUMFREEREGISTERS; i++) {
        // Mark all registers as free
        freeRegisters[i] = true;
    }
}

/**
 * allocateRegister - Allocates a free register and returns its index.
 * Dies with an error if no registers are available.
 *
 * @return Index of the allocated register.
 */
int allocateRegister(void) {
    for (int i = 0; i < NUMFREEREGISTERS; i++) {
        if (freeRegisters[i]) {
            freeRegisters[i] = false; // Mark as used
            return i;
        }
    }

    fprintf(stderr, "Error: No free registers available\n");
    exit(1);
}

/**
 * freeRegister - Frees the register at the given index.
 * Dies with an error if the register is already free.
 *
 * @param r Index of the register to free.
 */
void freeRegister(int r) {
    if (freeRegisters[r] == 1) {
        fprintf(stderr, "Error: Register %s is already free\n",
                qwordRegisterList[r]);
        exit(1);
    }
    freeRegisters[r] = 1; // Mark as free
}
