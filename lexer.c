#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "share.h"

LexerState* init_lexer(const char* fileContent, int fileSize) {
    LexerState* lexer = malloc(sizeof(LexerState));
    if (lexer == NULL) {
        fprintf(stderr, "\033[31mUnable to Initalize Lexer");
        exit(1);
    }

    lexer->file = fileContent;
    lexer->fileSize = fileSize;
    lexer->position = 0;
    lexer->currentLine = 1;
    lexer->currentColumn = 1;
    lexer->capacity = ORIG_TOKEN_CAPACITY;
    lexer->count = 0;

    lexer->tokenArray = malloc(sizeof(Token) * lexer->capacity);
    if (lexer->tokenArray == NULL) {
        fprintf(stderr, "\033[31mFailed to allocate token array\n");
        exit(1);
    }

    return lexer;
}

void changePosition(LexerState* lexer){
    if(lexer->position < lexer->fileSize){
        if (lexer->file[lexer->position] == '\n') {
            lexer->currentLine++;
            lexer->currentColumn = 1;
        } else {
            lexer->currentColumn++;
        }
        lexer->position++;
    }
}

Token generateToken(LexerState* lexer, TokenType type, TokenLocation startPos){
    if (lexer->count == lexer->capacity) {
        int newCapacity = lexer->capacity + ORIG_TOKEN_CAPACITY;
        Token *temp = realloc(lexer->tokenArray, newCapacity * sizeof(Token));

        if (temp == NULL) {
            fprintf(stderr, "Out of memory!\n");
            exit(1); 
        }

        lexer->tokenArray = temp;
        lexer->capacity = newCapacity;
    }

    Token lexeme;
    lexeme.type = type;
    lexeme.pos.loc = startPos.loc;
    lexeme.pos.line = startPos.line;
    lexeme.pos.column = startPos.column;
    lexeme.length = lexer->position - startPos.loc;

    lexer->tokenArray[lexer->count] = lexeme;
    lexer->count++;

    return lexeme;
}

Token scanToken(LexerState* lexer){
    continueScan:

    while (lexer->position < lexer->fileSize && isspace(lexer->file[lexer->position])){
        changePosition(lexer);
    }

    TokenLocation startPos;
    startPos.line = lexer->currentLine;
    startPos.column = lexer->currentColumn;
    startPos.loc = lexer->position;

    if(lexer->position >= lexer->fileSize){
        return generateToken(lexer, TOKEN_EOF, startPos);
    }

    switch(lexer->file[lexer->position]){
        case '"':
            changePosition(lexer);
            while (lexer->file[lexer->position] != '"'){
                if(lexer->position >= lexer->fileSize){
                    throwError("Unterminated String", startPos.line, startPos.column);
                }
                changePosition(lexer);
            }
            changePosition(lexer);
            return generateToken(lexer, TOKEN_STRING, startPos);

        case '\'':
            changePosition(lexer);
            while (lexer->file[lexer->position] != '\''){
                if(lexer->position >= lexer->fileSize){
                    throwError("Unterminated Character Literal", startPos.line, startPos.column);
                }
                changePosition(lexer);
            }
            changePosition(lexer);
            return generateToken(lexer, TOKEN_CHAR, startPos);

        case '/':
            // Handle Comments
            if(lexer->file[lexer->position+1] == '/') {
                while(lexer->file[lexer->position] != '\n' && lexer->position < lexer->fileSize){
                    changePosition(lexer);
                }
                goto continueScan;
            } else if (lexer->position + 1 < lexer->fileSize && lexer->file[lexer->position+1] == '*') {
                changePosition(lexer);
                changePosition(lexer);
                while(lexer->position < lexer->fileSize) {
                    if (lexer->position + 1 < lexer->fileSize && lexer->file[lexer->position] == '*' && lexer->file[lexer->position+1] == '/'){
                        changePosition(lexer);
                        changePosition(lexer);
                        goto continueScan;
                    }
                    changePosition(lexer);
                }

                throwError("Unterminated block comment", startPos.line, startPos.column);
            }

            changePosition(lexer);
            return generateToken(lexer, TOKEN_DIVIDE, startPos);

                // general symbol cases
                
        case ';': changePosition(lexer); return generateToken(lexer, TOKEN_SEMICOLON, startPos);
        case ':': changePosition(lexer); return generateToken(lexer, TOKEN_COLON, startPos);
        case ',': changePosition(lexer); return generateToken(lexer, TOKEN_COMMA, startPos);
        case '(': changePosition(lexer); return generateToken(lexer, TOKEN_LPAREN, startPos);
        case ')': changePosition(lexer); return generateToken(lexer, TOKEN_RPAREN, startPos);
        case '{': changePosition(lexer); return generateToken(lexer, TOKEN_LBRACE, startPos);
        case '}': changePosition(lexer); return generateToken(lexer, TOKEN_RBRACE, startPos);
        case '.': changePosition(lexer); return generateToken(lexer, TOKEN_PERIOD, startPos);
        case '*': changePosition(lexer); return generateToken(lexer, TOKEN_MULTIPLY, startPos);
        case '+': changePosition(lexer); return generateToken(lexer, TOKEN_PLUS, startPos);
        case '-': changePosition(lexer); return generateToken(lexer, TOKEN_MINUS, startPos);
        case '=': changePosition(lexer); return generateToken(lexer, TOKEN_EQUAL, startPos);
        case '[': changePosition(lexer); return generateToken(lexer, TOKEN_LBRACKET, startPos);
        case ']': changePosition(lexer); return generateToken(lexer, TOKEN_RBRACKET, startPos);
        case '>': changePosition(lexer); return generateToken(lexer, TOKEN_GREATER, startPos);
        case '<': changePosition(lexer); return generateToken(lexer, TOKEN_LESS, startPos);
        case '%': changePosition(lexer); return generateToken(lexer, TOKEN_PERCENT, startPos);
        case '?': changePosition(lexer); return generateToken(lexer, TOKEN_QMARK, startPos);
        case '&': changePosition(lexer); return generateToken(lexer, TOKEN_AMPERSAND, startPos);

        default:
            if (isalpha(lexer->file[lexer->position]) || lexer->file[lexer->position] == '_') {
                while(isalnum(lexer->file[lexer->position]) || lexer->file[lexer->position] == '_') {
                    changePosition(lexer);
                }
                return generateToken(lexer, TOKEN_IDENTIFIER, startPos);
            } else if (isdigit(lexer->file[lexer->position])){
                while(isdigit(lexer->file[lexer->position])) {
                    changePosition(lexer);
                }
                return generateToken(lexer, TOKEN_NUMBER, startPos);
            } else {
                char unknownChar = lexer->file[lexer->position];
                char unknownCharMessageBuffer[28];
                snprintf(unknownCharMessageBuffer, sizeof(unknownCharMessageBuffer), "Unknown Character called: %c", unknownChar);
                throwError(unknownCharMessageBuffer, startPos.line, startPos.column);
                return generateToken(lexer, TOKEN_UNKNOWN, startPos);
            }
    }
}

void showLex(LexerState* lexer){
    const char *tokenTypeLabels[] = {
        "IDENTIFIER",
        "STRING",
        "CHAR", 
        "LBRACE", 
        "RBRACE", 
        "LPAREN", 
        "RPAREN", 
        "LBRACKET", 
        "RBRACKET", 
        "SEMICOLON",
        "COLON", 
        "COMMA", 
        "PERIOD",
        "NUMBER",
        "PLUS", 
        "MINUS", 
        "MULTIPLY",
        "DIVIDE", 
        "EQUAL",
        "GREATER",
        "LESS",
        "PERCENT",
        "QMARK",
        "AMPERSAND",
        "EOF",
        "UNKNOWN"
    };
    
    printf("\033[1;34mShowing File Contents:\n\n");
    for(int i = 0; i < lexer->fileSize; i++){
        printf("\033[0m%c", lexer->file[i]);
    }
    printf("\n\n\033[1;34mShowing Generated Tokens:\n");
    for(int i = 0; i < lexer->count; i++){
        printf("\n\033[0mType: %s\nLocation: %d\nLength: %d\nLine: %d\nColumn: %d\n", tokenTypeLabels[lexer->tokenArray[i].type], lexer->tokenArray[i].pos.loc, lexer->tokenArray[i].length, lexer->tokenArray[i].pos.line, lexer->tokenArray[i].pos.column);
    }
}

int lex(LexerState* lexer){
    while(lexer->position < lexer->fileSize){
        Token eofCheck = scanToken(lexer);
        if (eofCheck.type == TOKEN_EOF) break;
    }

    showLex(lexer);
    return 0;
}
