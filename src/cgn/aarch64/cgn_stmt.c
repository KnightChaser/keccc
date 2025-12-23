// src/cgn/aarch64/cgn_stmt.c

/**
 * NOTE:
 * AArch64 GNU as-style backend
 * Statement-level codegen: preamble, labels, jumps, calls, returns, printing.
 */

#include "cgn/aarch64/cgn_regs.h"
#include "data.h"
#include "decl.h"
#include "defs.h"

// Position of next local variable relative to frame pointer (x29).
// We track local allocation size as a positive number of bytes.
static int localOffset;
static int stackOffset;

void aarch64ResetLocalOffset(void) {
    localOffset = 0;
    stackOffset = 0;
}

int aarch64GetLocalOffset(int type, bool isFunctionParameter) {
    (void)isFunctionParameter;

    int size = aarch64GetPrimitiveTypeSize(type);
    if (size <= 0) {
        logFatald("Bad type size in aarch64GetLocalOffset: ", type);
    }

    // Keep at least 4-byte alignment for locals, like the NASM backend.
    localOffset += (size > 4) ? size : 4;
    return -localOffset;
}

/**
 * aarch64Preamble - Print the assembly code preamble.
 */
void aarch64Preamble(void) {
    aarch64ResetRegisterPool();

    fputs("\t.text\n", Outfile);
    fputs("\t.extern\tprintint\n", Outfile);
    fputs("\t.extern\tprintchar\n", Outfile);
    fputs("\t.extern\tprintstring\n", Outfile);
}

/**
 * aarch64Postamble - Outputs the assembly code postamble,
 * including function epilogue for main.
 *
 * NOTE:
 * Nothing special yet. Currently, functions each have their own epilogue.
 */
void aarch64Postamble(void) {}

/**
 * aarch64FunctionCall - Generates code to call a function with an argument in a
 * register.
 *
 * @param r Index of the register containing the argument.
 * @param functionSymbolId The function's symbol table ID.
 *
 * @return Index of the register containing the function's return value.
 */
int aarch64FunctionCall(int r, int functionSymbolId) {
    int out = aarch64AllocateRegister();

    fprintf(Outfile, "\tmov\tx0, %s\n", aarch64QwordRegisterList[r]);
    fprintf(Outfile, "\tbl\t%s\n", SymbolTable[functionSymbolId].name);
    fprintf(Outfile, "\tmov\t%s, x0\n", aarch64QwordRegisterList[out]);

    aarch64FreeRegister(r);
    return out;
}

/**
 * aarch64FunctionPreamble - Outputs the assembly code function preamble.
 *
 * @param id The function's symbol table ID.
 */
void aarch64FunctionPreamble(int id) {
    char *functionName = SymbolTable[id].name;

    // Allocate locals already recorded via aarch64GetLocalOffset().
    // Keep 16-byte stack alignment.
    stackOffset = (localOffset + 15) & ~15;

    fprintf(Outfile, "\t.text\n");
    fprintf(Outfile, "\t.global\t%s\n", functionName);
    fprintf(Outfile, "%s:\n", functionName);
    fprintf(Outfile, "\tstp\tx29, x30, [sp, -16]!\n");
    fprintf(Outfile, "\tmov\tx29, sp\n");

    if (stackOffset > 0) {
        fprintf(Outfile, "\tsub\tsp, sp, #%d\n", stackOffset);
    }
}

/**
 * aarch64ReturnFromFunction - Generates code to return a value from a function.
 *
 * @param reg Index of the register containing the return value.
 * @param id The function's symbol table ID.
 */
void aarch64ReturnFromFunction(int reg, int id) {
    int primitiveType = SymbolTable[id].primitiveType;

    switch (primitiveType) {
    case P_CHAR:
        fprintf(Outfile, "\tmov\tw0, %s\n", aarch64DwordRegisterList[reg]);
        break;
    case P_INT:
        fprintf(Outfile, "\tmov\tw0, %s\n", aarch64DwordRegisterList[reg]);
        break;
    case P_LONG:
        fprintf(Outfile, "\tmov\tx0, %s\n", aarch64QwordRegisterList[reg]);
        break;
    default:
        logFatald(
            "Error: Unsupported primitive type in aarch64ReturnFromFunction: ",
            primitiveType);
    }

    // After moving return value to x0, branch to function end label.
    fprintf(Outfile, "\tb\tL%d\n", SymbolTable[id].endLabel);
}

/**
 * aarch64FunctionPostamble - Outputs the assembly code function postamble.
 *
 * @param id The function's symbol table ID.
 */
void aarch64FunctionPostamble(int id) {
    (void)id;
    // end label is emitted by aarch64Label from gen.c
    // (we’ll call aarch64Label(SymbolTable[id].endLabel) there)
    // and then we output epilogue:
    aarch64Label(SymbolTable[id].endLabel);
    // Discard local stack space.
    fprintf(Outfile, "\tmov\tsp, x29\n");
    fputs("\tldp\tx29, x30, [sp], 16\n"
          "\tret\n",
          Outfile);
}

/**
 * aarch64Label - Outputs a label in the assembly code.
 *
 * @param label The label number to output.
 */
void aarch64Label(int label) { fprintf(Outfile, "L%d:\n", label); }

/**
 * aarch64Jump - Generates an unconditional jump to a label.
 *
 * @param label The label number to jump to.
 */
void aarch64Jump(int label) { fprintf(Outfile, "\tb\tL%d\n", label); }

/**
 * aarch64CompareAndJump - Generates code to compare two registers and jump to a
 * label based on the comparison result.
 *
 * @param ASTop The AST operation code representing the comparison.
 * @param r1 Index of the first register.
 * @param r2 Index of the second register.
 * @param label The label number to jump to if the comparison is true.
 *
 * @return NOREG (indicating no register is returned).
 */
int aarch64CompareAndJump(int ASTop, int r1, int r2, int label) {
    if (!((ASTop == A_EQ) || (ASTop == A_NE) || (ASTop == A_LT) ||
          (ASTop == A_LE) || (ASTop == A_GT) || (ASTop == A_GE))) {
        fprintf(stderr,
                "Error: Invalid AST operation %d in aarch64CompareAndJump\n",
                ASTop);
        exit(1);
    }

    fprintf(Outfile, "\tcmp\t%s, %s\n", aarch64QwordRegisterList[r1],
            aarch64QwordRegisterList[r2]);

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
                "Error: Unknown AST operation %d in aarch64CompareAndJump\n",
                ASTop);
        exit(1);
    }

    fprintf(Outfile, "\t%s\tL%d\n", branch, label);

    aarch64ResetRegisterPool();
    return NOREG;
}
