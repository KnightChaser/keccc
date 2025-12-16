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
 * tokenToASTOperator - Covnert a binary operator token into a binary AST
 * operation.
 * e.g. T_PLUS(+) token means A_ADD, which is the AST node type
 *
 * @param token The token to map.
 *
 * @return int The corresponding arithmetic operator.
 */
int tokenToASTOperator(int token) {
    switch (token) {
    // Assignment
    case T_ASSIGN:
        return A_ASSIGN;

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
 * isTokenRightAssociative - Check if a token is right associative.
 *
 * @param tokentype The token type to check.
 *
 * @return bool True if the token is right associative, false otherwise.
 */
static bool isTokenRightAssociative(int tokentype) {
    switch (tokentype) {
    case T_ASSIGN:
        return true;
    default:
        return false;
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
 *
 * @return int The precedence of the operator.
 */
static int operatorPrecedence(int tokentype) {
    switch (tokentype) {
    case T_EOF:    // (end of file)
        return 0;  //
    case T_ASSIGN: // Assignment (=)
        return 10; //
    case T_PLUS:   // Addition (+)
    case T_MINUS:  // Subtraction (-)
        return 20; //
    case T_STAR:   // Multiplication (*)
    case T_SLASH:  // Division (/)
        return 30; //
    case T_EQ:     // Equal (==)
    case T_NE:     // Not Equal (!=)
        return 40; //
    case T_LT:     // Less Than (<)
    case T_GT:     // Greater Than (>)
    case T_LE:     // Less Than or Equal (<=)
    case T_GE:     // Greater Than or Equal (>=)
        return 50; //
    default:       //
        return 0;  // e.g. T_SEMICOLON, T_RPAREN, etc.
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
 *
 * @return ASTnode* The AST node representing the prefix expression.
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
 *
 * @return ASTnode* The AST node representing the binary expression.
 */
struct ASTnode *binexpr(int ptp) {
    struct ASTnode *left;
    struct ASTnode *right;
    struct ASTnode *leftTemp;
    struct ASTnode *rightTemp;
    int tokentype;
    int ASToperation;

    // Parse the left-hand side expression
    // And, fetch the next token at the same time
    left = prefix();

    // If we hit a semicolon(";") or right parenthesis(")"),
    // it means it's end of the expression,
    // so we return just the left node. OvO
    tokentype = Token.token;
    if (tokentype == T_SEMICOLON || tokentype == T_RPAREN) {
        left->isRvalue = true; // Means this node is an r-value
        return left;
    }

    // NOTE:
    // While
    // - the precedence of this token is more than that of the previous
    //   token precedence, or,
    // - It's right associative and the precedence is equal to that of
    //   the previous token precedence ("=" chains, like "a = b = c")
    while ((operatorPrecedence(tokentype) > ptp) ||
           (isTokenRightAssociative(tokentype) &&
            operatorPrecedence(tokentype) == ptp)) {
        // Fetch in the next integer literal
        scan(&Token);

        // Recursively parse the RHS
        right = binexpr(operatorPrecedence(tokentype));

        // Determine the operation to be performed on the sub-trees
        ASToperation = tokenToASTOperator(tokentype);
        if (ASToperation == A_ASSIGN) {
            // assignment, the current node is a r-value
            // because "b = (something)" needs the value of "something"
            right->isRvalue = true;

            // Ensure the right's type matches the left.
            right = modifyASTType(right, left->primitiveType, A_NOTHING);

            // NOTE: Sure about this?
            if (left == NULL) {
                logFatal("Incompatible expression in assignment");
            }

            // Make an assignment AST tree.
            // However, switch left and right around,
            // so that the right expression's code will be generated before the
            // left expression.
            leftTemp = left;
            left = right;
            right = leftTemp;

            // Mark the LHS (now in right subtree) as an lvalue so that
            // dereference on the LHS yields an address, not a loaded value.
            if (right) {
                right->isRvalue = false;
            }
        } else {
            // Normal arithmetic operations or comparisons
            // We are not doing an assignment, so both trees should be rvalues
            // Convert both trees into rvalues if they are lvalue trees
            left->isRvalue = true;
            right->isRvalue = true;

            // Ensure the two types are compatible by trying to modify each tree
            // to match the other's type
            leftTemp = modifyASTType(left, right->primitiveType, ASToperation);
            rightTemp = modifyASTType(right, left->primitiveType, ASToperation);

            if (leftTemp == NULL && rightTemp == NULL) {
                logFatal("Incompatible types in binary expression");
            }

            // If one side could be converted, use that side's converted tree
            if (leftTemp != NULL) {
                left = leftTemp;
            }
            if (rightTemp != NULL) {
                right = rightTemp;
            }
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
            left->isRvalue = true; // Means this node is an r-value
            return left;
        }
    }

    // Return the constructed AST node,
    // when the precedence is not higher than ptp.
    left->isRvalue = true; // Means this node is an r-value
    return left;
}
