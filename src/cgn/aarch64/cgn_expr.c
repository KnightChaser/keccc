// src/cgn/aarch64/cgn_expr.c

/**
 * NOTE:
 * AArch64 GNU as-style backend
 * Expression-level codegen: loads/stores, arithmetic, compares.
 */

#include "cgn/aarch64/cgn_regs.h"
#include "data.h"
#include "decl.h"
#include "defs.h"

static int aarch64PrimitiveSizeInBytes[] = {
    0, // P_NONE
    0, // P_VOID
    1, // P_CHAR
    4, // P_INT
    8  // P_LONG
};

/**
 * aarch64GetPrimitiveTypeSize - Returns the size in bytes of a primitive type.
 *
 * @type: The primitive type (e.g., P_INT).
 *
 * Returns: Size in bytes of the primitive type.
 */
int aarch64GetPrimitiveTypeSize(int type) {
    if (type < P_NONE || type > P_LONG) {
        fprintf(
            stderr,
            "Error: Invalid primitive type %d in aarch64GetPrimitiveTypeSize\n",
            type);
        exit(1);
    }
    return aarch64PrimitiveSizeInBytes[type];
}

/**
 * aarch64LoadImmediateInt - Generates code to load an integer constant into a
 * register.
 *
 * @value: The integer constant to load.
 * @primitiveType: The primitive type of the integer (e.g., P_INT).
 *
 * NOTE:
 * For AArch64, type is not used since all integers are treated as 64-bit.
 *
 * Returns: Index of the register containing the loaded integer.
 */
int aarch64LoadImmediateInt(int value, int primitiveType) {
    int r = aarch64AllocateRegister();
    (void)primitiveType; // unused (all are represented as 64-bit)

    fprintf(Outfile, "\tmov\t%s, #%d\n", aarch64QwordRegisterList[r], value);
    return r;
}

/**
 * aarch64LoadGlobalAddressIntoX0 - Generates code to load the address of a
 * global symbol into register x0. (helper function)
 *
 * @name: The name of the global symbol.
 */
static void aarch64LoadGlobalAddressIntoX0(const char *name) {
    // PC-relative addressing:
    //   adrp x0, name
    //   add  x0, x0, :lo12:name
    fprintf(Outfile, "\tadrp\tx0, %s\n", name);
    fprintf(Outfile, "\tadd\tx0, x0, :lo12:%s\n", name);
}

/**
 * aarch64LoadGlobalSymbol - Generates code to load a global symbol's value into
 * a register.
 *
 * @id: The ID of the global symbol in the symbol table.
 *
 * Returns: Index of the register containing the loaded value.
 */
int aarch64LoadGlobalSymbol(int id) {
    int r = aarch64AllocateRegister();
    int primitiveType = GlobalSymbolTable[id].primitiveType;
    char *name = GlobalSymbolTable[id].name;

    aarch64LoadGlobalAddressIntoX0(name);

    switch (primitiveType) {
    case P_CHAR:
        // NOTE: zero-extend 8-bit char into w-reg (and thus x-reg)
        // (Load Register Byte)
        fprintf(Outfile, "\tldrb\t%s, [x0]\n", aarch64DwordRegisterList[r]);
        break;
    case P_INT:
        fprintf(Outfile, "\tldr\t%s, [x0]\n", aarch64DwordRegisterList[r]);
        break;
    case P_LONG:
        fprintf(Outfile, "\tldr\t%s, [x0]\n", aarch64QwordRegisterList[r]);
        break;
    default:
        fprintf(
            stderr,
            "Error: Unsupported primitive type %d in aarch64LoadGlobalSymbol\n",
            primitiveType);
        exit(1);
    }

    return r;
}

/**
 * aarch64StoreGlobalSymbol - Generates code to store a register's value into a
 * global symbol.
 *
 * @r: Index of the register containing the value to store.
 * @id: The ID of the global symbol in the symbol table.
 *
 * Returns: Index of the register that was stored.
 */
int aarch64StoreGlobalSymbol(int r, int id) {
    int primitiveType = GlobalSymbolTable[id].primitiveType;
    char *name = GlobalSymbolTable[id].name;

    aarch64LoadGlobalAddressIntoX0(name);

    switch (primitiveType) {
    case P_CHAR:
        // NOTE: Store Register Byte
        fprintf(Outfile, "\tstrb\t%s, [x0]\n", aarch64DwordRegisterList[r]);
        break;
    case P_INT:
        fprintf(Outfile, "\tstr\t%s, [x0]\n", aarch64DwordRegisterList[r]);
        break;
    case P_LONG:
        fprintf(Outfile, "\tstr\t%s, [x0]\n", aarch64QwordRegisterList[r]);
        break;
    default:
        fprintf(stderr,
                "Error: Unsupported primitive type %d in "
                "aarch64StoreGlobalSymbol\n",
                primitiveType);
        exit(1);
    }

    return r;
}

/**
 * aarch64DeclareGlobalSymbol - Generates code to declare a global symbol.
 * It automatically calculates size and alignment
 * based on the given primitive type.
 *
 * @id: The ID of the global symbol in the symbol table.
 */
void aarch64DeclareGlobalSymbol(int id) {
    int primitiveType = GlobalSymbolTable[id].primitiveType;
    int typeSize = aarch64GetPrimitiveTypeSize(primitiveType);

    fprintf(Outfile, "\t.comm\t%s, %d, %d\n", GlobalSymbolTable[id].name,
            typeSize, typeSize);
}

/**
 * aarch64AddRegs - Generates code to add values in two registers.
 * (r2 = r2 + r1, free r1)
 *
 * @r1: Index of the first register.
 * @r2: Index of the secondition register.
 *
 * Returns: Index of the register containing the result.
 */
int aarch64AddRegs(int r1, int r2) {
    fprintf(Outfile, "\tadd\t%s, %s, %s\n",
            aarch64QwordRegisterList[r2], // destination
            aarch64QwordRegisterList[r2], // source 1
            aarch64QwordRegisterList[r1]  // source 2
    );
    aarch64FreeRegister(r1);
    return r2;
}

/**
 * aarch64SubRegs - Generates code to subtract values in two registers.
 * (r1 = r1 - r2, free r2)
 *
 * @r1: Index of the first register.
 * @r2: Index of the secondition register.
 *
 * Returns: Index of the register containing the result.
 */
int aarch64SubRegs(int r1, int r2) {
    fprintf(Outfile, "\tsub\t%s, %s, %s\n",
            aarch64QwordRegisterList[r1], // destination
            aarch64QwordRegisterList[r1], // minuend
            aarch64QwordRegisterList[r2]  // subtrahend
    );
    aarch64FreeRegister(r2);
    return r1;
}

/**
 * aarch64MulRegs - Generates code to multiply values in two registers.
 * (r2 = r2 * r1, free r1)
 *
 * @r1: Index of the first register.
 * @r2: Index of the secondition register.
 *
 * Returns: Index of the register containing the result.
 */
int aarch64MulRegs(int r1, int r2) {
    fprintf(Outfile, "\tmul\t%s, %s, %s\n",
            aarch64QwordRegisterList[r2], // destination
            aarch64QwordRegisterList[r2], // source 1
            aarch64QwordRegisterList[r1]  // source 2
    );
    aarch64FreeRegister(r1);
    return r2;
}

/**
 * aarch64DivRegsSigned - Generates code to divide values in two registers.
 * (r1 = r1 / r2, free r2)
 *
 * @r1: Index of the dividend register.
 * @r2: Index of the divisor register.
 *
 * Returns: Index of the register containing the result (quotient).
 */
int aarch64DivRegsSigned(int r1, int r2) {
    fprintf(Outfile, "\tsdiv\t%s, %s, %s\n",
            aarch64QwordRegisterList[r1], // destination
            aarch64QwordRegisterList[r1], // dividend
            aarch64QwordRegisterList[r2]  // divisor
    );
    aarch64FreeRegister(r2);
    return r1;
}

/**
 * aarch64CompareAndSet - Generates code to compare two registers and set a
 * third register based on the comparison result. (Compare and set 0/1 in r2,
 * free r1.)
 *
 * @ASTop: The AST operation code representing the comparison.
 * @r1: Index of the first register.
 * @r2: Index of the second register.
 *
 * Returns: Index of the register containing the comparison result (0 or 1).
 */
int aarch64CompareAndSet(int ASTop, int r1, int r2) {
    if (!((ASTop == A_EQ) || (ASTop == A_NE) || (ASTop == A_LT) ||
          (ASTop == A_LE) || (ASTop == A_GT) || (ASTop == A_GE))) {
        fprintf(stderr,
                "Error: Invalid AST operation %d in aarch64CompareAndSet\n",
                ASTop);
        exit(1);
    }

    fprintf(Outfile, "\tcmp\t%s, %s\n", aarch64QwordRegisterList[r1],
            aarch64QwordRegisterList[r2]);

    const char *condition = NULL;
    switch (ASTop) {
    case A_EQ:
        condition = "eq";
        break;
    case A_NE:
        condition = "ne";
        break;
    case A_LT:
        condition = "lt";
        break; // signed
    case A_LE:
        condition = "le";
        break;
    case A_GT:
        condition = "gt";
        break;
    case A_GE:
        condition = "ge";
        break;
    default:
        fprintf(stderr,
                "Error: Unknown AST operation %d in aarch64CompareAndSet\n",
                ASTop);
        exit(1);
    }

    // cset wN, condition => wN = 0 or 1, high bits of xN are zeroed.
    fprintf(Outfile, "\tcset\t%s, %s\n", aarch64DwordRegisterList[r2],
            condition);

    aarch64FreeRegister(r1);
    return r2;
}

/**
 * aarch64WidenPrimitiveType - In AArch64, all integer types are treated as
 * 64-bit, so no action is needed.
 *
 * @r: Index of the register containing the value. (ignored)
 * @oldPrimitiveType: The original primitive type. (ignored)
 * @newPrimitiveType: The new primitive type. (ignored)
 *
 * Returns: Index of the register containing the value.
 */
int aarch64WidenPrimitiveType(int r, int oldPrimitiveType,
                              int newPrimitiveType) {
    (void)oldPrimitiveType;
    (void)newPrimitiveType;
    return r;
}
