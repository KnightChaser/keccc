// src/interpret.c

#include "data.h"
#include "decl.h"
#include "defs.h"

/**
 * interpretAST - interpret an AST and return the integer result
 *
 * @param n The AST node to interpret
 *
 * @return The integer result of interpreting the AST
 */
int interpretAST(struct ASTnode *n) {
    int leftval, rightval;

    if (n->left) {
        leftval = interpretAST(n->left);
    }
    if (n->right) {
        rightval = interpretAST(n->right);
    }

    // NOTE:
    // Debugging, print what operation is being performed
    //
    // if (n->op == A_INTLIT) {
    //     printf("Integer literal: %d\n", n->intvalue);
    // } else {
    //     printf("Performing operation: %d %s %d\n", leftval,
    //     ASToperators[n->op],
    //            rightval);
    // }

    switch (n->op) {
    case A_ADD:
        return leftval + rightval;
    case A_SUBTRACT:
        return leftval - rightval;
    case A_MULTIPLY:
        return leftval * rightval;
    case A_DIVIDE:
        return leftval / rightval;
    case A_INTLIT:
        return n->v.intvalue;
    default:
        fprintf(stderr, "Unknown AST operator: %d, line: %d\n", n->op, Line);
        exit(1);
    }
}
