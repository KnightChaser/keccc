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
