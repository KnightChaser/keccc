// src/expr.c

#include "data.h"
#include "decl.h"
#include "defs.h"

/**
 * primary - Parse a primary expression.
 * e.g., integer literals.
 *
 * @return ASTnode* The AST node representing the primary expression.
 */
static struct ASTnode *primary(void) {
    struct ASTnode *n = NULL;
    int id;

    // For an INTLIT token, make a leaf AST node for it
    // and scan in the next token. Otherwise, a syntax error
    // for any other token type.
    switch (Token.token) {
    case T_INTLIT:
        // If it's an integer literal, create a leaf node.
        // Then scan the next token. It will be used by the caller.
        n = makeASTLeaf(A_INTLIT, Token.intvalue);
        break;

    case T_IDENTIFIER:
        // Check that if this identifier exists
        id = findGlobalSymbol(Text);
        if (id == -1) {
            logFatals("Undeclared identifier: ", Text);
        }

        n = makeASTLeaf(A_IDENTIFIER, id);
        break;

    default:
        logFatald("Syntax error: unexpected token ", Token.token);
    }

    // Scan the next token and return the leaf node
    scan(&Token);
    return n;
}

/**
 * tokenToASTOperator - Convert token into corresponding AST node
 * e.g. T_PLUS(+) token means A_ADD, which is the AST node type
 *
 * @param token The token to map.
 * @return int The corresponding arithmetic operator.
 */
int tokenToASTOperator(int token) {
    switch (token) {
    // Arithmetic operators
    case T_PLUS:
        return A_ADD;
    case T_MINUS:
        return A_SUBTRACT;
    case T_STAR:
        return A_MULTIPLY;
    case T_SLASH:
        return A_DIVIDE;

    // Comparison operators
    case T_EQ:
        return A_EQ;
    case T_NE:
        return A_NE;
    case T_LT:
        return A_LT;
    case T_GT:
        return A_GT;
    case T_LE:
        return A_LE;
    case T_GE:
        return A_GE;

    default:
        fprintf(stderr, "Unknown arithmetic operator: %d, line: %d\n", token,
                Line);
        exit(1);
    }
}

// Operator precedence for each token
//
// NOTE:
// Match up with the order of tokens declared in src/defs.h
//
// NOTE:
// Based on the C language operator precedence:
// https://en.cppreference.com/w/c/language/operator_precedence.html
static int OpPrecedence[] = {
    [T_EOF] = 0,    // End of file
    [T_PLUS] = 10,  // Composes additive expressions
    [T_MINUS] = 10, // Composes additive expressions
    [T_STAR] = 20,  // Composes multiplicative expressions
    [T_SLASH] = 20, // Composes multiplicative expressions
    [T_EQ] = 30,    // Equality operator
    [T_NE] = 30,    // Inequality operator
    [T_LT] = 40,    // Relational operators
    [T_GT] = 40,    // Relational operators
    [T_LE] = 40,    // Relational operators
    [T_GE] = 40,    // Relational operators
    [T_INTLIT] = 0, // Integer literals
};

/**
 * operatorPrecedence - Get the precedence of a given operator token.
 *
 * @param tokentype The token type to check.
 * @return int The precedence of the operator.
 */
static int operatorPrecedence(int tokentype) {
    int precedence = OpPrecedence[tokentype];
    if (precedence == 0) {
        fprintf(stderr, "Unknown operator: %d, line: %d\n", tokentype, Line);
        exit(1);
    }
    return precedence;
}

/**
 * binexpr - Parse a binary expression based on operator precedence.
 *
 * @param ptp The previous token precedence level.
 * @return ASTnode* The AST node representing the binary expression.
 */
struct ASTnode *binexpr(int ptp) {
    struct ASTnode *left, *right;
    int tokentype;

    // Get the integer literal on the leftest side,
    // and fetch the next token at the same time.
    left = primary();

    // If we hit a semicolon(";") or right parenthesis(")"),
    // it means it's end of the expression,
    // so we return just the left node. OvO
    tokentype = Token.token;
    if (tokentype == T_SEMICOLON || tokentype == T_RPAREN) {
        return left;
    }

    // While the precedence of this token is
    // more than that of the previous token precedence
    while (operatorPrecedence(tokentype) > ptp) {
        scan(&Token);

        // Recurse to get the right-hand side expression
        right = binexpr(operatorPrecedence(tokentype));

        // Combine left and right nodes into a binary AST node
        left = makeASTNode(tokenToASTOperator(tokentype), left, NULL, right, 0);

        // Update the details of the current token.
        // If we hit a semicolon, it means it's end of the sentence,
        // so we return just the left node. OvO
        tokentype = Token.token;
        if (tokentype == T_SEMICOLON) {
            return left;
        }
    }

    // Return the constructed AST node,
    // when the precedence is not higher than ptp.
    return left;
}
