// src/data.h

#ifndef extern_
#define extern_ extern
#endif

#include "defs.h"
#include <stdio.h> // Just for FILE

/**
 * NOTE:
 * Compiler configuration options
 */
// Selected code generation target (see defs.h TARGET_*)
extern_ int CurrentTarget;
// Print the dump of the AST to stdout during compilation
extern_ bool Option_dumpAST;
// If true, dump a compacted AST (flattens A_GLUE chains)
extern_ bool Option_dumpASTCompacted;

/**
 * NOTE:
 * Compiler-wide global data or state
 */

// Current Line number
extern_ int Line;
// Character put back by scanner for re-reading
extern_ int Putback;
// Symbol ID of the current function being processed
extern_ int CurrentFunctionSymbolID;
// Position of the next free global symbol slot
extern_ int NextGlobalSymbolIndex;
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
