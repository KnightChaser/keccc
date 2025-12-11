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
 * codegenlabel - Generates a unique label number for code generation.
 *
 * @return int A unique label number.
 */
int codegenLabel(void) {
    static int id = 1;
    return (id++);
}

/**
 * NOTE:
 * Small helpers to route to the correct backend
 * TODO:
 * Organize 'em
 */

/**
 * backendLabel - Outputs a label in the assembly code
 * for the current target backend.
 *
 * @label: The label number to output.
 */
static void backendLabel(int label) {
    if (CurrentTarget == TARGET_NASM) {
        nasmLabel(label);
        return;
    } else if (CurrentTarget == TARGET_ARM64) {
        arm64Label(label);
        return;
    }

    logFatal("Error: Unsupported target in backendLabel");
}

/**
 * backendJump - Generates an unconditional jump to a label
 * for the current target backend.
 *
 * @label: The label number to jump to.
 */
static void backendJump(int label) {
    if (CurrentTarget == TARGET_NASM) {
        nasmJump(label);
        return;
    } else if (CurrentTarget == TARGET_ARM64) {
        arm64Jump(label);
        return;
    }

    logFatal("Error: Unsupported target in backendJump");
}

/**
 * backendCompareAndJump - Generates code to compare two registers
 * and jump to a label based on the comparison result
 * for the current target backend.
 *
 * @ASTop: The AST operation code representing the comparison.
 * @r1: Index of the first register.
 * @r2: Index of the second register.
 * @label: The label number to jump to if the comparison is true.
 */
static int backendCompareAndJump(int ASTop, int r1, int r2, int label) {
    if (CurrentTarget == TARGET_NASM) {
        return nasmCompareAndJump(ASTop, r1, r2, label);
    } else if (CurrentTarget == TARGET_ARM64) {
        return arm64CompareAndJump(ASTop, r1, r2, label);
    }

    logFatal("Error: Unsupported target in backendCompareAndJump");
    return NOREG; // unreachable
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
    labelFalseStatement = codegenLabel();
    if (n->right) {
        labelEndStatement = codegenLabel();
    }

    // WARNING:
    // Jump to false label when condition is FALSE
    codegenAST(n->left, labelFalseStatement, n->op);
    codegenResetRegisters();

    // Generate the true branch's compound statement
    codegenAST(n->middle, NOREG, n->op);
    codegenResetRegisters();

    if (n->right) {
        // nasmJump(labelEndStatement);
        backendJump(labelEndStatement);
    }

    // nasmLabel(labelFalseStatement);
    backendLabel(labelFalseStatement);

    // Optional ELSE clause exists
    // Generate the false compount statement and the end label
    if (n->right) {
        codegenAST(n->right, NOREG, n->op);
        codegenResetRegisters();
        // nasmLabel(labelEndStatement);
        backendLabel(labelEndStatement);
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
    labelStartLoop = codegenLabel();
    labelEndLoop = codegenLabel();
    // nasmLabel(labelStartLoop);
    backendLabel(labelStartLoop);

    // Generate the loop condition
    codegenAST(n->left, labelEndLoop, n->op);
    codegenResetRegisters();

    // Generate the loop body (stored in right child for WHILE)
    codegenAST(n->right, NOREG, n->op);
    codegenResetRegisters();

    // Jump back to the start of the loop
    // nasmJump(labelStartLoop);
    // nasmLabel(labelEndLoop);
    backendJump(labelStartLoop);
    backendLabel(labelEndLoop);

    return NOREG;
}

/**
 * codegenAST - Generates code for the given AST node and its subtrees.
 *
 * @n:                 The AST node to generate code for.
 * @param reg:         The register index to use for code generation.
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
        // TODO: Refactor backend-specific reset register pool
        if (CurrentTarget == TARGET_NASM) {
            nasmResetRegisterPool();
        } else if (CurrentTarget == TARGET_ARM64) {
            arm64ResetRegisterPool();
        }
        codegenAST(n->right, NOREG, n->op);
        // TODO: Refactor backend-specific reset register pool
        if (CurrentTarget == TARGET_NASM) {
            nasmResetRegisterPool();
        } else if (CurrentTarget == TARGET_ARM64) {
            arm64ResetRegisterPool();
        }
        return NOREG;
    case A_FUNCTION:
        // TODO: Refactor backend-specific function preamble
        if (CurrentTarget == TARGET_NASM) {
            nasmFunctionPreamble(n->v.identifierIndex);
        } else if (CurrentTarget == TARGET_ARM64) {
            arm64FunctionPreamble(n->v.identifierIndex);
        }
        codegenAST(n->left, NOREG, n->op);
        // TODO: Refactor backend-specific function postamble
        if (CurrentTarget == TARGET_NASM) {
            nasmFunctionPostamble(n->v.identifierIndex);
        } else if (CurrentTarget == TARGET_ARM64) {
            arm64FunctionPostamble(n->v.identifierIndex);
        }
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
    // TODO: Refactor backend-specific arithmetic operations
    case A_ADD:
        return (CurrentTarget == TARGET_NASM)
                   ? nasmAddRegs(leftRegister, rightRegister)
                   : arm64AddRegs(leftRegister, rightRegister);
    case A_SUBTRACT:
        return (CurrentTarget == TARGET_NASM)
                   ? nasmSubRegs(leftRegister, rightRegister)
                   : arm64SubRegs(leftRegister, rightRegister);
    case A_MULTIPLY:
        return (CurrentTarget == TARGET_NASM)
                   ? nasmMulRegs(leftRegister, rightRegister)
                   : arm64MulRegs(leftRegister, rightRegister);
    case A_DIVIDE:
        return (CurrentTarget == TARGET_NASM)
                   ? nasmDivRegsSigned(leftRegister, rightRegister)
                   : arm64DivRegsSigned(leftRegister, rightRegister);

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
            // return nasmCompareAndJump(n->op, leftRegister, rightRegister,
            // reg);
            return backendCompareAndJump(n->op, leftRegister, rightRegister,
                                         reg);
        } else {
            // return nasmCompareAndSet(n->op, leftRegister, rightRegister);
            // TODO: Refactor backend-specific compare and set
            if (CurrentTarget == TARGET_NASM) {
                return nasmCompareAndSet(n->op, leftRegister, rightRegister);
            } else if (CurrentTarget == TARGET_ARM64) {
                return arm64CompareAndSet(n->op, leftRegister, rightRegister);
            }
        }

    // Leaf nodes
    case A_INTLIT:
        return (CurrentTarget == TARGET_NASM)
                   ? nasmLoadImmediateInt(n->v.intvalue, n->primitiveType)
                   : arm64LoadImmediateInt(n->v.intvalue, n->primitiveType);
    case A_IDENTIFIER:
        return (CurrentTarget == TARGET_NASM)
                   ? nasmLoadGlobalSymbol(n->v.identifierIndex)
                   : arm64LoadGlobalSymbol(n->v.identifierIndex);
    case A_LVALUEIDENTIFIER:
        return (CurrentTarget == TARGET_NASM)
                   ? nasmStoreGlobalSymbol(leftRegister, n->v.identifierIndex)
                   : arm64StoreGlobalSymbol(leftRegister, n->v.identifierIndex);
    case A_ASSIGN:
        // The work has already been done, return the result
        return rightRegister;
    case A_PRINT:
        codegenPrintInt(leftRegister);
        codegenResetRegisters();
        return NOREG;
    case A_WIDENTYPE:
        // Widen the child node's primitive type to the parent node's type
        // return nasmWidenPrimitiveType(leftRegister, n->left->primitiveType,
        //                               n->primitiveType);
        // TODO: Refactor backend-specific widen primitive type
        return (CurrentTarget == TARGET_NASM)
                   ? nasmWidenPrimitiveType(
                         leftRegister, n->left->primitiveType, n->primitiveType)
                   : arm64WidenPrimitiveType(leftRegister,
                                             n->left->primitiveType,
                                             n->primitiveType);
    case A_RETURN:
        // nasmReturnFromFunction(leftRegister, CurrentFunctionSymbolID);
        // TODO: Refactor backend-specific return from function
        (CurrentTarget == TARGET_NASM)
            ? nasmReturnFromFunction(leftRegister, CurrentFunctionSymbolID)
            : arm64ReturnFromFunction(leftRegister, CurrentFunctionSymbolID);
        return NOREG;
    case A_FUNCTIONCALL:
        return (CurrentTarget == TARGET_NASM)
                   ? nasmFunctionCall(leftRegister, n->v.identifierIndex)
                   : arm64FunctionCall(leftRegister, n->v.identifierIndex);

    default:
        // Should not reach here; unsupported operation
        logFatald("Unknown AST operator: ", n->op);
        return -1;
    }
}

/**
 * codegenPreamble - Wraps CPU-specific preamble generation.
 */
void codegenPreamble() {
    switch (CurrentTarget) {
    case TARGET_NASM:
        nasmPreamble();
        break;
    case TARGET_ARM64:
        arm64Preamble();
        break;
    default:
        logFatal("Error: Unsupported target in codegenPreamble");
    }
}

/**
 * codegenPostamble - Wraps CPU-specific postamble generation.
 */
void codegenPostamble() {
    switch (CurrentTarget) {
    case TARGET_NASM:
        nasmPostamble();
        break;
    case TARGET_ARM64:
        arm64Postamble();
        break;
    default:
        logFatal("Error: Unsupported target in codegenPostamble");
    }
}

/**
 * codegenResetRegisters - Frees all registers used during code generation.
 */
void codegenResetRegisters() {
    switch (CurrentTarget) {
    case TARGET_NASM:
        nasmResetRegisterPool();
        break;
    case TARGET_ARM64:
        arm64ResetRegisterPool();
        break;
    default:
        logFatal("Error: Unsupported target in codegenResetRegisters");
    }
}

/**
 * codegenPrintInt - Wraps CPU-specific integer printing.
 *
 * @reg: The register index containing the integer to print.
 */
void codegenPrintInt(int reg) {
    switch (CurrentTarget) {
    case TARGET_NASM:
        nasmPrintIntFromReg(reg);
        break;
    case TARGET_ARM64:
        arm64PrintIntFromReg(reg);
        break;
    default:
        logFatal("Error: Unsupported target in codegenPrintInt");
    }
}

/**
 * codegenDeclareGlobalSymbol - Wraps CPU-specific global symbol generation.
 *
 * @id: The index of the global symbol to declare.
 */
void codegenDeclareGlobalSymbol(int id) {
    switch (CurrentTarget) {
    case TARGET_NASM:
        nasmDeclareGlobalSymbol(id);
        break;
    case TARGET_ARM64:
        arm64DeclareGlobalSymbol(id);
        break;
    default:
        logFatal("Error: Unsupported target in codegenDeclareGlobalSymbol");
    }
}

/**
 * codegenGetPrimitiveTypeSize - Wraps CPU-specific primitive type size query.
 *
 * @primitiveType: The primitive type to query.
 *
 * @return int The size of the primitive type in bytes.
 */
int codegenGetPrimitiveTypeSize(int primitiveType) {
    switch (CurrentTarget) {
    case TARGET_NASM:
        return nasmGetPrimitiveTypeSize(primitiveType);
    case TARGET_ARM64:
        return arm64GetPrimitiveTypeSize(primitiveType);
    default:
        logFatal("Error: Unsupported target in codegenGetPrimitiveTypeSize");
        return -1; // unreachable
    }
}
