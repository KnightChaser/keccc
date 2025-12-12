// src/decl.c

#include "decl.h"
#include "data.h"
#include "defs.h"

/**
 * parsePrimitiveType - Parses a (primitive-type) token and returns its internal
 * representation.
 *
 * @param t: Token representing the primitive type.
 * @return: Internal representation of the primitive type.
 */
int parsePrimitiveType(int t) {
    switch (t) {
    case T_VOID:
        return P_VOID;
    case T_CHAR:
        return P_CHAR;
    case T_INT:
        return P_INT;
    case T_LONG:
        return P_LONG;
    }

    logFatald("Error: Unknown primitive type token in parsePrimitiveType", t);

    return P_NONE; // Unreachable, but avoids compiler warning
}

/**
 * variableDeclaration - Parses a variable declaration.
 *
 * NOTE:
 * variable_declaration: type identifier ";" ;
 *
 */
void variableDeclaration(void) {
    int id;
    int type;

    // Get the type of the variable, then the identifier
    type = parsePrimitiveType(Token.token);
    scan(&Token);
    matchIdentifierToken();

    // Text now has the identifier's name
    id = addGlobalSymbol(Text, type, S_VARIABLE, 0);
    codegenDeclareGlobalSymbol(id);

    // Finally, match the semicolon(";")
    matchSemicolonToken();
}

/**
 * functionDeclaration - Parses a function declaration and returns its AST.
 *
 * NOTE:
 * function_declaration: type identifier "(" ")" compound_statement ;
 *
 * @return: AST node representing the function declaration.
 */
struct ASTnode *functionDeclaration(void) {
    struct ASTnode *treeNode;
    struct ASTnode *finalStatementNode;
    int functionNameIndex;
    int type;
    int endLabel;

    // Get the type of the variable, then the identifier...
    type = parsePrimitiveType(Token.token);
    scan(&Token);
    matchIdentifierToken();

    // Get a label-id for the end label,
    // add the function to the symbol table as declared,
    // and set the CurrentFunctionSymbolID to the function's symbol ID
    endLabel = codegenGetLabelNumber();
    functionNameIndex = addGlobalSymbol(Text, type, S_FUNCTION, endLabel);
    CurrentFunctionSymbolID = functionNameIndex;

    // Scan the parenthesis
    matchLeftParenthesisToken();
    matchRightParenthesisToken();

    // Get the AST tree for the compound statement
    treeNode = compoundStatement();

    // If the function type isn't P_VOID,
    // check that the last AST operation in the compound statement
    // was a return statement (returning somthing)
    if (type != P_VOID) {
        finalStatementNode =
            (treeNode->op == A_GLUE) ? treeNode->right : treeNode;
        if (finalStatementNode == NULL || finalStatementNode->op != A_RETURN) {
            logFatals("Error: Non-void function '", Text);
            logFatal("' missing return statement");
        }
    }

    return makeASTUnary(A_FUNCTION, type, treeNode, functionNameIndex);
}
