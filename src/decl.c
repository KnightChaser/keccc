// src/decl.c

#include "decl.h"
#include "data.h"
#include "defs.h"

/**
 * parsePrimitiveType - Parses the current token and return
 * a primitive type enum value. Also, scan in the next token.
 *
 * @return: Primitive type enum value.
 */
int parsePrimitiveType(void) {
    int type;
    switch (Token.token) {
    case T_VOID:
        type = P_VOID;
        break;
    case T_CHAR:
        type = P_CHAR;
        break;
    case T_INT:
        type = P_INT;
        break;
    case T_LONG:
        type = P_LONG;
        break;
    default:
        logFatald("Error: Invalid primitive type token in parsePrimitiveType",
                  Token.token);
    }

    // Scan in one or more further '*' tokens and
    // determine the correct pointer type
    while (true) {
        scan(&Token);
        if (Token.token != T_STAR) {
            break;
        }
        type = primitiveTypeToPointerType(type);
    }

    return type;
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

    // Get the type of the variable,
    // which also scans in the identifier
    type = parsePrimitiveType();
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

    // Get the function return type,
    // which also scans in the identifier
    type = parsePrimitiveType();
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

    if (type != P_VOID) {
        if (treeNode == NULL) {
            fprintf(stderr,
                    "No statements in fucntion with non-void type: %s\n", Text);
            exit(1);
        }

        // Check that the last AST operation in the given
        // compound statement was a return statement
        finalStatementNode =
            (treeNode->op == A_GLUE) ? treeNode->right : treeNode;
        if (finalStatementNode == NULL || finalStatementNode->op != A_RETURN) {
            fprintf(stderr,
                    "Error: Non-void function '%s' missing return statement\n",
                    Text);
        }
    }

    return makeASTUnary(A_FUNCTION, type, treeNode, functionNameIndex);
}
