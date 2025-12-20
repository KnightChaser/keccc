// src/cgn/nasm/cgn_ops.c

#include "cgn/cg_ops.h"
#include "decl.h" // for nasm* prototypes already declared there

const struct CodegenOps nasmOps = {
    .resetRegisters = nasmResetRegisterPool,

    .preamble = nasmPreamble,
    .postamble = nasmPostamble,

    .functionCall = nasmFunctionCall,
    .functionPreamble = nasmFunctionPreamble,
    .returnFromFunction = nasmReturnFromFunction,
    .functionPostamble = nasmFunctionPostamble,

    .declareGlobalSymbol = nasmDeclareGlobalSymbol,
    .declareGlobalString = nasmDeclareGlobalString,

    .loadImmediateInt = nasmLoadImmediateInt,
    .loadGlobalSymbol = nasmLoadGlobalSymbol,
    .storeGlobalSymbol = nasmStoreGlobalSymbol,
    .loadGlobalString = nasmLoadGlobalString,

    .addRegs = nasmAddRegs,
    .subRegs = nasmSubRegs,
    .mulRegs = nasmMulRegs,
    .divRegsSigned = nasmDivRegsSigned,
    .shiftLeftConst = nasmShiftLeftConst,
    .shiftLeftRegs = nasmShiftLeftRegs,
    .shiftRightRegs = nasmShiftRightRegs,

    .logicalNegate = nasmLogicalNegate,
    .logicalInvert = nasmLogicalInvert,
    .logicalNot = nasmLogicalNot,
    .bitwiseAndRegs = nasmBitwiseAndRegs,
    .bitwiseOrRegs = nasmBitwiseOrRegs,
    .bitwiseXorRegs = nasmBitwiseXorRegs,
    .toBoolean = nasmToBoolean,

    .compareAndSet = nasmCompareAndSet,
    .compareAndJump = nasmCompareAndJump,

    .label = nasmLabel,
    .jump = nasmJump,

    .widenPrimitiveType = nasmWidenPrimitiveType,
    .getPrimitiveTypeSize = nasmGetPrimitiveTypeSize,

    .addressOfGlobalSymbol = nasmAddressOfGlobalSymbol,
    .dereferencePointer = nasmDereferencePointer,
    .storeDereferencedPointer = nasmStoreDereferencedPointer,
};
