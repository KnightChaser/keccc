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
 * @param primitiveType1 Pointer to the first primitive type
 * @param primitiveType2 Pointer to the second primitive type
 * @param onlyRight      If true, only allow widening of the right type
 *
 * @return true if the types are compatible, false otherwise
 */
bool checkPrimitiveTypeCompatibility(int *primitiveType1, int *primitiveType2,
                                     bool onlyRight) {
    // Void type is incompatible with any type
    if (*primitiveType1 == P_VOID || *primitiveType2 == P_VOID) {
        return false;
    }

    // Same types, they are compatible
    if (*primitiveType1 == *primitiveType2) {
        *primitiveType1 = *primitiveType2 = 0;
        return true;
    }

    // Widen P_CHARs to P_INTs as required
    if ((*primitiveType1 == P_CHAR) && (*primitiveType2 == P_INT)) {
        *primitiveType1 = A_WIDENTYPE;
        *primitiveType2 = 0;
        return true;
    }

    if ((*primitiveType1 == P_INT) && (*primitiveType2 == P_CHAR)) {
        if (onlyRight) {
            // P_INT can't be narrowed to P_CHAR
            return false;
        }
        *primitiveType1 = 0;
        *primitiveType2 = A_WIDENTYPE;
        return true;
    }

    // Anything remaining is compatible
    *primitiveType1 = *primitiveType2 = 0;
    return true;
}
