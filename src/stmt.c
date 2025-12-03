// src/stmt.c

#include "data.h"
#include "decl.h"
#include "defs.h"

#include <stdbool.h>

/**
 * BNF expressions for statements:
 *
 * statements: statement
 *      | statement statements
 *      ;
 *
 * statement: 'print' expression ';'
 *      |     'int' identifier ';'
 *      |     identifier '=' expression ';'
 *      ;
 *
 * identifier = T_IDENTIFIER;
 *      ;
 *
 */

/**
 * printStatement - Parse and generate code for a print statement.
 */
void printStatement(void) {
    struct ASTnode *tree;
    int reg;

    // Match "print" string at the first token
    match(T_PRINT, "print");

    // Parse the following expression and generate the assembly code
    tree = binexpr(0);
    reg = genAST(tree, -1);
    genprintint(reg);
    genfreeregs();

    // Match the following semicolon(;)
    semicolon();
}

/**
 * assignmentStatement - Parse and handle a variable declaration statement.
 */
void assignmentStatement(void) {
    struct ASTnode *exprNode = NULL;
    struct ASTnode *lvalueNode = NULL;
    struct ASTnode *tree = NULL;
    int identifierIndex;

    // Ensure we have an identifier
    identifier();

    // Check it's been defined then make a leaf node for it
    if ((identifierIndex = findGlobalSymbol(Text)) == -1) {
        logFatals("Undeclared identifier: ", Text);
    }
    lvalueNode = makeASTLeaf(A_LVALUEIDENTIFIER, identifierIndex);

    // Match the '=' token
    match(T_EQUALS, "=");

    // Parse the expression on the right-hand side of the '='
    exprNode = binexpr(0);

    // Create an assignment AST node
    tree = makeASTNode(A_ASSIGN, exprNode, lvalueNode, 0);

    // Generate code for the assignment
    genAST(tree, -1);
    genfreeregs();

    // Match the following semicolon(;)
    semicolon();
}

/**
 * statmenets - Parse and handle a series of statements.
 */
void statements(void) {
    while (true) {
        switch (Token.token) {
        case T_PRINT:
            printStatement();
            break;
        case T_INT:
            variableDeclaration();
            break;
        case T_IDENTIFIER:
            assignmentStatement();
            break;
        case T_EOF:
            return;
        default:
            logFatald("Syntax error, unexpected token in statements(): ",
                      Token.token);
        }
    }
}
