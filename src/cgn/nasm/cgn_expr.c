// src/cgn/nasm/cgn_expr.c

#include "cgn/nasm/cgn_regs.h"
#include "data.h"
#include "decl.h"
#include "defs.h"

#include <limits.h>

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

static int primitiveSizeInBytes[] = {
    0, // P_NONE
    0, // P_VOID
    1, // P_CHAR
    4, // P_INT
    8, // P_LONG
       // NOTE: Long is 8 bytes (64 bits) in x86_64
       // Later, we can add support 32 bits long(x86) if needed

    8, // P_CHARPTR
    8, // P_INTPTR
    8, // P_LONGPTR
    8  // Extra entry for safety/P_LONGPTR if enum values shifted
};

/**
 * nasmGetPrimitiveTypeSize - Returns the size in bytes of a primitive type.
 *
 * @param type The primitive type (e.g., P_INT).
 *
 * @return Size in bytes of the primitive type.
 */
int nasmGetPrimitiveTypeSize(int type) {
    if ((type < P_NONE) || (type > P_LONGPTR)) {
        fprintf(
            stderr,
            "Error: Invalid primitive type %d in nasmGetPrimitiveTypeSize\n",
            type);
        exit(1);
    }
    return primitiveSizeInBytes[type];
}

/**
 * nasmLoadImmediateInt - Generates code to load an integer constant into a
 * register.
 *
 * @param value The integer constant to load.
 * @param primitiveType The primitive type of the integer (e.g., P_INT).
 *
 * NOTE:
 * For x86_64, type is not used since all integers are treated as 64-bit.
 *
 * @return Index of the register containing the loaded integer.
 */
int nasmLoadImmediateInt(int value, int primitiveType) {
    int registerIndex = allocateRegister();

    fprintf(Outfile, "\tmov\t%s, %d\n", qwordRegisterList[registerIndex],
            value);
    return registerIndex;
}

/**
 * nasmLoadGlobalSymbol - Generates code to load a global symbol's value into a
 *                        register.
 *
 * @param id The ID of the global symbol in the symbol table.
 * @param op The AST operation code
 *
 * If the operation(@param op) is whether pre- or post-
 * increment/decrement, it performs additional actions.
 *
 * @return Index of the register containing the loaded value.
 */
int nasmLoadGlobalSymbol(int id, int op) {
    int registerIndex = allocateRegister();
    int primitiveType = GlobalSymbolTable[id].primitiveType;

    switch (primitiveType) {
    case P_CHAR:
        if (op == A_PREINCREMENT) {
            // Increase first, then load
            fprintf(Outfile, "\tinc\tBYTE [%s]\n", GlobalSymbolTable[id].name);
        }
        if (op == A_PREDECREMENT) {
            // Decrease first, then load
            fprintf(Outfile, "\tdec\tBYTE [%s]\n", GlobalSymbolTable[id].name);
        }

        // Load
        fprintf(Outfile, "\tmovzx\t%s, BYTE [%s]\n", // Zero-extend for char
                qwordRegisterList[registerIndex],    // destination register
                GlobalSymbolTable[id].name           // source global symbol
        );

        if (op == A_POSTINCREMENT) {
            // Load first, then increase
            fprintf(Outfile, "\tinc\tBYTE [%s]\n", GlobalSymbolTable[id].name);
        }
        if (op == A_POSTDECREMENT) {
            // Load first, then decrease
            fprintf(Outfile, "\tdec\tBYTE [%s]\n", GlobalSymbolTable[id].name);
        }

        break;

    case P_INT:
        if (op == A_PREINCREMENT) {
            // Increase first, then load
            fprintf(Outfile, "\tinc\tDWORD [%s]\n", GlobalSymbolTable[id].name);
        }
        if (op == A_PREDECREMENT) {
            // Decrease first, then load
            fprintf(Outfile, "\tdec\tDWORD [%s]\n", GlobalSymbolTable[id].name);
        }

        // Load
        fprintf(Outfile, "\txor\t%s, %s\n", qwordRegisterList[registerIndex],
                qwordRegisterList[registerIndex]);  // Clear the register first)
        fprintf(Outfile, "\tmov\t%s, DWORD [%s]\n", // Load 4 bytes only
                dwordRegisterList[registerIndex],   // destination register
                GlobalSymbolTable[id].name          // source global symbol
        );

        if (op == A_POSTINCREMENT) {
            // Load first, then increase
            fprintf(Outfile, "\tinc\tDWORD [%s]\n", GlobalSymbolTable[id].name);
        }
        if (op == A_POSTDECREMENT) {
            // Load first, then decrease
            fprintf(Outfile, "\tdec\tDWORD [%s]\n", GlobalSymbolTable[id].name);
        }

        break;

    case P_LONG:
    // NOTE:
    // pointer types are treated as long (64 bits) in x86_64
    case P_CHARPTR:
    case P_INTPTR:
    case P_LONGPTR:
        if (op == A_PREINCREMENT) {
            // Increase first, then load
            fprintf(Outfile, "\tinc\tQWORD [%s]\n", GlobalSymbolTable[id].name);
        }
        if (op == A_PREDECREMENT) {
            // Decrease first, then load
            fprintf(Outfile, "\tdec\tQWORD [%s]\n", GlobalSymbolTable[id].name);
        }

        // Load
        fprintf(Outfile, "\tmov\t%s, [%s]\n",
                qwordRegisterList[registerIndex], // destination register
                GlobalSymbolTable[id].name        // source global symbol
        );

        if (op == A_POSTINCREMENT) {
            // Load first, then increase
            fprintf(Outfile, "\tinc\tQWORD [%s]\n", GlobalSymbolTable[id].name);
        }
        if (op == A_POSTDECREMENT) {
            // Load first, then decrease
            fprintf(Outfile, "\tdec\tQWORD [%s]\n", GlobalSymbolTable[id].name);
        }

        break;

    default:
        fprintf(
            stderr,
            "Error: Unsupported primitive type %d in nasmLoadGlobalSymbol\n",
            primitiveType);
        exit(1);
    }

    return registerIndex;
}

/**
 * nasmLoadGlobalString - Generates code to load the address of a global
 * string into a register.
 *
 * @param id The ID of the global string in the symbol table.
 *
 * @return Index of the register containing the address of the string.
 */
int nasmLoadGlobalString(int id) {
    int registerIndex = allocateRegister();

    fprintf(Outfile, "\tlea\t%s, [rel L%d]\n",
            qwordRegisterList[registerIndex], // destination register
            id                                // string label
    );
    return registerIndex;
}

/**
 * nasmStoreGlobalSymbol - Generates code to store a register's value into a
 * global symbol.
 *
 * @param registerIndex Index of the register containing the value to store.
 * @param id The ID of the global symbol in the symbol table.
 *
 * @return Index of the register that was stored.
 */
int nasmStoreGlobalSymbol(int registerIndex, int id) {
    int primitiveType = GlobalSymbolTable[id].primitiveType;

    switch (primitiveType) {
    case P_CHAR:
        fprintf(
            Outfile, "\tmov\t[%s], BYTE %s\n",
            GlobalSymbolTable[id].name,     // destination global symbol
            byteRegisterList[registerIndex] // source register (lower 8 bits)
        );
        break;
    case P_INT:
        fprintf(
            Outfile, "\tmov\t[%s], DWORD %s\n",
            GlobalSymbolTable[id].name,      // destination global symbol
            dwordRegisterList[registerIndex] // source register (lower 32 bits)
        );
        break;
    case P_LONG:
    case P_CHARPTR:
    case P_INTPTR:
    case P_LONGPTR:
        fprintf(Outfile, "\tmov\t[%s], QWORD %s\n",
                GlobalSymbolTable[id].name,      // destination global symbol
                qwordRegisterList[registerIndex] // source register
        );
        break;
    default:
        fprintf(
            stderr,
            "Error: Unsupported primitive type %d in nasmStoreGlobalSymbol\n",
            primitiveType);
        exit(1);
    }

    return registerIndex;
}

/**
 * nasmAlignPow2 - Returns the largest power-of-two alignment <= n, capped at 8.
 * (e.g. 1->1, 2->2, 3->2, 4->4, 5->4, 8->8, 16->8, etc.)
 *
 * @param n The requested alignment in bytes.
 *
 * @return The aligned power-of-two value.
 */
static int nasmAlignPow2(int n) {
    if (n >= 8) {
        return 8;
    } else if (n >= 4) {
        return 4;
    } else if (n >= 2) {
        return 2;
    }

    return 1;
}

/**
 * nasmDeclareGlobalSymbol - Generates code to declare a global symbol and
 * reserve storage for it.
 *
 * @param id The ID of the global symbol in the symbol table.
 */
void nasmDeclareGlobalSymbol(int id) {
    int primitiveType = GlobalSymbolTable[id].primitiveType;

    int elementSize = nasmGetPrimitiveTypeSize(primitiveType);
    if (elementSize <= 0) {
        fprintf(stderr, "Error: bad elemSize %d for symbol %s\n", elementSize,
                GlobalSymbolTable[id].name);
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

    int alignment = nasmAlignPow2(elementSize);

    // Prefer BSS for zero-initialized storage
    fprintf(Outfile, "\tsection\t.bss\n");
    fprintf(Outfile, "\talign\t%d\n", alignment);
    fprintf(Outfile, "\tglobal\t%s\n", GlobalSymbolTable[id].name);
    fprintf(Outfile, "%s:\n", GlobalSymbolTable[id].name);

    // Reserve storage: choose the directive that matches element width.
    // This emits ONE directive with a COUNT (e.g., resd 5), which is
    // exactly what you want.
    switch (elementSize) {
    case 1:
        fprintf(Outfile, "\tresb\t%d\n", count);
        break;
    case 2:
        fprintf(Outfile, "\tresw\t%d\n", count);
        break;
    case 4:
        fprintf(Outfile, "\tresd\t%d\n", count); // 5 => 20 bytes total
        break;
    case 8:
        fprintf(Outfile, "\tresq\t%d\n", count);
        break;
    default:
        // Fallback: reserve raw bytes (still correct)
        fprintf(Outfile, "\tresb\t%lld\n", totalBytesRequired);
        break;
    }
}

/**
 * nasmDeclareGlobalString - Generates code to store a string constant as a
 * global string in the read-only data section.
 *
 * @param labelIndex The label index for the global string.
 * @param stringValue The string constant to store.
 */
void nasmDeclareGlobalString(int labelIndex, char *stringValue) {
    const unsigned char *s = (const unsigned char *)stringValue;

    fprintf(Outfile, "\tsection .rodata\n");
    nasmLabel(labelIndex);

    fprintf(Outfile, "\tdb ");
    fputc('"', Outfile); // Opening quote for string
    for (const unsigned char *p = s; *p != '\0'; p++) {
        unsigned char c = *p;

        switch (c) {
        case '\n':
            // NASM doesn't interpret C-style escapes in quoted strings.
            // Emit a real newline byte.
            fputs("\", 10, \"", Outfile);
            break;
        case '\r':
            fputs("\", 13, \"", Outfile);
            break;
        case '\t':
            fputs("\", 9, \"", Outfile);
            break;
        case '\\':
            // Emit a literal backslash byte.
            fputs("\", 92, \"", Outfile);
            break;
        case '"':
            // Emit a literal double-quote byte.
            fputs("\", 34, \"", Outfile);
            break;
        default:
            if (c >= 32 && c <= 126) {
                // Printable
                fputc((char)c, Outfile);
            } else {
                // Non-printable: close string, emit byte, reopen string
                fputs("\", ", Outfile);
                fprintf(Outfile, "%u", (unsigned)c);
                fputs(", \"", Outfile);
            }
            break;
        }
    }
    fputc('"', Outfile); // Closing quote for string

    fprintf(Outfile, ", 0\n"); // NUL terminator
}

/**
 * nasmAddRegs - Generates code to add values in two registers.
 *
 * @param r1 Index of the first register.
 * @param r2 Index of the second register.
 *
 * @return Index of the register containing the result.
 */
int nasmAddRegs(int r1, int r2) {
    fprintf(Outfile, "\tadd\t%s, %s\n", qwordRegisterList[r2],
            qwordRegisterList[r1]);
    freeRegister(r1);

    return r2;
}

/**
 * nasmSubRegs - Generates code to subtract values in two registers.
 *
 * @param r1 Index of the first register.
 * @param r2 Index of the second register.
 *
 * @return Index of the register containing the result.
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
 * @param r1 Index of the first register.
 * @param r2 Index of the second register.
 *
 * @return Index of the register containing the result.
 */
int nasmMulRegs(int r1, int r2) {
    fprintf(Outfile, "\timul\t%s, %s\n", qwordRegisterList[r2],
            qwordRegisterList[r1]);
    freeRegister(r1);

    return r2;
}

/**
 * nasmDivRegsSigned - Generates code to divide values in two registers.
 *
 * @param r1 Index of the dividend register.
 * @param r2 Index of the divisor register.
 *
 * @return Index of the register containing the result (quotient).
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
 * nasmLogicalNegate - Generates code to logically negate a register's value.
 *
 * @param reg Index of the register to negate.
 *
 * @return Index of the register containing the negated value.
 */
int nasmLogicalNegate(int reg) {
    fprintf(Outfile, "\tneg\t%s\n", qwordRegisterList[reg]);

    return reg;
}

/**
 * nasmLogicalInvert - Generates code to logically invert (bitwise NOT) a
 * register's value.
 *
 * @param reg Index of the register to invert.
 *
 * @return Index of the register containing the inverted value.
 */
int nasmLogicalInvert(int reg) {
    fprintf(Outfile, "\tnot\t%s\n", qwordRegisterList[reg]);

    return reg;
}

/**
 * nasmLogicalNot - Generates code to logically NOT a register's value.
 * (i.e., set to 1 if zero, else set to 0)
 *
 * @param reg Index of the register to NOT.
 *
 * @return Index of the register containing the NOTed value.
 */
int nasmLogicalNot(int reg) {
    fprintf(Outfile, "\ttest\t%s, %s\n", qwordRegisterList[reg],
            qwordRegisterList[reg]);
    fprintf(Outfile, "\tsete\t%s\n", byteRegisterList[reg]);
    fprintf(Outfile, "\tmovzx\t%s, %s\n", qwordRegisterList[reg],
            byteRegisterList[reg]);

    return reg;
}

/**
 * nasmToBoolean - Generates code to convert a register's value to boolean.
 *
 * If used in an A_IF or A_WHILE operation, generates a jump to the given
 * label if the value is zero. Otherwise, sets the register to 0 or 1 based on
 * its truthiness.
 *
 * @param reg Index of the register to convert.
 * @param op The AST operation code (A_IF, A_WHILE, etc.)
 * @param label The label to jump to if the value is zero (for A_IF/A_WHILE).
 *
 * @return Index of the register containing the boolean value (0 or 1).
 */
int nasmToBoolean(int reg, int op, int label) {
    fprintf(Outfile, "\ttest\t%s, %s\n", qwordRegisterList[reg],
            qwordRegisterList[reg]);
    if (op == A_IF || op == A_WHILE) {
        fprintf(Outfile, "\tje\tL%d\n", label);
    } else {
        fprintf(Outfile, "\tsetnz\t%s\n", byteRegisterList[reg]);
        fprintf(Outfile, "\tmovzx\t%s, %s\n", qwordRegisterList[reg],
                byteRegisterList[reg]);
    }

    return reg;
}

/**
 * nasmBitwiseAndRegs - Generates code to perform bitwise AND between two
 * registers.
 *
 * @param dstReg Index of the destination register (also holds one operand).
 * @param srcReg Index of the source register (holds the other operand).
 *
 * @return Index of the register containing the result.
 */
int nasmBitwiseAndRegs(int dstReg, int srcReg) {
    fprintf(Outfile, "\tand\t%s, %s\n", qwordRegisterList[dstReg],
            qwordRegisterList[srcReg]);
    freeRegister(srcReg);

    return dstReg;
}

/**
 * nasmBitwiseOrRegs - Generates code to perform bitwise OR between two
 * registers.
 *
 * @param dstReg Index of the destination register (also holds one operand).
 * @param srcReg Index of the source register (holds the other operand).
 *
 * @return Index of the register containing the result.
 */
int nasmBitwiseOrRegs(int dstReg, int srcReg) {
    fprintf(Outfile, "\tor\t%s, %s\n", qwordRegisterList[dstReg],
            qwordRegisterList[srcReg]);
    freeRegister(srcReg);

    return dstReg;
}

/**
 * nasmBitwiseXorRegs - Generates code to perform bitwise XOR between two
 * registers.
 *
 * @param dstReg Index of the destination register (also holds one operand).
 * @param srcReg Index of the source register (holds the other operand).
 *
 * @return Index of the register containing the result.
 */
int nasmBitwiseXorRegs(int dstReg, int srcReg) {
    fprintf(Outfile, "\txor\t%s, %s\n", qwordRegisterList[dstReg],
            qwordRegisterList[srcReg]);
    freeRegister(srcReg);

    return dstReg;
}

/**
 * nasmShiftLeftConst - Generates code to shift a register's value left by a
 * constant amount.
 *
 * @param reg Index of the register to shift.
 * @param shiftAmount The constant amount to shift left.
 *
 * @return Index of the register containing the shifted value.
 */
int nasmShiftLeftConst(int reg, int shiftAmount) {
    fprintf(Outfile, "\tshl\t%s, %d\n", qwordRegisterList[reg], shiftAmount);
    return reg;
}

/**
 * nasmShiftRightConst - Generates code to shift a register's value right by
 * a constant amount.
 *
 * @param reg Index of the register to shift.
 * @param shiftAmount The constant amount to shift right.
 *
 * @return Index of the register containing the shifted value.
 */
int nasmShiftRightConst(int reg, int shiftAmount) {
    fprintf(Outfile, "\tshr\t%s, %d\n", qwordRegisterList[reg], shiftAmount);
    return reg;
}

/**
 * nasmCompareAndSet - Generates code to compare two registers and set a
 * third register based on the comparison result.
 *
 * @param ASTop The AST operation code representing the comparison.
 * @param r1 Index of the first register.
 * @param r2 Index of the second register.
 *
 * @return Index of the register containing the comparison result (0 or 1).
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
 * @param r Index of the register containing the value.
 * @param oldPrimitiveType The original primitive type.
 * @param newPrimitiveType The target primitive type.
 *
 * NOTE:
 * For x86_64, no action is needed since all integers are treated as 64-bit.
 *
 * @return Index of the register containing the (possibly widened) value.
 */
int nasmWidenPrimitiveType(int r, int oldPrimitiveType, int newPrimitiveType) {
    // No action needed for x86_64 as all integers are treated as 64-bit
    return r;
}

/**
 * nasmAddressOfGlobalSymbol - Generates code to get the address of a global
 * symbol. Returns the address in a register.
 *
 * @param id The ID of the global symbol in the symbol table.
 *
 * @return Index of the register containing the address of the global
 * symbol.
 */
int nasmAddressOfGlobalSymbol(int id) {
    int r = allocateRegister();

    fprintf(Outfile, "\tlea\t%s, [rel %s]\n",
            qwordRegisterList[r],      // destination register
            GlobalSymbolTable[id].name // source global symbol
    );
    return r;
}

/**
 * nasmDereferencePointer - Generates code to dereference a pointer stored
 * in a register.
 *
 * @param pointerReg Index of the register containing the pointer.
 * @param primitiveType The primitive type of the value being pointed to.
 *
 * @return Index of the register containing the dereferenced value.
 */
int nasmDereferencePointer(int pointerReg, int primitiveType) {
    switch (primitiveType) {
    case P_CHARPTR:
        fprintf(Outfile, "\tmovzx\t%s, BYTE [%s]\n",
                qwordRegisterList[pointerReg], // destination register
                qwordRegisterList[pointerReg]  // source pointer register
        );
        break;
    case P_INTPTR:
        fprintf(Outfile, "\tmov\t%s, DWORD [%s]\n",
                dwordRegisterList[pointerReg], // destination register
                qwordRegisterList[pointerReg]  // source pointer register
        );
        break;
    case P_VOIDPTR:
    case P_LONGPTR:
        fprintf(Outfile, "\tmov\t%s, QWORD [%s]\n",
                qwordRegisterList[pointerReg], // destination register
                qwordRegisterList[pointerReg]  // source pointer register
        );
        break;
    }

    return pointerReg;
}

/**
 * nasmStoreDereferencedPointer - Generates code to store a value from a
 * register into a memory location pointed to by another register.
 *
 * @param valueReg Index of the register containing the value to store.
 * @param pointerReg Index of the register containing the pointer.
 * @param primitiveType The primitive type of the value being stored.
 *
 * @return Index of the register that was stored.
 */
int nasmStoreDereferencedPointer(int valueReg, int pointerReg,
                                 int primitiveType) {
    switch (primitiveType) {
    case P_CHAR:
        fprintf(Outfile, "\tmov\tBYTE [%s], %s\n",
                qwordRegisterList[pointerReg], // destination pointer register
                byteRegisterList[valueReg]     // source register (lower 8 bits)
        );
        break;
    case P_INT:
        fprintf(Outfile, "\tmov\tDWORD [%s], %s\n",
                qwordRegisterList[pointerReg], // destination pointer register
                dwordRegisterList[valueReg]    // source register (lower 32
                                               // bits)
        );
        break;
    case P_LONG:
        fprintf(Outfile, "\tmov\tQWORD [%s], %s\n",
                qwordRegisterList[pointerReg], // destination pointer register
                qwordRegisterList[valueReg]    // source register
        );
        break;
    default:
        fprintf(stderr,
                "Error: Unsupported primitive type %d in "
                "nasmStoreDereferencedPointer\n",
                primitiveType);
        exit(1);
    }

    return valueReg;
}
