#include "share.h"

#define NODE_PAGE_SIZE 4096
#define BUFFER_SIZE 2048

NodePage* createNodePage(){
    NodePage* nodes = allocate(sizeof(NodePage));

    nodes->nodesArray = allocate(NODE_PAGE_SIZE);
    nodes->capacity = NODE_PAGE_SIZE;
    nodes->offset = 0;
    nodes->next = NULL;

    return nodes;
}

void* allocNode(NodePages* nodePages, size_t size) {
    NodePage* nodes = nodePages->currentPage;
    size = (size + 7) & ~7;

    if (size > NODE_PAGE_SIZE) {
        fprintf(stderr, "Fatal: AST node too large.\n");
        criticalFailure = true;
    }

    if((nodes->offset + size) >= nodes->capacity) {
        nodePages->currentPage = createNodePage();
        nodes->next = nodePages->currentPage;
        nodes = nodePages->currentPage;
    }

    void* ptr = &nodes->nodesArray[nodes->offset];
    nodes->offset += size;

    return ptr;
}

ParserState* initParser(char* filename, int fileSize, const char* file, Token* tokenArray, int count) {
    ParserState* initParse = allocate(sizeof(ParserState));

    initParse->tokenArray = tokenArray;
    initParse->count = count;
    initParse->current = 0;
    initParse->nodePages = allocate(sizeof(NodePages));
    initParse->nodePages->firstPage = createNodePage();
    initParse->nodePages->currentPage = initParse->nodePages->firstPage;

    initParse->fileSize = fileSize;
    initParse->file = file;
    initParse->filename = filename;

    initParse->elementBuffer = allocate(sizeof(ASTNode*) * BUFFER_SIZE);
    initParse->elementBufferCount = 0;
    initParse->elementBufferCapacity = BUFFER_SIZE;

    initParse->directionBuffer = allocate(sizeof(StateType) * BUFFER_SIZE);
    initParse->directionBufferCount = 0;

    initParse->pipeTypeBuffer = allocate(sizeof(TokenType) * BUFFER_SIZE);
    initParse->pipeTypeBufferCount = 0;

    initParse->opBuffer = allocate(sizeof(StateType) * BUFFER_SIZE);
    initParse->opBufferCount = 0;

    initParse->castBuffer = allocate(sizeof(Token*) * BUFFER_SIZE);
    initParse->castBufferCount = 0;
    initParse->castBufferCapacity = BUFFER_SIZE;

    initParse->pragmas = allocate(sizeof(Pragma*) * BUFFER_SIZE);
    initParse->pragmaCount = 0;
    initParse->pragmaCapacity = BUFFER_SIZE;

    initParse->AST = allocNode(initParse->nodePages, sizeof(ProgramNode));
    initParse->AST->base.type = AST_PROGRAM;
    ((ProgramNode*)initParse->AST)->statementCount = 0;

    initParse->hadError = false;

    return initParse;
}

static inline Token tokenPeek(int n, ParserState* parser) {
    if ((parser->current + n) >= parser->count) {
        Token eofToken = {0};
        eofToken.type = TOKEN_EOF;
        return eofToken;
    }

    return parser->tokenArray[parser->current + n];
}

static inline Token previous(ParserState* parser) {
    if(parser->current > 0) {
        return parser->tokenArray[parser->current - 1];
    }

    Token nullToken = {0};
    return nullToken;
}

static inline bool isAtEnd(ParserState* parser) {
    return (tokenPeek(0, parser).type == TOKEN_EOF);
}

static inline Token advance(ParserState* parser) {
    if (!isAtEnd(parser)) parser->current++;
    return previous(parser);
}

// Check functions check if the type matches, and match functions do that, and then advance

static inline bool checkType(TokenType type, ParserState* parser) {
    if (isAtEnd(parser)) return false;
    return tokenPeek(0, parser).type == type;
}

static inline bool matchType(TokenType type, ParserState* parser) {
    if (checkType(type, parser)) {
        advance(parser);
        return true;
    }
    
    return false;
}

static inline bool checkState(StateType state, ParserState* parser) {
    if (isAtEnd(parser)) return false;
    return tokenPeek(0, parser).state == state;
}

static inline bool matchState(StateType state, ParserState* parser) {
    if (checkState(state, parser)) {
        advance(parser);
        return true;
    }
    
    return false;
}

static inline bool matchToken(TokenType type, StateType state, ParserState* parser) {
    if (checkState(state, parser) && checkType(type, parser)) {
        advance(parser);
        return true;
    }
    
    return false;
}

static inline bool checkToken(TokenType type, StateType state, ParserState* parser) {
    return checkState(state, parser) && checkType(type, parser);
}

#define TYPE_MASK ((1ULL << TYPE_INT) | (1ULL << TYPE_CHAR) | (1ULL << TYPE_FLOAT) | (1ULL << TYPE_DOUBLE) | (1ULL << TYPE_BOOL))
#define ROOT_IDENTIFIER_MASK ((1ULL << IDENTIFIER_OBJ) | (1ULL << IDENTIFIER_NAMESPACE) | (1ULL << IDENTIFIER_STRUCT) | (1ULL << IDENTIFIER_ENUM))
#define DIRECTIVE_MASK ((1ULL << IDENTIFIER_PRAGMA))
#define PIPELINE_MASK ((1ULL << TOKEN_PIPE) | (1ULL << TOKEN_ASSIGN_PIPE) | (1ULL << TOKEN_ASSIGN_OP_LEFT) | (1ULL << TOKEN_ASSIGN_OP_RIGHT))

static inline void synchronize(ParserState* parser) {
    parser->elementBufferCount = 0;
    parser->directionBufferCount = 0;
    parser->pipeTypeBufferCount = 0;
    parser->opBufferCount = 0;

    // Always advance past the invalid token first
    advance(parser);

    while (!isAtEnd(parser)) {
        // If the previous token was a semicolon, we reached the end of a statement
        if (previous(parser).type == TOKEN_SEMICOLON) return;

        Token currentToken = tokenPeek(0, parser);

        // Check if current token starts a new statement/declaration
        if (currentToken.type == TOKEN_IDENTIFIER) {
            if (((1ULL << currentToken.state) & (TYPE_MASK | ROOT_IDENTIFIER_MASK)) != 0) {
                return;
            }
        }

        advance(parser);
    }
}

static inline Token consume(TokenType type, StateType state, ParserState* parser) {
    Token currentToken = tokenPeek(0, parser);
    
    if ((checkType(type, parser)) && (state == -1 || checkState(state, parser))) return advance(parser);

    const char* expectedWord = fetchTokenTypeString(type);

    char receivedWord[128];
    extractTokenText(parser, currentToken, receivedWord, sizeof(receivedWord));

    throwError(parser->filename, parser->fileSize, parser->file, currentToken.pos, ERROR_EXPECTED, type, ERROR, expectedWord, receivedWord);

    synchronize(parser);

    return currentToken;
}

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_PIPE,
    PREC_OR,
    PREC_XOR,
    PREC_AND,
    PREC_EQUALITY,
    PREC_COMPARISON,
    PREC_ADDITIVE,
    PREC_MULTIPLICATIVE,
    PREC_UNARY,
    PREC_POSTFIX,
    PREC_PRIMARY
} Precedence;

typedef ASTNode* (*PrefixHandler)(ParserState* parser);
typedef ASTNode* (*InfixHandler)(ASTNode* left, ParserState* parser);

typedef struct {
    PrefixHandler prefix;
    InfixHandler infix;
    Precedence rank;
} ParseRule;

ASTNode* parseLiteral(ParserState* parser);
ASTNode* parseIdentifier(ParserState* parser);
ASTNode* parseKeyword(ParserState* parser);
ASTNode* parsePrefixUnary(ParserState* parser);
ASTNode* parsePostfixUnary(ASTNode* left, ParserState* parser);
ASTNode* parseBinary(ASTNode* left, ParserState* parser);
ASTNode* parsePipe(ASTNode* left, ParserState* parser);
ASTNode* parseBucket(ParserState* parser) ;
ASTNode* parseGrouping(ParserState* parser);
ASTNode* parseList(ParserState* parser);
ASTNode* parseIndex(ASTNode* left, ParserState* parser);
ASTNode* parseProc(ParserState* parser);
ASTNode* parseCast(ParserState* parser);
ASTNode* parseMemberAccess(ASTNode* left, ParserState* parser);
ASTNode* parseCompileTime(ParserState* parser);
ASTNode* parseSpecial(ParserState* parser);

ParseRule fetchRule(Token token) {
    ParseRule rule = {NULL, NULL, PREC_NONE};

    switch (token.type) {
        case TOKEN_LITERAL:
            rule.prefix = parseLiteral;
            rule.rank = PREC_NONE;
            break;
            
        case TOKEN_BUCKET:
            rule.prefix = parseBucket;
            rule.rank = PREC_PRIMARY;
            break;

        case TOKEN_IDENTIFIER:
            if(((1ULL << token.state) & (ROOT_IDENTIFIER_MASK | TYPE_MASK)) == 0) {
                rule.prefix = parseIdentifier;
            } else {
                rule.prefix = parseKeyword;
            }
            rule.rank = PREC_NONE;
            break;

        case TOKEN_PIPE:
            rule.infix = parsePipe;
            rule.rank = PREC_PIPE;
            break;

        case TOKEN_ASSIGN_PIPE:
        case TOKEN_ASSIGN_OP:
        case TOKEN_ASSIGN_OP_LEFT:
        case TOKEN_ASSIGN_OP_RIGHT:
            rule.infix = parsePipe;
            rule.rank = PREC_ASSIGNMENT;
            break;

        case TOKEN_PAREN:
            if(token.state == LEFT) {
                rule.prefix = parseGrouping;
            }
            break;
        
        case TOKEN_BRACKET:
            if(token.state == LEFT) {
                rule.prefix = parseList;
                rule.infix = parseIndex;
                rule.rank = PREC_PRIMARY;
            }
            break;
        case TOKEN_BRACE:
            if(token.state == LEFT) {
                rule.prefix = parseProc;
            }
            break;
        case TOKEN_ANGLE:
            if(token.state == LEFT) {
                rule.prefix = parseCast;
                rule.infix = parseBinary;
                rule.rank = PREC_PRIMARY;
            } 
            break;
        case TOKEN_DOT:
        case TOKEN_SCOPE_RESOLVE:
            rule.infix = parseMemberAccess;
            rule.rank = PREC_PRIMARY;
            break;
        case TOKEN_OCTOTHROPE:
            rule.prefix = parseCompileTime;
            rule.rank = PREC_PRIMARY;
            break;
        case TOKEN_DOLLAR:
            rule.prefix = parseSpecial;
            rule.rank = PREC_PRIMARY;
            break;
    }

    switch(token.state) {
        case IDENTIFIER_NOT:
            rule.prefix = parsePrefixUnary;
            rule.rank = PREC_UNARY;
            break;
        case IDENTIFIER_AND:
            rule.infix = parseBinary;
            rule.rank = PREC_AND;
            break;
        case IDENTIFIER_OR:
            rule.infix = parseBinary;
            rule.rank = PREC_OR;
            break;
        case IDENTIFIER_XOR:
            rule.infix = parseBinary;
            rule.rank = PREC_XOR;
            break;
        case OP_PLUS:
            rule.infix = parseBinary;
            rule.rank = PREC_ADDITIVE;
            break;
        case OP_MINUS:
            rule.prefix = parsePrefixUnary;
            rule.infix = parseBinary;
            rule.rank = PREC_ADDITIVE;
            break;
        case OP_MULTIPLICATION:
        case OP_DIVISION:
        case OP_MODULO:
            rule.infix = parseBinary;
            rule.rank = PREC_MULTIPLICATIVE;
            break;
        case OP_INCREMENT:
            rule.prefix = parsePrefixUnary;
            rule.infix = parsePostfixUnary;
            rule.rank = PREC_POSTFIX;
            break;
        case OP_DECREMENT:
            rule.prefix = parsePrefixUnary;
            rule.infix = parsePostfixUnary;
            rule.rank = PREC_POSTFIX;
            break;
    }

    return rule;
}

ASTNode* parsePrecedence(Precedence rank, ParserState* parser) {
    parser->depth++;

    advance(parser);
    ParseRule rule = fetchRule(previous(parser));
    if (rule.prefix == NULL) {
        Token errorToken = previous(parser);

        char receivedWord[128];
        extractTokenText(parser, errorToken, receivedWord, sizeof(receivedWord));

        throwError(parser->filename, parser->fileSize, parser->file, errorToken.pos, ERROR_EXPECTED, TOKEN_UNKNOWN + 1, ERROR, "expression", receivedWord);
        synchronize(parser);

        parser->depth--;
        return NULL; 
    }
    ASTNode* left = rule.prefix(parser);
    ParseRule nextRule = fetchRule(tokenPeek(0, parser));
    while(rank < nextRule.rank){
        advance(parser);
        nextRule = fetchRule(previous(parser));
        left = nextRule.infix(left, parser);
        nextRule = fetchRule(tokenPeek(0, parser));
    }

    parser->depth--;
    return left;
}

ASTNode* parseLiteral(ParserState* parser) {
    LiteralNode* node = allocNode(parser->nodePages, sizeof(LiteralNode));

    node->base.mainToken = previous(parser);
    switch(node->base.mainToken.state) {
        case LITERAL_CHAR:
            node->base.type = AST_CHAR;
            break;
        case LITERAL_STRING:
            node->base.type = AST_STRING;
            break;
        case LITERAL_NUMBER:
            node->base.type = AST_NUMBER;
            break;
    }

    return (ASTNode*)node;
}

ASTNode* parseIdentifier(ParserState* parser) {
    IdentifierNode* node = allocNode(parser->nodePages, sizeof(IdentifierNode));

    node->base.mainToken = previous(parser);
    node->base.type = AST_IDENTIFIER;
    node->special = false;

    return (ASTNode*)node;
}

ASTNode* parseSpecial(ParserState* parser) {
    IdentifierNode* node = allocNode(parser->nodePages, sizeof(IdentifierNode));

    node->base.mainToken = previous(parser);
    node->base.type = AST_SPECIAL_IDENTIFIER;
    node->special = true;

    return (ASTNode*)node;
}

ASTNode* parseKeyword(ParserState* parser) {
    KeywordNode* node = allocNode(parser->nodePages, sizeof(KeywordNode));

    node->base.mainToken = previous(parser);
    node->base.type = AST_KEYWORD;

    node->name = parsePrecedence(PREC_PIPE, parser);

    return (ASTNode*)node;
}

ASTNode* parseGrouping(ParserState* parser) {
    ASTNode* innerNode = parsePrecedence(PREC_NONE, parser);
    consume(TOKEN_PAREN, RIGHT, parser);

    return innerNode;
}

ASTNode* parsePipe(ASTNode* left, ParserState* parser) {
    PipelineNode* node = allocNode(parser->nodePages, sizeof(PipelineNode));
    node->base.type = AST_PIPE;
    node->base.mainToken = previous(parser);
    parser->directionBuffer = reallocate(parser->directionBuffer, parser->elementBufferCapacity * sizeof(ASTNode*));
    
    int elemIdx = parser->elementBufferCount;
    int dirIdx = parser->directionBufferCount;
    int pipeTypeIdx = parser->pipeTypeBufferCount;
    int opIdx = parser->opBufferCount;

    parser->elementBuffer[parser->elementBufferCount++] = left;
    parser->directionBuffer[parser->directionBufferCount++] = NONE;
    parser->pipeTypeBuffer[parser->pipeTypeBufferCount++] = TOKEN_UNKNOWN;
    parser->opBuffer[parser->opBufferCount++] = NONE;

    while (true) {
        if (parser->elementBufferCount >= parser->elementBufferCapacity) {
            parser->elementBuffer = reallocate(parser->elementBuffer, (parser->elementBufferCapacity += BUFFER_SIZE) * sizeof(ASTNode*));
            parser->directionBuffer = reallocate(parser->directionBuffer, parser->elementBufferCapacity * sizeof(StateType));
            parser->pipeTypeBuffer = reallocate(parser->pipeTypeBuffer, parser->elementBufferCapacity * sizeof(TokenType));
            parser->opBuffer = reallocate(parser->opBuffer, parser->elementBufferCapacity * sizeof(StateType));
        }

        TokenType currentPipe = previous(parser).type;
        StateType currentState = previous(parser).state;

        if (currentPipe == TOKEN_PIPE || currentPipe== TOKEN_ASSIGN_PIPE) {
            parser->directionBuffer[parser->directionBufferCount++] = currentState;
            parser->pipeTypeBuffer[parser->pipeTypeBufferCount++] = currentPipe;
            parser->opBuffer[parser->opBufferCount++] = NONE;
        } else {
            parser->pipeTypeBuffer[parser->pipeTypeBufferCount++] = currentPipe;
            parser->opBuffer[parser->opBufferCount++] = currentState;

            if (currentPipe == TOKEN_ASSIGN_OP_RIGHT) {
                parser->directionBuffer[parser->directionBufferCount++] = RIGHT;
            } else if (currentPipe == TOKEN_ASSIGN_OP_LEFT) {
                parser->directionBuffer[parser->directionBufferCount++] = LEFT;
            } else {
                parser->directionBuffer[parser->directionBufferCount++] = NONE; // For neutral +=
            }
        }

        parser->elementBuffer[parser->elementBufferCount++] = parsePrecedence(PREC_PIPE, parser);

        if ((((1ULL << tokenPeek(0, parser).type) & PIPELINE_MASK) != 0) && !checkType(TOKEN_EOF, parser)) {
            advance(parser);
        } else {
            break;
        }
    }

    node->elements = allocate((parser->elementBufferCount - elemIdx) * sizeof(ASTNode*));
    memcpy(node->elements, &parser->elementBuffer[elemIdx], (parser->elementBufferCount - elemIdx) * sizeof(ASTNode*));

    node->directions = allocate((parser->directionBufferCount - dirIdx) * sizeof(StateType));
    memcpy(node->directions, &parser->directionBuffer[dirIdx], (parser->directionBufferCount - dirIdx) * sizeof(StateType));

    node->pipeTypes = allocate((parser->pipeTypeBufferCount - pipeTypeIdx) * sizeof(TokenType));
    memcpy(node->pipeTypes, &parser->pipeTypeBuffer[pipeTypeIdx], (parser->pipeTypeBufferCount - pipeTypeIdx) * sizeof(TokenType));

    node->operations = allocate((parser->opBufferCount - opIdx) * sizeof(StateType));
    memcpy(node->operations, &parser->opBuffer[opIdx], (parser->opBufferCount - opIdx) * sizeof(StateType));

    node->elementsCount = parser->elementBufferCount - elemIdx;

    parser->elementBufferCount = elemIdx;
    parser->directionBufferCount = dirIdx;
    parser->pipeTypeBufferCount = pipeTypeIdx;
    parser->opBufferCount = opIdx;

    return (ASTNode*)node;
}

ASTNode* parsePrefixUnary (ParserState* parser) {
    UnaryOpNode* node = allocNode(parser->nodePages, sizeof(UnaryOpNode));
    
    node->base.mainToken = previous(parser);
    node->base.type = AST_UNARY_OP;
    node->operand = parsePrecedence(PREC_UNARY, parser);
    node->isPostfix = false;

    return (ASTNode*)node;
}

ASTNode* parsePostfixUnary(ASTNode* left, ParserState* parser) {
    UnaryOpNode* node = allocNode(parser->nodePages, sizeof(UnaryOpNode));

    node->base.mainToken = previous(parser);
    node->base.type = AST_UNARY_OP;
    node->operand = left;
    node->isPostfix = true;

    return (ASTNode*)node;
}

ASTNode* parseBinary (ASTNode* left, ParserState* parser) {
    BinaryOpNode* node = allocNode(parser->nodePages, sizeof(BinaryOpNode));

    ParseRule rule = fetchRule(previous(parser));

    node->base.mainToken = previous(parser);
    node->base.type = AST_BINARY_OP;
    node->left = left;
    node->right = parsePrecedence(rule.rank + 1, parser);

    return (ASTNode*)node;
}

ASTNode* parseIndex(ASTNode* left, ParserState* parser) {
    IndexNode* node = allocNode(parser->nodePages, sizeof(IndexNode));

    node->base.mainToken = previous(parser);
    node->base.type = AST_INDEX;
    node->left = left;
    node->index = parsePrecedence(PREC_NONE, parser);

    consume(TOKEN_BRACKET, RIGHT, parser);

    return (ASTNode*)node;
}

ASTNode* parseMemberAccess(ASTNode* left, ParserState* parser) {
    MemberAccessNode* node = allocNode(parser->nodePages, sizeof(MemberAccessNode));

    node->base.mainToken = previous(parser);
    switch (node->base.mainToken.type) {
        case TOKEN_DOT:
            node->base.type = AST_MEMBER_ACCESS;
            break;
        case TOKEN_SCOPE_RESOLVE:
            node->base.type = AST_SCOPE_RESOLVE;
            break;
    }

    node->left = left;
    node->property = consume(TOKEN_IDENTIFIER, -1, parser);

    return (ASTNode*)node;
}

ASTNode* parseList (ParserState* parser) {
    int listIdx = parser->elementBufferCount;
    ListNode* node = allocNode(parser->nodePages, sizeof(ListNode));
    node->base.type = AST_LIST;

    node->base.mainToken = previous(parser);

    if (checkToken(TOKEN_BRACKET, RIGHT, parser)) {
        node->elementCount = 0;
        advance(parser);
    } else {
        do {
            if (parser->elementBufferCount >= parser->elementBufferCapacity) {
                parser->elementBuffer = reallocate(parser->elementBuffer, (parser->elementBufferCapacity += BUFFER_SIZE) * sizeof(ASTNode*));
            }

            parser->elementBuffer[parser->elementBufferCount++] = parsePrecedence(PREC_NONE, parser);
        } while (matchType(TOKEN_COMMA, parser) && !checkToken(TOKEN_BRACKET, RIGHT, parser) && !checkType(TOKEN_EOF, parser));

        consume(TOKEN_BRACKET, RIGHT, parser);

        node->elements = allocate((parser->elementBufferCount - listIdx) * sizeof(ASTNode*));
        memcpy(node->elements, &parser->elementBuffer[listIdx], (parser->elementBufferCount - listIdx) * sizeof(ASTNode*));

        node->elementCount = parser->elementBufferCount - listIdx;
        parser->elementBufferCount = listIdx;

    }

    return (ASTNode*)node;
}

ASTNode* parseCast (ParserState* parser) {
    int castIdx = parser->castBufferCount;
    CastNode* node = allocNode(parser->nodePages, sizeof(CastNode));
    node->base.type = AST_CAST;
    node->base.mainToken = previous(parser);

    if (checkToken(TOKEN_ANGLE, RIGHT, parser)) {
        node->castCount = 0;
        node->casts = NULL;
        node->target = NULL;

        Token errorToken = tokenPeek(0, parser);

        char receivedWord[128];
        extractTokenText(parser, errorToken, receivedWord, sizeof(receivedWord));
        
        throwError(parser->filename, parser->fileSize, parser->file, errorToken.pos, ERROR_EMPTY, 1,ERROR, "expression", receivedWord);
        synchronize(parser);
    } else {
        do {
            if (parser->castBufferCount >= parser->castBufferCapacity) {
                parser->castBuffer = reallocate(parser->castBuffer, (parser->castBufferCapacity += BUFFER_SIZE) * sizeof(StateType));
            }

            parser->castBuffer[parser->castBufferCount] = consume(TOKEN_IDENTIFIER, -1, parser);
            parser->castBufferCount++;
        } while (matchType(TOKEN_COMMA, parser) && !checkToken(TOKEN_ANGLE, RIGHT, parser) && !checkType(TOKEN_EOF, parser));

        consume(TOKEN_ANGLE, RIGHT, parser);

        node->casts = allocate((parser->castBufferCount - castIdx) * sizeof(Token));
        memcpy(node->casts, &parser->castBuffer[castIdx], (parser->castBufferCount - castIdx) * sizeof(Token));

        node->target = parsePrecedence(PREC_UNARY, parser);

        node->castCount = parser->castBufferCount - castIdx;
        parser->castBufferCount = castIdx;
    }

    return (ASTNode*)node;
}

ASTNode* parseBucket(ParserState* parser) {
    int bucketIdx = parser->elementBufferCount;
    BucketNode* node = allocNode(parser->nodePages, sizeof(BucketNode));
    node->base.type = AST_BUCKET;
    node->base.mainToken = previous(parser);

    node->minimumBucketCount = 0;
    node->maximumBucketCount = 0;

    if (checkType(TOKEN_BUCKET,parser)) {
        node->buckets = NULL;

        Token errorToken = tokenPeek(0, parser);
        
        char receivedWord[128];
        extractTokenText(parser, errorToken, receivedWord, sizeof(receivedWord));

        throwError(parser->filename, parser->fileSize, parser->file, errorToken.pos, ERROR_EXPECTED, TOKEN_UNKNOWN + 1, ERROR, "expression", receivedWord);
        synchronize(parser);
    } else {
        do {
            if (parser->elementBufferCount >= parser->elementBufferCapacity) {
                parser->elementBuffer = reallocate(parser->elementBuffer, (parser->elementBufferCapacity += BUFFER_SIZE) * sizeof(ASTNode*));
            }

            parser->elementBuffer[parser->elementBufferCount++] = parsePrecedence(PREC_NONE, parser);
            node->maximumBucketCount++;

            switch(tokenPeek(0, parser).type) {
            case TOKEN_QMARK:
                matchType(TOKEN_QMARK, parser);
                node->minimumBucketCount++;
                break;
            case TOKEN_ELLIPSIS:
                matchType(TOKEN_ELLIPSIS, parser);
                node->maximumBucketCount = -1;
                break;
            }
        } while (matchType(TOKEN_COMMA, parser) && !checkToken(TOKEN_BUCKET, NONE, parser) && !checkType(TOKEN_EOF, parser));

        consume(TOKEN_BUCKET, -1, parser);
        
        node->buckets = allocate((parser->elementBufferCount - bucketIdx) * sizeof(ASTNode*));
        memcpy(node->buckets, &parser->elementBuffer[bucketIdx], (parser->elementBufferCount - bucketIdx) * sizeof(ASTNode*));

        parser->elementBufferCount = bucketIdx;

        ASTNode* targetExpression = parsePrecedence(PREC_PIPE, parser);
        targetExpression->buckets = node;

        return targetExpression;
    }
}

ASTNode* parseProc(ParserState* parser) {
    int procIdx = parser->elementBufferCount;
    ProcNode* node = allocNode(parser->nodePages, sizeof(ProcNode));
    node->base.type = AST_PROC;
    node->base.mainToken = previous(parser);

    if (checkToken(TOKEN_BRACE, RIGHT, parser)) {
        node->statementCount = 0;
        node->statements = NULL;
        advance(parser);
    } else {
        do {
            if (parser->elementBufferCount >= parser->elementBufferCapacity) {
                parser->elementBuffer = reallocate(parser->elementBuffer, (parser->elementBufferCapacity += BUFFER_SIZE) * sizeof(ASTNode*));
            }

            parser->elementBuffer[parser->elementBufferCount++] = parsePrecedence(PREC_NONE, parser);

            consume(TOKEN_SEMICOLON, -1, parser);
        } while(!checkToken(TOKEN_BRACE, RIGHT, parser) && !checkType(TOKEN_EOF, parser));
    
        consume(TOKEN_BRACE, RIGHT, parser);

        node->statements = allocate((parser->elementBufferCount - procIdx) * sizeof(ASTNode*));
        memcpy(node->statements, &parser->elementBuffer[procIdx], (parser->elementBufferCount - procIdx) * sizeof(ASTNode*));

        node->statementCount = parser->elementBufferCount - procIdx;
        parser->elementBufferCount = procIdx;
    }

    return (ASTNode*)node;
}

ASTNode* parseCompileTime(ParserState* parser) {
    Token directive = consume(TOKEN_IDENTIFIER, -1, parser);
        switch(directive.state) {
            case IDENTIFIER_PRAGMA: {
                Token typeToken = advance(parser);
                StateType pragmaType = typeToken.state;
                Token pragmaValue = {0};

                switch(pragmaType) {
                    case PRAGMA_TARGET:
                        pragmaValue = consume(TOKEN_IDENTIFIER, -1, parser);
                        break;
                    case PRAGMA_OPTIMIZE:
                        pragmaValue = consume(TOKEN_LITERAL, LITERAL_NUMBER, parser);
                        break;
                }

                if (parser->pragmaCount >= parser->pragmaCapacity) {
                    parser->pragmas = reallocate(parser->pragmas, (parser->pragmaCapacity += BUFFER_SIZE) * sizeof(Pragma));
                }

                Pragma pragma = {
                    .type = pragmaType,
                    .value = pragmaValue,
                };

                parser->pragmas[parser->pragmaCount++] = pragma;
                break;
            }

            case IDENTIFIER_MACRO: {
                consume(TOKEN_BRACE, LEFT, parser);

                int inputCapacity = 8;
                int inputCount = 0;
                Token* input = allocate(inputCapacity * sizeof(Token));

                while (!isAtEnd(parser)) {
                    Token current = tokenPeek(0, parser);
                    if (current.type == TOKEN_BRACE && current.state == RIGHT) {
                        break;
                    }

                    if (inputCount >= inputCapacity) {
                        inputCapacity += 8;
                        input = reallocate(input, inputCapacity * sizeof(Token));
                    }

                    input[inputCount++] = advance(parser);
                }
                consume(TOKEN_BRACE, RIGHT, parser);

                consume(TOKEN_BRACE, LEFT, parser);

                int targetCapacity = 8;
                int targetCount = 0;
                Token* target = allocate(targetCapacity * sizeof(Token));

                while (!isAtEnd(parser)) {
                    Token current = tokenPeek(0, parser);
                    if (current.type == TOKEN_BRACE && current.state == RIGHT) {
                        break;
                    }

                    if (targetCount >= targetCapacity) {
                        targetCapacity += 8;
                        target = reallocate(target, targetCapacity * sizeof(Token));
                    }

                    target[targetCount++] = advance(parser);
                }

                consume(TOKEN_BRACE, RIGHT, parser);

                macro(parser, input, inputCount, target, targetCount, parser->current);

                break;
            }

            default: {
                char receivedWord[128];
                extractTokenText(parser, directive, receivedWord, sizeof(receivedWord));

                throwError(parser->filename, parser->fileSize, parser->file, directive.pos, 
                        ERROR_EXPECTED, TOKEN_IDENTIFIER, ERROR, 
                        "compile-time directive (lowercase, e.g. macro, pragma)", receivedWord);

                synchronize(parser);
                break;
            }
        }

    return NULL;
}

void printIndent(int depth) {
    for(int i = 0; i < depth; i++) {
        printf("  ");
    }
}

void showAST(ParserState* parser, ASTNode* node, int depth) {
    if (node == NULL) return;

    printIndent(depth);

    if (node->type != AST_PROGRAM) {
        printf("[Line %d, Col %d] ", node->mainToken.pos.line, node->mainToken.pos.column);
    }

    switch (node->type) {
        case AST_IDENTIFIER:
        case AST_KEYWORD:
        case AST_SPECIAL_IDENTIFIER: {
            char buffer[64];
            extractTokenText(parser, node->mainToken, buffer, 64);
            printf("\033[0mIDENTIFIER (%s): %s\n", ((IdentifierNode*)node)->base.mainToken.type == TOKEN_IDENTIFIER ? "REGULAR" : (node->type == AST_SPECIAL_IDENTIFIER ? "SPECIAL" : "KEYWORD"), buffer);
            break;
        }

        case AST_CHAR:
        case AST_STRING:
        case AST_NUMBER: {
            char buffer[64];
            extractTokenText(parser, node->mainToken, buffer, 64);
            printf("\033[0mLITERAL (%s): %s\n", fetchTokenStateString(node->mainToken.state), buffer);
            break;
        }

        case AST_BINARY_OP:
            printf("\033[0mBINARY_OP:\n");
            printIndent(depth + 1);
            printf("Left:\n");
            showAST(parser, ((BinaryOpNode*)node)->left, depth + 2);
            printIndent(depth + 1);
            printf("Right:\n");
            showAST(parser, ((BinaryOpNode*)node)->right, depth + 2);
            break;
        
        case AST_UNARY_OP:
            printf("\033[0mUNARY_OP:\n");
            printIndent(depth + 1);
            printf("Operand: \n");
            showAST(parser, ((UnaryOpNode*)node)->operand, depth + 2);
            printIndent(depth + 1);
            printf("Operator");
            if(((UnaryOpNode*)node)->isPostfix == true) {
                printf(" (postfix):");
            } else {
                printf(" (prefix):");
            }
            printf("\n");
            printIndent(depth + 2);
            printf("%s\n", fetchTokenStateString(((UnaryOpNode*)node)->base.mainToken.state));
            break;

        case AST_PIPE:
            printf("PIPELINE:\n");
            printIndent(depth + 1);
            printf("Elements (%d):\n", ((PipelineNode*)node)->elementsCount);
            for(int i = 0; i < ((PipelineNode*)node)->elementsCount; i++) {
                showAST(parser, ((PipelineNode*)node)->elements[i], depth + 2);
            }
            break;

        case AST_PROC:
            printf("CODEBLOCK:\n");
            printIndent(depth + 1);
            printf("Statements (%d):\n", ((ProcNode*)node)->statementCount);
            for(int i = 0; i < ((ProcNode*)node)->statementCount; i++) {
                showAST(parser, ((ProcNode*)node)->statements[i], depth + 2);
            }
            break;

        case AST_LIST:
            printf("LIST:\n");
            printIndent(depth + 1);
            printf("Elements (%d):\n", ((ListNode*)node)->elementCount);
            for(int i = 0; i < ((ListNode*)node)->elementCount; i++) {
                showAST(parser, ((ListNode*)node)->elements[i], depth + 2);
            }
            break;

        case AST_MEMBER_ACCESS:
        case AST_SCOPE_RESOLVE: {
            printf("%s:\n", node->type == AST_MEMBER_ACCESS ? "MEMBER ACCESS" : "SCOPE RESOLUTION");
            printIndent(depth + 1);
            printf("Target:\n");
            showAST(parser, ((MemberAccessNode*)node)->left, depth + 2);
            char buffer[64];
            extractTokenText(parser, ((MemberAccessNode*)node)->property, buffer, 64);
            printIndent(depth + 1);
            printf("Member: %s\n", buffer);
            break;
        }

        case AST_INDEX:
            printf("INDEX:\n");
            printIndent(depth + 1);
            printf("Target:\n");
            showAST(parser, ((IndexNode*)node)->left, depth + 2);
            printIndent(depth + 1);
            printf("Member:\n");
            showAST(parser, ((IndexNode *)node)->index, depth + 2);
            break;

        case AST_CAST:
            printf("CAST:\n");
            printIndent(depth + 1);
            printf("Target:\n");
            showAST(parser, ((CastNode*)node)->target, depth + 2);
            break;

        case AST_BUCKET:
            printf("BUCKET:\n");
            // Add any specific prints for buckets here if needed
            break;

        case AST_PROGRAM:
            printf("\n\033[1;34mShowing Generated AST:\033[0m\n");

            for(int i = 0; i < ((ProgramNode*)node)->statementCount; i++) {
                showAST(parser, ((ProgramNode*)node)->statements[i], 0);
            }

            printf("\n");
            break;
    }
}

int parse(File *file) {
    file->parser->elementBufferCount = 0;

    while(!isAtEnd(file->parser)){
        if (file->parser->elementBufferCount >= file->parser->elementBufferCapacity) {
            file->parser->elementBuffer = reallocate(file->parser->elementBuffer, (file->parser->elementBufferCapacity += BUFFER_SIZE) * sizeof(ASTNode*));
            file->parser->directionBuffer = reallocate(file->parser->directionBuffer, (file->parser->elementBufferCapacity) * sizeof(StateType));
        }

        ASTNode* node =  parsePrecedence(PREC_NONE, file->parser);
        if (node != NULL) {
            file->parser->elementBuffer[file->parser->elementBufferCount++] = node;
        }

        consume(TOKEN_SEMICOLON, -1, file->parser);
    }

    file->parser->AST->statements = file->parser->elementBuffer;

    file->parser->AST->statementCount = file->parser->elementBufferCount;

    return 0;
}
