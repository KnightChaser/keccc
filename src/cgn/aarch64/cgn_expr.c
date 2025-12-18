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

#include <limits.h>

static int aarch64PrimitiveSizeInBytes[] = {
    0, // P_NONE
    0, // P_VOID
    1, // P_CHAR
    4, // P_INT
    8, // P_LONG
    8, // P_CHARPTR
    8, // P_INTPTR
    8  // P_LONGPTR
};

/**
 * aarch64GetPrimitiveTypeSize - Returns the size in bytes of a primitive type.
 *
 * @param type The primitive type (e.g., P_INT).
 *
 * @return Size in bytes of the primitive type.
 */
int aarch64GetPrimitiveTypeSize(int type) {
    if (type < P_NONE || type > P_LONGPTR) {
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
 * @param value The integer constant to load.
 * @param primitiveType The primitive type of the integer (e.g., P_INT).
 *
 * NOTE:
 * For AArch64, type is not used since all integers are treated as 64-bit.
 *
 * @return Index of the register containing the loaded integer.
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
 *
 * @param name The name of the global symbol.
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
 * @param id The ID of the global symbol in the symbol table.
 *
 * @return Index of the register containing the loaded value.
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
    // NOTE:
    // pointer types are treated as long (64 bits) in AArch64
    case P_CHARPTR:
    case P_INTPTR:
    case P_LONGPTR:
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
 * aarch64LoadGlobalString - Generates code to load the address of a global
 * string into a register.
 *
 * @param id The ID of the global string in the symbol table.
 *
 * @return Index of the register containing the address of the string.
 */
int aarch64LoadGlobalString(int id) {
    int r = aarch64AllocateRegister();

    fprintf(Outfile, "\tadrp\t%s, L%d\n", aarch64QwordRegisterList[r], id);
    fprintf(Outfile, "\tadd\t%s, %s, :lo12:L%d\n", aarch64QwordRegisterList[r],
            aarch64QwordRegisterList[r], id);

    return r;
}

/**
 * aarch64StoreGlobalSymbol - Generates code to store a register's value into a
 * global symbol.
 *
 * @param r Index of the register containing the value to store.
 * @param id The ID of the global symbol in the symbol table.
 *
 * @return Index of the register that was stored.
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
    case P_CHARPTR:
    case P_INTPTR:
    case P_LONGPTR:
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
 * aarch64P2AlignFor - Returns the log2 of the alignment in bytes for .p2align.
 *
 * @param alignmentBytes The requested alignment in bytes.
 *
 * @return log2(alignmentBytes) for .p2align, or -1 if not power-of-two.
 */
static int aarch64P2AlignFor(int alignmentBytes) {
    // returns log2(alignmentBytes) for .p2align
    // clamp to 0..4-ish if you want, but 0..3 is enough for 1/2/4/8
    switch (alignmentBytes) {
    case 8:
        return 3;
    case 4:
        return 2;
    case 2:
        return 1;
    case 1:
        return 0;
    default:
        // if it's not power-of-two, fall back to 0 (no alignment directive)
        return -1;
    }
}

/**
 * aarch64DeclareGlobalSymbol - Generates code to declare a global symbol.
 *
 * @param id The ID of the global symbol in the symbol table.
 */
void aarch64DeclareGlobalSymbol(int id) {
    int primitiveType = GlobalSymbolTable[id].primitiveType;

    int elementSize = aarch64GetPrimitiveTypeSize(primitiveType);
    if (elementSize <= 0) {
        fprintf(stderr,
                "Error: Invalid element size %d for symbol %s in "
                "aarch64DeclareGlobalSymbol\n",
                elementSize, GlobalSymbolTable[id].name);
        exit(1);
    }

    int count = 1;
    if (GlobalSymbolTable[id].structuralType == S_ARRAY) {
        count = GlobalSymbolTable[id].size;
        if (count <= 0 || count > INT_MAX / elementSize) {
            fprintf(stderr, "Error: bad array count %d for symbol %s\n", count,
                    GlobalSymbolTable[id].name);
            exit(1);
        }
    }

    if (elementSize > LLONG_MAX / count) {
        fprintf(stderr, "Error: total size overflow for symbol %s\n",
                GlobalSymbolTable[id].name);
        exit(1);
    }

    long long totalBytesRequired = (long long)elementSize * (long long)count;

    int alignment = (elementSize >= 8)   ? 8
                    : (elementSize >= 4) ? 4
                    : (elementSize >= 2) ? 2
                                         : 1;
    int p2 = aarch64P2AlignFor(alignment);

    // Prefer BSS for zero-initialized storage
    fprintf(Outfile, "\t.section\t.bss\n");
    fprintf(Outfile, "\t.globl\t%s\n", GlobalSymbolTable[id].name);
    if (p2 >= 0) {
        fprintf(Outfile, "\t.p2align\t%d\n", p2);
    }

    fprintf(Outfile, "%s:\n", GlobalSymbolTable[id].name);

    // Reserve zeroed bytes
    fprintf(Outfile, "\t.zero\t%lld\n", totalBytesRequired);
}

/**
 * aarch64DeclareGlobalString - Generates code to declare a global string in
 * the read-only data segment.
 *
 * @param labelIndex The label index to assign to the string.
 * @param stringValue The string value to declare.
 */
void aarch64DeclareGlobalString(int labelIndex, char *stringValue) {
    const unsigned char *s = (const unsigned char *)stringValue;

    fprintf(Outfile, "\t.section\t.rodata\n");
    aarch64Label(labelIndex);

    fprintf(Outfile, "\t.ascii\t");
    fprintf(Outfile, "\"");

    for (const unsigned char *p = s; *p != '\0'; p++) {
        unsigned char c = *p;

        switch (c) {
        case '\\':
            fputs("\\\\", Outfile);
            break;
        case '"':
            fputs("\\\"", Outfile);
            break;
        case '\n':
            fputs("\\n", Outfile);
            break;
        case '\r':
            fputs("\\r", Outfile);
            break;
        case '\t':
            fputs("\\t", Outfile);
            break;
        default:
            if (c >= 32 && c <= 126) {
                // Printable
                fputc((char)c, Outfile);
            } else {
                // Non-printable: use octal escape
                fputs("\" \n\t.byte ", Outfile);
                fprintf(Outfile, "%u", (unsigned)c);
                fputs("\n\t.ascii \"", Outfile);
            }
            break;
        }
    }
    fputc('"', Outfile); // Closing quote for string

    fputc('\n', Outfile); // Newline after .asciz

    fprintf(Outfile, "\t.byte\t0\n");
}

/**
 * aarch64AddRegs - Generates code to add values in two registers.
 * (r2 = r2 + r1, free r1)
 *
 * @param r1 Index of the first register.
 * @param r2 Index of the second register.
 *
 * @return Index of the register containing the result.
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
 * @param r1 Index of the first register.
 * @param r2 Index of the second register.
 *
 * @return Index of the register containing the result.
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
 * @param r1 Index of the first register.
 * @param r2 Index of the second register.
 *
 * @return Index of the register containing the result.
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
 * @param r1 Index of the dividend register.
 * @param r2 Index of the divisor register.
 *
 * @return Index of the register containing the result (quotient).
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
 * aarch64ShiftLeftConst - Generates code to shift a register left by a
 * constant amount.
 *
 * @param reg Index of the register to shift.
 * @param shiftAmount The constant amount to shift left.
 *
 * @return Index of the register containing the shifted value.
 */
int aarch64ShiftLeftConst(int reg, int shiftAmount) {
    fprintf(Outfile, "\tlsl\t%s, %s, #%d\n", aarch64QwordRegisterList[reg],
            aarch64QwordRegisterList[reg], shiftAmount);
    return reg;
}

/**
 * aarch64CompareAndSet - Generates code to compare two registers and set a
 * third register based on the comparison result. (Compare and set 0/1 in r2,
 * free r1.)
 *
 * @param ASTop The AST operation code representing the comparison.
 * @param r1 Index of the first register.
 * @param r2 Index of the second register.
 *
 * @return Index of the register containing the comparison result (0 or 1).
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
 * @param r Index of the register containing the value. (ignored)
 * @param oldPrimitiveType The original primitive type. (ignored)
 * @param newPrimitiveType The new primitive type. (ignored)
 *
 * @return Index of the register containing the value.
 */
int aarch64WidenPrimitiveType(int r, int oldPrimitiveType,
                              int newPrimitiveType) {
    (void)oldPrimitiveType;
    (void)newPrimitiveType;
    return r;
}

/**
 * aarch64AddressOfGlobalSymbol - Generates code to get the address of a global
 * symbol. Returns the address in a register.
 *
 * @param id The ID of the global symbol in the symbol table.
 *
 * @return Index of the register containing the address of the global symbol.
 */
int aarch64AddressOfGlobalSymbol(int id) {
    int r = aarch64AllocateRegister();

    // PC-relative addressing:
    //   adrp xN, name             ; compute page address
    //   add  xN, xN, :lo12:name   ; add page offset
    fprintf(Outfile, "\tadrp\t%s, %s\n", aarch64QwordRegisterList[r],
            GlobalSymbolTable[id].name);
    fprintf(Outfile, "\tadd\t%s, %s, :lo12:%s\n", aarch64QwordRegisterList[r],
            aarch64QwordRegisterList[r], GlobalSymbolTable[id].name);
    return r;
}

/**
 * aarch64DereferencePointer - Generates code to dereference a pointer in a
 * register and load the value into the same register.
 *
 * @param pointerReg Index of the register containing the pointer.
 * @param primitiveType The primitive type of the value being loaded.
 *
 * @return Index of the register containing the loaded value.
 */
int aarch64DereferencePointer(int pointerReg, int primitiveType) {
    const char *x = aarch64QwordRegisterList[pointerReg];
    const char *w = aarch64DwordRegisterList[pointerReg];

    switch (primitiveType) {
    case P_CHARPTR:
        // zero-extend byte into wN (upper bites cleared)
        fprintf(Outfile, "\tldrb\t%s, [%s]\n", w, x);
        break;
    case P_INTPTR:
        // loads 32-bit into wN (upper bits cleared)
        fprintf(Outfile, "\tldr\t%s, [%s]\n", w, x);
        break;
    case P_LONGPTR:
        // loads 64-bit into xN
        fprintf(Outfile, "\tldr\t%s, [%s]\n", x, x);
        break;
    default:
        fprintf(stderr,
                "Error: Unsupported primitive type %d in "
                "aarch64DereferencePointer\n",
                primitiveType);
        exit(1);
    }

    return pointerReg;
}

/**
 * aarch64StoreDereferencedPointer - Generates code to store a value from a
 * register into a memory location pointed to by another register.
 *
 * @param valueReg Index of the register containing the value to store.
 * @param pointerReg Index of the register containing the pointer (address).
 * @param primitiveType The primitive type of the value being stored.
 *
 * @return Index of the register that was stored.
 */
int aarch64StoreDereferencedPointer(int valueReg, int pointerReg,
                                    int primitiveType) {
    switch (primitiveType) {
    case P_CHAR:
        // Store 1 byte: uses W register, low 8 bits written.
        fprintf(Outfile, "\tstrb\t%s, [%s]\n",
                aarch64DwordRegisterList[valueReg],  // source (wN)
                aarch64QwordRegisterList[pointerReg] // address (xM)
        );
        break;

    case P_INT:
        // Store 4 bytes: STR Wt, [Xn]
        fprintf(Outfile, "\tstr\t%s, [%s]\n",
                aarch64DwordRegisterList[valueReg],  // source (wN)
                aarch64QwordRegisterList[pointerReg] // address (xM)
        );
        break;

    case P_LONG:
        // Store 8 bytes: STR Xt, [Xn]
        fprintf(Outfile, "\tstr\t%s, [%s]\n",
                aarch64QwordRegisterList[valueReg],  // source (xN)
                aarch64QwordRegisterList[pointerReg] // address (xM)
        );
        break;

    default:
        fprintf(stderr,
                "Error: Unsupported primitive type %d in "
                "aarch64StoreDereferencedPointer\n",
                primitiveType);
        exit(1);
    }

    return valueReg;
}
