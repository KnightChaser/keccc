// src/main.c

#include "cgn/cg_ops.h"
#include "decl.h"

#define extern_
#include "data.h"
#undef extern_

#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * initCompilerState - Initialize global compiler state variables.
 */
static void initCompilerState(void) {
    Line = 1;
    Putback = '\n';
}

/**
 * dieUsage - Print usage message and exit.
 *
 * @program: Name of the program (typically argv[0]).
 */
static void dieUsage(const char *program) {
    fprintf(stderr,
            "Usage: %s [--target [nasm|aarch64]|-t [nasm|aarch64]] infile\n",
            program);
    exit(1);
}

/**
 * parseTargetOrDie - Parse the target name and return the corresponding
 * target constant. Exit if the target is unsupported.
 *
 * @targetName: The name of the target (e.g., "nasm", "aarch64").
 * @program: Name of the program (typically argv[0]).
 *
 * Returns: The target constant.
 */
static int parseTargetOrDie(const char *targetName, const char *program) {
    if (strcmp(targetName, "nasm") == 0) {
        return TARGET_NASM;
    }
    if (strcmp(targetName, "aarch64") == 0) {
        return TARGET_AARCH64;
    }

    fprintf(stderr,
            "Unsupported target: %s (only 'nasm' or 'aarch64' is supported)\n",
            targetName);
    dieUsage(program);
    return TARGET_NASM; // unreachable, but keeps compilers quiet
}

/**
 * parseArgsOrDie - Parse command-line arguments and set output parameters.
 *
 * @argc: Argument count.
 * @argv: Argument vector.
 * @outTargetName: Output parameter for the target name.
 * @outInfilePath: Output parameter for the input file path.
 */
static void parseArgsOrDie(int argc, char **argv, const char **outTargetName,
                           const char **outInfilePath) {
    // Defaults
    const char *targetName = "nasm"; // TARGET_NASM
    const char *infilePath = NULL;

    static struct option longopts[] = {
        {"target", required_argument, 0, 't'},
        {0, 0, 0, 0},
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "t:", longopts, NULL)) != -1) {
        switch (opt) {
        case 't':
            targetName = optarg;
            break;
        default:
            dieUsage(argv[0]);
        }
    }

    // Expect exactly one positional argument: infile
    if (optind != argc - 1) {
        dieUsage(argv[0]);
    }
    infilePath = argv[optind];

    *outTargetName = targetName;
    *outInfilePath = infilePath;
}

/**
 * openFilesOrDie - Open input and output files, or exit on failure.
 *
 * @infilePath: Path to the input file.
 * @outfilePath: Path to the output file.
 */
static void openFilesOrDie(const char *infilePath, const char *outfilePath) {
    Infile = fopen(infilePath, "r");
    if (Infile == NULL) {
        fprintf(stderr, "Cannot open %s: %s\n", infilePath, strerror(errno));
        exit(1);
    }

    Outfile = fopen(outfilePath, "w");
    if (Outfile == NULL) {
        fclose(Infile);
        fprintf(stderr, "Cannot open %s for writing: %s\n", outfilePath,
                strerror(errno));
        exit(1);
    }
}

/**
 * closeFiles - Close input and output files if they are open.
 */
static void closeFiles(void) {
    if (Outfile)
        fclose(Outfile);
    if (Infile)
        fclose(Infile);
}

int main(int argc, char **argv) {
    struct ASTnode *tree;

    const char *targetName = NULL;
    const char *infilePath = NULL;

    parseArgsOrDie(argc, argv, &targetName, &infilePath);

    CurrentTarget = parseTargetOrDie(targetName, argv[0]);
    codegenSelectTargetBackend(CurrentTarget);

    initCompilerState();

    // TODO: make outfile configurable later. Keeping your behavior.
    const char *outfilePath = "out.s";
    openFilesOrDie(infilePath, outfilePath);

    // Ensure runtime-provided function is known to the compiler.
    // Prefer the correct type for future typechecking.
    // If your language only has int right now: use P_INT.
    addGlobalSymbol("printint", P_INT, S_FUNCTION, 0);

    scan(&Token);      // Prime first token
    codegenPreamble(); // Emit target preamble

    while (true) {
        tree = functionDeclaration();
        codegenAST(tree, NOREG, 0);

        if (Token.token == T_EOF) {
            break;
        }
    }

    closeFiles();
    return 0;
}
