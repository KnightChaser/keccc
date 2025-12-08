// src/main.c

#include "decl.h"

#define extern_
#include "data.h"
#undef extern_

#include <errno.h>
#include <stdbool.h>

static void init() {
    Line = 1;
    Putback = '\n';
}

static void usage(char *program) {
    fprintf(stderr, "Usage: %s infile\n", program);
    exit(1);
}

int main(int argc, char **argv) {
    struct ASTnode *tree;

    if (argc != 2) {
        usage(argv[0]);
    }

    init();

    // Open up the input file
    Infile = fopen(argv[1], "r");
    if (Infile == NULL) {
        fprintf(stderr, "Cannot open %s: %s\n", argv[1], strerror(errno));
        exit(1);
    }

    // Create the output file
    // TODO: make the output file name customizable later
    if ((Outfile = fopen("out.s", "w")) == NULL) {
        fprintf(stderr, "Cannot open out.s for writing: %s\n", strerror(errno));
        exit(1);
    }

    scan(&Token);      // First token
    codegenPreamble(); // Emit preamble(global, printint, main prologue)
    while (true) {     // Loop to process all input
        tree = functionDeclaration(); // Parse a function declaration and...
        codegenAST(tree, NOREG, 0);   // Generate code for the AST
        if (Token.token == T_EOF) {   // Stop when we reach the end of file
            break;
        }
    }

    fclose(Outfile);

    exit(0);
}
