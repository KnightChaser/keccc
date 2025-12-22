// src/stmt.c

#include "data.h"
#include "decl.h"
#include "defs.h"

#include <stdbool.h>

// Forward declarations
static struct ASTnode *singleStatement(void);

/**
 * ifStatement - Parse and handle an if statement.
 *
 * NOTE:
 * If statement is composed of:
 * -----------------------------------
 * if (condition) {   (condition AST)
 *    then-statements (then AST)
 * } else {
 *    else-statements (else AST)
 * }
 * -----------------------------------
 *
 * @return AST node representing the if statement.
 */
static struct ASTnode *ifStatement(void) {
    struct ASTnode *conditionAST;   // condition
    struct ASTnode *thenAST;        // true branch
    struct ASTnode *elseAST = NULL; // false branch

    // Ensure we have 'if' then '('
    match(T_IF, "if");
    matchLeftParenthesisToken();

    // Parse the following expression and the following ')'
    // Ensure the tree's operation is a comparison.
    conditionAST = binexpr(0);

    // For a non-comparison to be boolean
    if (!(conditionAST->op == A_EQ) && !(conditionAST->op == A_NE) &&
        !(conditionAST->op == A_LT) && !(conditionAST->op == A_LE) &&
        !(conditionAST->op == A_GT) && !(conditionAST->op == A_GE)) {
        conditionAST = makeASTUnary(A_TOBOOLEAN, P_INT, conditionAST, 0);
    }
    matchRightParenthesisToken();

    // Get the AST for the compount statement; this is the 'then' branch
    thenAST = compoundStatement();

    if (Token.token == T_ELSE) {
        scan(&Token);
        elseAST = compoundStatement();
    }

    return makeASTNode(A_IF, P_NONE, conditionAST, thenAST, elseAST, 0);
}

/**
 * whileStatement - Parse and handle a while statement.
 *
 * NOTE:
 * While statement is composed of:
 * -----------------------------------
 * while (condition) {
 *     body-statements
 *     ...
 * }
 * -----------------------------------
 *
 * @return AST node representing the while statement.
 */
static struct ASTnode *whileStatement(void) {
    struct ASTnode *conditionAST;
    struct ASTnode *bodyAST;

    // Ensure we have 'while' then '('
    match(T_WHILE, "while");
    matchLeftParenthesisToken();

    // Parse the following expression and the following ')'
    conditionAST = binexpr(0);

    // Force non-comparisons to be boolean
    if (!(conditionAST->op == A_EQ) && !(conditionAST->op == A_NE) &&
        !(conditionAST->op == A_LT) && !(conditionAST->op == A_LE) &&
        !(conditionAST->op == A_GT) && !(conditionAST->op == A_GE)) {
        conditionAST = makeASTUnary(A_TOBOOLEAN, P_INT, conditionAST, 0);
    }
    matchRightParenthesisToken();

    // Get the AST for the compount statement; this is the body of the loop
    bodyAST = compoundStatement();

    // Store body in the right child for structural consistency
    return makeASTNode(A_WHILE, P_NONE, conditionAST, NULL, bodyAST, 0);
}

/**
 * forStatement - Parse and handle a for statement.
 *
 * NOTE:
 * For statement is composed of:
 * ----------------------------------------------
 * for (pre-operation; condition; post-operation) {
 *     body-statements
 *     ...
 * }
 * ----------------------------------------------
 * This will be converted into the following AST:
 * ----------------------------------------------
 *               A_GLUE
 *             /       \
 *       preOperation  A_WHILE
 *                     /      \
 *               condition   A_GLUE
 *                          /      \
 *                     bodyAST   postOperation
 *                   (compound)
 * ----------------------------------------------
 * It means that the for loop is
 * transformed into a while loop
 *
 * @return AST node representing the for statement.
 */
static struct ASTnode *forStatement(void) {
    struct ASTnode *conditionAST;
    struct ASTnode *bodyAST;
    struct ASTnode *preOperationAST;
    struct ASTnode *postOperationAST;
    struct ASTnode *treeNode;

    // Ensure we have 'for' '('
    match(T_FOR, "for");
    matchLeftParenthesisToken();

    // Get the preOperation statement and the semicolon(';')
    preOperationAST = singleStatement();
    matchSemicolonToken();

    // Get the condition and the ';'
    conditionAST = binexpr(0);

    // Force non-comparisons to be boolean
    if (!(conditionAST->op == A_EQ) && !(conditionAST->op == A_NE) &&
        !(conditionAST->op == A_LT) && !(conditionAST->op == A_LE) &&
        !(conditionAST->op == A_GT) && !(conditionAST->op == A_GE)) {
        conditionAST = makeASTUnary(A_TOBOOLEAN, P_INT, conditionAST, 0);
    }
    matchSemicolonToken();

    // Get the postOperation statement and the ')'
    postOperationAST = singleStatement();
    matchRightParenthesisToken();

    // Get the compound statement in the body part
    bodyAST = compoundStatement();

    // TODO:
    // For now, all four sub-trees have to be non-NULL.
    // Later on, we'll change the semantics for when some are missing

    // Glue the compount statement and the postOperationAST
    treeNode = makeASTNode(A_GLUE, P_NONE, bodyAST, NULL, postOperationAST, 0);

    // Glue the conditionAST and the WHILE node
    treeNode = makeASTNode(A_WHILE, P_NONE, conditionAST, NULL, treeNode, 0);

    // Finally, glue the preOperationAST and the WHILE node
    treeNode = makeASTNode(A_GLUE, P_NONE, preOperationAST, NULL, treeNode, 0);

    return treeNode;
}

/**
 * returnStatement - Parse and handle a return statement.
 *
 * NOTE:
 * Return statement is composed of:
 * -----------------------------------
 * return (expression)
 * -----------------------------------
 *
 * @return AST node representing the return statement.
 */
static struct ASTnode *returnStatement(void) {
    struct ASTnode *treeNode;

    // Can't return a value if function returns P_VOID
    if (SymbolTable[CurrentFunctionSymbolID].primitiveType == P_VOID) {
        logFatal("Cannot return a value from a void function");
    }

    // Ensure 'return' and '(' token
    match(T_RETURN, "return");
    matchLeftParenthesisToken();

    // Parse the following expression
    treeNode = binexpr(0);

    // Ensure the two types are compatible
    treeNode = coerceASTTypeForOp(
        treeNode, SymbolTable[CurrentFunctionSymbolID].primitiveType,
        A_NOTHING);
    if (treeNode == NULL) {
        logFatal("Type error: incompatible type in return statement");
    }

    // Wrap the expression in a return node so statement handling
    // can correctly require a trailing semicolon.
    treeNode = makeASTUnary(A_RETURN, P_NONE, treeNode, 0);

    // Match the closing ')'
    matchRightParenthesisToken();

    return treeNode;
}

/**
 * singleStatement - Parse and handle a single statement.
 *
 * @return AST node representing the single statement.
 */
static struct ASTnode *singleStatement(void) {
    int type;

    switch (Token.token) {
    case T_CHAR: // primitive data type (char, 1 byte)
    case T_INT:  // primitive data type (int, 4 bytes)
    case T_LONG: // primitive data type (long, 8 bytes)

        // Parse the type and get the identifier.
        // Then parse the rest of the declaration.
        type = parsePrimitiveType();
        matchIdentifierToken();
        variableDeclaration(type, true);

        return NULL; // No AST node for declarations
    case T_IF:
        return ifStatement();
    case T_WHILE:
        return whileStatement();
    case T_FOR:
        return forStatement();
    case T_RETURN:
        return returnStatement();
    default:
        // TODO:
        // For now, see if this is an expression.
        // This will catch an assignment statements.
        return binexpr(0);
    }

    // default
    logFatald("Unexpected token in single statement: %d", Token.token);
    return NULL; // Unreachable, just to suppress compiler warning
}

/**
 * compoundStatement - Parse and handle a compound statement.
 *
 * @return AST node representing the compound statement.
 */
struct ASTnode *compoundStatement(void) {
    struct ASTnode *leftASTNode = NULL;
    struct ASTnode *treeNode;

    // Accorind to the rule of compound statements,
    // It requires, at least, a left curly bracket '{'
    // when code starts
    matchLeftBraceToken();

    while (true) {

        treeNode = singleStatement();

        // Some statement must be followed by a semicolon
        if (treeNode != NULL &&
            (treeNode->op == A_ASSIGN ||     // e.g. "identifier = expr;"
             treeNode->op == A_RETURN ||     // e.g. "return expr;"
             treeNode->op == A_FUNCTIONCALL) // e.g. "functionCall(
        ) {
            matchSemicolonToken();
        }

        /**
         * For each new tree, either save it in left
         * or leave the left node empty, or glued the left and
         * the new tree together.
         *
         * example)
         * ```
         * {
         *     int i;
         *     int j;
         *     i = 6;
         *     j = 12;
         *     if (i < j) {
         *         print i;
         *     } else {
         *         print j;
         *     }
         * }
         * ```
         * will produce something like
         * ```
         *               A_GLUE
         *              /     \
         *          A_GLUE    A_IF
         *         /    \
         *       (i=6) (j=12)
         * ```
         * whereas each (i=6), (j=12), and A_IF are AST nodes.
         * Especially, A_IF node has its own sub-nodes.
         * ```
         *         [    A_IF   ]
         *        /     |      \
         *     A_LT print(i) print(j)
         *    (cond)   (T)     (F)
         * ``
         */

        if (treeNode != NULL) {
            if (leftASTNode == NULL) {
                // First AST node in the compound statement
                leftASTNode = treeNode;
            } else {
                leftASTNode =
                    makeASTNode(A_GLUE, P_NONE, leftASTNode, NULL, treeNode, 0);
            }
        }

        // When we hit a right curly bracket('}'), end of compound
        // statement. Skip past it and return the AST.
        if (Token.token == T_RBRACE) {
            matchRightBraceToken();
            return leftASTNode;
        }
    }
}
