// src/stmt.c

#include "data.h"
#include "decl.h"
#include "defs.h"

#include <stdbool.h>

/**
 * A brief BNF expressions note:
 *
 * compound_statements:  '{' '}' // empty compound statement
 *      |     '{' statement '}'
 *      |     '{' statement compound_statements '}'
 *      ;
 *
 * statement: print_statement
 *      |     declaration
 *      |     assignment_statement
 *      |     if_statement
 *      |     while_statement
 *      |     for_statement
 *      ;
 *
 * print_statement: 'print' expression ';' ;
 *
 * declaration: 'int' identifier ';' ; // only int type supported
 *
 * assignment_statement: identifier '=' expression ';' ;
 *
 * if_statement: if_head
 *      |        if_head 'else' compound_statements
 *      ;
 *
 * if_head: 'if' '(' true_false_expression ')' compound_statements ;
 *
 * while_statement: 'while' '(' true_false_expression ')' compound_statements ;
 *
 * for_statement: 'for' '(' preop_statement ';'
 *                        true_false_expression ';'
 *                        postop_statement ')' compound_statement  ;
 *
 * preop_statement: staetment ;   // (for now)
 * postop_statement: statement ;  // (for now)
 *
 * identifier = T_IDENTIFIER;
 *      ;
 *
 */

// Forward declarations
static struct ASTnode *singleStatement(void);

/**
 * printStatement - Parse and generate code for a print statement.
 *
 * @return AST node representing the print statement.
 */
static struct ASTnode *printStatement(void) {
    struct ASTnode *tree;
    int leftPrimitiveType;
    int rightPrimitiveType;

    // Match "print" string at the first token
    match(T_PRINT, "print");

    // Parse the following expression
    tree = binexpr(0);

    // Make an print AST tree
    leftPrimitiveType = P_INT;
    rightPrimitiveType = tree->primitiveType;
    if (!checkPrimitiveTypeCompatibility(&leftPrimitiveType,
                                         &rightPrimitiveType, false)) {
        logFatal("Incompatible type for print statement");
    }

    // Widen the tree if required.
    if (rightPrimitiveType) {
        tree = makeASTUnary(A_WIDENTYPE, leftPrimitiveType, tree, 0);
    }

    // Wrap the expression in a print node so statement handling
    // can correctly require a trailing semicolon.
    tree = makeASTUnary(A_PRINT, P_NONE, tree, 0);

    return tree;
}

/**
 * assignmentStatement - Parse and handle a variable declaration statement.
 *
 * @return AST node representing the assignment statement.
 */
static struct ASTnode *assignmentStatement(void) {
    struct ASTnode *leftNode = NULL;
    struct ASTnode *rightNode = NULL;
    struct ASTnode *treeNode = NULL;
    int leftPrimitiveType;
    int rightPrimitiveType;
    int identifierIndex;

    // Ensure we have an identifier
    identifier();

    // Check it's been defined then make a leaf node for it
    if ((identifierIndex = findGlobalSymbol(Text)) == -1) {
        logFatals("Undeclared identifier: ", Text);
    }
    rightNode = makeASTLeaf(A_LVALUEIDENTIFIER,
                            GlobalSymbolTable[identifierIndex].primitiveType,
                            identifierIndex);

    // Match the '=' token
    match(T_ASSIGN, "=");

    // Parse the expression on the right-hand side of the '='
    leftNode = binexpr(0);

    // Ensure type compatibility between left and right nodes
    leftPrimitiveType = leftNode->primitiveType;
    rightPrimitiveType = rightNode->primitiveType;
    if (!checkPrimitiveTypeCompatibility(&leftPrimitiveType,
                                         &rightPrimitiveType, false)) {
        logFatal("Type error: incompatible types in assignment statement");
    }

    // Widen the primitive type if necessary
    if (leftPrimitiveType) {
        leftNode = makeASTNode(A_WIDENTYPE, leftPrimitiveType, leftNode, NULL,
                               NULL, 0);
    }

    // Create an assignment AST node
    treeNode = makeASTNode(A_ASSIGN, P_INT, leftNode, NULL, rightNode, 0);

    return treeNode;
}

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
 */
static struct ASTnode *ifStatement(void) {
    struct ASTnode *conditionAST;   // condition
    struct ASTnode *thenAST;        // true branch
    struct ASTnode *elseAST = NULL; // false branch

    // Ensure we have 'if' then '('
    match(T_IF, "if");
    leftParenthesis();

    // Parse the following expression and the following ')'
    // Ensure the tree's operation is a comparison.
    conditionAST = binexpr(0);

    if (!(conditionAST->op == A_EQ) && !(conditionAST->op == A_NE) &&
        !(conditionAST->op == A_LT) && !(conditionAST->op == A_LE) &&
        !(conditionAST->op == A_GT) && !(conditionAST->op == A_GE)) {
        logFatal("If statement condition is not a comparison");
    }
    rightParenthesis();

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
    leftParenthesis();

    // Parse the following expression and the following ')'
    conditionAST = binexpr(0);

    if (!(conditionAST->op == A_EQ) && !(conditionAST->op == A_NE) &&
        !(conditionAST->op == A_LT) && !(conditionAST->op == A_LE) &&
        !(conditionAST->op == A_GT) && !(conditionAST->op == A_GE)) {
        logFatal("While statement condition is not a comparison");
    }
    rightParenthesis();

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
    leftParenthesis();

    // Get the preOperation statement and the semicolon(';')
    preOperationAST = singleStatement();
    semicolon();

    // Get the condition and the ';'
    conditionAST = binexpr(0);
    if (!(conditionAST->op == A_EQ) && !(conditionAST->op == A_NE) &&
        !(conditionAST->op == A_LT) && !(conditionAST->op == A_LE) &&
        !(conditionAST->op == A_GT) && !(conditionAST->op == A_GE)) {
        logFatal("For statement condition is not a comparison");
    }
    semicolon();

    // Get the postOperation statement and the ')'
    postOperationAST = singleStatement();
    rightParenthesis();

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
 * singleStatement - Parse and handle a single statement.
 *
 * @return AST node representing the single statement.
 */
static struct ASTnode *singleStatement(void) {
    switch (Token.token) {
    case T_PRINT:
        return printStatement();
    case T_CHAR: // primitive data type (char, 1 byte)
    case T_INT:  // primitive data type (int, 4 bytes)
        variableDeclaration();
        return NULL; // No AST node for declarations
    case T_IDENTIFIER:
        return assignmentStatement();
    case T_IF:
        return ifStatement();
    case T_WHILE:
        return whileStatement();
    case T_FOR:
        return forStatement();
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
    leftBrace();

    while (true) {

        treeNode = singleStatement();

        // Some statement must be followed by a semicolon
        if (treeNode != NULL &&
            (treeNode->op == A_PRINT || treeNode->op == A_ASSIGN)) {
            semicolon();
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
            rightBrace();
            return leftASTNode;
        }
    }
}
