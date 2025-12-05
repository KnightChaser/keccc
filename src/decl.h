// src/decl.h

// Declarations for scanner, parser, AST, interpreter, and code generator
// Used in various source files

struct token;

// NOTE: scan.c
int scan(struct token *t);

// NOTE: tree.c
struct ASTnode *makeASTNode(int op, struct ASTnode *left, struct ASTnode *right,
                            int intvalue);
struct ASTnode *makeASTLeaf(int op, int intvalue);
struct ASTnode *makeASTUnary(int op, struct ASTnode *left, int intvalue);

// NOTE: gen.c (target-agnostic code generation)
int codegenAST(struct ASTnode *n, int reg);
void codegenPreamble();
void codegenPostamble();
void codegenResetRegisters();
void codegenPrintInt(int reg);
void codegenDeclareGlobalSymbol(char *s);

// NOTE: cgn.c
// Code generation utilities (NASM x86-64)
void nasmResetRegisterPool(void);
void nasmPreamble();
void nasmPostamble();
int nasmLoadImmediateInt(int value);
int nasmLoadGlobalSymbol(char *identifier);
int nasmStoreGlobalSymbol(int registerIndex, char *identifier);
void nasmDeclareGlobalSymbol(char *symbol);
int nasmAddRegs(int dstReg, int srcReg);
int nasmSubRegs(int dstReg, int srcReg);
int nasmMulRegs(int dstReg, int srcReg);
int nasmDivRegsSigned(int dividendReg, int divisorReg);
void nasmPrintIntFromReg(int reg);
int nasmCompareEqual(int r1, int r2);
int nasmCompareNotEqual(int r1, int r2);
int nasmCompareLessThan(int r1, int r2);
int nasmCompareLessThanOrEqual(int r1, int r2);
int nasmCompareGreaterThan(int r1, int r2);
int nasmCompareGreaterThanOrEqual(int r1, int r2);

// NOTE: expr.c
struct ASTnode *binexpr(int rbp);

// NOTE: stmt.c
void statements(void);

// NOTE: misc.c
void match(int t, char *what);
void semicolon(void);
void identifier(void);
void logFatal(char *s);
void logFatals(char *s1, char *s2);
void logFatald(char *s, int d);
void logFatalc(char *s, int c);

// NOTE: symbol.c
int findGlobalSymbol(char *s);
int addGlobalSymbol(char *name);

// NOTE: interpret.c
int interpretAST(struct ASTnode *n);

// NOTE: decl.c
void variableDeclaration(void);
