#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "share.h"

Token *tokenArray = NULL;
int count = 0;
int capacity = ORIG_TOKEN_CAPACITY;

int position = 0;
int storedPosition = 0;
int currentLine = 1;
int currentColumn = 1;

void changePosition(){
    if(position < fileSize){
        if (file[position] == '\n') {
            currentLine++;
            currentColumn = 1;
        } else {
            currentColumn++;
        }
        position++;
    }
}

Token generateToken(TokenType type, TokenLocation startPos){
    if (count == capacity) {
        int newCapacity = capacity + ORIG_TOKEN_CAPACITY;
        Token *temp = realloc(tokenArray, newCapacity * sizeof(Token));

        if (temp == NULL) {
            fprintf(stderr, "Out of memory!\n");
            exit(1); 
        }

        tokenArray = temp;
        capacity = newCapacity;
    }

    Token lexeme;
    lexeme.type = type;
    lexeme.pos.loc = startPos.loc;
    lexeme.pos.line = startPos.line;
    lexeme.pos.column = startPos.column;
    lexeme.length = position - startPos.loc;

    tokenArray[count] = lexeme;
    count++;

    return lexeme;
}

Token scanToken(){
    continueScan:

    while (position < fileSize && isspace(file[position])){
        changePosition();
    }

    TokenLocation startPos;
    startPos.line = currentLine;
    startPos.column = currentColumn;
    startPos.loc = position;

    if(position >= fileSize){
        return generateToken(TOKEN_EOF, startPos);
    }

    switch(file[position]){
        case '"':
            changePosition();
            while (file[position] != '"'){
                if(position >= fileSize){
                    throwError("Unterminated String", startPos.line, startPos.column);
                }
                changePosition();
            }
            changePosition();
            return generateToken(TOKEN_STRING, startPos);

        case '\'':
            changePosition();
            while (file[position] != '\''){
                if(position >= fileSize){
                    throwError("Unterminated Character Literal", startPos.line, startPos.column);
                }
                changePosition();
            }
            changePosition();
            return generateToken(TOKEN_CHAR, startPos);

        case '/':
            // Handle Comments
            if(file[position+1] == '/') {
                while(file[position] != '\n' && position < fileSize){
                    changePosition();
                }
                goto continueScan;
            } else if (position + 1 < fileSize && file[position+1] == '*') {
                changePosition();
                changePosition();
                while(position < fileSize) {
                    if (position + 1 < fileSize && file[position] == '*' && file[position+1] == '/'){
                        changePosition();
                        changePosition();
                        goto continueScan;
                    }
                    changePosition();
                }

                throwError("Unterminated block comment", startPos.line, startPos.column);
            }

            changePosition();
            return generateToken(TOKEN_DIVIDE, startPos);

                // general symbol cases
                
        case ';': changePosition(); return generateToken(TOKEN_SEMICOLON, startPos);
        case ':': changePosition(); return generateToken(TOKEN_COLON, startPos);
        case ',': changePosition(); return generateToken(TOKEN_COMMA, startPos);
        case '(': changePosition(); return generateToken(TOKEN_LPAREN, startPos);
        case ')': changePosition(); return generateToken(TOKEN_RPAREN, startPos);
        case '{': changePosition(); return generateToken(TOKEN_LBRACE, startPos);
        case '}': changePosition(); return generateToken(TOKEN_RBRACE, startPos);
        case '.': changePosition(); return generateToken(TOKEN_PERIOD, startPos);
        case '*': changePosition(); return generateToken(TOKEN_MULTIPLY, startPos);
        case '+': changePosition(); return generateToken(TOKEN_PLUS, startPos);
        case '-': changePosition(); return generateToken(TOKEN_MINUS, startPos);
        case '=': changePosition(); return generateToken(TOKEN_EQUAL, startPos);
        case '[': changePosition(); return generateToken(TOKEN_LBRACKET, startPos);
        case ']': changePosition(); return generateToken(TOKEN_RBRACKET, startPos);
        case '>': changePosition(); return generateToken(TOKEN_GREATER, startPos);
        case '<': changePosition(); return generateToken(TOKEN_LESS, startPos);
        case '%': changePosition(); return generateToken(TOKEN_PERCENT, startPos);
        case '?': changePosition(); return generateToken(TOKEN_QMARK, startPos);
        case '&': changePosition(); return generateToken(TOKEN_AMPERSAND, startPos);

        default:
            if (isalpha(file[position]) || file[position] == '_') {
                while(isalnum(file[position]) || file[position] == '_') {
                    changePosition();
                }
                return generateToken(TOKEN_IDENTIFIER, startPos);
            } else if (isdigit(file[position])){
                while(isdigit(file[position])) {
                    changePosition();
                }
                return generateToken(TOKEN_NUMBER, startPos);
            } else {
                char unknownChar = file[position];
                        char unknownCharMessageBuffer[28];
                snprintf(unknownCharMessageBuffer, sizeof(unknownCharMessageBuffer), "Unknown Character called: %c", unknownChar);
                throwError(unknownCharMessageBuffer, startPos.line, startPos.column);
                return generateToken(TOKEN_UNKNOWN, startPos);
            }
    }
}

void showLex(){
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
    for(int i = 0; i < fileSize; i++){
        printf("\033[0m%c", file[i]);
    }
    printf("\n\n\033[1;34mShowing Generated Tokens:\n");
    for(int i = 0; i < count; i++){
        printf("\n\033[0mType: %s\nLocation: %d\nLength: %d\nLine: %d\nColumn: %d\n", tokenTypeLabels[tokenArray[i].type], tokenArray[i].pos.loc, tokenArray[i].length, tokenArray[i].pos.line, tokenArray[i].pos.column);
    }
}

int lex(){
    while(position < fileSize){
        Token eofCheck = scanToken();
        if (eofCheck.type == TOKEN_EOF) break;
    }

    showLex();
    return 0;
}
