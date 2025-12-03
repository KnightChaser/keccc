// src/decl.c

#include "decl.h"
#include "data.h"
#include "defs.h"

void variableDeclaration(void) {
    match(T_INT, "int");
    identifier();
    addGlobalSymbol(Text);
    genglobsym(Text);
    semicolon();
}
