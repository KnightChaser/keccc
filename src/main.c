#include "decl.h"
#include "defs.h"

#define extern_
#include "data.h"
#undef extern_

#include <errno.h>

static void init() {
    Line = 1;
    Putback = '\n';
}

static void usage(char *prog) {
    fprintf(stderr, "Usage: %s infile\n", prog);
    exit(1);
}

int main(int argc, char **argv) {
    struct ASTnode *n;

    if (argc != 2) {
        usage(argv[0]);
    }

    init();

    Infile = fopen(argv[1], "r");
    if (Infile == NULL) {
        fprintf(stderr, "Cannot open %s: %s\n", argv[1], strerror(errno));
        exit(1);
    }

    scan(&Token);   // Get the first token from the input
    n = binexpr(0); // Parse the expression,
                    // starting at precedence level 0 (lowest)
    printf("%d", interpretAST(n));

    fclose(Infile);
    exit(0);
}
