// src/cgn/nasm/cgn_regs.h

#pragma once

#include <stdbool.h>

// Exposed register name tables for NASM x86-64
extern char *qwordRegisterList[4];
extern char *dwordRegisterList[4];
extern char *byteRegisterList[4];

// Register pool management
void nasmResetRegisterPool(void);
int allocateRegister(void);
void freeRegister(int r);
