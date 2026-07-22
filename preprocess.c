#include <stdbool.h>
#include <string.h>

#include "share.h"

bool tokensMatch(ParserState* parser, Token a, Token b) {
    if (a.type != b.type) return false;
    if (a.type == TOKEN_IDENTIFIER || a.type == TOKEN_LITERAL) {
        if (a.pos.length != b.pos.length) {
            return false;
        }

        if (strncmp(&parser->file[a.pos.loc], &parser->file[b.pos.loc], a.pos.length) != 0) {
            return false;
        }
    }

    return true;
}

// TODO: Revamp macroing that rebuilds tokenArray once per macro, instead of constantly using memmove

void splice(ParserState* parser, int startIndex, int patternCount, Token* replacement, int replacementCount) {
    int delta = replacementCount - patternCount;
    int newCount = parser->count + delta;

    if (delta > 0) {
        parser->tokenArray = reallocate(parser->tokenArray, newCount * sizeof(Token));
    }

    int tailCount = parser->count - (startIndex + patternCount);
    if (tailCount > 0) {
        memmove(&parser->tokenArray[startIndex + replacementCount], 
                &parser->tokenArray[startIndex + patternCount], 
                tailCount * sizeof(Token));
    }

    if (replacementCount > 0) {
        memcpy(&parser->tokenArray[startIndex], replacement, replacementCount * sizeof(Token));
    }

    if (delta < 0) {
        parser->tokenArray = reallocate(parser->tokenArray, newCount * sizeof(Token));
    }

    parser->count = newCount;

    if (startIndex < parser->current) {
        parser->current += delta;
    }
}

void macro(ParserState* parser, Token* pattern, int patternCount, Token* replacement, int replacementCount, int start) {
    if (patternCount <= 0) return;

    for (int i = start; i <= parser->count - patternCount; i++) {
        bool matched = true;

        for (int j = 0; j < patternCount; j++) {
            if (!tokensMatch(parser, pattern[j], parser->tokenArray[i + j])) {
                matched = false;
                break;
            }
        }

        if (matched) {
            splice(parser, i, patternCount, replacement, replacementCount);
            // Ensure i advances by at least 1 even if replacementCount is 0
            i += (replacementCount > 0) ? (replacementCount - 1) : 0; 
        }
    }
}
