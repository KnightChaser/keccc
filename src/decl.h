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

// NOTE: gen.c
int genAST(struct ASTnode *n, int reg);
void genpreamble();
void genpostamble();
void genfreeregs();
void genprintint(int reg);
void genglobsym(char *s);

// NOTE: cgn.c
// Code generation utilities (NASM x86-64)
void freeAllRegisters(void);
void cgpreamble();
void cgpostamble();
int cgloadint(int value);
int cgloadglobsym(char *identifier);
int cgadd(int r1, int r2);
int cgsub(int r1, int r2);
int cgmul(int r1, int r2);
int cgdiv(int r1, int r2);
void cgprintint(int r);
int cgstoreglobsym(int registerIndex, char *identifier);
void cgglobsym(char *symbol);

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
