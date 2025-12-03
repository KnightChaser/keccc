// src/gen.c

// Generic code generator

#include "data.h"
#include "decl.h"
#include "defs.h"

/**
 * genAST - Generates code for the given AST node and its subtrees.
 *
 * @n: The AST node to generate code for.
 * @param reg: The register index to use for code generation.
 *
 * @return int The register index where the result is stored.
 */
int genAST(struct ASTnode *n, int reg) {
    int leftRegister, rightRegister;

    // Get the left and right sub-tree values
    if (n->left) {
        leftRegister = genAST(n->left, -1);
    }
    if (n->right) {
        rightRegister = genAST(n->right, leftRegister);
    }

    switch (n->op) {
    case A_ADD:
        return cgadd(leftRegister, rightRegister);
    case A_SUBTRACT:
        return cgsub(leftRegister, rightRegister);
    case A_MULTIPLY:
        return cgmul(leftRegister, rightRegister);
    case A_DIVIDE:
        return cgdiv(leftRegister, rightRegister);
    case A_INTLIT:
        return cgloadint(n->v.intvalue);
    case A_IDENTIFIER:
        return cgloadglobsym(GlobalSymbolTable[n->v.identifierIndex].name);
    case A_LVALUEIDENTIFIER:
        return cgstoreglobsym(reg,
                              GlobalSymbolTable[n->v.identifierIndex].name);
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
 * genpreamble - Wraps CPU-specific preamble generation.
 */
void genpreamble() { cgpreamble(); }

/**
 * genpostamble - Wraps CPU-specific postamble generation.
 */
void genpostamble() { cgpostamble(); }

/**
 * genfreeregs - Frees all registers used during code generation.
 */
void genfreeregs() { freeAllRegisters(); }

/**
 * genprintint - Wraps CPU-specific integer printing.
 *
 * @reg: The register index containing the integer to print.
 */
void genprintint(int reg) { cgprintint(reg); }

/**
 * genglobsym - Wraps CPU-specific global symbol generation.
 *
 * @name: The name of the global symbol.
 */
void genglobsym(char *name) { cgglobsym(name); }
