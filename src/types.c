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
 * modifyASTType - Modify the AST node type to match the right type if possible
 *
 * @param treeNode AST node to modify
 * @param rightType Primitive type to match against
 * @param operation Operation being performed (for context)
 * @return Modified AST node if types are compatible, NULL otherwise
 */
struct ASTnode *modifyASTType(struct ASTnode *treeNode, int rightType,
                              int operation) {
    int leftType;
    int leftSize;
    int rightSize;

    leftType = treeNode->primitiveType;

    // Compare scalar integer types
    if (isIntegerType(leftType) && isIntegerType(rightType)) {
        // Both types are the same, it's ok
        if (leftType == rightType) {
            return treeNode;
        }

        leftSize = codegenGetPrimitiveTypeSize(leftType);
        rightSize = codegenGetPrimitiveTypeSize(rightType);

        // Tree's size is too big
        if (leftSize > rightSize) {
            return NULL;
        }

        // Widen the right size
        if (rightSize > leftSize) {
            return makeASTUnary(A_WIDENTYPE, rightType, treeNode, 0);
        }
    }

    // Pointer type is on the left
    if (isPointerType(leftType)) {
        // If it's the same type and right node is doing nothing
        if (operation == A_NOTHING && leftType == rightType) {
            return treeNode;
        }
    }

    // NOTE:
    // We can scale only on A_ADD or A_SUBTRACT operation
    // e.g. pointer arithmetic like "ptr + int" or "ptr - int"
    if (operation == A_ADD || operation == A_SUBTRACT) {
        // Left is the integer type, and the right is the pointer type and the
        // size of the original type is >1, then scale the left
        if (isIntegerType(leftType) && isPointerType(rightType)) {
            // Scale by the size of the pointed-to type, not by the pointer
            // size. E.g. int* + 1 => +4 bytes on LP64.
            rightSize =
                codegenGetPrimitiveTypeSize(pointerToPrimitiveType(rightType));
            if (rightSize > 1) {
                return makeASTUnary(A_SCALETYPE, rightType, treeNode,
                                    rightSize);
            }
        }
    }

    // Type is not compatible
    return NULL;
}
