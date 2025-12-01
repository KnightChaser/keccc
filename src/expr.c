// src/expr.c

#include "data.h"
#include "decl.h"
#include "defs.h"

/**
 * Parse a primary expression.
 * e.g., integer literals.
 *
 * @return ASTnode* The AST node representing the primary expression.
 */
static struct ASTnode *primary(void) {
    struct ASTnode *n;

    // For an INTLIT token, make a leaf AST node for it
    // and scan in the next token. Otherwise, a syntax error
    // for any other token type.
    switch (Token.token) {
    case T_INTLIT:
        n = mkastleaf(A_INTLIT, Token.intvalue);
        scan(&Token);
        return n;
    default:
        fprintf(stderr, "Syntax error, line: %d\n", Line);
        exit(1);
    }
}

/**
 * Convert a binary operator token to its corresponding AST arithmetic operator.
 *
 * @param token The token to map.
 * @return int The corresponding arithmetic operator.
 */
int arithmeticOperator(int token) {
    switch (token) {
    case T_PLUS:
        return (A_ADD);
    case T_MINUS:
        return (A_SUBTRACT);
    case T_STAR:
        return (A_MULTIPLY);
    case T_SLASH:
        return (A_DIVIDE);
    default:
        fprintf(stderr, "Unknown arithmetic operator: %d, line: %d\n", token,
                Line);
        exit(1);
    }
}

// Predefinition
struct ASTnode *additive_expr(void);

// Operator precedence for each token
static int OpPrecedence[] = {
    [T_EOF] = 0,    // End of file (receives lowest precedence)
    [T_PLUS] = 10,  // Composes additive expressions (medium precedence)
    [T_MINUS] = 10, // Composes additive expressions (medium precedence)
    [T_STAR] = 20,  // Composes multiplicative expressions (high precedence)
    [T_SLASH] = 20, // Composes multiplicative expressions (high precedence)
    [T_INTLIT] = 0, // Integer literals (not an operator)
};

/**
 * Get the precedence of a given operator token.
 *
 * @param tokentype The token type to check.
 * @return int The precedence of the operator.
 */
static int op_precedence(int tokentype) {
    int precedence = OpPrecedence[tokentype];
    if (precedence == 0) {
        fprintf(stderr, "Unknown operator: %d, line: %d\n", tokentype, Line);
        exit(1);
    }
    return precedence;
}

/**
 * Parse a binary expression based on operator precedence.
 *
 * @param ptp The previous token precedence level.
 * @return ASTnode* The AST node representing the binary expression.
 */
struct ASTnode *binexpr(int ptp) {
    struct ASTnode *left, *right;
    int tokentype;

    // Get the integer literal on the leftest side
    left = primary();

    // After processing the left side, check for binary operators
    tokentype = Token.token;
    if (tokentype == T_EOF) {
        // If no more tokens, return the left node
        return left;
    }

    while (op_precedence(tokentype) > ptp) {
        scan(&Token);

        // Recurse to get the right-hand side expression
        right = binexpr(op_precedence(tokentype));

        // Combine left and right nodes into a binary AST node
        left = mkastnode(arithmeticOperator(tokentype), left, right, 0);

        // Update the token type for the next iteration
        tokentype = Token.token;
        if (tokentype == T_EOF) {
            return left;
        }
    }

    // Return the constructed AST node
    return left;
}
