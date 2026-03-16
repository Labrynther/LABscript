#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "share.h"

void throwError(char *message, int line, int column) {
    fprintf(stderr, "\033[31m%s at line %d, column %d\033[0m\n", message, line, column);
    exit(1);
}

char* file = NULL;
struct stat sb;
int fd;
size_t fileSize = 0;

Token *tokenArray = NULL;
LexerState* main_lexer = NULL;

void mmapInit(){
    fstat(fd, &sb);
    fileSize = sb.st_size;

    file = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    if (file == MAP_FAILED) {
        fprintf(stderr, "\033[31mError mapping file");
        close(fd);
        exit(1);
    }

    madvise(file, sb.st_size, MADV_SEQUENTIAL);
}

char* literalRead(Token str, char* fmt){
    int inner_length = str.length - 2; // For Strings and Char, it calculates the length minus the outer characters.
    if(str.type == TOKEN_CHAR || str.type == TOKEN_STRING){
        memcpy(fmt, &file[str.pos.loc + 1], inner_length);
        fmt[inner_length] = '\0';
    } else {
        fmt[0] = '\0';
    }

    return fmt;
}

int main(int argc, char *argv[]){
    if(argc != 2){
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        exit(1);
    }

    printf("\n██╗      █████╗ ██████╗ ███████╗ ██████╗██████╗ ██╗██████╗ ████████╗\n██║     ██╔══██╗██╔══██╗██╔════╝██╔════╝██╔══██╗██║██╔══██╗╚══██╔══╝\n██║     ███████║██████╔╝███████╗██║     ██████╔╝██║██████╔╝   ██║   \n██║     ██╔══██║██╔══██╗╚════██║██║     ██╔══██╗██║██╔═══╝    ██║   \n███████╗██║  ██║██████╔╝███████║╚██████╗██║  ██║██║██║        ██║   \n╚══════╝╚═╝  ╚═╝╚═════╝ ╚══════╝ ╚═════╝╚═╝  ╚═╝╚═╝╚═╝        ╚═╝   \n\n");

    mmapInit();

    // Initalize the lexer and lex the main file
    main_lexer = init_lexer(file, fileSize);
    lex(main_lexer);
    showLex(main_lexer);

    parse();

    // last
    munmap(file, sb.st_size); 
    close(fd);

    free(main_lexer->tokenArray);
    free(main_lexer);

    return 0;
}
