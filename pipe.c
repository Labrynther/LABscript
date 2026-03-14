#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h> 

#include "share.h"

void throwError(char *message, int line, int column) {
    printf("\033[31m%s at line %d, column %d\033[0m\n", message, line, column);
    exit(1);
}

char* file = NULL;
struct stat sb;
int fd;
size_t fileSize = 0;

void tokenArrayInit() { tokenArray = malloc(ORIG_TOKEN_CAPACITY * sizeof(Token)); }

void mmapInit(){
    fstat(fd, &sb);
    fileSize = sb.st_size;

    file = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    if (file == MAP_FAILED) {
        perror("Error mapping file");
        close(fd);
        exit(1);
    }

    madvise(file, sb.st_size, MADV_SEQUENTIAL);
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
    tokenArrayInit();

    lex();

    munmap(file, sb.st_size); 
    close(fd);

    parse();
}
