// src/cgn/arm64/cgn_stmt.c

/**
 * NOTE:
 * AArch64 GNU as-style backend
 * Statement-level codegen: preamble, labels, jumps, calls, returns, printing.
 */

#include "cgn/arm64/cgn_regs.h"
#include "data.h"
#include "decl.h"
#include "defs.h"

/**
 * arm64Preamble - Print the assembly code preamble.
 */
void arm64Preamble(void) {
    arm64ResetRegisterPool();

    // We'll layout like:
    //   .text
    //   .global printint
    //   .extern printf
    //   .section .rodata
    // fmtint: .string "%ld\n"
    //   .text
    // printint:
    //   prologue, move arg, call printf, epilogue

    fputs("\t.text\n", Outfile);
    fputs("\t.global\tprintint\n", Outfile);
    fputs("\t.extern\tprintf\n", Outfile);
    fputs("\t.section\t.rodata\n", Outfile);

    fputs("fmtint:\n", Outfile);
    fputs("\t.string\t\"%ld\\n\"\n", Outfile);
    fputs("\t.text\n", Outfile);

    fputs("printint:\n"
          "\tstp\tx29, x30, [sp, -16]!\n"
          "\tmov\tx29, sp\n"
          "\tmov\tx1, x0\n"
          "\tadrp\tx0, fmtint\n"
          "\tadd\tx0, x0, :lo12:fmtint\n"
          "\tbl\tprintf\n"
          "\tldp\tx29, x30, [sp], 16\n"
          "\tret\n",
          Outfile);
}

/**
 * arm64Postamble - Outputs the assembly code postamble,
 * including function epilogue for main.
 *
 * NOTE:
 * Nothing special yet. Currently, functions each have their own epilogue.
 */
void arm64Postamble(void) {}

/**
 * arm64FunctionCall - Generates code to call a function with an argument in a
 * register.
 *
 * @r: Index of the register containing the argument.
 * @functionSymbolId: The function's symbol table ID.
 *
 * Returns: Index of the register containing the function's return value.
 */
int arm64FunctionCall(int r, int functionSymbolId) {
    int out = arm64AllocateRegister();

    fprintf(Outfile, "\tmov\tx0, %s\n", arm64QwordRegisterList[r]);
    fprintf(Outfile, "\tbl\t%s\n", GlobalSymbolTable[functionSymbolId].name);
    fprintf(Outfile, "\tmov\t%s, x0\n", arm64QwordRegisterList[out]);

    arm64FreeRegister(r);
    return out;
}

/**
 * arm64FunctionPreamble - Outputs the assembly code function preamble.
 *
 * @id: The function's symbol table ID.
 */
void arm64FunctionPreamble(int id) {
    char *functionName = GlobalSymbolTable[id].name;

    fprintf(Outfile, "\t.text\n");
    fprintf(Outfile, "\t.global\t%s\n", functionName);
    fprintf(Outfile, "%s:\n", functionName);
    fprintf(Outfile, "\tstp\tx29, x30, [sp, -16]!\n");
    fprintf(Outfile, "\tmov\tx29, sp\n");
}

/**
 * arm64ReturnFromFunction - Generates code to return a value from a function.
 *
 * @reg: Index of the register containing the return value.
 * @id: The function's symbol table ID.
 */
void arm64ReturnFromFunction(int reg, int id) {
    int primitiveType = GlobalSymbolTable[id].primitiveType;

    switch (primitiveType) {
    case P_CHAR:
        fprintf(Outfile, "\tmov\tw0, %s\n", arm64DwordRegisterList[reg]);
        break;
    case P_INT:
        fprintf(Outfile, "\tmov\tw0, %s\n", arm64DwordRegisterList[reg]);
        break;
    case P_LONG:
        fprintf(Outfile, "\tmov\tx0, %s\n", arm64QwordRegisterList[reg]);
        break;
    default:
        logFatald(
            "Error: Unsupported primitive type in arm64ReturnFromFunction: ",
            primitiveType);
    }

    // After moving return value to x0, branch to function end label.
    fprintf(Outfile, "\tb\tL%d\n", GlobalSymbolTable[id].endLabel);
}

/**
 * arm64FunctionPostamble - Outputs the assembly code function postamble.
 *
 * @id: The function's symbol table ID.
 */
void arm64FunctionPostamble(int id) {
    (void)id;
    // end label is emitted by arm64Label from gen.c
    // (we’ll call arm64Label(GlobalSymbolTable[id].endLabel) there)
    // and then we output epilogue:
    arm64Label(GlobalSymbolTable[id].endLabel);
    fputs("\tldp\tx29, x30, [sp], 16\n"
          "\tret\n",
          Outfile);
}

/**
 * arm64PrintIntFromReg - Generates code to print an integer value from a
 * register.
 *
 * @r: Index of the register containing the integer to print.
 */
void arm64PrintIntFromReg(int r) {
    fprintf(Outfile, "\tmov\tx0, %s\n", arm64QwordRegisterList[r]);
    fprintf(Outfile, "\tbl\tprintint\n");
    arm64FreeRegister(r);
}

/**
 * arm64Label - Outputs a label in the assembly code.
 *
 * @label: The label number to output.
 */
void arm64Label(int label) { fprintf(Outfile, "L%d:\n", label); }

/**
 * arm64Jump - Generates an unconditional jump to a label.
 *
 * @label: The label number to jump to.
 */
void arm64Jump(int label) { fprintf(Outfile, "\tb\tL%d\n", label); }

/**
 * arm64CompareAndJump - Generates code to compare two registers and jump to a
 * label based on the comparison result.
 *
 * @ASTop: The AST operation code representing the comparison.
 * @r1: Index of the first register.
 * @r2: Index of the second register.
 * @label: The label number to jump to if the comparison is true.
 *
 * Returns: NOREG (indicating no register is returned).
 */
int arm64CompareAndJump(int ASTop, int r1, int r2, int label) {
    if (!((ASTop == A_EQ) || (ASTop == A_NE) || (ASTop == A_LT) ||
          (ASTop == A_LE) || (ASTop == A_GT) || (ASTop == A_GE))) {
        fprintf(stderr,
                "Error: Invalid AST operation %d in arm64CompareAndJump\n",
                ASTop);
        exit(1);
    }

    fprintf(Outfile, "\tcmp\t%s, %s\n", arm64QwordRegisterList[r1],
            arm64QwordRegisterList[r2]);

    const char *branch = NULL;
    // We invert condition, same as NASM cgn_* code:
    // jump when the condition is FALSE.
    switch (ASTop) {
    case A_EQ:
        branch = "bne";
        break; // !=
    case A_NE:
        branch = "beq";
        break; // ==
    case A_LT:
        branch = "bge";
        break; // >=
    case A_LE:
        branch = "bgt";
        break; // >
    case A_GT:
        branch = "ble";
        break; // <=
    case A_GE:
        branch = "blt";
        break; // <
    default:
        fprintf(stderr,
                "Error: Unknown AST operation %d in arm64CompareAndJump\n",
                ASTop);
        exit(1);
    }

    fprintf(Outfile, "\t%s\tL%d\n", branch, label);

    arm64ResetRegisterPool();
    return NOREG;
}
