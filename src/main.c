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
 * NOTE:
 *
 * @param program Name of the program (typically argv[0]).
 */
static void dieUsage(const char *program) {
    fprintf(stderr,
            "Usage: %s [--output outfile | -o outfile] "
            "[--target [nasm|aarch64]|-t [nasm|aarch64]] "
            "[--dump-ast|-a] "
            "[--dump-ast-compacted|-A] "
            "infile\n",
            program);
    exit(1);
}

/**
 * parseTargetOrDie - Parse the target name and return the corresponding
 * target constant. Exit if the target is unsupported.
 *
 * @param targetName The name of the target (e.g., "nasm", "aarch64").
 * @param program Name of the program (typically argv[0]).
 *
 * @return The target constant.
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
 * @param argc Argument count.
 * @param argv Argument vector.
 * @param outTargetName Output parameter for the target name.
 * @param outInfilePath Output parameter for the input file path.
 * @param outOutfilePath Output parameter for the output file path.
 */
static void parseArgsOrDie(int argc, char **argv, const char **outTargetName,
                           const char **outInfilePath,
                           const char **outOutfilePath) {
    // Defaults
    const char *targetName = "nasm"; // TARGET_NASM
    const char *infilePath = NULL;
    const char *outfilePath = "out.asm"; // default

    static struct option longopts[] = {
        {"target", required_argument, 0, 't'},
        {"output", required_argument, 0, 'o'},
        {"dump-ast", no_argument, 0, 'a'},
        {"dump-ast-compacted", no_argument, 0, 'A'},
        {0, 0, 0, 0},
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "t:o:aA", longopts, NULL)) != -1) {
        switch (opt) {
        case 't':
            targetName = optarg;
            break;
        case 'o':
            outfilePath = optarg;
            break;
        case 'a':
            Option_dumpAST = true;
            Option_dumpASTCompacted = false;
            break;
        case 'A':
            Option_dumpAST = true;
            Option_dumpASTCompacted = true;
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
    *outOutfilePath = outfilePath;
}

/**
 * openFilesOrDie - Open input and output files, or exit on failure.
 *
 * @param infilePath Path to the input file.
 * @param outfilePath Path to the output file.
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
    const char *targetName = NULL;
    const char *infilePath = NULL;
    const char *outfilePath = NULL;

    // Defaults (may be overridden by CLI flags)
    Option_dumpAST = false;
    Option_dumpASTCompacted = false;

    parseArgsOrDie(argc, argv, &targetName, &infilePath, &outfilePath);

    CurrentTarget = parseTargetOrDie(targetName, argv[0]);
    codegenSelectTargetBackend(CurrentTarget);

    initCompilerState();

    openFilesOrDie(infilePath, outfilePath);

    // Ensure runtime-provided function is known to the compiler.
    // Prefer the correct type for future typechecking.
    // If your language only has int right now: use P_INT.
    addGlobalSymbol("printint", P_CHAR, S_FUNCTION, 0, 0);
    addGlobalSymbol("printchar", P_CHAR, S_FUNCTION, 0, 0);
    addGlobalSymbol("printstring", P_LONG, S_FUNCTION, 0, 0);

    scan(&Token);      // Prime first token
    codegenPreamble(); // Emit target preamble
    globalDeclaration();
    codegenPostamble();

    closeFiles();
    return 0;
}
