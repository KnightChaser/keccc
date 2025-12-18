// src/dumptree.c
//
// Functions to dump the AST tree for debugging purposes.
// Supports both full dump and compacted dump (flatten A_GLUE chains).

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "data.h"
#include "decl.h"
#include "defs.h"

// ------------------------------------------------------------
// Label generator
// ------------------------------------------------------------

static int DumpLabelId = 1;

/**
 * gendumpLabel - Generate a new unique label ID for AST nodes.
 *
 * @return New label ID.
 */
static int gendumpLabel(void) { return DumpLabelId++; }

/**
 * resetDumpLabel - Reset the label ID counter to 1.
 */
static void resetDumpLabel(void) { DumpLabelId = 1; }

// ------------------------------------------------------------
// Pretty printing helpers
// ------------------------------------------------------------

/**
 * dumpIndent - Print indentation for the given level.
 *
 * @param level Indentation level.
 */
static void dumpIndent(int level) {
    for (int i = 0; i < level; i++) {
        printf("   ");
    }
}

/**
 * astOpToString - Convert AST operation code to string.
 *
 * @param op AST operation code.
 *
 * @return String representation of the operation.
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
    case A_INTEGERLITERAL:
        return "A_INTEGERLITERAL";
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
 * primitiveTypeToString - Convert primitive type code to string.
 *
 * @param primitiveType Primitive type code.
 *
 * @return String representation of the primitive type.
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
 * dumpASTNodeHeader - Dump the header information of an AST node.
 *
 * @param n     AST node to dump.
 * @param label Label ID for the node.
 * @param level Indentation level.
 */
static void dumpASTNodeHeader(struct ASTnode *n, int label, int level) {
    dumpIndent(level);
    printf("L%03d: %s", label, astOpToString(n->op));
    printf(" (%s)", primitiveTypeToString(n->primitiveType));

    if (n->isRvalue) {
        printf(" rvalue");
    }

    switch (n->op) {
    case A_INTEGERLITERAL:
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

// ------------------------------------------------------------
// Internal dumping
// ------------------------------------------------------------

// Forward declaration
static void dumpASTInternal(struct ASTnode *n, int label, int level,
                            bool compacted);

// Dynamic array for AST nodes
typedef struct NodeVec {
    struct ASTnode **items;
    size_t len;
    size_t capacity;
} NodeVec;

/**
 * nodeVecFree - Free the resources of a NodeVec.
 *
 * @param v NodeVec to free.
 */
static void nodeVecFree(NodeVec *v) {
    free(v->items);
    v->items = NULL;
    v->len = 0;
    v->capacity = 0;
}

/**
 * nodeVecPush - Push an AST node onto a NodeVec, resizing if necessary.
 * It works like a dynamic array. (vector in C++)
 *
 * @param v    NodeVec to push onto.
 * @param node AST node to add.
 */
static void nodeVecPush(NodeVec *v, struct ASTnode *node) {
    if (v->len == v->capacity) {
        size_t newCap = (v->capacity == 0) ? 8 : v->capacity * 2;
        void *p = realloc(v->items, newCap * sizeof(v->items[0]));
        if (!p) {
            fprintf(stderr, "out of memory while dumping AST\n");
            exit(1);
        }
        v->items = (struct ASTnode **)p;
        v->capacity = newCap;
    }
    v->items[v->len++] = node;
}

/**
 * dumpGlueStatements - Dump a left-heavy A_GLUE chain as a flat list.
 *
 * We expect glue nodes shaped like:
 *   GLUE(left = previous_chain, right = next_statement)
 *
 * To preserve source order, we:
 *  - Walk left through the glue ladder collecting the "right" statements
 *  - Dump the left-most statement
 *  - Dump collected rights in reverse
 *
 * @param n        AST node (A_GLUE) to start from.
 * @param level    Indentation level.
 * @param compacted Whether we are in compacted mode.
 */
static void dumpGlueStatements(struct ASTnode *n, int level, bool compacted) {
    struct ASTnode *current = n;
    NodeVec rights = {0};

    while (current && current->op == A_GLUE) {
        if (current->right) {
            nodeVecPush(&rights, current->right);
        }
        current = current->left;
    }

    // Dump the left-most (oldest) statement first
    if (current) {
        dumpASTInternal(current, gendumpLabel(), level, compacted);
    }

    // Then dump the stored rights in reverse to restore source order
    // (Dump the stored node; newest-to-older as this walks)
    for (size_t i = rights.len; i > 0; i--) {
        dumpASTInternal(rights.items[i - 1], gendumpLabel(), level, compacted);
    }

    nodeVecFree(&rights);
}

/**
 * dumpASTInternal - Internal recursive function to dump the AST.
 *
 * @param n        AST node to dump.
 * @param label    Label ID for the node.
 * @param level    Indentation level.
 * @param compacted Whether to use compacted mode (flatten A_GLUE chains).
 */
static void dumpASTInternal(struct ASTnode *n, int label, int level,
                            bool compacted) {
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
            printf("cond -> L%03d\n", leftLabel);
            dumpASTInternal(n->left, leftLabel, level + 2, compacted);
        }
        if (n->middle) {
            middleLabel = gendumpLabel();
            dumpIndent(level + 1);
            printf("then -> L%03d\n", middleLabel);
            dumpASTInternal(n->middle, middleLabel, level + 2, compacted);
        }
        if (n->right) {
            rightLabel = gendumpLabel();
            dumpIndent(level + 1);
            printf("else -> L%03d\n", rightLabel);
            dumpASTInternal(n->right, rightLabel, level + 2, compacted);
        }
        return;

    case A_WHILE:
        if (n->left) {
            leftLabel = gendumpLabel();
            dumpIndent(level + 1);
            printf("cond -> L%03d\n", leftLabel);
            dumpASTInternal(n->left, leftLabel, level + 2, compacted);
        }
        if (n->right) {
            rightLabel = gendumpLabel();
            dumpIndent(level + 1);
            printf("body -> L%03d\n", rightLabel);
            dumpASTInternal(n->right, rightLabel, level + 2, compacted);
        }
        return;

    case A_GLUE:
        if (compacted) {
            // Flatten the entire glue ladder under this node
            dumpGlueStatements(n, level + 1, compacted);
        } else {
            if (n->left) {
                leftLabel = gendumpLabel();
                dumpASTInternal(n->left, leftLabel, level + 1, compacted);
            }
            if (n->right) {
                rightLabel = gendumpLabel();
                dumpASTInternal(n->right, rightLabel, level + 1, compacted);
            }
        }
        return;

    case A_FUNCTION:
        // Typical layout: FUNCTION(left=body)
        if (n->left) {
            leftLabel = gendumpLabel();
            dumpASTInternal(n->left, leftLabel, level + 1, compacted);
        }
        return;

    default:
        break;
    }

    // General AST node: dump children (left, middle, right)
    if (n->left) {
        leftLabel = gendumpLabel();
        dumpASTInternal(n->left, leftLabel, level + 1, compacted);
    }
    if (n->middle) {
        middleLabel = gendumpLabel();
        dumpASTInternal(n->middle, middleLabel, level + 1, compacted);
    }
    if (n->right) {
        rightLabel = gendumpLabel();
        dumpASTInternal(n->right, rightLabel, level + 1, compacted);
    }
}

// ------------------------------------------------------------
// Public API
// ------------------------------------------------------------

/**
 * dumpASTTree - Dump the entire AST tree in full mode.
 * (A_GLUE chains are preserved)
 *
 * @param n AST node to dump.
 */
void dumpASTTree(struct ASTnode *n) {
    if (n == NULL)
        return;

    resetDumpLabel();

    printf("\n============= AST dump (full) =============\n");
    if (n->op == A_FUNCTION) {
        printf("function: %s\n", GlobalSymbolTable[n->v.identifierIndex].name);
    }
    dumpASTInternal(n, gendumpLabel(), 0, false);
    printf("============= end AST dump =============\n");
}

/**
 * dumpASTTreeCompacted - Dump the entire AST tree in compacted mode.
 * (A_GLUE chains are flattened)
 *
 * @param n AST node to dump.
 */
void dumpASTTreeCompacted(struct ASTnode *n) {
    if (n == NULL)
        return;

    resetDumpLabel();

    printf("\n============= AST dump (compacted) =============\n");
    if (n->op == A_FUNCTION) {
        printf("function: %s\n", GlobalSymbolTable[n->v.identifierIndex].name);
    }
    dumpASTInternal(n, gendumpLabel(), 0, true);
    printf("============= end AST dump =============\n");
}
