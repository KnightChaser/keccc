// src/cgn.c

/**
 * Code generation in NASM x86-64 assembly
 *
 * NOTE: Use the following command to earn executable from generated asm code
 * $ nasm -f elf64 (output assembly path) -o out.o
 * $ gcc -no-pie out.o -o out
 * $ ./out
 */

#include "data.h"
#include "decl.h"
#include "defs.h"

static int freeRegisters[4];
static char *registerList[4] = {"r8", "r9", "r10", "r11"};

/**
 * freeAllRegisters - Marks all registers as free for allocation.
 */
void freeAllRegisters(void) {
    int registerCount = sizeof(freeRegisters) / sizeof(freeRegisters[0]);
    for (int i = 0; i < registerCount; i++) {
        // Mark all registers as free
        freeRegisters[i] = 1;
    }
}

/**
 * allocateRegister - Allocates a free register and returns its index.
 * Dies with an error if no registers are available.
 *
 * Returns: Index of the allocated register.
 */
static int allocateRegister(void) {
    int registerCount = sizeof(freeRegisters) / sizeof(freeRegisters[0]);
    for (int i = 0; i < registerCount; i++) {
        if (freeRegisters[i]) {
            freeRegisters[i] = 0; // Mark as used
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
 * @r: Index of the register to free.
 */
static void freeRegister(int r) {
    if (freeRegisters[r] == 1) {
        fprintf(stderr, "Error: Register %s is already free\n",
                registerList[r]);
        exit(1);
    }
    freeRegisters[r] = 1; // Mark as free
}

/**
 * cgpreamble - Outputs the assembly code preamble, including
 *              function prologues for main and printint.
 */
void cgpreamble() {
    freeAllRegisters();
    fputs("\tglobal\tmain\n"

          "\textern\tprintf\n"

          "\tsection\t.text\n"
          "LC0:\tdb\t\"%d\",10,0\n"

          "printint:\n"
          "\tpush\trbp\n"
          "\tmov\trbp, rsp\n"
          "\tsub\trsp, 16\n"
          "\tmov\t[rbp-4], edi\n"
          "\tmov\teax, [rbp-4]\n"
          "\tmov\tesi, eax\n"
          "\tlea	rdi, [rel LC0]\n"
          "\tmov	eax, 0\n"
          "\tcall	printf\n"
          "\tnop\n"
          "\tleave\n"
          "\tret\n"
          "\n"

          "main:\n"
          "\tpush\trbp\n"
          "\tmov	rbp, rsp\n",
          Outfile);
}

/**
 * cgpostamble - Outputs the assembly code postamble,
 *               including function epilogue for main.
 */
void cgpostamble() {
    fputs("\tmov	eax, 0\n"
          "\tpop	rbp\n"
          "\tret\n",
          Outfile);
}

/**
 * cgload - Loads an integer value into a register.
 *
 * @value: The integer value to load.
 *
 * Returns: Index of the register containing the loaded value.
 */
int cgload(int value) {
    int r = allocateRegister();
    fprintf(Outfile, "\tmov\t%s, %d\n", registerList[r], value);

    return r;
}

/**
 * cgadd - Generates code to add values in two registers.
 *
 * @r1: Index of the first register.
 * @r2: Index of the second register.
 *
 * Returns: Index of the register containing the result.
 */
int cgadd(int r1, int r2) {
    fprintf(Outfile, "\tadd\t%s, %s\n", registerList[r1], registerList[r2]);
    freeRegister(r2);

    return r1;
}

/**
 * cgsub - Generates code to subtract values in two registers.
 *
 * @r1: Index of the first register.
 * @r2: Index of the second register.
 *
 * Returns: Index of the register containing the result.
 */
int cgsub(int r1, int r2) {
    fprintf(Outfile, "\tsub\t%s, %s\n", registerList[r1], registerList[r2]);
    freeRegister(r2);

    return r1;
}

/**
 * cgmul - Generates code to multiply values in two registers.
 *
 * @r1: Index of the first register.
 * @r2: Index of the second register.
 *
 * Returns: Index of the register containing the result.
 */
int cgmul(int r1, int r2) {
    fprintf(Outfile, "\timul\t%s, %s\n", registerList[r1], registerList[r2]);
    freeRegister(r2);

    return r1;
}

/**
 * cgdiv - Generates code to divide values in two registers.
 *
 * @r1: Index of the dividend register.
 * @r2: Index of the divisor register.
 *
 * Returns: Index of the register containing the result (quotient).
 */
int cgdiv(int r1, int r2) {
    fprintf(Outfile, "\tmov\trax, %s\n", registerList[r1]);
    fprintf(Outfile, "\tcqo\n"); // Sign-extend rax into rdx:rax
    fprintf(Outfile, "\tidiv\t%s\n", registerList[r2]);
    fprintf(Outfile, "\tmov\t%s, rax\n", registerList[r1]);
    freeRegister(r2);

    return r1;
}

/**
 * cgprintint - Generates code to print an integer value from a register.
 *
 * @r: Index of the register containing the integer to print.
 */
void cgprintint(int r) {
    fprintf(Outfile, "\tmov\trdi, %s\n", registerList[r]);
    fprintf(Outfile, "\tcall\tprintint\n");
    freeRegister(r);
}
