// src/cgn/arm64/cgn_ops.c

#include "cgn/cg_ops.h"
#include "decl.h"

const struct CodegenOps aarch64Ops = {
    .resetRegisters = aarch64ResetRegisterPool,
    .preamble = aarch64Preamble,
    .postamble = aarch64Postamble,

    .functionCall = aarch64FunctionCall,
    .functionPreamble = aarch64FunctionPreamble,
    .returnFromFunction = aarch64ReturnFromFunction,
    .functionPostamble = aarch64FunctionPostamble,

    .declareGlobalSymbol = aarch64DeclareGlobalSymbol,

    .loadImmediateInt = aarch64LoadImmediateInt,
    .loadGlobalSymbol = aarch64LoadGlobalSymbol,
    .storeGlobalSymbol = aarch64StoreGlobalSymbol,

    .addRegs = aarch64AddRegs,
    .subRegs = aarch64SubRegs,
    .mulRegs = aarch64MulRegs,
    .divRegsSigned = aarch64DivRegsSigned,

    .compareAndSet = aarch64CompareAndSet,
    .compareAndJump = aarch64CompareAndJump,

    .label = aarch64Label,
    .jump = aarch64Jump,

    .widenPrimitiveType = aarch64WidenPrimitiveType,
    .getPrimitiveTypeSize = aarch64GetPrimitiveTypeSize,

    .printIntFromReg = aarch64PrintIntFromReg,
};
