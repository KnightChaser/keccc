// src/defs.h
#ifndef DEFS_H
#define DEFS_H

/**
 * WARNING:
 * These header files are included in multiple source files,
 * so don't delete the include statements here.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Code generation targets
enum {
    TARGET_NASM = 1,    // NASM-flavored x86_64 assembly
    TARGET_AARCH64 = 2, // AArch64 (ARM64) GNU as-style assembly
};

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
    T_LBRACE,     // {
    T_RBRACE,     // }
    T_LPAREN,     // (
    T_RPAREN,     // )

    // Keywords
    T_PRINT,  // "print"
    T_IF,     // "if"
    T_ELSE,   // "else"
    T_WHILE,  // "while"
    T_FOR,    // "for" (will be converted into while statement)
    T_RETURN, // "return"

    // Types
    T_VOID, // "void"
    T_CHAR, // "char"
    T_INT,  // "int"
    T_LONG, // "long"
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
    A_ASSIGN,           // Assignment
    A_PRINT,            // Print statement
    A_GLUE,             // Statement glue (for sequencing statements)
    A_IF,               // If statement
    A_WHILE,            // While loop
    A_FUNCTION,         // Function definition
    A_WIDENTYPE,        // Widen data type (usually integer)
    A_RETURN,           // Return statement
    A_FUNCTIONCALL,     // Function call
};

// Primitive types
enum {
    P_NONE, // no type
    P_VOID, // void type
    P_CHAR, // character type (1 byte)
    P_INT,  // integer type (4 bytes)
    P_LONG, // long type (8 bytes)
            // TODO: Generally, long is used for 4 or 8 bytes,
            // depending on the system architecture.
            // Here, we assume long is 8 bytes.
            // Later, we may need to modify this to handle different
            // architectures.
};

// AST node structure
struct ASTnode {
    int op;                 // operation to be performed on this tree
                            // (e.g., A_ADD, A_INTLIT)
    int primitiveType;      // primitive type (e.g., P_INT, P_CHAR)
    struct ASTnode *left;   // left subtree
    struct ASTnode *middle; // middle subtree (for if-else statements)
    struct ASTnode *right;  // right subtree

    /**
     * NOTE:
     * For A_INTLIT,       use v.intvalue to store the integer value.
     * For A_IDENTIFIER,   use v.identifierIndex to store the index
     * For A_FUNCTION,     use v.identifierIndex to store the index
     * For A_FUNCTIONCALL, use v.identifierIndex to store the index
     */
    union {
        int intvalue;
        int identifierIndex;
    } v;
};

// NOTE:
// Use NOREG when AST generation;
// functions have no register to return
#define NOREG -1

// Structural types
enum {
    S_VARIABLE,
    S_FUNCTION,
};

// Symbol table structure
struct symbolTable {
    char *name;         // Name of a symbol
    int primitiveType;  // Primitive type for the symbol (e.g., P_INT)
    int structuralType; // Structural type (e.g., S_VARIABLE)
    int endLabel;       // NOTE: For S_FUNCTION, the end label
};

#endif
