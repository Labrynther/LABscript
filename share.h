#ifndef TOKENS_H
#define TOKENS_H

#define ORIG_TOKEN_CAPACITY 512

typedef enum {
    // Special Cases
    TOKEN_IDENTIFIER, 
    TOKEN_STRING,
    TOKEN_CHAR, 
    // Braces/Parentheses/Brackets
    TOKEN_LBRACE, 
    TOKEN_RBRACE, 
    TOKEN_LPAREN, 
    TOKEN_RPAREN,
    TOKEN_LBRACKET, 
    TOKEN_RBRACKET, 
    // Regular Symbols
    TOKEN_SEMICOLON, 
    TOKEN_COLON, 
    TOKEN_COMMA, 
    TOKEN_PERIOD,
    // Arithmetic/Logic
    TOKEN_NUMBER,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,   
    TOKEN_EQUAL,
    TOKEN_GREATER,
    TOKEN_LESS,
    TOKEN_PERCENT,
    TOKEN_QMARK,
    TOKEN_AMPERSAND,
    // Special Characters
    TOKEN_EOF,
    TOKEN_UNKNOWN
} TokenType;

typedef struct {
    size_t line;
    size_t column;
    size_t loc;
} TokenLocation;

typedef struct Token {
    TokenType type;
    TokenLocation pos;
    int length;
} Token;

extern char* file;

extern Token *tokenArray;
extern int count;
extern int capacity;
extern size_t fileSize;

int lex();
void parse();
void mmapInit();
void tokenArrayInit();
void throwError(char *message, int line, int column);

#endif
