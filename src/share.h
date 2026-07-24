#ifndef SHARE_H
#define SHARE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <string.h>

#include "tokens.h"

typedef struct {
    char* filename;
    char* file;
    size_t fileSize;
    size_t position;
    size_t currentLine;
    size_t currentColumn;

    Token* tokenArray;
    size_t capacity;
    size_t count;

    char currentChar;
} LexerState;

typedef struct NodePage {
    char* nodesArray;
    size_t capacity;
    size_t offset;

    struct NodePage* next;
} NodePage;

typedef struct {
    NodePage* firstPage;
    NodePage* currentPage;
} NodePages;

typedef enum {
    AST_PROGRAM,

    // Operations & Access
    AST_MEMBER_ACCESS,
    AST_SCOPE_RESOLVE,
    AST_BINARY_OP, // a + b, a - b, a * b, a / b
    AST_UNARY_OP, // a++, a--
    AST_INDEX,
    AST_CAST,
    AST_PIPE,

    // Primitives
    AST_IDENTIFIER,
    AST_SPECIAL_IDENTIFIER,
    AST_KEYWORD,
    AST_NUMBER,
    AST_STRING,
    AST_CHAR,
    AST_LIST,
    AST_PROC,
    AST_BUCKET,
} ASTNodeType;

typedef struct BucketNode BucketNode;

typedef struct ASTNode {
    ASTNodeType type;
    Token mainToken;

    BucketNode* buckets;
} ASTNode;

typedef struct {
    ASTNode base;

    Token* casts;
    unsigned short castCount;

    ASTNode* target;
} CastNode;

typedef struct BucketNode{
    ASTNode base;

    ASTNode** buckets;
    short minimumBucketCount;
    short maximumBucketCount;
} BucketNode;

typedef struct {
    ASTNode base;
    
    ASTNode* left;
    ASTNode* index;
} IndexNode;

typedef struct {
    ASTNode base;

    ASTNode* left;
    Token property;
} MemberAccessNode;

typedef struct {
    ASTNode base;

    ASTNode* left;
    ASTNode* right;
} BinaryOpNode;

typedef struct {
    ASTNode base;

    CastNode* casts;
    bool special;
} IdentifierNode;

typedef struct {
    ASTNode base;
    ASTNode* name;

    CastNode* casts;
} KeywordNode;

typedef struct {
    ASTNode base;

    CastNode* casts;
} LiteralNode;

typedef struct {
    ASTNode base;

    ASTNode** statements;
    uint32_t statementCount;
} ProcNode;

typedef struct {
    ASTNode base;

    ASTNode** statements;
    uint32_t statementCount;
} ProgramNode;

typedef struct {
    ASTNode base;

    ASTNode* target;
} CompileTimeNode;

typedef struct {
    ASTNode base;
    ASTNode* operand;
    bool isPostfix;
} UnaryOpNode;

typedef struct {
    ASTNode base;

    CastNode* casts;
    
    ASTNode** elements;
    uint32_t elementCount;
} ListNode;

typedef struct {
    ASTNode base;

    ASTNode** elements;
    StateType* directions;
    TokenType* pipeTypes;
    StateType* operations;

    unsigned short elementsCount;
} PipelineNode;

typedef struct {
    StateType type;
    Token value;
} Pragma;

typedef struct {
    Token* tokenArray;
    int count;
    int current;

    NodePages* nodePages;

    char* filename;
    int fileSize;
    const char* file;

    // Used for things like Lists, Pipelines, etc...
    ASTNode** elementBuffer;
    unsigned short elementBufferCount;
    unsigned short elementBufferCapacity;

    // Used for Pipelines
    StateType* directionBuffer;
    unsigned short directionBufferCount;

    TokenType* pipeTypeBuffer;
    unsigned short pipeTypeBufferCount;

    StateType* opBuffer;
    unsigned short opBufferCount;

    Token* castBuffer;
    unsigned short castBufferCount;
    unsigned short castBufferCapacity;

    Pragma* pragmas;
    unsigned short pragmaCount;
    unsigned short pragmaCapacity;

    ProgramNode* AST;

    int depth;

    bool hadError;
} ParserState;

typedef struct {
    char* filename;
    char* file;
    size_t fileSize;
    int fd;

    LexerState* lexer;
    ParserState* parser;
} File;

typedef enum {
    ERROR_UNTERMINATED,
    ERROR_UNKNOWN_CHARACTER,
    ERROR_EXPECTED,
    ERROR_EMPTY,
    ERROR_SYNTAX

} ErrorType;

typedef enum {
    INFO,
    WARNING,
    ERROR
} ErrorClass;

typedef struct {
    ErrorType errorType;
    int info;

    ErrorClass errorClass;

    char* context;
    char* filename;
    const char* file;
    int fileSize;

    TokenLocation location;
} Error;

typedef struct {
    Error* errorArray;

    int errorCapacity;
    int errorCount;
} ErrorState;

extern ErrorState* errors;
extern atomic_bool criticalFailure;

void* allocate(size_t size);
void* reallocate(void* ptr, size_t size);
void* freeAllocation(void* ptr);

LexerState* initLexer(char* filename, char* fileContent, size_t fileSize);
void setupKeywords();
int lex(File *file);
void showLex(LexerState* lexer);

ParserState* initParser(char* filename, int fileSize, const char* file, Token* tokenArray, int count);
int parse(File *file);
void showAST(ParserState* parser, ASTNode* node, int depth);

void throwError(char* filename, int fileSize, const char* file, TokenLocation location, ErrorType errorCode, int info, ErrorClass errorClass, ...);

void macro(ParserState* parser, Token* input, int inputCount, Token* target, int targetCount, int start);

const char* fetchTokenTypeString(TokenType type);
const char* fetchTokenStateString(StateType type);

static inline void extractTokenText(ParserState* parser, Token token, char* buffer, size_t bufferSize) {
    if (token.type == TOKEN_EOF) {
        strncpy(buffer, "EOF", bufferSize); 
        return;
    }

    size_t copyLength = token.pos.length < (bufferSize - 1) ? token.pos.length : (bufferSize - 1);

    memcpy(buffer, parser->file + token.pos.loc, copyLength);
    buffer[copyLength] = '\0';
}

#endif
