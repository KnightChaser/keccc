// src/cgn/nasm/cgn_stmt.c

#include "cgn/nasm/cgn_regs.h"
#include "data.h"
#include "decl.h"
#include "defs.h"

/**
 * NOTE:
 * Code generation in NASM x86-64 assembly
 * (Target-specific layer)
 * Statement level jobs such as control flow, preambles/postambles, and printing
 *
 * NOTE: Use the following command to earn executable from generated asm code
 * $ nasm -f elf64 (output assembly path) -o out.o
 * $ gcc -no-pie out.o -o out
 * $ ./out
 */

/**
 * nasmPreamble - Print the assembly code preamble.
 */
void nasmPreamble() {
    nasmResetRegisterPool();

    // printint is provided by custom runtime object
    fputs("\textern\tprintint\n", Outfile);
    fputs("\textern\tprintchar\n", Outfile);
    fputs("\textern\tprintstring\n", Outfile);

    fputs("\tsection\t.text\n", Outfile);
}

/**
 * nasmFunctionCall - Generates code to call a function with an argument in a
 * register.
 *
 * @param registerIndex Index of the register containing the argument.
 * @param functionSymbolId The function's symbol table ID.
 *
 * @return Index of the register containing the function's return value.
 */
int nasmFunctionCall(int registerIndex, int functionSymbolId) {
    int outRegister = allocateRegister();
    fprintf(Outfile, "\tmov\trdi, %s\n", qwordRegisterList[registerIndex]);
    fprintf(Outfile, "\tcall\t%s\n", GlobalSymbolTable[functionSymbolId].name);
    fprintf(Outfile, "\tmov\t%s, rax\n", qwordRegisterList[outRegister]);
    freeRegister(registerIndex);

    return outRegister;
}

/**
 * nasmFunctionPreamble - Outputs the assembly code function preamble.
 *
 * @param id The function's symbol table ID.
 */
void nasmFunctionPreamble(int id) {
    char *functionName = GlobalSymbolTable[id].name;
    fprintf(Outfile, "\tsection\t.text\n");
    fprintf(Outfile, "\tglobal\t%s\n", functionName);
    fprintf(Outfile, "%s:\n", functionName);
    fprintf(Outfile, "\tpush\trbp\n");
    fprintf(Outfile, "\tmov\trbp, rsp\n");
}

/**
 * nasmReturnFromFunction - Generates code to return a value from a function.
 * (After moving the return value to rax, jumps to function end label)
 *
 * @param reg Index of the register containing the return value.
 * @param id The function's symbol table ID.
 */
void nasmReturnFromFunction(int reg, int id) {
    int primitiveType = GlobalSymbolTable[id].primitiveType;

    switch (primitiveType) {
    case P_CHAR:
        fprintf(Outfile, "\tmovzx\teax, %s\n", byteRegisterList[reg]);
        break;
    case P_INT:
        fprintf(Outfile, "\tmov\teax, %s\n", dwordRegisterList[reg]);
        break;
    case P_LONG:
        fprintf(Outfile, "\tmov\trax, %s\n", qwordRegisterList[reg]);
        break;
    default:
        logFatald("Error: Unsupported primitive type in nasmReturnFromFunction",
                  primitiveType);
    }

    // After moving the return value to rax, jump to function end label
    nasmJump(GlobalSymbolTable[id].endLabel);
}

/**
 * nasmFunctionPostamble - Outputs the assembly code function postamble.
 *
 * @param id The function's symbol table ID.
 */
void nasmFunctionPostamble(int id) {
    nasmLabel(GlobalSymbolTable[id].endLabel);
    // TODO: Return value from main
    fputs("\tpop\trbp\n"
          "\tret\n",
          Outfile);
}

/**
 * nasmPostamble - Outputs the assembly code postamble,
 *               including function epilogue for main.
 */
void nasmPostamble() {
    fputs("\tmov\teax, 0\n"
          "\tpop\trbp\n"
          "\tret\n",
          Outfile);
}

/**
 * nasmLabel - Outputs a label in the assembly code.
 *
 * @param label The label number to output.
 */
void nasmLabel(int label) { fprintf(Outfile, "L%d:\n", label); }

/**
 * nasmJump - Generates an unconditional jump to a label.
 *
 * @param label The label number to jump to.
 */
void nasmJump(int label) { fprintf(Outfile, "\tjmp\tL%d\n", label); }

/**
 * nasmCompareAndJump - Generates code to compare two registers and jump to a
 * label based on the comparison result.
 *
 * @param ASTop The AST operation code representing the comparison.
 * @param r1 Index of the first register.
 * @param r2 Index of the second register.
 * @param label The label number to jump to if the comparison is true.
 *
 * @return NOREG (indicating no register is returned).
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
