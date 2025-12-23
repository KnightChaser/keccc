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
    .loadLocalSymbol = nasmLoadLocalSymbol,
    .storeGlobalSymbol = nasmStoreGlobalSymbol,
    .storeLocalSymbol = nasmStoreLocalSymbol,
    .loadGlobalString = nasmLoadGlobalString,

    .addRegs = nasmAddRegs,
    .subRegs = nasmSubRegs,
    .mulRegs = nasmMulRegs,
    .divRegsSigned = nasmDivRegsSigned,
    .shiftLeftConst = nasmShiftLeftConst,
    .shiftLeftRegs = nasmShiftLeftRegs,
    .shiftRightRegs = nasmShiftRightRegs,

    .ArithmeticNegate = nasmArithmeticNegate,
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

    .addressOfSymbol = nasmAddressOfSymbol,
    .dereferencePointer = nasmDereferencePointer,
    .storeDereferencedPointer = nasmStoreDereferencedPointer,

    .resetLocalOffset = nasmResetLocalOffset,
    .getLocalOffset = nasmGetLocalOffset,
};
