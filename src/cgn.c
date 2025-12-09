// src/cgn.c

/**
 * NOTE:
 * Code generation in NASM x86-64 assembly
 * (Target-specific layer)
 *
 * NOTE: Use the following command to earn executable from generated asm code
 * $ nasm -f elf64 (output assembly path) -o out.o
 * $ gcc -no-pie out.o -o out
 * $ ./out
 */

#include "data.h"
#include "decl.h"

static int freeRegisters[4];
static char *qwordRegisterList[4] = {
    "r8",  // x64 general-purpose register #1
    "r9",  // x64 general-purpose register #2
    "r10", // x64 general-purpose register #3
    "r11"  // x64 general-purpose register #4
};
static char *byteRegisterList[4] = {
    "r8b",  // lower 8 bits of r8
    "r9b",  // lower 8 bits of r9
    "r10b", // lower 8 bits of r10
    "r11b"  //  lower 8 bits of r11
};

/**
 * nasmResetRegisterPool - Marks all registers as free for allocation.
 */
void nasmResetRegisterPool(void) {
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
                qwordRegisterList[r]);
        exit(1);
    }
    freeRegisters[r] = 1; // Mark as free
}

/**
 * nasmPreamble - Outputs the assembly code preamble, including
 *              function prologues for main and printint.
 */
void nasmPreamble() {
    nasmResetRegisterPool();
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
          "\n",
          Outfile);
}

/**
 * nasmFunctionPreamble - Outputs the assembly code function preamble.
 *
 * @functionName: The name of the function.
 */
void nasmFunctionPreamble(char *functionName) {
    fprintf(Outfile, "\tsection\t.text\n");
    fprintf(Outfile, "\tglobal\t%s\n", functionName);
    fprintf(Outfile, "%s:\n", functionName);
    fprintf(Outfile, "\tpush\trbp\n");
    fprintf(Outfile, "\tmov\trbp, rsp\n");
}

/**
 * nasmFunctionPostamble - Outputs the assembly code function postamble.
 */
void nasmFunctionPostamble() {
    // TODO: Return value from main
    fputs("\tmov eax, 0\n"
          "\tpop rbp\n"
          "\tret\n",
          Outfile);
}

/**
 * nasmPostamble - Outputs the assembly code postamble,
 *               including function epilogue for main.
 */
void nasmPostamble() {
    fputs("\tmov	eax, 0\n"
          "\tpop	rbp\n"
          "\tret\n",
          Outfile);
}

/**
 * nasmLoadImmediateInt - Generates code to load an integer constant into a
 * register.
 *
 * @value: The integer constant to load.
 *
 * Returns: Index of the register containing the loaded integer.
 */
int nasmLoadImmediateInt(int value) {
    int registerIndex = allocateRegister();

    fprintf(Outfile, "\tmov\t%s, %d\n", qwordRegisterList[registerIndex],
            value);
    return registerIndex;
}

/**
 * nasmLoadGlobalSymbol - Generates code to load a global symbol's value into a
 * register.
 *
 * @identifier: The name of the global symbol.
 *
 * Returns: Index of the register containing the loaded value.
 */
int nasmLoadGlobalSymbol(char *identifier) {
    int registerIndex = allocateRegister();

    fprintf(Outfile, "\tmov\t%s, [%s]\n", qwordRegisterList[registerIndex],
            identifier);
    return registerIndex;
}

/**
 * nasmStoreGlobalSymbol - Generates code to store a register's value into a
 * global symbol.
 *
 * @registerIndex: Index of the register containing the value to store.
 * @identifier: The name of the global symbol.
 *
 * Returns: Index of the register that was stored.
 */
int nasmStoreGlobalSymbol(int registerIndex, char *identifier) {
    fprintf(Outfile, "\tmov\t[%s], %s\n", identifier,
            qwordRegisterList[registerIndex]);
    return registerIndex;
}

/**
 * nasmDeclareCommonGlobal - Generates code to declare a global symbol.
 *
 * @symbol: The name of the global symbol.
 */
void nasmDeclareGlobalSymbol(char *symbol) {
    fprintf(Outfile, "\tcommon\t%s 8:8\n", symbol);
}

/**
 * nasmAddRegs - Generates code to add values in two registers.
 *
 * @r1: Index of the first register.
 * @r2: Index of the second register.
 *
 * Returns: Index of the register containing the result.
 */
int nasmAddRegs(int r1, int r2) {
    fprintf(Outfile, "\tadd\t%s, %s\n", qwordRegisterList[r1],
            qwordRegisterList[r2]);
    freeRegister(r2);

    return r1;
}

/**
 * nasmSubRegs - Generates code to subtract values in two registers.
 *
 * @r1: Index of the first register.
 * @r2: Index of the second register.
 *
 * Returns: Index of the register containing the result.
 */
int nasmSubRegs(int r1, int r2) {
    fprintf(Outfile, "\tsub\t%s, %s\n", qwordRegisterList[r1],
            qwordRegisterList[r2]);
    freeRegister(r2);

    return r1;
}

/**
 * nasmMulRegs - Generates code to multiply values in two registers.
 *
 * @r1: Index of the first register.
 * @r2: Index of the second register.
 *
 * Returns: Index of the register containing the result.
 */
int nasmMulRegs(int r1, int r2) {
    fprintf(Outfile, "\timul\t%s, %s\n", qwordRegisterList[r1],
            qwordRegisterList[r2]);
    freeRegister(r2);

    return r1;
}

/**
 * nasmDivRegsSigned - Generates code to divide values in two registers.
 *
 * @r1: Index of the dividend register.
 * @r2: Index of the divisor register.
 *
 * Returns: Index of the register containing the result (quotient).
 */
int nasmDivRegsSigned(int r1, int r2) {
    fprintf(Outfile, "\tmov\trax, %s\n", qwordRegisterList[r1]);
    fprintf(Outfile, "\tcqo\n"); // Sign-extend rax into rdx:rax
    fprintf(Outfile, "\tidiv\t%s\n", qwordRegisterList[r2]);
    fprintf(Outfile, "\tmov\t%s, rax\n", qwordRegisterList[r1]);
    freeRegister(r2);

    return r1;
}

/**
 * nasmPrintIntFromReg - Generates code to print an integer value from a
 * register.
 *
 * @r: Index of the register containing the integer to print.
 */
void nasmPrintIntFromReg(int r) {
    fprintf(Outfile, "\tmov\trdi, %s\n", qwordRegisterList[r]);
    fprintf(Outfile, "\tcall\tprintint\n");
    freeRegister(r);
}

/**
 * nasmCompareAndSet - Generates code to compare two registers and set a third
 * register based on the comparison result.
 *
 * @ASTop: The AST operation code representing the comparison.
 * @r1: Index of the first register.
 * @r2: Index of the second register.
 *
 * Returns: Index of the register containing the comparison result (0 or 1).
 */
int nasmCompareAndSet(int ASTop, int r1, int r2) {
    if (!((ASTop == A_EQ) || (ASTop == A_NE) || (ASTop == A_LT) ||
          (ASTop == A_LE) || (ASTop == A_GT) || (ASTop == A_GE))) {
        fprintf(stderr,
                "Error: Invalid AST operation %d in nasmCompareAndSet\n",
                ASTop);
        exit(1);
    }

    fprintf(Outfile, "\tcmp\t%s, %s\n", qwordRegisterList[r1],
            qwordRegisterList[r2]);

    // Set the lower 8 bits of r1 based on the comparison
    char *byteRegister = byteRegisterList[r2];
    switch (ASTop) {
    case A_EQ:
        fprintf(Outfile, "\tsete\t%s\n", byteRegister);
        break;
    case A_NE:
        fprintf(Outfile, "\tsetne\t%s\n", byteRegister);
        break;
    case A_LT:
        fprintf(Outfile, "\tsetl\t%s\n", byteRegister);
        break;
    case A_LE:
        fprintf(Outfile, "\tsetle\t%s\n", byteRegister);
        break;
    case A_GT:
        fprintf(Outfile, "\tsetg\t%s\n", byteRegister);
        break;
    case A_GE:
        fprintf(Outfile, "\tsetge\t%s\n", byteRegister);
        break;
    default:
        fprintf(stderr,
                "Error: Unknown AST operation %d in nasmCompareAndSet\n",
                ASTop);
        exit(1);
    }

    // Zero-extend the result to the full register
    fprintf(Outfile, "\tmovzx\t%s, %s\n", qwordRegisterList[r2], byteRegister);

    freeRegister(r1);

    return r2;
}

/**
 * nasmLabel - Outputs a label in the assembly code.
 *
 * @label: The label number to output.
 */
void nasmLabel(int label) { fprintf(Outfile, "L%d:\n", label); }

/**
 * nasmJump - Generates an unconditional jump to a label.
 *
 * @label: The label number to jump to.
 */
void nasmJump(int label) { fprintf(Outfile, "\tjmp\tL%d\n", label); }

/**
 * nasmCompareAndJump - Generates code to compare two registers and jump to a
 * label based on the comparison result.
 *
 * @ASTop: The AST operation code representing the comparison.
 * @r1: Index of the first register.
 * @r2: Index of the second register.
 * @label: The label number to jump to if the comparison is true.
 *
 * Returns: NOREG (indicating no register is returned).
 */
int nasmCompareAndJump(int ASTop, int r1, int r2, int label) {
    if (!((ASTop == A_EQ) || (ASTop == A_NE) || (ASTop == A_LT) ||
          (ASTop == A_LE) || (ASTop == A_GT) || (ASTop == A_GE))) {
        fprintf(stderr,
                "Error: Invalid AST operation %d in nasmCompareAndJump\n",
                ASTop);
        exit(1);
    }

    fprintf(Outfile, "\tcmp\t%s, %s\n", qwordRegisterList[r1],
            qwordRegisterList[r2]);

    // WARNING:
    // Jump when the condition is FALSE
    switch (ASTop) {
    case A_EQ:
        // !=
        fprintf(Outfile, "\tjne\tL%d\n", label);
        break;
    case A_NE:
        // ==
        fprintf(Outfile, "\tje\tL%d\n", label);
        break;
    case A_LT:
        // >=
        fprintf(Outfile, "\tjge\tL%d\n", label);
        break;
    case A_LE:
        // >
        fprintf(Outfile, "\tjg\tL%d\n", label);
        break;
    case A_GT:
        // <=
        fprintf(Outfile, "\tjle\tL%d\n", label);
        break;
    case A_GE:
        // <
        fprintf(Outfile, "\tjl\tL%d\n", label);
        break;
    default:
        fprintf(stderr,
                "Error: Unknown AST operation %d in nasmCompareAndJump\n",
                ASTop);
        exit(1);
    }

    nasmResetRegisterPool();

    return NOREG;
}
