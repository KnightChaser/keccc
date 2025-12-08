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
 * identifier = T_IDENTIFIER;
 *      ;
 *
 */

/**
 * printStatement - Parse and generate code for a print statement.
 *
 * @return AST node representing the print statement.
 */
static struct ASTnode *printStatement(void) {
    struct ASTnode *tree;

    // Match "print" string at the first token
    match(T_PRINT, "print");

    // Parse the following expression
    tree = binexpr(0);

    // Make an print AST tree
    tree = makeASTUnary(A_PRINT, tree, 0);

    // Match the following semicolon(;)
    semicolon();

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
    int identifierIndex;

    // Ensure we have an identifier
    identifier();

    // Check it's been defined then make a leaf node for it
    if ((identifierIndex = findGlobalSymbol(Text)) == -1) {
        logFatals("Undeclared identifier: ", Text);
    }
    rightNode = makeASTLeaf(A_LVALUEIDENTIFIER, identifierIndex);

    // Match the '=' token
    match(T_ASSIGN, "=");

    // Parse the expression on the right-hand side of the '='
    leftNode = binexpr(0);

    // Create an assignment AST node
    treeNode = makeASTNode(A_ASSIGN, leftNode, NULL, rightNode, 0);

    // Match the following semicolon(;)
    semicolon();

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

    return makeASTNode(A_IF, conditionAST, thenAST, elseAST, 0);
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

    return makeASTNode(A_WHILE, conditionAST, bodyAST, NULL, 0);
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
        switch (Token.token) {
        case T_PRINT:
            treeNode = printStatement();
            break;
        case T_INT:
            variableDeclaration();
            treeNode = NULL; // No AST node for declarations
            break;
        case T_IDENTIFIER:
            treeNode = assignmentStatement();
            break;
        case T_IF:
            treeNode = ifStatement();
            break;
        case T_WHILE:
            treeNode = whileStatement();
            break;
        case T_RBRACE:
            // When we hit the right curly bracket,
            // we are done with this compound statement.
            // Return the left AST node.
            rightBrace();
            return leftASTNode;
        default:
            logFatal("Unexpected token in compound statement");
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

        if (leftASTNode == NULL) {
            // First AST node in the compound statement
            leftASTNode = treeNode;
        } else {
            leftASTNode = makeASTNode(A_GLUE, leftASTNode, NULL, treeNode, 0);
        }
    }
}
