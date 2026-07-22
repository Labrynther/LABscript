#include <ctype.h>

#include "share.h"
#include "keywordHash.h"
#include "tokens.h"

#define ORIG_TOKEN_CAPACITY 512

LexerState* initLexer(char* filename, char* fileContent, size_t fileSize) {
    LexerState* initLex = allocate(sizeof(LexerState));

    initLex->filename = filename;
    initLex->file = fileContent;
    initLex->fileSize = fileSize;
    initLex->position = 0;
    initLex->currentLine = 1;
    initLex->currentColumn = 1;
    initLex->capacity = ORIG_TOKEN_CAPACITY;
    initLex->count = 0;

    initLex->tokenArray = allocate(sizeof(Token) * initLex->capacity);

    return initLex;
}

char charPeek(size_t n, LexerState *lexer) {
    if(lexer->position + n < lexer->fileSize) 
        return lexer->file[lexer->position + n];

    return '\0';
}

bool changePosition(int n, LexerState* lexer) {
    for (int i = 0; i < n; i++) {
        if(lexer->position < lexer->fileSize) {
            if (lexer->currentChar == '\n') {
                lexer->currentLine++;
                lexer->currentColumn = 1;
            } else {
                lexer->currentColumn++;
            }
            lexer->position++;
        } else {
            return false;
        }
    }

    lexer->currentChar = charPeek(0, lexer);

    return true;
}

Token generateToken(TokenType type, StateType state, TokenLocation startPos, LexerState *lexer) {
    if (lexer->count == lexer->capacity) {
        int newCapacity = lexer->capacity + ORIG_TOKEN_CAPACITY;

        lexer->tokenArray = reallocate(lexer->tokenArray, newCapacity * sizeof(Token));
        lexer->capacity = newCapacity;
    }

    Token lexeme = {
        .type = type,
        .state = state,
        .pos.loc = startPos.loc,
        .pos.line = startPos.line,
        .pos.column = startPos.column,
        .pos.length = lexer->position - startPos.loc,
    };
    
    lexer->tokenArray[lexer->count] = lexeme;
    lexer->count++;

    return lexeme;
}

Token scanToken(LexerState* lexer) {
    continueScan:

    while (isspace((unsigned char)charPeek(0, lexer))) changePosition(1, lexer);

    lexer->currentChar = charPeek(0, lexer);

    TokenLocation startPos = {
        .line = lexer->currentLine,
        .column = lexer->currentColumn,
        .loc = lexer->position,
    };

    StateType opState = NONE;

    if(lexer->position >= lexer->fileSize) return generateToken(TOKEN_EOF, NONE, startPos, lexer);

    switch(lexer->currentChar) {
        case '"':
            changePosition(1, lexer);
            while (lexer->currentChar != '"') {
                if(lexer->position >= lexer->fileSize) {
                    throwError(lexer->filename, lexer->fileSize, lexer->file, startPos, ERROR_UNTERMINATED, 0, ERROR, "string");
                    return generateToken(TOKEN_LITERAL, LITERAL_STRING, startPos, lexer);
                }
                changePosition(1, lexer);
            }
            changePosition(1, lexer);
            return generateToken(TOKEN_LITERAL, LITERAL_STRING, startPos, lexer);

        case '\'':
            changePosition(1, lexer);
            while (lexer->currentChar != '\'') {
                if(lexer->position >= lexer->fileSize) {
                    throwError(lexer->filename, lexer->fileSize, lexer->file, startPos, ERROR_UNTERMINATED, 2, ERROR, "character literal");
                    return generateToken(TOKEN_LITERAL, LITERAL_CHAR, startPos, lexer);
                }
                changePosition(1, lexer);
            }
            changePosition(1, lexer);
            return generateToken(TOKEN_LITERAL, LITERAL_CHAR, startPos, lexer);

        case '.':
            if(charPeek(1, lexer) == '.' && charPeek(2, lexer) == '.') { 
                changePosition(3, lexer);
                return generateToken(TOKEN_ELLIPSIS, NONE, startPos, lexer); 
            }

            changePosition(1, lexer);
            return generateToken(TOKEN_DOT, NONE, startPos, lexer);

        case '<':
            if(charPeek(1, lexer) == '-') {
                changePosition(2, lexer);
                return generateToken(TOKEN_PIPE, LEFT, startPos, lexer);
            } 
            
            if (charPeek(1, lexer) == '=') {
                switch (charPeek(2, lexer)) {
                    case '+': opState = OP_PLUS;            break;
                    case '-': opState = OP_MINUS;           break;
                    case '*': opState = OP_MULTIPLICATION;  break;
                    case '/': opState = OP_DIVISION;        break;
                    case '^': opState = OP_EXPONENT;        break;
                    case '%': opState = OP_MODULO;          break;
                }

                if (opState != NONE) {
                    changePosition(3, lexer);
                    return generateToken(TOKEN_ASSIGN_OP_LEFT, opState, startPos, lexer);
                }

                changePosition(2, lexer);
                return generateToken(TOKEN_ASSIGN_PIPE, LEFT, startPos, lexer);
            }

            changePosition(1, lexer);
            return generateToken(TOKEN_ANGLE, LEFT, startPos, lexer);
        
        case '|':
            if(charPeek(1, lexer) == '|') {
                changePosition(2, lexer);
                return generateToken(TOKEN_BUCKET, NONE, startPos, lexer);
            }
            changePosition(1, lexer);
            return generateToken(TOKEN_PIPE, NONE, startPos, lexer);

        case '=': 
            if(charPeek(1, lexer) == '>') {
                changePosition(2, lexer);
                return generateToken(TOKEN_ASSIGN_PIPE, RIGHT, startPos, lexer);
            }

            if(charPeek(1, lexer) == '=') {
                changePosition(2, lexer);
                return generateToken(TOKEN_EQUAL, NONE, startPos, lexer);
            }

            changePosition(1, lexer);
            return generateToken(TOKEN_ASSIGN_PIPE, NONE, startPos, lexer);

        case '+': 
            if(charPeek(1, lexer) == '+') {
                changePosition(2, lexer);
                return generateToken(TOKEN_OP, OP_INCREMENT, startPos, lexer);
            }

            opState = OP_PLUS;
            goto checkAssignments;

        case '-':
            if(charPeek(1, lexer) == '>') {
                changePosition(2, lexer);
                return generateToken(TOKEN_PIPE, RIGHT, startPos, lexer);
            }

            if(charPeek(1, lexer) == '-') {
                changePosition(2, lexer);
                return generateToken(TOKEN_OP, OP_DECREMENT, startPos, lexer);
            }

            opState = OP_MINUS;
            goto checkAssignments;

        case '*':
            opState = OP_MULTIPLICATION;
            goto checkAssignments;

        case '/':
            // Handle Comments
            if(charPeek(1, lexer) == '/') {
                while(lexer->currentChar != '\n') {
                    changePosition(1, lexer);
                }
                goto continueScan;
            } else if (charPeek(1, lexer) == '*') {
                changePosition(2, lexer);
                do {
                    if (lexer->currentChar == '*' && charPeek(1, lexer) == '/') {
                        changePosition(2, lexer);
                        goto continueScan;
                    }
                } while (changePosition(1, lexer));

                startPos.length = 2;

                throwError(lexer->filename, lexer->fileSize, lexer->file, startPos, ERROR_UNTERMINATED, 1, ERROR, "block comment");
                while(lexer->position < lexer->fileSize) changePosition(1, lexer);
                goto continueScan;
            }

            opState = OP_DIVISION;
            goto checkAssignments;

        case '^':
            opState = OP_EXPONENT;
            goto checkAssignments;

        case '%':
            opState = OP_MODULO;
            goto checkAssignments;

        case ';': changePosition(1, lexer); return generateToken(TOKEN_SEMICOLON, NONE, startPos, lexer);
        case ':': changePosition(1, lexer); return generateToken(TOKEN_COLON, NONE, startPos, lexer);
        case ',': changePosition(1, lexer); return generateToken(TOKEN_COMMA, NONE, startPos, lexer);
        case '(': changePosition(1, lexer); return generateToken(TOKEN_PAREN, NONE, startPos, lexer);
        case ')': changePosition(1, lexer); return generateToken(TOKEN_PAREN, NONE, startPos, lexer);
        case '{': changePosition(1, lexer); return generateToken(TOKEN_BRACE, LEFT, startPos, lexer);
        case '}': changePosition(1, lexer); return generateToken(TOKEN_BRACE, RIGHT, startPos, lexer);
        case '[': changePosition(1, lexer); return generateToken(TOKEN_BRACKET, LEFT, startPos, lexer);
        case ']': changePosition(1, lexer); return generateToken(TOKEN_BRACKET, RIGHT, startPos, lexer);
        case '>': changePosition(1, lexer); return generateToken(TOKEN_ANGLE, RIGHT, startPos, lexer);
        case '?': changePosition(1, lexer); return generateToken(TOKEN_QMARK, NONE, startPos, lexer);
        case '&': changePosition(1, lexer); return generateToken(TOKEN_AMPERSAND, NONE, startPos, lexer);
        case '#': changePosition(1, lexer); return generateToken(TOKEN_OCTOTHROPE, NONE, startPos, lexer);
        case '$': changePosition(1, lexer); return generateToken(TOKEN_DOLLAR, NONE, startPos, lexer);

        default:
            if (isalpha(lexer->currentChar) || lexer->currentChar == '_') {
                while(isalnum(lexer->currentChar) || lexer->currentChar == '_') {
                    changePosition(1, lexer);
                }

                const struct keyword *k = in_word_set(&lexer->file[startPos.loc], lexer->position - startPos.loc);

                if(k) return generateToken(TOKEN_IDENTIFIER, k->state, startPos, lexer);

                return generateToken(TOKEN_IDENTIFIER, NONE, startPos, lexer);

            } else if (isdigit((unsigned char)lexer->currentChar)) {
                bool seen_dot = false;

                while(lexer->currentChar != '\n') {
                    if (isdigit((unsigned char)lexer->currentChar)) {
                        changePosition(1, lexer);
                        continue;
                    }

                    if (lexer->currentChar == '.' && !seen_dot && isdigit((unsigned char)charPeek(1, lexer))) {
                        seen_dot = true;
                        changePosition(1, lexer); // consume '.'
                        continue;
                    }
                    break;
                }

                return generateToken(TOKEN_LITERAL, LITERAL_NUMBER, startPos, lexer);
            } else {
                uint32_t info = 0;
                unsigned char unknownChar = (unsigned char)lexer->file[lexer->position];
                
                if (unknownChar > 127) info = 1;

                throwError(lexer->filename, lexer->fileSize, lexer->file, startPos, ERROR_UNKNOWN_CHARACTER, info, ERROR, lexer->file[lexer->position]);
                
                do {
                    changePosition(1, lexer);
                    unknownChar = (unsigned char)lexer->file[lexer->position];

                } while (unknownChar != 0 && (unknownChar & 0xC0) == 0x80);

                return generateToken(TOKEN_UNKNOWN, NONE, startPos, lexer);
            }
    }

    checkAssignments:
        if(charPeek(1, lexer) == '=' && charPeek(2, lexer) == '>') {
            changePosition(3, lexer);
            return generateToken(TOKEN_ASSIGN_OP_RIGHT, opState, startPos, lexer); 
        } else if (charPeek(1, lexer) == '=') {
            changePosition(2, lexer);
            return generateToken(TOKEN_ASSIGN_OP, opState, startPos, lexer);
        } else {
            changePosition(1, lexer); 
            return generateToken(TOKEN_OP, opState, startPos, lexer);
        }
}

void showLex(LexerState* lexer){
    printf("\033[92mFilename: %s\033[0m\n", lexer->filename);
    printf("\033[1;34mShowing File Contents:\n\n\033[0m");
    for(int i = 0; i < lexer->fileSize; i++){
        printf("\033[0m%c", lexer->file[i]);
    }
    printf("\033[0m");

    printf("\n\n\033[1;34mShowing Generated Tokens:\n");
    for(int i = 0; i < lexer->count; i++){
        printf("\n\033[0mType: %s\nINFO: %s\nLocation: %zu\nLength: %d\nLine: %u\nColumn: %u\n",
                fetchTokenTypeString(lexer->tokenArray[i].type),
                fetchTokenStateString(lexer->tokenArray[i].state),
                lexer->tokenArray[i].pos.loc,
                lexer->tokenArray[i].pos.length,
                lexer->tokenArray[i].pos.line,
                lexer->tokenArray[i].pos.column);
    }
    printf("\033[0m");
}

int lex(File *file) {
    LexerState *lexer = file->lexer;

    while(1) {
        if (criticalFailure) return 1;
        Token eofCheck = scanToken(lexer);
        if (eofCheck.type == TOKEN_EOF) break;
    }

    return 0;
}
