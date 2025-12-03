// src/data.h

#ifndef extern_
#define extern_ extern
#endif

#include "defs.h"
#include <stdio.h> // Just for FILE

// Current Line number
extern_ int Line;
// Character put back by scanner for re-reading
extern_ int Putback;
// Input file (source code)
extern_ FILE *Infile;
// Output file (generated code, currently Assembly)
extern_ FILE *Outfile;
// Latest token scanned
extern_ struct token Token;

// Last identifier scanned (e.g. "print")
extern_ char Text[TEXTLEN + 1];
// Global symbol table
extern_ struct symbolTable GlobalSymbolTable[NSYMBOLS];
