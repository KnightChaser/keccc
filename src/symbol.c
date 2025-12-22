// src/symbol.c

#include "data.h"
#include "decl.h"
#include "defs.h"

/**
 * findGlobalSymbol - Find a global symbol in the symbol table.
 *
 * @param s The name of the symbol to add
 *
 * @return The index of the symbol in the symbol table. -1 if not present
 */
int findGlobalSymbol(char *s) {
    for (int i = 0; i < NextGlobalSymbolIndex; i++) {
        if (*s == *SymbolTable[i].name && !strcmp(s, SymbolTable[i].name)) {
            return i;
        }
    }
    return -1;
}

/**
 * getNewGlobalSymbolIndex - Get a new index for a global symbol.
 *
 * NOTE:
 * Global symbol index grows in an ascending manner.
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
 * findLocalSymbol - Find a local symbol in the symbol table.
 *
 * @param s The name of the symbol to add
 *
 * @return The index of the symbol in the symbol table. -1 if not present
 */
int findLocalSymbol(char *s) {
    for (int i = NextLocalSymbolIndex + 1; i < NSYMBOLS; i++) {
        if (*s == *SymbolTable[i].name && !strcmp(s, SymbolTable[i].name)) {
            return i;
        }
    }

    return -1;
}

/**
 * getNewLocalSymbolIndex - Get a new index for a local symbol.
 *
 * NOTE:
 * Local symbol index grows in a descending manner.
 *
 * @return The new index for the local symbol
 *
 * @note Logs a fatal error if the symbol table is full
 */
static int getNewLocalSymbolIndex(void) {
    int p;

    if ((p = NextLocalSymbolIndex--) < NextGlobalSymbolIndex) {
        logFatal("Too many local symbols");
    }

    return p;
}

/**
 * addGlobalSymbol - Add a global symbol to the symbol table.
 *
 * @param slotIndex      The slot index for the symbol.
 * @param name           The name of the symbol to add.
 * @param primitiveType  The primitive data type of the symbol.
 * @param structuralType The structural data type of the symbol.
 * @param classType      The class type of the symbol (if applicable).
 * @param endLabel       The end label for the symbol (if applicable).
 * @param size           The number of elements (for arrays, etc.).
 * @param offsetPosition The offset position from the base pointer (if
 * applicable).
 *
 * @return The index of the added symbol in the symbol table.
 *         If the symbol already exists, returns its existing index.
 */
static void updateSymbolTable(int slotIndex, char *name, int primitiveType,
                              int structuralType, int classType, int endLabel,
                              int size, int offsetPosition) {
    if (slotIndex < 0 || slotIndex >= NSYMBOLS) {
        logFatal("Invalid symbol slot number in updatesym()");
    }

    SymbolTable[slotIndex].name = strdup(name);
    SymbolTable[slotIndex].primitiveType = primitiveType;
    SymbolTable[slotIndex].structuralType = structuralType;
    SymbolTable[slotIndex].class = classType;
    SymbolTable[slotIndex].endLabel = endLabel;
    SymbolTable[slotIndex].size = size;
    SymbolTable[slotIndex].offset = offsetPosition;
}

/**
 * addGlobalSymbol - Add a global symbol to the symbol table.
 *
 * @param name           The name of the symbol to add.
 * @param primitiveType  The primitive data type of the symbol.
 * @param structuralType The structural data type of the symbol.
 * @param endLabel       The end label for the symbol (if applicable).
 * @param size           The number of elements (for arrays, etc.).
 *
 * @return The index of the added symbol in the symbol table.
 */
int addGlobalSymbol(char *name, int primitiveType, int structuralType,
                    int endLabel, int size) {
    int slotIndex;

    if ((slotIndex = findGlobalSymbol(name)) != -1) {
        // Symbol already exists, return its index
        return slotIndex;
    }

    slotIndex = getNewGlobalSymbolIndex();
    updateSymbolTable(slotIndex, name, primitiveType, structuralType, C_GLOBAL,
                      endLabel, size, 0);
    codegenDeclareGlobalSymbol(slotIndex);
    return slotIndex;
}

/**
 * addLocalSymbol - Add a local symbol to the symbol table.
 *
 * @param name           The name of the symbol to add.
 * @param primitiveType  The primitive data type of the symbol.
 * @param structuralType The structural data type of the symbol.
 * @param endLabel       The end label for the symbol (if applicable).
 * @param size           The number of elements (for arrays, etc.).
 *
 * @return The index of the added symbol in the symbol table.
 */
int addLocalSymbol(char *name, int primitiveType, int structuralType,
                   int endLabel, int size) {
    int slotIndex;

    if ((slotIndex = findLocalSymbol(name)) != -1) {
        // Symbol already exists, return its index
        return slotIndex;
    }

    slotIndex = getNewLocalSymbolIndex();
    int offsetPosition =
        // TODO: "isFunctionParameter=false" for now
        codegenGetLocalOffset(primitiveType, false /* not a function param */);
    updateSymbolTable(slotIndex, name, primitiveType, structuralType, C_LOCAL,
                      endLabel, size, offsetPosition);
    return slotIndex;
}

/**
 * findSymbol - Find a local symbol in the symbol table.
 *
 * NOTE:
 * It first tries to find the symbol in the local scope,
 * and then looks for it in the global scope.
 * It's in concord with typical scoping rules in programming languages.
 *
 * @param s The name of the symbol to add
 *
 * @return The index of the symbol in the symbol table.
 */
int findSymbol(char *s) {
    int slotIndex;

    slotIndex = findLocalSymbol(s);
    if (slotIndex == -1) {
        // There was no locally defined variable
        slotIndex = findGlobalSymbol(s);
    }

    return slotIndex;
}
