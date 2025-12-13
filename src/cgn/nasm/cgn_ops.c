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

    .loadImmediateInt = nasmLoadImmediateInt,
    .loadGlobalSymbol = nasmLoadGlobalSymbol,
    .storeGlobalSymbol = nasmStoreGlobalSymbol,

    .addRegs = nasmAddRegs,
    .subRegs = nasmSubRegs,
    .mulRegs = nasmMulRegs,
    .divRegsSigned = nasmDivRegsSigned,

    .compareAndSet = nasmCompareAndSet,
    .compareAndJump = nasmCompareAndJump,

    .label = nasmLabel,
    .jump = nasmJump,

    .widenPrimitiveType = nasmWidenPrimitiveType,
    .getPrimitiveTypeSize = nasmGetPrimitiveTypeSize,

    .printIntFromReg = nasmPrintIntFromReg,

    .addressOfGlobalSymbol = nasmAddressOfGlobalSymbol,
    .dereferencePointer = nasmDereferencePointer,
};
