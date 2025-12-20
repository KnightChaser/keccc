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
static struct ASTnode *functionCall(void) {
    struct ASTnode *treeNode = NULL;
    int id;

    // Identifier
    if ((id = findGlobalSymbol(Text)) == -1 ||
        GlobalSymbolTable[id].structuralType != S_FUNCTION) {
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
 * arrayAccess - Parse an array access expression.
 * e.g., arr[5];
 *
 * @return ASTnode* The AST node representing the array access.
 */
static struct ASTnode *arrayAccess(void) {
    struct ASTnode *leftNode = NULL;
    struct ASTnode *rightNode = NULL;
    int id;

    // NOTE:
    // Check that the identifier has been defined as an array,
    // then make a leaf node for it that points at the base.
    if ((id = findGlobalSymbol(Text)) == -1 ||
        GlobalSymbolTable[id].structuralType != S_ARRAY) {
        logFatals("Undeclared array: ", Text);
    }
    leftNode =
        makeASTLeaf(A_IDENTIFIER, GlobalSymbolTable[id].primitiveType, id);

    // '['
    scan(&Token);

    // Parse the following expression
    rightNode = binexpr(0);

    // ']'
    match(T_RBRACKET, "]");

    // Ensure the array index is an integer type
    if (!isIntegerType(rightNode->primitiveType)) {
        logFatal("Array index must be an integer type");
    }

    // Scale the index by the size of the element's type
    rightNode = coerceASTTypeForOp(rightNode, leftNode->primitiveType, A_ADD);

    // NOTE:
    // Return an AST tree where the array's base has the offset
    // added to it, and dereference the element. Still an lvalue
    // at this point.
    leftNode = makeASTNode(A_ADD, GlobalSymbolTable[id].primitiveType, leftNode,
                           NULL, rightNode, 0);
    leftNode = makeASTUnary(A_DEREFERENCE,
                            pointerToPrimitiveType(leftNode->primitiveType),
                            leftNode, 0);

    return leftNode;
}

/**
 * postfix - Parse a postfix expression.
 * e.g., variable with post-increment/decrement.
 *
 * @return ASTnode* The AST node representing the postfix expression.
 */
static struct ASTnode *postfix(void) {
    struct ASTnode *n;
    int id;

    // Scan in the next token to see if we have a postfix expression
    scan(&Token);

    // Scan in the next token to see if we have a postfix expression
    if (Token.token == T_LPAREN) {
        return functionCall();
    }

    // Or is this an array reference?
    if (Token.token == T_LBRACKET) {
        return arrayAccess();
    }

    // A variable
    id = findGlobalSymbol(Text);
    if (id == -1 || GlobalSymbolTable[id].structuralType != S_VARIABLE) {
        logFatals("Undeclared variable: ", Text);
    }

    switch (Token.token) {
    case T_INCREMENT:
        // Post increment: skip over the token
        scan(&Token);
        n = makeASTLeaf(A_POSTINCREMENT, GlobalSymbolTable[id].primitiveType,
                        id);
        break;

    case T_DECREMENT:
        // Post decrement: skip over the token
        scan(&Token);
        n = makeASTLeaf(A_POSTDECREMENT, GlobalSymbolTable[id].primitiveType,
                        id);
        break;

    default:
        // Just a variable inference
        n = makeASTLeaf(A_IDENTIFIER, GlobalSymbolTable[id].primitiveType, id);
        break;
    }

    return n;
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
    case T_INTEGERLITERAL:
        // If it's an integer literal, create a leaf node.
        // Then scan the next token. It will be used by the caller.

        // NOTE:
        // Make the primitive integer leaf node as P_CHAR if
        // the value is within char range. because we can optimize
        // memory usage later.
        if (Token.intvalue >= 0 && Token.intvalue <= 255) {
            n = makeASTLeaf(A_INTEGERLITERAL, P_CHAR, Token.intvalue);
        } else {
            n = makeASTLeaf(A_INTEGERLITERAL, P_INT, Token.intvalue);
        }

        break;

    case T_STRINGLITERAL:
        // For a string literal token, generate the assembly for this,
        // and then make a leaf AST node for it. "id" is the string's label
        id = codegenDeclareGlobalString(Text);
        n = makeASTLeaf(A_STRINGLITERAL, P_CHARPTR, id);
        break;

    case T_IDENTIFIER:
        return postfix();

    case T_LPAREN:
        // Beginning of a parenthesised expression, skip the '('.
        // Scan in the expression and the right parenthesis
        // It expects expression "( ... )", like "(a + b)".
        scan(&Token);
        n = binexpr(0);
        matchRightParenthesisToken();

        // matchRightParenthesisToken() already scanned the token after ')'.
        // Do not scan again at the end of primary(), or we will skip the
        // operator/token that follows the parenthesized expression.
        return n;

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
    case T_ASSIGN:           // =
        return A_ASSIGN;     //
    case T_LOGICALOR:        // ||
        return A_LOGICALOR;  //
    case T_LOGICALAND:       // &&
        return A_LOGICALAND; //
    case T_BITWISEOR:        // |
        return A_BITWISEOR;  //
    case T_BITWISEXOR:       // ^
        return A_BITWISEXOR; //
    case T_AMPERSAND:        // & (bitwise AND, address-of operator)
        return A_BITWISEAND; //
    case T_EQ:               // ==
        return A_EQ;         //
    case T_NE:               // !=
        return A_NE;         //
    case T_LT:               // <
        return A_LT;         //
    case T_GT:               // >
        return A_GT;         //
    case T_LE:               // <=
        return A_LE;         //
    case T_GE:               // >=
        return A_GE;         //
    case T_LSHIFT:           // <<
        return A_LSHIFT;     //
    case T_RSHIFT:           // >>
        return A_RSHIFT;     //
    case T_PLUS:             // +
        return A_ADD;        //
    case T_MINUS:            // - (subtraction or (unary) negation)
        return A_SUBTRACT;   //
    case T_STAR:             // *
        return A_MULTIPLY;   //
    case T_SLASH:            // /
        return A_DIVIDE;     //

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
 * WARNING:
 * Doesn't accept unexpected token types: T_VOID, T_CHAR, T_INT, T_LONG.
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
    case T_EOF:
        return 0;
    case T_ASSIGN:
        return 10;
    case T_LOGICALOR:
        return 20;
    case T_LOGICALAND:
        return 30;
    case T_BITWISEOR:
        return 40;
    case T_BITWISEXOR:
        return 50;
    case T_AMPERSAND:
        return 60;
    case T_EQ:
    case T_NE:
        return 70;
    case T_LT:
    case T_GT:
    case T_LE:
    case T_GE:
        return 80;
    case T_LSHIFT:
    case T_RSHIFT:
        return 90;
    case T_PLUS:
    case T_MINUS:
        return 100;
    case T_STAR:
    case T_SLASH:
        return 110;
    default: //
        if ((tokentype == T_VOID) || (tokentype == T_CHAR) ||
            (tokentype == T_INT) || (tokentype == T_LONG)) {
            // Unexpected token types
            logFatald("Unexpected token in expression: ", tokentype);
            logFatal("operatorPrecedence doesn't handle this token");
        }
        return 0; // e.g. T_SEMICOLON, T_RPAREN, etc.
    }
}

/**
 * prefix - Parse a prefix expression and return a sub-tree representing it
 *
 * NOTE:
 * prefix_expression := primary_expression
 *      | '*' prefix_expression
 *      | '&' prefix_expression
 *      | '-' prefix_expression
 *      | '++' prefix_expression
 *      | '--' prefix_expression
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
            logFatal("Dereference operator '*' must be applied to a "
                     "pointer (*)");
        }

        // Prepend an A_DEREF operation to the tree
        tree =
            makeASTUnary(A_DEREFERENCE,
                         pointerToPrimitiveType(tree->primitiveType), tree, 0);
        break;

    case T_MINUS:
        // Get the next token and parse it recursively as a prefix expression
        scan(&Token);
        tree = prefix();

        // Prepend an A_LOGICALNEGATE operation to the tree and make the child an
        // rvalue. Because character type (T_CHAR) is unsigned, also widen this
        // to int so that it's signed
        tree->isRvalue = true;
        tree = coerceASTTypeForOp(tree, P_INT, 0);
        tree = makeASTUnary(A_LOGICALNEGATE, P_INT, tree, 0);
        break;

    case T_LOGICALINVERT:
        // Get the next token and parse it recursively as a prefix expression
        scan(&Token);
        tree = prefix();

        // Prepend an A_INVERT operation to the tree and make the child an
        // rvalue.
        tree->isRvalue = true;
        tree = makeASTUnary(A_LOGICALINVERT, tree->primitiveType, tree, 0);
        break;

    case T_LOGICALNOT:
        // Get the next token and parse it recursively as a prefix expression
        scan(&Token);
        tree = prefix();

        // Prepend an A_LOGNOT operation to the tree and make the child an
        // rvalue
        tree->isRvalue = 1;
        tree = makeASTUnary(A_LOGICALNOT, tree->primitiveType, tree, 0);
        break;

    case T_INCREMENT:
        // Get the next token and parse it recursively as a prefix expression
        scan(&Token);
        tree = prefix();

        // For now, ensure it's an identifier.
        if (tree->op != A_IDENTIFIER) {
            logFatal(
                "Pre-increment operator '++' must be applied to an identifier");
        }

        // Prepend an A_PREINCREMENT operation to the tree
        tree = makeASTUnary(A_PREINCREMENT, tree->primitiveType, tree, 0);
        break;

    case T_DECREMENT:
        // Get the next token and parse it recursively as a prefix expression
        scan(&Token);
        tree = prefix();

        // For now, ensure it's an identifier.
        if (tree->op != A_IDENTIFIER) {
            logFatal(
                "Pre-decrement operator '--' must be applied to an identifier");
        }

        // Prepend an A_PREDECREMENT operation to the tree
        tree = makeASTUnary(A_PREDECREMENT, tree->primitiveType, tree, 0);
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

    // If we hit a semicolon(";"), right parenthesis(")"), or right
    // bracket("]"), it means it's end of the expression, so we return just the
    // left node. OvO
    tokentype = Token.token;
    if (tokentype == T_SEMICOLON || tokentype == T_RPAREN ||
        tokentype == T_RBRACKET) {
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
            right = coerceASTTypeForOp(right, left->primitiveType, A_NOTHING);

            // NOTE: Sure about this?
            if (left == NULL) {
                logFatal("Incompatible expression in assignment");
            }

            // Make an assignment AST tree.
            // However, switch left and right around,
            // so that the right expression's code will be generated before
            // the left expression.
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
            // We are not doing an assignment, so both trees should be
            // rvalues Convert both trees into rvalues if they are lvalue
            // trees
            left->isRvalue = true;
            right->isRvalue = true;

            // Ensure the two types are compatible by trying to modify each
            // tree to match the other's type
            leftTemp =
                coerceASTTypeForOp(left, right->primitiveType, ASToperation);
            rightTemp =
                coerceASTTypeForOp(right, left->primitiveType, ASToperation);

            if (leftTemp == NULL && rightTemp == NULL) {
                logFatal("Incompatible types in binary expression");
            }

            // If one side could be converted, use that side's converted
            // tree
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
        // If we hit a semicolon(";"), right parenthesis(")"), or right
        // bracket("]"), it means it's end of the sentence, so we return just
        // the left node. OvO
        tokentype = Token.token;
        if (tokentype == T_SEMICOLON || tokentype == T_RPAREN ||
            tokentype == T_RBRACKET) {
            left->isRvalue = true; // Means this node is an r-value
            return left;
        }
    }

    // Return the constructed AST node,
    // when the precedence is not higher than ptp.
    left->isRvalue = true; // Means this node is an r-value
    return left;
}
