// src/symbol.c

#include "data.h"
#include "decl.h"
#include "defs.h"

// Position of the next free global symbol slot
static int NextGlobalSymbolIndex = 0;

/**
 * findGlobalSymbol - Find a global symbol in the symbol table.
 *
 * @param s The name of the symbol to add
 *
 * @return The index of the symbol in the symbol table.
 */
int findGlobalSymbol(char *s) {
    for (int i = 0; i < NextGlobalSymbolIndex; i++) {
        if (!strcmp(GlobalSymbolTable[i].name, s)) {
            return i;
        }
    }
    return -1;
}

/**
 * getNewGlobalSymbolIndex - Get a new index for a global symbol.
 *
 * @return The new index for the global symbol
 *
 * @note Logs a fatal error if the symbol table is full
 */
static int getNewGlobalSymbolIndex(void) {
    int p;

    if ((p = NextGlobalSymbolIndex++) >= NSYMBOLS) {
        logFatal("Too many global symbols");
    }

    return p;
}

/**
 * addGlobalSymbol - Add a global symbol to the symbol table.
 *
 * @param name           The name of the symbol to add.
 * @param primitiveType  The primitive data type of the symbol.
 * @param structuralType The structural data type of the symbol.
 * @param endLabel       The end label for the symbol (if applicable).
 *
 * @return The index of the added symbol in the symbol table.
 *         If the symbol already exists, returns its existing index.
 */
int addGlobalSymbol(char *name, int primitiveType, int structuralType,
                    int endLabel) {
    int symbolIndex;

    // If this is already in the symbol table,
    // simply return the existing slot index
    if ((symbolIndex = findGlobalSymbol(name)) != -1) {
        return symbolIndex;
    }

    symbolIndex = getNewGlobalSymbolIndex();
    GlobalSymbolTable[symbolIndex].name = strdup(name);
    if (GlobalSymbolTable[symbolIndex].name == NULL) {
        logFatal("Memory allocation failed for symbol name");
    }
    GlobalSymbolTable[symbolIndex].primitiveType = primitiveType;
    GlobalSymbolTable[symbolIndex].structuralType = structuralType;
    GlobalSymbolTable[symbolIndex].endLabel = endLabel;

    return symbolIndex;
}
