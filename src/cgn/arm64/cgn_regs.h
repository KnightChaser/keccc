// src/cgn/arm64/cgn_regs.h
#pragma once

#include <stdbool.h>

// Simple 4-register pool for ARM64 backend.
// We’ll use caller-saved registers x9–x12 / w9–w12.
extern char *arm64QwordRegisterList[8];
extern char *arm64DwordRegisterList[8];
extern char *arm64ByteRegisterList[8];

void arm64ResetRegisterPool(void);
int arm64AllocateRegister(void);
void arm64FreeRegister(int r);
