// src/cgn/cg_ops.c

#include "cgn/cg_ops.h"
#include "defs.h"
#include <stdio.h>
#include <stdlib.h>

// Backends expose these operation tables
extern const struct CodegenOps nasmOps;
extern const struct CodegenOps aarch64Ops;

const struct CodegenOps *CG = NULL;

/**
 * codegenSelectTargetBackend - Select the code generation target backend.
 *
 * @param target Target identifier (e.g., TARGET_NASM, TARGET_AARCH64).
 */
void codegenSelectTargetBackend(int target) {
    switch (target) {
    case TARGET_NASM:
        CG = &nasmOps;
        break;
    case TARGET_AARCH64:
        CG = &aarch64Ops;
        break;
    default:
        fprintf(stderr,
                "Unknown codegen target %d, only TARGET_NASM (%d) and "
                "TARGET_AARCH64 (%d) are supported.\n",
                target, TARGET_NASM, TARGET_AARCH64);
        exit(1);
    }
}
