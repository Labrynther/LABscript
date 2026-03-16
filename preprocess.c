#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "share.h"

bool tokensMatch(LexerState* lexer, Token a, Token b){
    if (a.type != b.type) return false;
    if(a.type == TOKEN_IDENTIFIER || a.type == TOKEN_NUMBER || a.type == TOKEN_STRING || a.type == TOKEN_CHAR) {
        if (a.length != b.length) {
            return false;
        }

        if (strncmp(&lexer->file[a.pos.loc], &lexer->file[b.pos.loc], a.length) != 0) return false;
    }

    return true;
}

void macro(LexerState* lexer, Token* input, int inputCount, Token* target, int targetCount, int start){
    for(int i = start; i < lexer->count - targetCount; i++){
        for(int j = 0; j < targetCount; j++){
            if(!tokensMatch(lexer, target[j], lexer->tokenArray[i+j])) break;
            if(j == targetCount - 1){
                Token* tokenRealloc = realloc(lexer->tokenArray, (inputCount - targetCount + lexer->count) * sizeof(Token));
                if(tokenRealloc == NULL) { fprintf(stderr, "\033[31mFailed to reallocated memory for Macro"); exit(1); }
                lexer->tokenArray = tokenRealloc;

                memmove(&lexer->tokenArray[i+inputCount], &lexer->tokenArray[i + targetCount], (lexer->count - (i + targetCount)) * sizeof(Token));
                memcpy(&lexer->tokenArray[i], input, inputCount * sizeof(Token));

                lexer->count = lexer->count - targetCount + inputCount;
                i += inputCount - 1; 
            }
        }
    }
}