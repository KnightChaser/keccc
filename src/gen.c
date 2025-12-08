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
 * getLabelNumber - Generates a unique label number for code generation.
 *
 * @return int A unique label number.
 */
static int getLabelNumber(void) {
    static int id = 1;
    return (id++);
}

/**
 * codegenIfStatementAST - Generates code for an IF statement AST node.
 *
 * NOTE:
 * The If statement is represented in the AST as follows:
 * ----------------------------------------
 *        [  A_IF  ]
 *        /   |    \
 *    cond  true  false
 *  (left)(middle)(right)
 * ----------------------------------------
 * Conventional if statement will be
 * converted into the following assembly
 * structure
 * ----------------------------------------
 *        perform the opposite comparison
 *        jump to L1 if true
 *        perform the first block of code
 *        jump to L2
 * L1:
 *        perform the other block of code
 * L2:
 * ----------------------------------------
 *
 * @n: The AST node representing the IF statement.
 *
 * @return int The register index where the result is stored (NOREG).
 */
static int codegenIfStatementAST(struct ASTnode *n) {
    int labelFalseStatement;
    int labelEndStatement;

    // Generate two labels:
    // - one for the false branch
    // - one for the end of the if statement
    // (When there is no ELSE clause, labelFalseStatement is the ending label.
    labelFalseStatement = getLabelNumber();
    if (n->right) {
        labelEndStatement = getLabelNumber();
    }

    // WARNING:
    // Jump to false label when condition is FALSE
    codegenAST(n->left, labelFalseStatement, n->op);
    codegenResetRegisters();

    // Generate the true branch's compound statement
    codegenAST(n->middle, NOREG, n->op);
    codegenResetRegisters();

    if (n->right) {
        nasmJump(labelEndStatement);
    }

    nasmLabel(labelFalseStatement);

    // Optional ELSE clause exists
    // Generate the false compount statement and the end label
    if (n->right) {
        codegenAST(n->right, NOREG, n->op);
        codegenResetRegisters();
        nasmLabel(labelEndStatement);
    }

    return NOREG;
}

/**
 * codegenWhileStatementAST - Generates code for a WHILE statement AST node.
 *
 * NOTE:
 * The While statement is represented in the AST as follows:
 * ----------------------------------------
 *       [  A_WHILE  ]
 *        /     \
 *      cond     body
 *     (left)   (middle)
 * ----------------------------------------
 * Conventional while statement will be
 * converted into the following assembly
 * structure
 * ----------------------------------------
 * L1:    perform the comparison
 *        jump to L2 if false
 *        perform the loop body
 *        jump to L1
 * L2:
 * ----------------------------------------
 *
 * @n: The AST node representing the WHILE statement.
 *
 * @return int The register index where the result is stored (NOREG).
 */
static int codegenWhileStatementAST(struct ASTnode *n) {
    int labelStartLoop;
    int labelEndLoop;

    // Generate two labels:
    // - one for the start of the loop
    // - one for the end of the loop
    labelStartLoop = getLabelNumber();
    labelEndLoop = getLabelNumber();
    nasmLabel(labelStartLoop);

    // Generate the loop condition
    codegenAST(n->left, labelEndLoop, n->op);
    codegenResetRegisters();

    // Generate the loop body (stored in right child for WHILE)
    codegenAST(n->right, NOREG, n->op);
    codegenResetRegisters();

    // Jump back to the start of the loop
    nasmJump(labelStartLoop);
    nasmLabel(labelEndLoop);

    return NOREG;
}

/**
 * codegenAST - Generates code for the given AST node and its subtrees.
 *
 * @n: The AST node to generate code for.
 * @param reg: The register index to use for code generation.
 * @param parentASTop: The operator of the parent AST node.
 *
 * NOTE:
 * If parentASTop is A_IF, comparison operations will generate
 * a jump instruction instead of setting a register value.
 * (e.g., if (b < c) { ... } )
 * Otherwise, comparison operations will set a register to 1 or 0
 * (e.g., int a = (b < c); )
 *
 * @return int The register index where the result is stored.
 */
int codegenAST(struct ASTnode *n, int reg, int parentASTop) {
    int leftRegister, rightRegister;

    if (n == NULL) {
        return NOREG;
    }

    switch (n->op) {
    case A_IF:
        // If statement
        return codegenIfStatementAST(n);
    case A_WHILE:
        // While statement
        return codegenWhileStatementAST(n);
    case A_GLUE:
        // Do each sub-tree separately,
        // and return NOREG since GLUE does not produce a value
        // Then free registers used in each sub-tree
        codegenAST(n->left, NOREG, n->op);
        nasmResetRegisterPool();
        if (n->right) {
            codegenAST(n->right, NOREG, n->op);
            nasmResetRegisterPool();
        }
        return NOREG;
    case A_FUNCTION:
        nasmFunctionPreamble(GlobalSymbolTable[n->v.identifierIndex].name);
        codegenAST(n->left, NOREG, n->op);
        nasmFunctionPostamble();
        return NOREG;
    }

    // NOTE:
    // General AST node handling below

    // Get the left and right sub-tree value
    if (n->left) {
        // Use NOREG because left subtree can use any register
        leftRegister = codegenAST(n->left, NOREG, n->op);
    }
    if (n->right) {
        rightRegister = codegenAST(n->right, leftRegister, n->op);
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
    case A_NE:
    case A_LT:
    case A_GT:
    case A_LE:
    case A_GE:
        // If the parent ASFT node is an A_IF,
        // generate a compare followed by a jjump.
        // Otherwise, compare registers and set one to 1 or 0 based on the
        // comparison.
        if (parentASTop == A_IF || parentASTop == A_WHILE) {
            return nasmCompareAndJump(n->op, leftRegister, rightRegister, reg);
        } else {
            return nasmCompareAndSet(n->op, leftRegister, rightRegister);
        }

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
    case A_PRINT:
        codegenPrintInt(leftRegister);
        codegenResetRegisters();
        return NOREG;

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
