// src/expr.c

#include "data.h"
#include "decl.h"
#include "defs.h"

/**
 * functionCall - Parse a function call expression.
 * e.g., foo(42);
 *
 * @return ASTnode* The AST node representing the function call.
 */
struct ASTnode *functionCall(void) {
    struct ASTnode *treeNode = NULL;
    int id;

    // Identifier
    if ((id = findGlobalSymbol(Text)) == -1) {
        logFatals("Undeclared function: ", Text);
    }

    // Left parenthesis ("(")
    matchLeftParenthesisToken();

    // Parse the following expression
    treeNode = binexpr(0);

    // Build the function call AST node.
    // - Store the function's return type as this node's type.
    // - Record the function's symbol ID
    treeNode = makeASTUnary(A_FUNCTIONCALL, GlobalSymbolTable[id].primitiveType,
                            treeNode, id);

    // Right parenthesis (")")
    matchRightParenthesisToken();

    return treeNode;
}

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
        // This could be a variable name or a function name (to call function).
        // Scan in the next token to find out. (LL(1))
        scan(&Token);

        if (Token.token == T_LPAREN) {
            return functionCall();
        }

        // Not a function call, so reject the new token
        rejectToken(&Token);

        // Check if the variable exists
        id = findGlobalSymbol(Text);
        if (id == -1) {
            logFatals("Undeclared variable: ", Text);
        }

        // Create a leaf AST node for the variable
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
 * prefix - Parse a prefix expression and return a sub-tree representing it
 *
 * NOTE:
 * prefix_expression := primary_expression
 *      | '*' prefix_expression
 *      | '&' prefix_expression
 *      ;
 */
struct ASTnode *prefix(void) {
    struct ASTnode *tree;
    switch (Token.token) {
    case T_AMPERSAND:
        /**
         * NOTE: & operator (address-of)
         * It must be applied to an identifier only.
         * This operator returns the address of the variable.
         */

        // Get the next token and parse it,
        // recursively as a prefix expression
        scan(&Token);
        tree = prefix();

        if (tree->op != A_IDENTIFIER) {
            logFatal(
                "Address-of operator '&' must be applied to an identifier");
        }

        // Change the operator to A_ADDRESSOF and the type to
        // a pointer to the original type
        tree->op = A_ADDRESSOF;
        tree->primitiveType = primitiveTypeToPointerType(tree->primitiveType);
        break;
    case T_STAR:
        /**
         * NOTE: * operator (pointer dereference)
         * It must be applied to a pointer type only.
         * This operator returns the value at the address pointed to.
         */

        // Get the next token and parse it,
        // recursively as a prefix expression
        scan(&Token);
        tree = prefix();

        if (tree->op != A_IDENTIFIER && tree->op != A_DEREFERENCE) {
            logFatal(
                "Dereference operator '*' must be applied to a pointer (*)");
        }

        // Prepend an A_DEREF operation to the tree
        tree =
            makeASTUnary(A_DEREFERENCE,
                         pointerToPrimitiveType(tree->primitiveType), tree, 0);
        break;
    default:
        // Otherwise, parse as a primary expression
        tree = primary();
        break;
    }
    return tree;
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

    // Parse the left-hand side expression
    // And, fetch the next token at the same time
    left = prefix();

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
            left = makeASTUnary(leftPrimitiveType,    // A_WIDENTYPE,
                                right->primitiveType, // type of RHS
                                left, 0);
        }
        if (rightPrimitiveType) {
            right = makeASTUnary(rightPrimitiveType,  // A_WIDENTYPE,
                                 left->primitiveType, // type of LHS
                                 right, 0);
        }

        left =
            makeASTNode(tokenToASTOperator(tokentype),
                        left->primitiveType, // Result type is the widened type
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
