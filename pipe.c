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

char* mmapInit(int fileDescriptor, size_t *fileSize){
    struct stat sb;
    if(fstat(fileDescriptor, &sb) == -1){fprintf(stderr, "Error finding file stats\n");
        exit(1);}
    *fileSize = sb.st_size;

    char* mappedFile = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fileDescriptor, 0);

    if (mappedFile == MAP_FAILED) {
        fprintf(stderr, "\033[31mError mapping file");
        close(fileDescriptor);
        exit(1);
    }

    madvise(mappedFile, sb.st_size, MADV_SEQUENTIAL);

    return mappedFile;
}

char* literalRead(Token str, const char* sourceText, char* fmt){
    int inner_length = str.length - 2; // For Strings and Char, it calculates the length minus the outer characters.
    if(str.type == TOKEN_CHAR || str.type == TOKEN_STRING){
        memcpy(fmt, &sourceText[str.pos.loc + 1], inner_length);
        fmt[inner_length] = '\0';
    } else {
        fmt[0] = '\0';
    }

    return fmt;
}

File* initFile(char* filename){
    File* newFile = malloc(sizeof(File)); 

    if (newFile == NULL) {
        fprintf(stderr, "Memory allocation failed for File struct\n");
        exit(1);
    }
    
    newFile->filename = filename;
    newFile->fd = open(filename, O_RDONLY);

    if (newFile->fd == -1) {
        fprintf(stderr, "Error opening file\n");
        free(newFile);
        exit(1);
    }

    newFile->file = mmapInit(newFile->fd, &newFile->fileSize);
    newFile->lexer = init_lexer(newFile->file, newFile->fileSize);

    return newFile;
}

int main(int argc, char *argv[]){
    if(argc != 2){
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    printf("\n██╗      █████╗ ██████╗ ███████╗ ██████╗██████╗ ██╗██████╗ ████████╗\n██║     ██╔══██╗██╔══██╗██╔════╝██╔════╝██╔══██╗██║██╔══██╗╚══██╔══╝\n██║     ███████║██████╔╝███████╗██║     ██████╔╝██║██████╔╝   ██║   \n██║     ██╔══██║██╔══██╗╚════██║██║     ██╔══██╗██║██╔═══╝    ██║   \n███████╗██║  ██║██████╔╝███████║╚██████╗██║  ██║██║██║        ██║   \n╚══════╝╚═╝  ╚═╝╚═════╝ ╚══════╝ ╚═════╝╚═╝  ╚═╝╚═╝╚═╝        ╚═╝   \n\n");


    // Initalize the lexer and lex the main file
    File* currentFile = initFile(argv[1]);
    lex(currentFile->lexer);
    showLex(currentFile->lexer);

    parse();

    // last
    munmap((void*)currentFile->file, currentFile->fileSize); 
    close(currentFile->fd);

    free(currentFile->lexer->tokenArray);
    free(currentFile->lexer);
    free(currentFile);

    return 0;
}
