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
    TOKEN_ASSIGN,
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

typedef struct {
    const char* file;
    size_t fileSize;
    size_t position;
    size_t currentLine;
    size_t currentColumn;

    Token* tokenArray;
    size_t capacity;
    size_t count;
} LexerState;

typedef enum {
    AST_START,
    AST_NUMBER,
    AST_BINARY_OP,
    AST_END,
    // add more lol
} ASTNodeType;

typedef struct {
    ASTNodeType type;

} ASTNode;

typedef struct {
    char* filename;
    const char* file;
    size_t fileSize;
    int fd;
    LexerState* lexer;
} File;

LexerState* init_lexer(const char* fileContent, int fileSize);
int lex(LexerState* lexer);
void showLex(LexerState* lexer);

void macro(LexerState* lexer, Token* input, int inputCount, Token* target, int targetCount, int start);

void parse();

char* mmapInit(int fileDescriptor, size_t *fileSize);
void throwError(char *message, int line, int column);
char* literalRead(Token str, const char* sourceText, char* fmt);

File* initFile(char* filename);

#endif