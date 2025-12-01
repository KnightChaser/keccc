// src/scan.c
//
#include "data.h"
#include "decl.h"
#include "defs.h"

/**
 * chrpos - return the position of character c in string scan s
 *
 * @param s The string to search
 * @param c The character to find
 * @return The position of character c in string s, or -1 if not found
 */
static int chrpos(char *s, int c) {
    char *p;

    p = strchr(s, c);
    if (p == NULL) {
        return -1;
    } else {
        return p - s;
    }
}

/**
 * next - get the next character from input file
 *
 * @return The next character from input file
 */
static int next(void) {
    int c;

    if (Putback) { // is there a putback character?
        c = Putback;
        Putback = 0;
        return c;
    }

    c = fgetc(Infile);
    if (c == '\n') {
        Line++;
    }

    return c;
}

static void putback(int c) { Putback = c; }

/**
 * skip - skip whitespace characters and
 * return the next non-whitespace character
 *
 * @return The next non-whitespace character from input file
 */
static int skip(void) {
    int c;

    c = next();
    while (' ' == c || '\t' == c || '\n' == c || '\r' == c) {
        c = next();
    }
    return c;
}

/**
 * scanint - scan an integer literal from input
 *
 * @param c The first character of the integer literal
 * @return The integer value of the scanned integer literal
 */
static int scanint(int c) {
    int k = 0;
    int value = 0;

    while ((k = chrpos("0123456789", c)) >= 0) {
        value = value * 10 + k;
        c = next();
    }

    // Hit a non-integer character, put it back for future processing
    putback(c);
    return value;
}

/**
 * scan - scan the next token from input
 *
 * @param t Pointer to the token structure to store the scanned token
 * @return 1 if a token was successfully scanned, 0 if end of file
 */
int scan(struct token *t) {
    int c;

    c = skip();

    // Determine the token type based on the character
    switch (c) {
    case EOF:
        t->token = T_EOF;
        return 0; // End of file
    case '+':
        t->token = T_PLUS;
        break;
    case '-':
        t->token = T_MINUS;
        break;
    case '*':
        t->token = T_STAR;
        break;
    case '/':
        t->token = T_SLASH;
        break;
    default:
        // If it's a digit, scan the literal integer value in
        if (isdigit(c)) {
            t->intvalue = scanint(c);
            t->token = T_INTLIT;
            break;
        }

        printf("Unrecognized character '%c' on line %d\n", c, Line);
        exit(1);
    }

    // Successfully scanned a token
    return 1;
}
