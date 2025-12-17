// src/treedump.c

/**
 * Functions to dump the AST tree for debugging purposes.
 */

#include "data.h"
#include "decl.h"
#include "defs.h"

/**
 * gendumpLabel - Generate a unique label number for AST dump
 *
 * @return Unique label number
 */
static int gendumpLabel(void) {
    static int id = 1;
    return (id++);
}

/**
 * dumpIndent - Print indentation for AST dump
 *
 * @param level Indentation level
 */
static void dumpIndent(int level) {
    for (int i = 0; i < level; i++) {
        printf("   ");
    }
}

/**
 * astOpToString - Convert AST operation code to string
 *
 * @param op AST operation code
 *
 * @return String representation of the AST operation code
 */
static const char *astOpToString(int op) {
    switch (op) {
    case A_NOTHING:
        return "A_NOTHING";
    case A_ASSIGN:
        return "A_ASSIGN";
    case A_ADD:
        return "A_ADD";
    case A_SUBTRACT:
        return "A_SUBTRACT";
    case A_MULTIPLY:
        return "A_MULTIPLY";
    case A_DIVIDE:
        return "A_DIVIDE";
    case A_EQ:
        return "A_EQ";
    case A_NE:
        return "A_NE";
    case A_LT:
        return "A_LT";
    case A_GT:
        return "A_GT";
    case A_LE:
        return "A_LE";
    case A_GE:
        return "A_GE";
    case A_INTLIT:
        return "A_INTLIT";
    case A_IDENTIFIER:
        return "A_IDENTIFIER";
    case A_GLUE:
        return "A_GLUE";
    case A_IF:
        return "A_IF";
    case A_WHILE:
        return "A_WHILE";
    case A_FUNCTION:
        return "A_FUNCTION";
    case A_WIDENTYPE:
        return "A_WIDENTYPE";
    case A_RETURN:
        return "A_RETURN";
    case A_FUNCTIONCALL:
        return "A_FUNCTIONCALL";
    case A_DEREFERENCE:
        return "A_DEREFERENCE";
    case A_ADDRESSOF:
        return "A_ADDRESSOF";
    case A_SCALETYPE:
        return "A_SCALETYPE";
    default:
        return "A_?";
    }
}

/**
 * primitiveTypeToString - Convert primitive type code to string
 *
 * @param primitiveType Primitive type code
 *
 * @return String representation of the primitive type code
 */
static const char *primitiveTypeToString(int primitiveType) {
    switch (primitiveType) {
    case P_NONE:
        return "P_NONE";
    case P_VOID:
        return "P_VOID";
    case P_CHAR:
        return "P_CHAR";
    case P_INT:
        return "P_INT";
    case P_LONG:
        return "P_LONG";
    case P_VOIDPTR:
        return "P_VOIDPTR";
    case P_CHARPTR:
        return "P_CHARPTR";
    case P_INTPTR:
        return "P_INTPTR";
    case P_LONGPTR:
        return "P_LONGPTR";
    default:
        return "P_?";
    }
}

/**
 * dumpASTNodeHeader - Print the header information of an AST node
 *
 * @param n     AST node
 * @param label Label number for the AST node
 * @param level Indentation level
 */
static void dumpASTNodeHeader(struct ASTnode *n, int label, int level) {
    dumpIndent(level);
    printf("L%03d: %s", label, astOpToString(n->op));
    printf(" (%s)", primitiveTypeToString(n->primitiveType));
    if (n->isRvalue) {
        printf(" rvalue");
    }

    switch (n->op) {
    case A_INTLIT:
        printf(" value=%d", n->v.intvalue);
        break;
    case A_IDENTIFIER:
    case A_FUNCTION:
    case A_FUNCTIONCALL:
    case A_ADDRESSOF:
        printf(" name=%s", GlobalSymbolTable[n->v.identifierIndex].name);
        break;
    case A_SCALETYPE:
        printf(" size=%d", n->v.size);
        break;
    default:
        break;
    }

    printf("\n");
}

/**
 * dumpAST - Recursively dump the AST nodes
 * Given an AST tree, print it out and follow the
 * traversal of the tree that codegenAST() follows
 *
 * @param n     AST node
 * @param label Label number for the AST node
 * @param level Indentation level
 */
void dumpAST(struct ASTnode *n, int label, int level) {
    int leftLabel, middleLabel, rightLabel;

    if (n == NULL) {
        return;
    }

    dumpASTNodeHeader(n, label, level);

    switch (n->op) {
    case A_IF:
        if (n->left) {
            leftLabel = gendumpLabel();
            dumpIndent(level + 1);
            printf("cond -> L%d\n", leftLabel);
            dumpAST(n->left, leftLabel, level + 2);
        }
        if (n->middle) {
            middleLabel = gendumpLabel();
            dumpIndent(level + 1);
            printf("then -> L%d\n", middleLabel);
            dumpAST(n->middle, middleLabel, level + 2);
        }
        if (n->right) {
            rightLabel = gendumpLabel();
            dumpIndent(level + 1);
            printf("else -> L%d\n", rightLabel);
            dumpAST(n->right, rightLabel, level + 2);
        }
        return;

    case A_WHILE:
        if (n->left) {
            leftLabel = gendumpLabel();
            dumpIndent(level + 1);
            printf("cond -> L%d\n", leftLabel);
            dumpAST(n->left, leftLabel, level + 2);
        }
        if (n->right) {
            rightLabel = gendumpLabel();
            dumpIndent(level + 1);
            printf("body -> L%d\n", rightLabel);
            dumpAST(n->right, rightLabel, level + 2);
        }
        return;

    case A_GLUE:
        if (n->left) {
            leftLabel = gendumpLabel();
            dumpAST(n->left, leftLabel, level + 1);
        }
        if (n->right) {
            rightLabel = gendumpLabel();
            dumpAST(n->right, rightLabel, level + 1);
        }
        return;

    case A_FUNCTION:
        if (n->left) {
            leftLabel = gendumpLabel();
            dumpAST(n->left, leftLabel, level + 1);
        }
        return;

    default:
        break;
    }

    // NOTE:
    // General AST node handling below: dump left then right.
    if (n->left) {
        leftLabel = gendumpLabel();
        dumpAST(n->left, leftLabel, level + 1);
    }
    if (n->middle) {
        middleLabel = gendumpLabel();
        dumpAST(n->middle, middleLabel, level + 1);
    }
    if (n->right) {
        rightLabel = gendumpLabel();
        dumpAST(n->right, rightLabel, level + 1);
    }
}

/**
 * dumpASTTree - Dump the entire AST tree
 *
 * @param n AST node (root of the AST tree)
 */
void dumpASTTree(struct ASTnode *n) {
    if (n == NULL) {
        return;
    }

    printf("\n============= AST dump =============\n");
    if (n->op == A_FUNCTION) {
        printf("function: %s\n", GlobalSymbolTable[n->v.identifierIndex].name);
    }
    dumpAST(n, gendumpLabel(), 0);
    printf("============= end AST dump =============\n");
}
