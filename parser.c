#include <stdio.h>
#include <stdlib.h>

#include "share.h"

void parse(){
    printf("DEBUG: Made it inside parse()\n");
    char fmtString[main_lexer->tokenArray[0].length-1];
    printf("DEBUG: About to read string\n");
    printf("%s", literalRead(main_lexer->tokenArray[0], fmtString));
}
