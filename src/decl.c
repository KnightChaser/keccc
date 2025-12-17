// src/decl.c

#include "decl.h"
#include "data.h"
#include "defs.h"

/**
 * parsePrimitiveType - Parses the current token and return
 * a primitive type enum value. Also, scan in the next token.
 *
 * @return Primitive type enum value.
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
 * variableDeclaration - Parse the declaration of a variable
 *
 * NOTE:
 * variable_declaration: type identifier ";" ;
 *
 * @param type The primitive type of the variable.
 */
void variableDeclaration(int type) {
    int id;

    // Text now has the identifier's name.
    // Add it as a known identifier,
    // and generate its space in assembly
    id = addGlobalSymbol(Text, type, S_VARIABLE, 0);
    codegenDeclareGlobalSymbol(id);

    // Match the semicolon
    matchSemicolonToken();
}

/**
 * functionDeclaration - Parses a function declaration and returns its AST.
 * The identifier has been scanned and we have the type
 *
 * NOTE:
 * function_declaration: type identifier "(" ")" compound_statement ;
 * 
 * @param type The primitive type of the function.
 *
 * @return AST node representing the function declaration.
 */
struct ASTnode *functionDeclaration(int type) {
    struct ASTnode *treeNode;
    struct ASTnode *finalStatementNode;
    int functionNameIndex;
    int endLabel;

    // Text now has the identifier's name
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

/**
 * globalDeclaration - Parses global declarations (functions and variables).
 *
 * NOTE:
 * global_declaration: (function_declaration | variable_declaration)* ;
 */
void globalDeclaration(void) {
    struct ASTnode *treeNode;
    int type;

    while (true) {
        // We have to read past the type and identifier to see
        // either a '(' (T_LPAREN) for function declaration
        // or a ',' or ';' for a variable declaration
        type = parsePrimitiveType();
        matchIdentifierToken();

        if (Token.token == T_LPAREN) {
            // parse the function declaration and generate the assembly code for
            // it
            treeNode = functionDeclaration(type);
            if (Option_dumpAST) {
                dumpASTTree(treeNode);
            }
            codegenAST(treeNode, NOREG, NOREG);
        } else {
            // Assume
            variableDeclaration(type);
        }

        // Stop when we reach the end of the file
        if (Token.token == T_EOF) {
            break;
        }
    }
}
