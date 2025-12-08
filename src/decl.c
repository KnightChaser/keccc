// src/decl.c

#include "decl.h"
#include "data.h"
#include "defs.h"

/**
 * variableDeclaration - Parses a variable declaration.
 *
 * NOTE:
 * Currently, we only support integer variable declarations.
 * Thus, we ensure that the type is 'int', followed by an identifier
 * and a semicolon(;).
 */
void variableDeclaration(void) {
    match(T_INT, "int");
    identifier(); // Text now has the identifier's name
    addGlobalSymbol(Text);
    codegenDeclareGlobalSymbol(Text);
    semicolon();
}

/**
 * functionDeclaration - Parses a function declaration and returns its AST.
 *
 * NOTE:
 * TODO:
 * Currently, we only support 'void' functions with no parameters.
 * Thus, we ensure that the type is 'void', followed by an identifier,
 * an opening parenthesis, a closing parenthesis, and a compound statement.
 *
 * @return: AST node representing the function declaration.
 */
struct ASTnode *functionDeclaration(void) {
    struct ASTnode *treeNode;
    int functionNameIndex;

    // NOTE:
    // Find the 'void', the identifier, and the '(' ')'.
    // For now, do nothing with them
    match(T_VOID, "void");
    identifier(); // Text now has the function name
    functionNameIndex = addGlobalSymbol(Text);
    leftParenthesis();
    rightParenthesis();

    // Get the AST for the compound statement
    treeNode = compoundStatement();

    // Return an A_FUNCTION node which has the function's nameslot,
    // and the compount statement as its child
    return makeASTUnary(A_FUNCTION, treeNode, functionNameIndex);
}
