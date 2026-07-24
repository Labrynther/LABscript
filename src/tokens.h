#ifndef TOKENS_H
#define TOKENS_H

#include <stdint.h>

/*
    DO NOT CHANGE THE TWO ENUMS BELOW UNLESS YOU HAVE:
    - changed the "tokenTypeLabels" array
    - and/or the "stateTypeLabels" array

    These are situated on a seperate file for gperf.
*/

typedef enum {
    TOKEN_IDENTIFIER,
    TOKEN_LITERAL,
    TOKEN_TYPE,

    TOKEN_PIPE,
    TOKEN_ASSIGN_PIPE,
    TOKEN_ASSIGN_OP,
    TOKEN_ASSIGN_OP_LEFT,
    TOKEN_ASSIGN_OP_RIGHT,

    TOKEN_DOT,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_BUCKET,
    TOKEN_DOLLAR,

    TOKEN_BRACE,
    TOKEN_PAREN,
    TOKEN_BRACKET,
    TOKEN_ANGLE,

    TOKEN_OP,

    TOKEN_EQUAL,
    TOKEN_QMARK,
    TOKEN_OCTOTHROPE,
    TOKEN_AMPERSAND,
    TOKEN_ELLIPSIS,
    TOKEN_SCOPE_RESOLVE,

    TOKEN_EOF,
    TOKEN_UNKNOWN
} TokenType;

typedef enum {
    NONE,

    IDENTIFIER_OBJ,
    IDENTIFIER_STRUCT,
    IDENTIFIER_ENUM,
    IDENTIFIER_NAMESPACE,
    IDENTIFIER_RETURN,
    IDENTIFIER_MACRO,
    IDENTIFIER_NOT,
    IDENTIFIER_OR,
    IDENTIFIER_XOR,
    IDENTIFIER_AND,
    IDENTIFIER_BITWISE,
    IDENTIFIER_REFERENCE,
    IDENTIFIER_DEFERENCE,
    IDENTIFIER_PRAGMA,

    PRAGMA_TARGET,
    PRAGMA_OPTIMIZE,
    PRAGMA_DIAGNOSTIC,
    PRAGMA_QUIET,
    PRAGMA_STRICT,

    TYPE_INT,
    TYPE_CHAR,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_BOOL,
    TYPE_CONST,
    TYPE_POINTER,
    TYPE_PRIVATE,

    LITERAL_NUMBER,
    LITERAL_STRING,
    LITERAL_CHAR,

    OP_PLUS,
    OP_MINUS,
    OP_MULTIPLICATION,
    OP_DIVISION,
    OP_EXPONENT,
    OP_MODULO,
    OP_INCREMENT,
    OP_DECREMENT,

    LEFT,
    RIGHT
} StateType;

typedef struct {
    uint32_t line;
    uint32_t column;
    uint64_t loc;
    int length;
} TokenLocation;

typedef struct Token {
    TokenType type;
    StateType state;

    TokenLocation pos;
} Token;

#endif
