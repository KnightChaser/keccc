// src/cgn/aarch64/cgn_regs.h
#pragma once

#include <stdbool.h>

// Simple 4-register pool for aarch64 backend.
// We’ll use caller-saved registers x9–x12 / w9–w12.
extern char *aarch64QwordRegisterList[8];
extern char *aarch64DwordRegisterList[8];
extern char *aarch64ByteRegisterList[8];

void aarch64ResetRegisterPool(void);
int aarch64AllocateRegister(void);
void aarch64FreeRegister(int r);
