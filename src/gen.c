// src/gen.c

/**
 * NOTE:
 * Generic code generator
 * (Backend-specific layer)
 */

#include "data.h"
#include "decl.h"
#include "defs.h"

/**
 * codegenAST - Generates code for the given AST node and its subtrees.
 *
 * @n: The AST node to generate code for.
 * @param reg: The register index to use for code generation.
 *
 * @return int The register index where the result is stored.
 */
int codegenAST(struct ASTnode *n, int reg) {
    int leftRegister, rightRegister;

    // Get the left and right sub-tree values
    if (n->left) {
        leftRegister = codegenAST(n->left, -1);
    }
    if (n->right) {
        rightRegister = codegenAST(n->right, leftRegister);
    }

    switch (n->op) {
    // Arithmetic operations
    case A_ADD:
        return nasmAddRegs(leftRegister, rightRegister);
    case A_SUBTRACT:
        return nasmSubRegs(leftRegister, rightRegister);
    case A_MULTIPLY:
        return nasmMulRegs(leftRegister, rightRegister);
    case A_DIVIDE:
        return nasmDivRegsSigned(leftRegister, rightRegister);

    // Comparison operations
    case A_EQ:
        return nasmCompareEqual(leftRegister, rightRegister);
    case A_NE:
        return nasmCompareNotEqual(leftRegister, rightRegister);
    case A_LT:
        return nasmCompareLessThan(leftRegister, rightRegister);
    case A_GT:
        return nasmCompareGreaterThan(leftRegister, rightRegister);
    case A_LE:
        return nasmCompareLessThanOrEqual(leftRegister, rightRegister);
    case A_GE:
        return nasmCompareGreaterThanOrEqual(leftRegister, rightRegister);

    // Leaf nodes
    case A_INTLIT:
        return nasmLoadImmediateInt(n->v.intvalue);
    case A_IDENTIFIER:
        return nasmLoadGlobalSymbol(
            GlobalSymbolTable[n->v.identifierIndex].name);
    case A_LVALUEIDENTIFIER:
        return nasmStoreGlobalSymbol(
            reg, GlobalSymbolTable[n->v.identifierIndex].name);
    case A_ASSIGN:
        // The work has already been done, return the result
        return rightRegister;

    default:
        // Should not reach here; unsupported operation
        logFatald("Unknown AST operator: ", n->op);
        return -1;
    }
}

/**
 * codegenPreamble - Wraps CPU-specific preamble generation.
 */
void codegenPreamble() { nasmPreamble(); }

/**
 * codegenPostamble - Wraps CPU-specific postamble generation.
 */
void codegenPostamble() { nasmPostamble(); }

/**
 * codegenResetRegisters - Frees all registers used during code generation.
 */
void codegenResetRegisters() { nasmResetRegisterPool(); }

/**
 * codegenPrintInt - Wraps CPU-specific integer printing.
 *
 * @reg: The register index containing the integer to print.
 */
void codegenPrintInt(int reg) { nasmPrintIntFromReg(reg); }

/**
 * codegenDeclareGlobalSymbol - Wraps CPU-specific global symbol generation.
 *
 * @name: The name of the global symbol.
 */
void codegenDeclareGlobalSymbol(char *name) { nasmDeclareGlobalSymbol(name); }
