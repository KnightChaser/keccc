// src/defs.h
#ifndef DEFS_H
#define DEFS_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Length of symbols in input
#define TEXTLEN 512

// Number of symbol table entries
// = maximum number of unique symbols in input
#define NSYMBOLS 1024

// Token types
enum {
    // Single-character tokens
    T_EOF,        // end of file
    T_PLUS,       // +
    T_MINUS,      // -
    T_STAR,       // *
    T_SLASH,      // /
    T_EQ,         // ==
    T_NE,         // !=
    T_LT,         // <
    T_GT,         // >
    T_LE,         // <=
    T_GE,         // >=
    T_INTLIT,     // integer literal
                  // (decimal whole number which have 1 or more digits of 0-9)
    T_SEMICOLON,  // ;
    T_ASSIGN,     // =
    T_IDENTIFIER, // variable names

    // Keywords
    T_PRINT, // "print"
    T_INT    // "int"
};

// Token structure
struct token {
    int token;    // Token type
    int intvalue; // Integer value if token is T_INTLIT
};

// AST node types
enum {
    A_ADD = 1,          // Addition
    A_SUBTRACT,         // Subtraction
    A_MULTIPLY,         // Multiplication
    A_DIVIDE,           // Division
    A_EQ,               // Equality comparison (==)
    A_NE,               // Inequality comparison (!=)
    A_LT,               // Less than comparison (<)
    A_GT,               // Greater than comparison (>)
    A_LE,               // Less than or equal comparison (<=)
    A_GE,               // Greater than or equal comparison (>=)
    A_INTLIT,           // Integer literal
    A_IDENTIFIER,       // Identifier (variable)
    A_LVALUEIDENTIFIER, // L-value Identifier
    A_ASSIGN            // Assignment
};

// AST node structure
struct ASTnode {
    int op;                  // operation to be performed on this tree
    struct ASTnode *left;    // left subtree
    struct ASTnode *right;   // right subtree
    union {                  //
        int intvalue;        // integer value if op == A_INTLIT
        int identifierIndex; // symbol name if op == A_IDENTIFIER
    } v;
};

// Symbol table structure
struct symbolTable {
    char *name; // Name of a symbol
};

#endif
