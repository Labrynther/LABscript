#include <stdio.h>
#include <stdlib.h>

#include "share.h"

void parse(){
    char fmtString[main_lexer->tokenArray[0].length-1];
    printf("%s", literalRead(main_lexer->tokenArray[0], fmtString));
}
