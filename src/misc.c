// src/misc.c

#include "data.h"
#include "decl.h"
#include "defs.h"

/**
 * match - Matches the current token with the expected token.
 *         If they match, it scans the next token.
 *         If they don't match, it prints an error message and exits.
 *
 * @t: The expected token type.
 * @what: A string description of the expected token (for error messages).
 */
void match(int t, char *what) {
    if (Token.token == t) {
        scan(&Token);
    } else {
        fprintf(stderr, "Expected %s, got token %d, line %d\n", what,
                Token.token, Line);
        exit(1);
    }
}

/**
 * matchSemicolonToken - Matches a semicolon token.
 */
void matchSemicolonToken(void) { match(T_SEMICOLON, ";"); }

/**
 * matchIdentifierToken - Matches an identifier token.
 */
void matchIdentifierToken(void) { match(T_IDENTIFIER, "identifier"); }

/**
 * matchLeftBraceToken - Matches a left brace token.
 */
void matchLeftBraceToken(void) { match(T_LBRACE, "{"); }

/**
 * matchRightBraceToken - Matches a right brace token.
 */
void matchRightBraceToken(void) { match(T_RBRACE, "}"); }

/**
 * matchLeftParenthesisToken - Matches a left parenthesis token.
 */
void matchLeftParenthesisToken(void) { match(T_LPAREN, "("); }

/**
 * matchRightParenthesisToken - Matches a right parenthesis token.
 */
void matchRightParenthesisToken(void) { match(T_RPAREN, ")"); }

/**
 * logFatal - Logs a fatal error message and exits.
 *
 * @s: The error message to log.
 */
void logFatal(char *s) {
    fprintf(stderr, "Fatal error: %s, line %d\n", s, Line);
    exit(1);
}

/**
 * logFatals - Logs a fatal error message with two strings and exits.
 *
 * @s1: The first part of the error message.
 * @s2: The second part of the error message.
 */
void logFatals(char *s1, char *s2) {
    fprintf(stderr, "Fatal error: %s%s, line %d\n", s1, s2, Line);
    exit(1);
}

/**
 * logFatald - Logs a fatal error message with a string and an integer, then
 * exits.
 *
 * @s: The string part of the error message.
 * @d: The integer part of the error message.
 */
void logFatald(char *s, int d) {
    fprintf(stderr, "Fatal error: %s%d, line %d\n", s, d, Line);
    exit(1);
}

/**
 * logFatalc - Logs a fatal error message with a string and a character, then
 * exits.
 *
 * @s: The string part of the error message.
 * @c: The character part of the error message.
 */
void logFatalc(char *s, int c) {
    fprintf(stderr, "Fatal error: %s:%c, line %d\n", s, c, Line);
    exit(1);
}
