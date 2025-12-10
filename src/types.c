// src/types.c

#include "data.h"
#include "decl.h"
#include "defs.h"

/**
 * Type system implemenetaion
 */

/**
 * checkPrimitiveTypeCompatibility - Check if two primitive types are compatible
 *
 * NOTE:
 * It's answering two fundamental questions about data type:
 * - "Are these two types compatible?"
 * - "If so, which side needs to be widened?"
 *
 * @param leftHandPrimitiveType  Pointer to the left-hand side primitive type
 * @param rightHandPrimitiveType Pointer to the right-hand side primitive type
 * @param onlyRight              If true, only the right-hand side can be
 * widened
 *
 * NOTE:
 *  "@param onlyRight" is about asymmetry.
 *  arithmetic(e.g. "a + b"), comparisons(e.g. "a < b") and print(e.g. "print
 * expr;") are considered symmetric contexts, which don't have destinations.
 *  However, assignment(e.g, "a = b;") or return statement(e.g, "return expr;")
 * are asymmetric contexts, which have destinations. Generally, left side (or
 * expected type) is the fixed destination; the right side must adjust to fit.
 *
 * @return true if the types are compatible, false otherwise
 */
bool checkPrimitiveTypeCompatibility(int *leftHandPrimitiveType,
                                     int *rightHandPrimitiveType,
                                     bool onlyRight) {
    // Same types, they are compatible
    if (*leftHandPrimitiveType == *rightHandPrimitiveType) {
        *leftHandPrimitiveType = *rightHandPrimitiveType = 0;
        return true;
    }

    int leftHandPrimitiveSize =
        codegenGetPrimitiveTypeSize(*leftHandPrimitiveType);
    int rightHandPrimitiveSize =
        codegenGetPrimitiveTypeSize(*rightHandPrimitiveType);

    // Types with zero size are not compatible with anything
    if (leftHandPrimitiveSize == 0 || rightHandPrimitiveSize == 0) {
        return false;
    }

    // Widen type as required
    if (leftHandPrimitiveSize < rightHandPrimitiveSize) {
        *leftHandPrimitiveType = A_WIDENTYPE;
        *rightHandPrimitiveType = 0;
        return true;
    }

    if (leftHandPrimitiveSize > rightHandPrimitiveSize) {
        if (onlyRight) {
            return false;
        }
        *leftHandPrimitiveType = 0;
        *rightHandPrimitiveType = A_WIDENTYPE;
        return true;
    }

    // Anything remaining is the same size
    // and thus compatible
    *leftHandPrimitiveType = *rightHandPrimitiveType = 0;
    return true;
}
