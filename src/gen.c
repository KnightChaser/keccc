// src/gen.c

/**
 * NOTE:
 * Generic code generator
 * (Backend-specific layer)
 */

#include "cgn/cg_ops.h"
#include "data.h"
#include "decl.h"
#include "defs.h"

/**
 * codegenGetLabelNumber - Generates a unique label number for code generation.
 *
 * @return int A unique label number.
 */
int codegenGetLabelNumber(void) {
    static int id = 1;
    return (id++);
}

/**
 * codegenLabel - Outputs a label in the assembly code
 * for the current target backend.
 *
 * @param label The label number to output.
 */
static void codegenLabel(int label) { CG->label(label); }

/**
 * codegenJump - Generates an unconditional jump to a label
 * for the current target backend.
 *
 * @param label The label number to jump to.
 */
static void codegenJump(int label) { CG->jump(label); }

/**
 * codegenCompareAndJump - Generates code to compare two registers
 * and jump to a label based on the comparison result
 * for the current target backend.
 *
 * @param ASTop The AST operation code representing the comparison.
 * @param r1 Index of the first register.
 * @param r2 Index of the second register.
 * @param label The label number to jump to if the comparison is true.
 *
 * @return The register index where the result is stored (NOREG).
 */
static int codegenCompareAndJump(int ASTop, int r1, int r2, int label) {
    return CG->compareAndJump(ASTop, r1, r2, label);
}

/**
 * codegenIfStatementAST - Generates code for an IF statement AST node.
 *
 * NOTE:
 * The If statement is represented in the AST as follows:
 * ----------------------------------------
 *        [  A_IF  ]
 *        /   |    \    cond  true  false
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
 * @param n The AST node representing the IF statement.
 *
 * @return The register index where the result is stored (NOREG).
 */
static int codegenIfStatementAST(struct ASTnode *n) {
    int labelFalseStatement;
    int labelEndStatement;

    // Generate two labels:
    // - one for the false branch
    // - one for the end of the if statement
    // (When there is no ELSE clause, labelFalseStatement is the ending label.
    labelFalseStatement = codegenGetLabelNumber();
    if (n->right) {
        labelEndStatement = codegenGetLabelNumber();
    }

    // WARNING:
    // Jump to false label when condition is FALSE
    codegenAST(n->left, labelFalseStatement, n->op);
    codegenResetRegisters();

    // Generate the true branch's compound statement
    codegenAST(n->middle, NOLABEL, n->op);
    codegenResetRegisters();

    if (n->right) {
        codegenJump(labelEndStatement);
    }

    codegenLabel(labelFalseStatement);

    // Optional ELSE clause exists
    // Generate the false compount statement and the end label
    if (n->right) {
        codegenAST(n->right, NOREG, n->op);
        codegenResetRegisters();
        codegenLabel(labelEndStatement);
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
 *          /      \
 *        cond     body
 *       (left)  (middle)
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
 * @param n The AST node representing the WHILE statement.
 *
 * @return The register index where the result is stored (NOREG).
 */
static int codegenWhileStatementAST(struct ASTnode *n) {
    int labelStartLoop;
    int labelEndLoop;

    // Generate two labels:
    // - one for the start of the loop
    // - one for the end of the loop
    labelStartLoop = codegenGetLabelNumber();
    labelEndLoop = codegenGetLabelNumber();
    codegenLabel(labelStartLoop);

    // Generate the loop condition
    codegenAST(n->left, labelEndLoop, n->op);
    codegenResetRegisters();

    // Generate the loop body (stored in right child for WHILE)
    codegenAST(n->right, NOLABEL, n->op);
    codegenResetRegisters();

    // Jump back to the start of the loop
    codegenJump(labelStartLoop);
    codegenLabel(labelEndLoop);

    return NOREG;
}

/**
 * codegenAST - Generates code for the given AST node and its subtrees.
 *
 * @param n           The AST node to generate code for.
 * @param label       The label number for jump instructions (if needed).
 * @param parentASTop The operator of the parent AST node.
 *
 * NOTE:
 * If parentASTop is A_IF, comparison operations will generate
 * a jump instruction instead of setting a register value.
 * (e.g., `if (b < c) { ... }` )
 * Otherwise, comparison operations will set a register to 1 or 0
 * (e.g., `int a = (b < c);` )
 *
 * @return The register index where the result is stored.
 */
int codegenAST(struct ASTnode *n, int label, int parentASTop) {
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
        codegenAST(n->left, NOLABEL, n->op);
        CG->resetRegisters();
        codegenAST(n->right, NOLABEL, n->op);
        CG->resetRegisters();
        return NOREG;
    case A_FUNCTION:
        CG->functionPreamble(n->v.identifierIndex);
        codegenAST(n->left, NOLABEL, n->op);
        CG->functionPostamble(n->v.identifierIndex);
        return NOREG;
    }

    // NOTE:
    // General AST node handling below

    // Get the left and right sub-tree value
    if (n->left) {
        // Use NOREG because left subtree can use any register
        leftRegister = codegenAST(n->left, NOLABEL, n->op);
    }
    if (n->right) {
        rightRegister = codegenAST(n->right, NOLABEL, n->op);
    }

    switch (n->op) {
    // Arithmetic operations
    case A_ADD:
        return CG->addRegs(leftRegister, rightRegister);
    case A_SUBTRACT:
        return CG->subRegs(leftRegister, rightRegister);
    case A_MULTIPLY:
        return CG->mulRegs(leftRegister, rightRegister);
    case A_DIVIDE:
        return CG->divRegsSigned(leftRegister, rightRegister);

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
            return codegenCompareAndJump(n->op, leftRegister, rightRegister,
                                         label);
        } else {
            return CG->compareAndSet(n->op, leftRegister, rightRegister);
        }

    // Leaf nodes
    case A_INTEGERLITERAL:
        return CG->loadImmediateInt(n->v.intvalue, n->primitiveType);
    case A_STRINGLITERAL:
        return CG->loadGlobalString(n->v.identifierIndex);
    case A_IDENTIFIER:
        // Arrays are not scalar variables holding a pointer value.
        // In expressions, an array name evaluates to the address of its first
        // element ("array-to-pointer decay").
        if (GlobalSymbolTable[n->v.identifierIndex].structuralType == S_ARRAY) {
            return CG->addressOfGlobalSymbol(n->v.identifierIndex);
        }

        if (n->isRvalue || parentASTop == A_DEREFERENCE) {
            return CG->loadGlobalSymbol(n->v.identifierIndex);
        } else {
            return NOREG; // Lvalue: return NOREG to indicate address needed
        }
    case A_ASSIGN:
        switch (n->right->op) {
        case A_IDENTIFIER:
            return CG->storeGlobalSymbol(leftRegister,
                                         n->right->v.identifierIndex);
        case A_DEREFERENCE:
            return CG->storeDereferencedPointer(leftRegister, rightRegister,
                                                n->right->primitiveType);
        default:
            logFatald("can't assign (A_ASSIGN) to this AST node type: ", n->op);
        }
    case A_WIDENTYPE:
        // Widen the child node's primitive type to the parent node's type
        return CG->widenPrimitiveType(leftRegister, n->left->primitiveType,
                                      n->primitiveType);
    case A_RETURN:
        CG->returnFromFunction(leftRegister, CurrentFunctionSymbolID);
        return NOREG;
    case A_FUNCTIONCALL:
        return CG->functionCall(leftRegister, n->v.identifierIndex);
    case A_ADDRESSOF:
        return CG->addressOfGlobalSymbol(n->v.identifierIndex);
    case A_DEREFERENCE:
        if (n->isRvalue) {
            return CG->dereferencePointer(leftRegister, n->left->primitiveType);
        } else {
            return leftRegister; // Lvalue: return address in leftRegister;
        }
    case A_SCALETYPE:
        // Small optimization:
        // use shift if the scale value is a known power of 2
        switch (n->v.size) {
        case 2:
            return CG->shiftLeftConst(leftRegister, 1);
        case 4:
            return CG->shiftLeftConst(leftRegister, 2);
        case 8:
            return CG->shiftLeftConst(leftRegister, 3);
        default:
            // Load a register with the size and multiply
            // the left register by this size...
            rightRegister = CG->loadImmediateInt(n->v.size, P_INT);
            return CG->mulRegs(leftRegister, rightRegister);
        }

    default:
        // Should not reach here; unsupported operation
        logFatald("Unknown AST operator: ", n->op);
        return NOREG; // Unreachable
    }
}

/**
 * codegenPreamble - Wraps CPU-specific preamble generation.
 */
void codegenPreamble() { CG->preamble(); }

/**
 * codegenPostamble - Wraps CPU-specific postamble generation.
 */
void codegenPostamble() { CG->postamble(); }

/**
 * codegenResetRegisters - Frees all registers used during code generation.
 */
void codegenResetRegisters() { CG->resetRegisters(); }

/**
 * codegenDeclareGlobalSymbol - Wraps CPU-specific global symbol generation.
 *
 * @param id The index of the global symbol to declare.
 */
void codegenDeclareGlobalSymbol(int id) { CG->declareGlobalSymbol(id); }

/**
 * codegenDeclareGlobalString - Declares a global string in the data segment.
 *
 * @param stringvalue The string value to declare.
 *
 * @return int The label number assigned to the string.
 */
int codegenDeclareGlobalString(char *stringvalue) {
    int label = codegenGetLabelNumber();
    CG->declareGlobalString(label, stringvalue);
    return label;
}

/**
 * codegenGetPrimitiveTypeSize - Wraps CPU-specific primitive type size query.
 *
 * @param primitiveType The primitive type to query.
 *
 * @return int The size of the primitive type in bytes.
 */
int codegenGetPrimitiveTypeSize(int primitiveType) {
    return CG->getPrimitiveTypeSize(primitiveType);
}
