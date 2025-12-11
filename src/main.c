// src/main.c

#include "decl.h"

#define extern_
#include "data.h"
#undef extern_

#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static void init() {
    Line = 1;
    Putback = '\n';
}

static void usage(char *program) {
    fprintf(stderr, "Usage: %s [--target nasm|arm64|-t nasm|arm64] infile\n",
            program);
    exit(1);
}

int main(int argc, char **argv) {
    struct ASTnode *tree;

    // Parse arguments using getopt_long
    char *infile_path = NULL;
    const char *target_name = "nasm"; // default target

    static struct option longopts[] = {{"target", required_argument, 0, 't'},
                                       {0, 0, 0, 0}};

    int opt;
    while ((opt = getopt_long(argc, argv, "t:", longopts, NULL)) != -1) {
        switch (opt) {
        case 't':
            target_name = optarg;
            break;
        default:
            usage(argv[0]);
        }
    }

    // Remaining non-option argument should be the input file
    if (optind == argc - 1) {
        infile_path = argv[optind];
    } else {
        usage(argv[0]);
    }

    // For now, only NASM is supported
    if (strcmp(target_name, "nasm") == 0) {
        CurrentTarget = TARGET_NASM;
    } else if (strcmp(target_name, "arm64") == 0) {
        CurrentTarget = TARGET_ARM64;
    } else {
        fprintf(
            stderr,
            "Unsupported target: %s (only 'nasm' or 'arm64' is supported)\n",
            target_name);
        usage(argv[0]);
    }

    init();

    // Open up the input file
    Infile = fopen(infile_path, "r");
    if (Infile == NULL) {
        fprintf(stderr, "Cannot open %s: %s\n", infile_path, strerror(errno));
        exit(1);
    }

    // Create the output file
    // TODO: make the output file name customizable later
    if ((Outfile = fopen("out.s", "w")) == NULL) {
        fprintf(stderr, "Cannot open out.s for writing: %s\n", strerror(errno));
        exit(1);
    }

    // FIX: For now, ensure that void printint() is defined
    addGlobalSymbol("printint", P_CHAR, S_FUNCTION, 0);

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
