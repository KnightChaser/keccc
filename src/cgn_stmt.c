// src/cgn_stmt.c

#include "data.h"
#include "decl.h"
#include "defs.h"
#include "cgn_regs.h"

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
