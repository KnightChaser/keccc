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

// NOTE: gen.c (target-agnostic code generation)
int codegenAST(struct ASTnode *n, int reg, int parentASTop);
int codegenLabel(void);
void codegenPreamble();
void codegenPostamble();
void codegenResetRegisters();
void codegenPrintInt(int reg);
void codegenDeclareGlobalSymbol(int id);
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
int nasmStoreGlobalSymbol(int registerIndex, int id);
void nasmDeclareGlobalSymbol(int id);
int nasmAddRegs(int dstReg, int srcReg);
int nasmSubRegs(int dstReg, int srcReg);
int nasmMulRegs(int dstReg, int srcReg);
int nasmDivRegsSigned(int dividendReg, int divisorReg);
void nasmPrintIntFromReg(int reg);
int nasmCompareAndSet(int ASTop, int r1, int r2);
int nasmCompareAndJump(int ASTop, int r1, int r2, int label);
void nasmLabel(int label);
void nasmJump(int label);
int nasmWidenPrimitiveType(int r, int oldPrimitiveType, int newPrimitiveType);
int nasmGetPrimitiveTypeSize(int primitiveType);

// ARM64 AArch64 backend
void arm64ResetRegisterPool(void);
void arm64Preamble(void);
void arm64Postamble(void);
int arm64FunctionCall(int registerId, int id);
void arm64FunctionPreamble(int id);
void arm64ReturnFromFunction(int reg, int id);
void arm64FunctionPostamble(int id);
int arm64LoadImmediateInt(int value, int primitiveType);
int arm64LoadGlobalSymbol(int id);
int arm64StoreGlobalSymbol(int registerIndex, int id);
void arm64DeclareGlobalSymbol(int id);
int arm64AddRegs(int dstReg, int srcReg);
int arm64SubRegs(int dstReg, int srcReg);
int arm64MulRegs(int dstReg, int srcReg);
int arm64DivRegsSigned(int dividendReg, int divisorReg);
void arm64PrintIntFromReg(int reg);
int arm64CompareAndSet(int ASTop, int r1, int r2);
int arm64CompareAndJump(int ASTop, int r1, int r2, int label);
void arm64Label(int label);
void arm64Jump(int label);
int arm64WidenPrimitiveType(int r, int oldPrimitiveType, int newPrimitiveType);
int arm64GetPrimitiveTypeSize(int primitiveType);

// NOTE: expr.c
struct ASTnode *binexpr(int rbp);
struct ASTnode *functionCall(void);

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
                    int endLabel);

// NOTE: decl.c
void variableDeclaration(void);
struct ASTnode *functionDeclaration(void);

// NOTE: types.c
bool checkPrimitiveTypeCompatibility(int *leftHandPrimitiveType,
                                     int *rightHandPrimitiveType,
                                     bool onlyRight);
