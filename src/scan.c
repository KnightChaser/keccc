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
 *
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
    while (' ' == c || '\t' == c || '\n' == c || '\r' == c || '\f' == c) {
        c = next();
    }
    return c;
}

/**
 * scanCharacter - Return the next character from a character or string literal
 *
 * @return The character value of the scanned character literal
 */
static int scanCharacter(void) {
    int c;

    c = next();
    if (c == '\\') {
        switch (c = next()) {
        case 'a':
            return '\a';
        case 'b':
            return '\b';
        case 'f':
            return '\f';
        case 'n':
            return '\n';
        case 'r':
            return '\r';
        case 't':
            return '\t';
        case 'v':
            return '\v';
        case '\\':
            return '\\';
        case '"':
            return '"';
        case '\'':
            return '\'';
        default:
            logFatalc("unknown escape sequence", c);
        }
    }

    return c; // Just an ordinary old character!
}

/**
 * scanInteger - scan an integer literal from input
 *
 * @param c The first character of the integer literal
 *
 * @return The integer value of the scanned integer literal
 */
static int scanInteger(int c) {
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
 * scanString - Scan a string literal from input, and store it into the buffer.
 * Return the length of the string
 *
 * @param buffer The buffer to store the scanned string
 *
 * @return The length of the scanned string
 */
static int scanString(char *buffer) {
    int c;

    for (size_t index = 0; index < TEXTLEN - 1; index++) {
        if ((c = scanCharacter()) == '"') {
            buffer[index] = '\0'; // NULL terminate the string
            return index;
        }
        buffer[index] = c;
    }

    // Ran out of buffer[] space
    buffer[TEXTLEN - 1] = '\0'; // NULL terminate the string
    fprintf(stderr,
            "String literal too long on line %d (Length limit: %d), tried "
            "to scan: %s\n",
            Line, TEXTLEN - 1, buffer);
    exit(1);
    return 0; // unreachable
}

/**
 * scanIdentifier - Scan an identifier from the input file and
 *                  store it in the provided buffer.
 *
 * @param c The first character of the identifier
 * @param buf The buffer to store the scanned identifier
 * @param lengthLimit The maximum length of the identifier (including NULL
 * terminator, '\0')
 *
 * @return The length of the scanned identifier
 */
static int scanIdentifier(int c, char *buf, int lengthLimit) {
    int i = 0;

    // Allow digits, alphabets, and underscores
    while (isalpha(c) || isdigit(c) || c == '_') {
        if (lengthLimit - 1 == i) {
            // Considering the NULL character, it's a signal of buffer overflow
            printf("Identifier too long on line %d (Length limit: %d)\n", Line,
                   lengthLimit);
            exit(1);
        } else if (i < lengthLimit - 1) {
            buf[i++] = c;
        }
        c = next();
    }

    putback(c);
    buf[i] = '\0'; // NULL terminate the string. Don't forget that! >_<

    return i;
}

/**
 * keyword - check if a string is a keyword and return its token type.
 *
 * NOTE:
 * Switch on the first character to reduce the number of expensive
 * string comparisons via strcmp().
 *
 * @param s The string to check
 *
 * @return The token type if the string is a keyword, 0 otherwise
 */
static int keyword(char *s) {
    switch (*s) {
    case 'c':
        if (!strcmp(s, "char")) {
            return T_CHAR;
        }
        break;
    case 'e':
        if (!strcmp(s, "else")) {
            return T_ELSE;
        }
        break;
    case 'f':
        if (!strcmp(s, "for")) {
            return T_FOR;
        }
        break;
    case 'i':
        if (!strcmp(s, "if")) {
            return T_IF;
        }
        if (!strcmp(s, "int")) {
            return T_INT;
        }
        break;
    case 'l':
        if (!strcmp(s, "long")) {
            return T_LONG;
        }
        break;
    case 'r':
        if (!strcmp(s, "return")) {
            return T_RETURN;
        }
        break;
    case 'w':
        if (!strcmp(s, "while")) {
            return T_WHILE;
        }
        break;
    case 'v':
        if (!strcmp(s, "void")) {
            return T_VOID;
        }
        break;
    }
    return 0;
}

// A pointer to a rejected token
static struct token *RejectToken = NULL;

/**
 * rejectToken - Reject the given token so that it will be returned
 *               on the next scan() call.
 *
 * @param t Pointer to the token structure to reject
 */
void rejectToken(struct token *t) {
    if (RejectToken != NULL) {
        printf(
            "Error: Multiple token rejections without scanning a new token\n");
        exit(1);
    }
    RejectToken = t;
}

/**
 * scan - Scan and return the next token found in the input.
 *
 * @param t Pointer to the token structure to store the scanned token
 *
 * @return true if a token was successfully scanned, false if end of file (EOF)
 */
bool scan(struct token *t) {
    int c;
    int tokenType;

    // If we have any rejected token, return true,
    // assuming the token structure is already filled
    if (RejectToken != NULL) {
        *t = *RejectToken;
        RejectToken = NULL;
        return true;
    }

    // Skip whitespace characters
    c = skip();

    // Determine the token type based on the character
    switch (c) {
    case EOF:
        t->token = T_EOF;
        return false; // End of file
    case '+':
        if ((c = next()) == '+') {
            // "++"
            t->token = T_INCREMENT;
        } else {
            // "+"
            putback(c);
            t->token = T_PLUS;
        }
        break;
    case '-':
        if ((c = next()) == '-') {
            // "--"
            t->token = T_DECREMENT;
        } else {
            // "-"
            putback(c);
            t->token = T_MINUS;
        }
        break;
    case '*':
        t->token = T_STAR;
        break;
    case '/':
        t->token = T_SLASH;
        break;
    case ';':
        t->token = T_SEMICOLON;
        break;
    case '{':
        t->token = T_LBRACE;
        break;
    case '}':
        t->token = T_RBRACE;
        break;
    case '(':
        t->token = T_LPARENTHESIS;
        break;
    case ')':
        t->token = T_RPARENTHESIS;
        break;
    case '[':
        t->token = T_LBRACKET;
        break;
    case ']':
        t->token = T_RBRACKET;
        break;
    case '~':
        t->token = T_LOGICALINVERT;
        break;
    case '^':
        t->token = T_BITWISEXOR;
        break;
    case '=':
        if ((c = next()) == '=') {
            // "=="
            t->token = T_EQ;
        } else {
            // "="
            putback(c);
            t->token = T_ASSIGN;
        }
        break;
    case '!':
        if ((c = next()) == '=') {
            // "!="
            t->token = T_NE;
        } else {
            putback(c);
            t->token = T_LOGICALNOT;
        }
        break;
    case '<':
        if ((c = next()) == '=') {
            // "<="
            t->token = T_LE;
        } else if (c == '<') {
            // "<<"
            t->token = T_LSHIFT;
        } else {
            // "<"
            putback(c);
            t->token = T_LT;
        }
        break;
    case '>':
        if ((c = next()) == '=') {
            // ">="
            t->token = T_GE;
        } else if (c == '>') {
            // ">>"
            t->token = T_RSHIFT;
        } else {
            // ">"
            putback(c);
            t->token = T_GT;
        }
        break;
    case '&':
        if ((c = next()) == '&') {
            // "&&"
            t->token = T_LOGICALAND;
        } else {
            // "&"
            putback(c);
            t->token = T_AMPERSAND;
        }
        break;
    case '|':
        if ((c = next()) == '|') {
            // "||"
            t->token = T_LOGICALOR;
        } else {
            // "|"
            putback(c);
            t->token = T_BITWISEOR;
        }
        break;
    case '\'':
        // If this is a single quote, scan in the literal character
        t->intvalue = scanCharacter();
        t->token = T_INTEGERLITERAL;
        if (next() != '\'') {
            printf("Unterminated character literal on line %d\n", Line);
            exit(1);
        }
        break;
    case '\"':
        // If this is a double-quote, scan in the literal string
        scanString(Text);
        t->token = T_STRINGLITERAL;
        break;
    default:
        if (isdigit(c)) {
            // If it's a digit, scan the literal integer value in
            t->intvalue = scanInteger(c);
            t->token = T_INTEGERLITERAL;
            break;
        } else if (isalpha(c) || c == '_') {
            // If it's supposed to be a keyword, return that token instead!
            scanIdentifier(c, Text, TEXTLEN);

            if ((tokenType = keyword(Text))) {
                t->token = tokenType;
                break;
            }

            // Not a recognized keyword, thus it's an identifier
            // (e.g. variable name)
            t->token = T_IDENTIFIER;
            break;
        }

        // The character isn't part of any recognized token, raise an error
        printf("Unrecognized character '%c' on line %d\n", c, Line);
        exit(1);
    }

    // Successfully scanned a token
    return true;
}
