// src/cgn/aarch64/cgn_ops.c

#include "cgn/cg_ops.h"
#include "decl.h" // for aarch64* prototypes already declared there

const struct CodegenOps aarch64Ops = {
    .resetRegisters = aarch64ResetRegisterPool,

    .preamble = aarch64Preamble,
    .postamble = aarch64Postamble,

    .functionCall = aarch64FunctionCall,
    .functionPreamble = aarch64FunctionPreamble,
    .returnFromFunction = aarch64ReturnFromFunction,
    .functionPostamble = aarch64FunctionPostamble,

    .declareGlobalSymbol = aarch64DeclareGlobalSymbol,
    .declareGlobalString = aarch64DeclareGlobalString,

    .loadImmediateInt = aarch64LoadImmediateInt,
    .loadGlobalSymbol = aarch64LoadGlobalSymbol,
    .storeGlobalSymbol = aarch64StoreGlobalSymbol,
    .loadGlobalString = aarch64LoadGlobalString,

    .addRegs = aarch64AddRegs,
    .subRegs = aarch64SubRegs,
    .mulRegs = aarch64MulRegs,
    .divRegsSigned = aarch64DivRegsSigned,
    .shiftLeftConst = aarch64ShiftLeftConst,
    .shiftLeftRegs = aarch64ShiftLeftRegs,
    .shiftRightRegs = aarch64ShiftRightRegs,

    .ArithmeticNegate = aarch64ArithmeticNegate,
    .logicalInvert = aarch64LogicalInvert,
    .logicalNot = aarch64LogicalNot,
    .bitwiseAndRegs = aarch64BitwiseAndRegs,
    .bitwiseOrRegs = aarch64BitwiseOrRegs,
    .bitwiseXorRegs = aarch64BitwiseXorRegs,
    .toBoolean = aarch64ToBoolean,

    .compareAndSet = aarch64CompareAndSet,
    .compareAndJump = aarch64CompareAndJump,

    .label = aarch64Label,
    .jump = aarch64Jump,

    .widenPrimitiveType = aarch64WidenPrimitiveType,
    .getPrimitiveTypeSize = aarch64GetPrimitiveTypeSize,

    .addressOfGlobalSymbol = aarch64AddressOfGlobalSymbol,
    .dereferencePointer = aarch64DereferencePointer,
    .storeDereferencedPointer = aarch64StoreDereferencedPointer,
};
