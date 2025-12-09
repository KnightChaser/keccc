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

        // NOTE:
        // Make the primitive integer leaf node as P_CHAR if
        // the value is within char range. because we can optimize
        // memory usage later.
        if (Token.intvalue >= 0 && Token.intvalue <= 255) {
            n = makeASTLeaf(A_INTLIT, P_CHAR, Token.intvalue);
        } else {
            n = makeASTLeaf(A_INTLIT, P_INT, Token.intvalue);
        }

        break;

    case T_IDENTIFIER:
        // Check that if this identifier exists
        id = findGlobalSymbol(Text);
        if (id == -1) {
            logFatals("Undeclared identifier: ", Text);
        }

        n = makeASTLeaf(A_IDENTIFIER, GlobalSymbolTable[id].primitiveType, id);
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

/**
 * operatorPrecedence - Get the precedence of a given operator token.
 *
 *  NOTE:
 *  Based on the C language operator precedence:
 *  https://en.cppreference.com/w/c/language/operator_precedence.html
 *
 * @param tokentype The token type to check.
 * @return int The precedence of the operator.
 */
static int operatorPrecedence(int tokentype) {
    switch (tokentype) {
    case T_PLUS:  // Addition (+)
    case T_MINUS: // Subtraction (-)
        return 10;
    case T_STAR:  // Multiplication (*)
    case T_SLASH: // Division (/)
        return 20;
    case T_EQ: // Equal (==)
    case T_NE: // Not Equal (!=)
        return 30;
    case T_LT: // Less Than (<)
    case T_GT: // Greater Than (>)
    case T_LE: // Less Than or Equal (<=)
    case T_GE: // Greater Than or Equal (>=)
        return 40;
    default:
        // e.g. T_SEMICOLON, T_RPAREN, etc.
        return 0;
    }
}

/**
 * binexpr - Parse a binary expression based on operator precedence.
 *
 * @param ptp The previous token precedence level.
 * @return ASTnode* The AST node representing the binary expression.
 */
struct ASTnode *binexpr(int ptp) {
    struct ASTnode *left;
    struct ASTnode *right;
    int leftPrimitiveType;
    int rightPrimitiveType;
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

        // Ensure type compatibility between left and right nodes
        leftPrimitiveType = left->primitiveType;
        rightPrimitiveType = right->primitiveType;
        if (!checkPrimitiveTypeCompatibility(&leftPrimitiveType,
                                             &rightPrimitiveType, false)) {
            logFatal("Type error: incompatible types in binary expression");
        }

        // Widen the primitive type if necessary
        if (leftPrimitiveType) {
            left = makeASTNode(A_WIDENTYPE, leftPrimitiveType, left, NULL, NULL,
                               0);
        }
        if (rightPrimitiveType) {
            right = makeASTNode(A_WIDENTYPE, rightPrimitiveType, right, NULL,
                                NULL, 0);
        }

        // Combine left and right nodes into a binary AST node
        // Convert the token into an AST operation at the same time.
        left = makeASTNode(tokenToASTOperator(tokentype), leftPrimitiveType,
                           left, NULL, right, 0);

        // Update the details of the current token.
        // If we hit a semicolon, or right parenthesis,
        // it means it's end of the sentence,
        // so we return just the left node. OvO
        tokentype = Token.token;
        if (tokentype == T_SEMICOLON || tokentype == T_RPAREN) {
            return left;
        }
    }

    // Return the constructed AST node,
    // when the precedence is not higher than ptp.
    return left;
}
