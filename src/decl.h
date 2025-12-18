// src/decl.h

// Declarations for scanner, parser, AST, interpreter, and code generator
// Used in various source files

#include <stdbool.h>

struct token;

// NOTE: scan.c
void rejectToken(struct token *t);
bool scan(struct token *t);

// NOTE: tree.c
struct ASTnode *
makeASTNode(int op,                 // AST operation code
            int primitiveType,      // Primitive data type
            struct ASTnode *left,   // Left child
            struct ASTnode *middle, // middle child (for ternary ops)
            struct ASTnode *right,  // Right child
            int intvalue            // Integer value (for leaf nodes)
);
struct ASTnode *makeASTLeaf(int op,            // AST operation code
                            int primitiveType, // Primitive data type
                            int intvalue       // Integer value (for leaf nodes)
);
struct ASTnode *makeASTUnary(int op,               // AST operation code
                             int primitiveType,    // Primitive data type
                             struct ASTnode *left, // Left child
                             int intvalue // Integer value (for leaf nodes)
);

// NOTE: treedump.c (AST dump)
void dumpAST(struct ASTnode *n, int label, int level);
void dumpASTTree(struct ASTnode *n);
void dumpASTTreeCompacted(struct ASTnode *n);

// NOTE: gen.c (target-agnostic code generation)
int codegenAST(struct ASTnode *n, int reg, int parentASTop);
int codegenGetLabelNumber(void);
void codegenPreamble();
void codegenPostamble();
void codegenResetRegisters();
void codegenDeclareGlobalSymbol(int id);
int codegenDeclareGlobalString(char *stringValue);
int codegenGetPrimitiveTypeSize(int primitiveType);
void codegenReturnFromFunction(int reg, int id);

// NOTE: cgn/*/*.c
// (cgn_expr.c, cgn_stmt.c, cgn_regs.c)
// Code generation utilities (NASM x86-64)

// NASM x86-64 backend
void nasmResetRegisterPool(void);
void nasmPreamble();
void nasmPostamble();
int nasmFunctionCall(int registerId, int id);
void nasmFunctionPreamble(int id);
void nasmReturnFromFunction(int reg, int id);
void nasmFunctionPostamble(int id);
int nasmLoadImmediateInt(int value, int primitiveType);
int nasmLoadGlobalSymbol(int id);
int nasmLoadGlobalString(int id);
int nasmStoreGlobalSymbol(int registerIndex, int id);
void nasmDeclareGlobalSymbol(int id);
void nasmDeclareGlobalString(int labelIndex, char *stringValue);
int nasmAddRegs(int dstReg, int srcReg);
int nasmSubRegs(int dstReg, int srcReg);
int nasmMulRegs(int dstReg, int srcReg);
int nasmDivRegsSigned(int dividendReg, int divisorReg);
int nasmShiftLeftConst(int reg, int shiftAmount);
int nasmCompareAndSet(int ASTop, int r1, int r2);
int nasmCompareAndJump(int ASTop, int r1, int r2, int label);
void nasmLabel(int label);
void nasmJump(int label);
int nasmWidenPrimitiveType(int r, int oldPrimitiveType, int newPrimitiveType);
int nasmGetPrimitiveTypeSize(int primitiveType);
int nasmAddressOfGlobalSymbol(int id);
int nasmDereferencePointer(int pointerReg, int primitiveType);
int nasmStoreDereferencedPointer(int valueReg, int pointerReg,
                                 int primitiveType);

// aarch64 AArch64 backend
void aarch64ResetRegisterPool(void);
void aarch64Preamble(void);
void aarch64Postamble(void);
int aarch64FunctionCall(int registerId, int id);
void aarch64FunctionPreamble(int id);
void aarch64ReturnFromFunction(int reg, int id);
void aarch64FunctionPostamble(int id);
int aarch64LoadImmediateInt(int value, int primitiveType);
int aarch64LoadGlobalSymbol(int id);
int aarch64LoadGlobalString(int id);
int aarch64StoreGlobalSymbol(int registerIndex, int id);
void aarch64DeclareGlobalSymbol(int id);
void aarch64DeclareGlobalString(int registerIndex, char *stringValue);
int aarch64AddRegs(int dstReg, int srcReg);
int aarch64SubRegs(int dstReg, int srcReg);
int aarch64MulRegs(int dstReg, int srcReg);
int aarch64DivRegsSigned(int dividendReg, int divisorReg);
int aarch64ShiftLeftConst(int reg, int shiftAmount);
int aarch64CompareAndSet(int ASTop, int r1, int r2);
int aarch64CompareAndJump(int ASTop, int r1, int r2, int label);
void aarch64Label(int label);
void aarch64Jump(int label);
int aarch64WidenPrimitiveType(int r, int oldPrimitiveType,
                              int newPrimitiveType);
int aarch64GetPrimitiveTypeSize(int primitiveType);
int aarch64AddressOfGlobalSymbol(int id);
int aarch64DereferencePointer(int pointerReg, int primitiveType);
int aarch64StoreDereferencedPointer(int valueReg, int pointerReg,
                                    int primitiveType);

// NOTE: expr.c
struct ASTnode *binexpr(int rbp);

// NOTE: stmt.c
// void statements(void);
struct ASTnode *compoundStatement(void);

// NOTE: misc.c
void match(int t, char *what);
void matchSemicolonToken(void);
void matchIdentifierToken(void);
void matchLeftBraceToken(void);        // {
void matchRightBraceToken(void);       // }
void matchLeftParenthesisToken(void);  // (
void matchRightParenthesisToken(void); // )
void logFatal(char *s);
void logFatals(char *s1, char *s2);
void logFatald(char *s, int d);
void logFatalc(char *s, int c);

// NOTE: symbol.c
int findGlobalSymbol(char *s);
int addGlobalSymbol(char *name, int primitiveType, int structuralType,
                    int endLabel, int size);

// NOTE: decl.c
int parsePrimitiveType(void);
void variableDeclaration(int type);
struct ASTnode *functionDeclaration(int type);
void globalDeclaration(void);

// NOTE: types.c
bool isIntegerType(int primitiveType);
bool isPointerType(int primitiveType);
int primitiveTypeToPointerType(int primitiveType);
int pointerToPrimitiveType(int primitiveType);
struct ASTnode *coerceASTTypeForOp(struct ASTnode *node, int contextType,
                                   int op);
