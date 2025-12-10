// src/cgn_expr.c

#include "data.h"
#include "decl.h"
#include "defs.h"
#include "cgn_regs.h"

/**
 * NOTE:
 * Code generation in NASM x86-64 assembly
 * (Target-specific layer)
 * Expression-level operations such as loads/stores, arithmetic, compare/set
 *
 * NOTE: Use the following command to earn executable from generated asm code
 * $ nasm -f elf64 (output assembly path) -o out.o
 * $ gcc -no-pie out.o -o out
 * $ ./out
 */

/**
 * nasmLoadImmediateInt - Generates code to load an integer constant into a
 * register.
 *
 * @value: The integer constant to load.
 * @primitiveType: The primitive type of the integer (e.g., P_INT).
 *
 * NOTE:
 * For x86_64, type is not used since all integers are treated as 64-bit.
 *
 * Returns: Index of the register containing the loaded integer.
 */
int nasmLoadImmediateInt(int value, int primitiveType) {
    int registerIndex = allocateRegister();

    fprintf(Outfile, "\tmov\t%s, %d\n", qwordRegisterList[registerIndex],
            value);
    return registerIndex;
}

/**
 * nasmLoadGlobalSymbol - Generates code to load a global symbol's value into a
 * register.
 *
 * @id: The ID of the global symbol in the symbol table.
 *
 * Returns: Index of the register containing the loaded value.
 */
int nasmLoadGlobalSymbol(int id) {
    int registerIndex = allocateRegister();
    int primitiveType = GlobalSymbolTable[id].primitiveType;

    if (primitiveType == P_INT) {
        fprintf(Outfile, "\tmov\t%s, [%s]\n",
                qwordRegisterList[registerIndex], // destination register
                GlobalSymbolTable[id].name        // source global symbol
        );
    } else if (primitiveType == P_CHAR) {
        fprintf(Outfile, "\tmovzx\t%s, BYTE [%s]\n",
                qwordRegisterList[registerIndex], // destination register
                GlobalSymbolTable[id].name        // source global symbol
        );
    } else {
        fprintf(
            stderr,
            "Error: Unsupported primitive type %d in nasmLoadGlobalSymbol\n",
            primitiveType);
        exit(1);
    }

    return registerIndex;
}

/**
 * nasmStoreGlobalSymbol - Generates code to store a register's value into a
 * global symbol.
 *
 * @registerIndex: Index of the register containing the value to store.
 * @id: The ID of the global symbol in the symbol table.
 *
 * Returns: Index of the register that was stored.
 */
int nasmStoreGlobalSymbol(int registerIndex, int id) {
    int primitiveType = GlobalSymbolTable[id].primitiveType;

    if (primitiveType == P_INT) {
        fprintf(Outfile, "\tmov\t[%s], %s\n",
                GlobalSymbolTable[id].name,      // destination global symbol
                qwordRegisterList[registerIndex] // source register
        );
    } else if (primitiveType == P_CHAR) {
        fprintf(
            Outfile, "\tmov\tBYTE [%s], %s\n",
            GlobalSymbolTable[id].name,     // destination global symbol
            byteRegisterList[registerIndex] // source register (lower 8 bits)
        );
    } else {
        fprintf(
            stderr,
            "Error: Unsupported primitive type %d in nasmStoreGlobalSymbol\n",
            primitiveType);
        exit(1);
    }

    return registerIndex;
}

/**
 * nasmDeclareCommonGlobal - Generates code to declare a global symbol.
 *
 * @symbol: The name of the global symbol.
 */
void nasmDeclareGlobalSymbol(int id) {
    // TODO: Use .bss section later

    int primitiveType = GlobalSymbolTable[id].primitiveType;

    if (primitiveType == P_INT) {
        fprintf(Outfile,
                "\tcommon\t%s 8:8\n",      // 8 bytes alignment
                GlobalSymbolTable[id].name // 8 bytes for 64-bit integer
        );
    } else if (primitiveType == P_CHAR) {
        fprintf(Outfile,
                "\tcommon\t%s 1:1\n",      //  1 byte alignment
                GlobalSymbolTable[id].name // 1 byte for char
        );
    } else {
        fprintf(stderr,
                "Error: Unsupported primitive type %d in "
                "nasmDeclareGlobalSymbol\n",
                primitiveType);
        exit(1);
    }
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
 * nasmWidenPrimitiveType - Handles widening of primitive types.
 *
 * @r: Index of the register containing the value.
 * @oldPrimitiveType: The original primitive type.
 * @newPrimitiveType: The target primitive type.
 *
 * NOTE:
 * For x86_64, no action is needed since all integers are treated as 64-bit.
 *
 * Returns: Index of the register containing the (possibly widened) value.
 */
int nasmWidenPrimitiveType(int r, int oldPrimitiveType, int newPrimitiveType) {
    // No action needed for x86_64 as all integers are treated as 64-bit
    return r;
}
