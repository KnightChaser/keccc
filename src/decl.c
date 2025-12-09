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
    if (t == T_CHAR) {
        return P_CHAR;
    } else if (t == T_INT) {
        return P_INT;
    } else if (t == T_VOID) {
        return P_VOID;
    } else {
        fprintf(stderr, "Error: Unknown primitive type token %d\n", t);
        exit(1);
    }
}

/**
 * variableDeclaration - Parses a variable declaration.
 *
 * NOTE:
 * Currently, we only support integer variable declarations.
 * Thus, we ensure that the type is 'int', followed by an identifier
 * and a semicolon(;).
 */
void variableDeclaration(void) {
    int id;
    int type;

    // Get the type of the variable, then the identifier
    type = parsePrimitiveType(Token.token);
    scan(&Token);
    matchIdentifierToken();

    // Text now has the identifier's name
    id = addGlobalSymbol(Text, type, S_VARIABLE);
    codegenDeclareGlobalSymbol(id);

    // Finally, match the semicolon(";")
    matchSemicolonToken();
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
    matchIdentifierToken(); // Text now has the function name
    functionNameIndex = addGlobalSymbol(Text, P_VOID, S_FUNCTION);
    matchLeftParenthesisToken();
    matchRightParenthesisToken();

    // Get the AST for the compound statement
    treeNode = compoundStatement();

    // Return an A_FUNCTION node which has the function's nameslot,
    // and the compount statement as its child
    return makeASTUnary(A_FUNCTION, P_VOID, treeNode, functionNameIndex);
}
