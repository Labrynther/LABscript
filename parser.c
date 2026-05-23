#include "share.h"

#define NODE_PAGE_SIZE 512

NodePage* createNodePage(){
    NodePage* nodes = allocate(sizeof(NodePage));

    nodes->nodesArray = allocate(NODE_PAGE_SIZE);
    nodes->capacity = NODE_PAGE_SIZE;
    nodes->offset = 0;
    nodes->next = NULL;

    return nodes;
}

ParserState* initParser(Token* tokenArray, int count) {
    ParserState* initParse = allocate(sizeof(ParserState));

    initParse->tokenArray = tokenArray;
    initParse->count = count;
    initParse->current = 0;
    initParse->nodePages = allocate(sizeof(NodePages));
    initParse->nodePages->firstPage = createNodePage();
    initParse->nodePages->currentPage = initParse->nodePages->firstPage;
    initParse->hadError = false;

    return initParse;
}

void* allocNode(NodePages* nodePages, size_t size) {
    NodePage* nodes = nodePages->currentPage;
    size = (size + 7) & ~7;

    if((nodes->offset + size) >= nodes->capacity) {
        nodePages->currentPage = createNodePage();
        nodes->next = nodePages->currentPage;
        nodes = nodePages->currentPage;
    }

    void* ptr = &nodes->nodesArray[nodes->offset];
    nodes->offset += size;

    return ptr;
}

void* createObjDeclNode(Token mainToken);

typedef struct {
    ASTNode** data;
    size_t capacity;
    size_t count;

    size_t head;
    size_t tail;
} PipeBuffer;

Token inline tokenPeek(size_t n, ParserState* parser) {
    if ((parser->current + n) >= parser->count) {
        Token eofToken = {0};
        eofToken.type = TOKEN_EOF;
        return eofToken;
    }

    return parser->tokenArray[parser->current + n];
}

Token inline previous(ParserState* parser) {
    return parser->tokenArray[parser->current - 1];
}

bool inline isAtEnd(ParserState* parser) {
    return (tokenPeek(0, parser).type == TOKEN_EOF);
}

Token inline advance(ParserState* parser) {
    if (!isAtEnd(parser)) parser->current++;
    return previous(parser);
}

bool inline checkType(TokenType type, ParserState* parser) {
    if (isAtEnd(parser)) return false;
    return tokenPeek(0, parser).type == type;
}

bool inline match(TokenType type, ParserState* parser) {
    if (checkType(type, parser)) {
        advance(parser);
        return true;
    }
    
    return false;
}

int parse(ParserState* parser) {
    while(!isAtEnd(parser)){
        
    }

    return 0;
}
