// src/cgn/cg_ops.h
#pragma once

#include <stdbool.h>

struct CodegenOps {
    // Register pool
    void (*declareDataSegment)(void);
    void (*declareTextSegment)(void);
    void (*resetRegisters)(void);

    // Preamble / postamble
    void (*preamble)(void);
    void (*postamble)(void);

    // Functions
    int (*functionCall)(int reg, int funcSymId);
    void (*functionPreamble)(int funcSymId);
    void (*returnFromFunction)(int reg, int funcSymId);
    void (*functionPostamble)(int funcSymId);

    // Data
    void (*declareGlobalSymbol)(int symId);
    void (*declareGlobalString)(int labelIndex, char *stringValue);

    // Expressions / loads / stores
    int (*loadImmediateInt)(int value, int primitiveType);
    int (*loadGlobalSymbol)(int symId, int op);
    int (*loadLocalSymbol)(int symId, int op);
    int (*loadGlobalString)(int symId);
    int (*storeGlobalSymbol)(int reg, int symId);

    // Arithmetic
    int (*addRegs)(int r1, int r2);
    int (*subRegs)(int r1, int r2);
    int (*mulRegs)(int r1, int r2);
    int (*divRegsSigned)(int r1, int r2);
    int (*shiftLeftConst)(int reg, int shiftAmount);
    int (*shiftLeftRegs)(int dstReg, int srcReg);
    int (*shiftRightRegs)(int dstReg, int srcReg);

    // Bitwise and logical operations
    int (*ArithmeticNegate)(int reg);
    int (*logicalInvert)(int reg);
    int (*logicalNot)(int reg);
    int (*bitwiseAndRegs)(int dstReg, int srcReg);
    int (*bitwiseOrRegs)(int dstReg, int srcReg);
    int (*bitwiseXorRegs)(int dstReg, int srcReg);
    int (*toBoolean)(int reg, int astOp, int label);

    // Comparisons
    int (*compareAndSet)(int astOp, int r1, int r2);
    int (*compareAndJump)(int astOp, int r1, int r2, int label);

    // Control flow helpers
    void (*label)(int label);
    void (*jump)(int label);

    // Types
    int (*widenPrimitiveType)(int r, int oldType, int newType);
    int (*getPrimitiveTypeSize)(int primitiveType);

    // Pointers
    int (*addressOfGlobalSymbol)(int symId);
    int (*dereferencePointer)(int pointerReg, int primitiveType);
    int (*storeDereferencedPointer)(int valueReg, int pointerReg,
                                    int primitiveType);

    // Offset
    void (*resetLocalOffset)(void);
    int (*getLocalOffset)(int type, bool isFunctionParameter);
};

// Selected backend-specific operation table
// (Set once in main() after parsing "--target" argument)
extern const struct CodegenOps *CG;

void codegenSelectTargetBackend(int target);
