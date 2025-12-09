// src/tree.c

#include "data.h"
#include "decl.h"
#include "defs.h"

/**
 * makeASTNode - Build and return a generic ASt node
 *
 * @param op              the operator
 * @param primitiveType   the primitive data type
 * @param left            pointer to left subtree
 * @param middle          pointer to middle subtree
 * @param right           pointer to right subtree
 * @param intvalue        integer value (for leaf nodes)
 *
 * @return pointer to the newly created AST node
 */
struct ASTnode *makeASTNode(int op, int primitiveType, struct ASTnode *left,
                            struct ASTnode *middle, struct ASTnode *right,
                            int intvalue) {
    struct ASTnode *n;

    n = (struct ASTnode *)malloc(sizeof(struct ASTnode));
    if (n == NULL) {
        fprintf(stderr, "out of memory in makeASTNode()\n");
        exit(1);
    }

    n->op = op;
    n->primitiveType = primitiveType;
    n->left = left;
    n->middle = middle;
    n->right = right;
    n->v.intvalue = intvalue;

    return n;
}

/**
 * makeASTLeaf - create a leaf AST node
 *
 * @param op            the operator
 * @param primitiveType the primitive data type
 * @param intvalue      integer value
 *
 * @return pointer to the newly created leaf AST node
 */
struct ASTnode *makeASTLeaf(int op, int primitiveType, int intvalue) {
    return makeASTNode(op, primitiveType, NULL, NULL, NULL, intvalue);
}

/**
 * makeASTUnary - create a unary AST node
 *
 * @param op            the operator
 * @param primitiveType the primitive data type
 * @param left          pointer to left subtree
 * @param intvalue      integer value
 *
 * @return pointer to the newly created unary AST node
 */
struct ASTnode *makeASTUnary(int op, int primitiveType, struct ASTnode *left,
                             int intvalue) {
    return makeASTNode(op, primitiveType, left, NULL, NULL, intvalue);
}
