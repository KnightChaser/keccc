// src/types.c

#include "data.h"
#include "decl.h"
#include "defs.h"

/**
 * Type system implemenetaion
 */

/**
 * isIntegerType - Check if a primitive type is an integer type
 *
 * @param primitiveType Primitive type to check
 *
 * @return true if the type is an integer type, false otherwise
 */
bool isIntegerType(int primitiveType) {
    switch (primitiveType) {
    case P_CHAR:
    case P_INT:
    case P_LONG:
        return true;
    default:
        return false;
    }
}

/**
 * isPointerType - Check if a primitive type is a pointer type
 *
 * @param primitiveType Primitive type to check
 *
 * @return true if the type is a pointer type, false otherwise
 */
bool isPointerType(int primitiveType) {
    switch (primitiveType) {
    case P_VOIDPTR:
    case P_CHARPTR:
    case P_INTPTR:
    case P_LONGPTR:
        return true;
    default:
        return false;
    }
}

/**
 * primitiveTypeToPointerType - Convert a primitive type to its pointer type
 *
 * @param primitiveType Primitive type to convert
 *
 * @return Corresponding pointer type
 */
int primitiveTypeToPointerType(int primitiveType) {
    switch (primitiveType) {
    case P_VOID:
        return P_VOIDPTR;
    case P_CHAR:
        return P_CHARPTR;
    case P_INT:
        return P_INTPTR;
    case P_LONG:
        return P_LONGPTR;
    default:
        logFatald("Internal compiler error: unknown primitive type ",
                  primitiveType);
        return -1; // Unreachable
    }
}

/**
 * pointerToPrimitiveType - Convert a pointer type to its primitive type
 *
 * @param primitiveType Pointer type to convert
 *
 * @return Corresponding primitive type
 */
int pointerToPrimitiveType(int primitiveType) {
    switch (primitiveType) {
    case P_VOIDPTR:
        return P_VOID;
    case P_CHARPTR:
        return P_CHAR;
    case P_INTPTR:
        return P_INT;
    case P_LONGPTR:
        return P_LONG;
    default:
        logFatald("Internal compiler error: unknown pointer type ",
                  primitiveType);
        return -1; // Unreachable
    }
}

/**
 * coerceASTTypeForOp - Coerce an AST node to be type-compatible in an operator
 * context.
 *
 * Depending on the operator context, this may:
 * - widen integer scalar types (e.g. char -> int/long)
 * - scale an integer index for pointer arithmetic (e.g. int* + 1 -> +4 bytes)
 * - accept identical pointer types in a no-op context (assign/return checking)
 *
 * @param node AST node to coerce
 * @param contextType Peer/expected primitive type (depends on op)
 * @param op AST operator context (e.g. A_NOTHING, A_ADD, A_SUBTRACT)
 *
 * @return Coerced AST node if types are compatible, NULL otherwise
 */
struct ASTnode *coerceASTTypeForOp(struct ASTnode *node, int contextType,
                                   int op) {
    int nodeType;
    int nodeSizeBytes;
    int contextSizeBytes;

    nodeType = node->primitiveType;

    // Compare scalar integer types
    if (isIntegerType(nodeType) && isIntegerType(contextType)) {
        // Both types are the same, it's ok
        if (nodeType == contextType) {
            return node;
        }

        nodeSizeBytes = codegenGetPrimitiveTypeSize(nodeType);
        contextSizeBytes = codegenGetPrimitiveTypeSize(contextType);

        // Tree's size is too big
        if (nodeSizeBytes > contextSizeBytes) {
            return NULL;
        }

        // Widen the right size
        if (contextSizeBytes > nodeSizeBytes) {
            return makeASTUnary(A_WIDENTYPE, contextType, node, 0);
        }
    }

    // Pointer type is on the left
    if (isPointerType(nodeType)) {
        // If it's the same type and right node is doing nothing
        if (op == A_NOTHING && nodeType == contextType) {
            return node;
        }
    }

    // NOTE:
    // We can scale only on A_ADD or A_SUBTRACT operation
    // e.g. pointer arithmetic like "ptr + int" or "ptr - int"
    if (op == A_ADD || op == A_SUBTRACT) {
        // NOTE:
        // Left is the integer type, and the right is the pointer type and the
        // size of the original type is >1, then scale the left
        if (isIntegerType(nodeType) && isPointerType(contextType)) {
            // Scale by the size of the pointed-to type, not by the pointer
            // size. E.g. int* + 1 => +4 bytes on LP64.
            contextSizeBytes = codegenGetPrimitiveTypeSize(
                pointerToPrimitiveType(contextType));
            if (contextSizeBytes > 1) {
                return makeASTUnary(A_SCALETYPE, contextType, node,
                                    contextSizeBytes);
            } else {
                // If the pointee size is 1 (e.g. "char*"), no scaling needed,
                // just returns the node unmodified.
                return node;
            }
        }
    }

    // Type is not compatible
    return NULL;
}
