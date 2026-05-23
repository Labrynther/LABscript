#include <string.h>
#include <ctype.h>

#include "share.h"
#include "keywordHash.h"

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

void changePosition(LexerState* lexer) {
    if(lexer->currentChar) {
        if (lexer->currentChar == '\n') {
            lexer->currentLine++;
            lexer->currentColumn = 1;
        } else {
            lexer->currentColumn++;
        }
        lexer->position++;
    }
    lexer->currentChar = charPeek(0, lexer);
}

Token generateToken(TokenType type, DataType state, TokenLocation startPos, LexerState *lexer) {
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
        .length = lexer->position - startPos.loc,
    };
    
    lexer->tokenArray[lexer->count] = lexeme;
    lexer->count++;

    return lexeme;
}

Token scanToken(LexerState* lexer) {
    continueScan:

    while (isspace((unsigned char)charPeek(0, lexer))) changePosition(lexer);

    lexer->currentChar = charPeek(0, lexer);

    TokenLocation startPos = {
        .line = lexer->currentLine,
        .column = lexer->currentColumn,
        .loc = lexer->position,
    };

    if(lexer->position >= lexer->fileSize) return generateToken(TOKEN_EOF, NONE, startPos, lexer);

    switch(lexer->currentChar) {
        case '"':
            changePosition(lexer);
            while (lexer->currentChar != '"') {
                if(lexer->position >= lexer->fileSize) {
                    throwError(lexer->filename, "Unterminated String\n", startPos.line, startPos.column, 1, ERROR_UNTERMINATED_STRING, ERROR);
                    return generateToken(TOKEN_LITERAL, LITERAL_STRING, startPos, lexer);
                }
                changePosition(lexer);
            }
            changePosition(lexer);
            return generateToken(TOKEN_LITERAL, LITERAL_STRING, startPos, lexer);

        case '/':
            // Handle Comments
            if(charPeek(1, lexer) == '/') {
                while(lexer->currentChar != '\n') {
                    changePosition(lexer);
                }
                goto continueScan;
            } else if (charPeek(1, lexer) == '*') {
                changePosition(lexer);
                changePosition(lexer);
                while(lexer->position < lexer->fileSize) {
                    if (lexer->currentChar == '*' && charPeek(1, lexer) == '/') {
                        changePosition(lexer);
                        changePosition(lexer);
                        goto continueScan;
                    }
                    changePosition(lexer);
                }

                throwError(lexer->filename, "Unterminated block comment", startPos.line, startPos.column, 2, ERROR_UNTERMINATED_BLOCK_COMMENT, ERROR);
                while(lexer->position < lexer->fileSize) changePosition(lexer);
                goto continueScan;
            }

            changePosition(lexer);
            return generateToken(TOKEN_OP, OP_DIVISION, startPos, lexer);

        case '\'':
            changePosition(lexer);
            while (lexer->currentChar != '\'') {
                if(lexer->position >= lexer->fileSize) {
                    throwError(lexer->filename, "Unterminated Character Literal\n", startPos.line, startPos.column, 1, ERROR_UNTERMINATED_CHARACTER_LITERAL, ERROR);
                    return generateToken(TOKEN_LITERAL, LITERAL_CHAR, startPos, lexer);
                }
                changePosition(lexer);
            }
            changePosition(lexer);
            return generateToken(TOKEN_LITERAL, LITERAL_CHAR, startPos, lexer);

        case '.':
            if(charPeek(1, lexer) == '.' && charPeek(2, lexer) == '.') { changePosition(lexer); changePosition(lexer); changePosition(lexer);
                return generateToken(TOKEN_ELLIPSIS, NONE, startPos, lexer); }

            changePosition(lexer);
            return generateToken(TOKEN_DOT, NONE, startPos, lexer);

        case '<':
            if(charPeek(1, lexer) == '-') {
                changePosition(lexer);
                changePosition(lexer);
                return generateToken(TOKEN_PIPE, LEFT, startPos, lexer);
            }
            changePosition(lexer);
            return generateToken(TOKEN_ANGLE, LEFT, startPos, lexer);
        
        case '-':
            if(charPeek(1, lexer) == '>') {
                changePosition(lexer);
                changePosition(lexer);
                return generateToken(TOKEN_PIPE, LEFT, startPos, lexer);
            }

            if(isdigit(charPeek(1, lexer))) {
                changePosition(lexer);
                goto lexNumber;
            }

            changePosition(lexer);
            return generateToken(TOKEN_OP, OP_MINUS, startPos, lexer);
        
        case '*': changePosition(lexer); return generateToken(TOKEN_OP, OP_MULTIPLICATION, startPos, lexer);
        case '+': changePosition(lexer); return generateToken(TOKEN_OP, OP_PLUS, startPos, lexer);
        case '=': changePosition(lexer); return generateToken(TOKEN_EQUAL, NONE, startPos, lexer);
        case ';': changePosition(lexer); return generateToken(TOKEN_SEMICOLON, NONE, startPos, lexer);
        case ':': changePosition(lexer); return generateToken(TOKEN_COLON, NONE, startPos, lexer);
        case ',': changePosition(lexer); return generateToken(TOKEN_COMMA, NONE, startPos, lexer);
        case '(': changePosition(lexer); return generateToken(TOKEN_PAREN, NONE, startPos, lexer);
        case ')': changePosition(lexer); return generateToken(TOKEN_PAREN, NONE, startPos, lexer);
        case '{': changePosition(lexer); return generateToken(TOKEN_BRACE, LEFT, startPos, lexer);
        case '}': changePosition(lexer); return generateToken(TOKEN_BRACE, RIGHT, startPos, lexer);
        case '[': changePosition(lexer); return generateToken(TOKEN_BRACKET, LEFT, startPos, lexer);
        case ']': changePosition(lexer); return generateToken(TOKEN_BRACKET, RIGHT, startPos, lexer);
        case '>': changePosition(lexer); return generateToken(TOKEN_ANGLE, RIGHT, startPos, lexer);
        case '%': changePosition(lexer); return generateToken(TOKEN_OP, OP_MODULO, startPos, lexer);
        case '?': changePosition(lexer); return generateToken(TOKEN_QMARK, NONE, startPos, lexer);
        case '&': changePosition(lexer); return generateToken(TOKEN_AMPERSAND, NONE, startPos, lexer);

        default:
            if (isalpha(lexer->currentChar) || lexer->currentChar == '_') {
                while(isalnum(lexer->currentChar) || lexer->currentChar == '_') {
                    changePosition(lexer);
                }

                const struct keyword *k = in_word_set(&lexer->file[startPos.loc], lexer->position - startPos.loc);

                if(k) return generateToken(TOKEN_IDENTIFIER, k->state, startPos, lexer);

                return generateToken(TOKEN_IDENTIFIER, NONE, startPos, lexer);

            } else if (isdigit((unsigned char)lexer->currentChar)) {
                lexNumber: ;
                bool seen_dot = false;

                while(lexer->currentChar != '\n') {
                    if (isdigit((unsigned char)lexer->currentChar)) {
                        changePosition(lexer);
                        continue;
                    }

                    if (lexer->currentChar == '.' && !seen_dot && isdigit((unsigned char)charPeek(1, lexer))) {
                        seen_dot = true;
                        changePosition(lexer); // consume '.'
                        continue;
                    }
                    break;
                }

                return generateToken(TOKEN_LITERAL, LITERAL_NUMBER, startPos, lexer);
            } else {
                char unknownChar = lexer->file[lexer->position];
                char buffer[64];
                snprintf(buffer, sizeof(buffer), "Character '%c' is not known or allowed\n", unknownChar);
                throwError(lexer->filename, buffer, startPos.line, startPos.column, 1, ERROR_UNKNOWN_CHARACTER, ERROR);
                changePosition(lexer);

                return generateToken(TOKEN_UNKNOWN, NONE, startPos, lexer);
            }
    }
}


void showLex(LexerState* lexer){
    const char *tokenTypeLabels[] = {
        "IDENTIFIER",
        "LITERAL",
        "TYPE",

        "PIPE",
        "DOT",
        "COMMA",
        "COLON",
        "SEMICOLON",

        "BRACE",
        "PAREN",
        "BRACKET",
        "ANGLE",

        "OP",

        "EQUAL",
        "QMARK",
        "AMPERSAND",
        "ELLIPSIS",

        "ARROW",

        "EOF",
        "UNKNOWN"
    };

    const char *dataTypeLabels[] = {
        "NONE",
        "OBJECT",
        "IMPORT",
        "RETURN",

        "INT",
        "CHAR",
        "FLOAT",
        "DOUBLE",
        "BOOL",
        "CONST",
        "POINTER",
        "PRIVATE",

        "NUMBER",
        "STRING",
        "CHAR",

        "PLUS",
        "MINUS",
        "MULTIPLICATION",
        "DIVISION",
        "MODULO",

        "LEFT",
        "RIGHT"
    };

    printf("\033[1;34mShowing File Contents:\n\n\033[0m");
    for(int i = 0; i < lexer->fileSize; i++){
        printf("\033[0m%c", lexer->file[i]);
    }
    printf("\033[0m");

    printf("\n\n\033[1;34mShowing Generated Tokens:\n");
    for(int i = 0; i < lexer->count; i++){
        printf("\n\033[0mType: %s\nINFO: %s\nLocation: %zu\nLength: %d\nLine: %u\nColumn: %u\n",
                tokenTypeLabels[lexer->tokenArray[i].type],
                dataTypeLabels[lexer->tokenArray[i].state],
                lexer->tokenArray[i].pos.loc,
                lexer->tokenArray[i].length,
                lexer->tokenArray[i].pos.line,
                lexer->tokenArray[i].pos.column);
    }
    printf("\033[0m");
}

int lex(File *file) {
    LexerState *lexer = file->lexer;

    while(1) {
        Token eofCheck = scanToken(lexer);
        if (eofCheck.type == TOKEN_EOF) break;
    }

    return 0;
}
