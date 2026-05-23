#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

#include "share.h"

#define ALLOCATIONS_INITAL_MAXIMUM 1024

uint allocationsMaximum = ALLOCATIONS_INITAL_MAXIMUM;
uint allocationsAmount = 0;

void** allocations;

File** files;

int filesCount;

ErrorState* errors = NULL;

pthread_mutex_t* errorMutex;
pthread_mutex_t* allocMutex;

void finish(uint filesAmount, int code) {
    for(int i = 0; i < allocationsAmount; i++) {
        free(allocations[i]);
    }
    
    for(int i = 0; i < filesAmount; i++) {
        munmap((void*)files[i]->file, files[i]->fileSize);
        close(files[i]->fd);
    }

    free(errorMutex);
    free(allocMutex);

    exit(code);
}

#define FNV_OFFSET 14695981039346656037ULL
#define FNV_PRIME 1099511628211ULL

uint64_t hash(const void* data, size_t length) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint64_t hashValue = FNV_OFFSET;

    for(size_t i = 0; i < length; i++) {
        hashValue ^= bytes[i];
        hashValue *= FNV_PRIME;
    }
    return hashValue;
}

typedef struct {
    void* key;
    uint32_t index;
    bool occupied;
    bool deleted;
} AllocSlot;

size_t allocTableCapacity = ALLOCATIONS_INITAL_MAXIMUM;
AllocSlot* allocTable;

uint32_t findAllocation(void* ptr) {
    uint64_t hashValue = hash(&ptr, sizeof(void*));
    uint32_t index = hashValue % allocTableCapacity;

    while (allocTable[index].occupied || allocTable[index].deleted) {
        if (allocTable[index].key == ptr && !allocTable[index].deleted) {
            return index;
        }
        index = (index + 1) % allocTableCapacity;
    }
    return index;
}

void rehashAllocationsTable() {
    size_t oldCapacity = allocTableCapacity;
    AllocSlot* oldTable = allocTable;

    allocTableCapacity *= 2;
    allocTable = calloc(allocTableCapacity, sizeof(AllocSlot));
    if (allocTable == NULL) {
        fprintf(stderr, "\033[91mOut of Memory: unable to rehash!\033[0m");
        finish(filesCount, 1);
    }

    for (size_t i = 0; i < oldCapacity; i++) {
        if (oldTable[i].occupied && !oldTable[i].deleted) {
            uint32_t newSlot = findAllocation(oldTable[i].key);
            allocTable[newSlot].key = oldTable[i].key;
            allocTable[newSlot].index = oldTable[i].index;
            allocTable[newSlot].occupied = true;
            allocTable[newSlot].deleted = false;
        }
    }
    free(oldTable);
}

void* allocate(size_t size) {
    pthread_mutex_lock(allocMutex);

    void* mem = malloc(size);
    if (mem == NULL) {
        fprintf(stderr, "\033[91mOut of Memory: unable to allocate!\0330m");
        finish(filesCount, 1);
    }

    if (allocationsAmount >= allocationsMaximum) {
        void* temp = realloc(allocations, allocationsMaximum += ALLOCATIONS_INITAL_MAXIMUM);
        if(temp == NULL) {
            fprintf(stderr, "\033[91mOut of Memory: unable to allocate!\0330m");
            finish(filesCount, 1);
        }
        allocations = temp;
        rehashAllocationsTable();
    }

    if (((float)allocationsAmount / allocationsMaximum) >= 0.75) rehashAllocationsTable();

    allocations[allocationsAmount] = mem;
    allocationsAmount++;

    uint32_t slot = findAllocation(mem);
    allocTable[slot].key = mem;
    allocTable[slot].index = allocationsAmount - 1;
    allocTable[slot].occupied = true;
    allocTable[slot].deleted = false;

    pthread_mutex_unlock(allocMutex);

    return mem;
}

 void* reallocate(void* ptr, size_t size) {
    pthread_mutex_lock(allocMutex);

    uint32_t slot = findAllocation(ptr);
    
    if (!allocTable[slot].occupied || allocTable[slot].deleted) {
        fprintf(stderr, "\033[91mAllocation not found. Unable to resize.\n\033[0m");
        return NULL;
    }

    uint32_t arrayIndex = allocTable[slot].index;
    
    void* temp = realloc(ptr, size);
    if(temp == NULL) {
        fprintf(stderr, "\033[91mOut of Memory: unable to allocate!\033[0m");
        finish(filesCount, 1);
    }

    allocations[arrayIndex] = temp;

    if (temp != ptr) {
        allocTable[slot].deleted = true;
        
        uint32_t newSlot = findAllocation(temp);
        allocTable[newSlot].key = temp;
        allocTable[newSlot].index = arrayIndex;
        allocTable[newSlot].occupied = true;
        allocTable[newSlot].deleted = false;
    }

    pthread_mutex_unlock(allocMutex);

    return temp;
}

void freeAllocation(void* ptr) {
    pthread_mutex_lock(allocMutex);

    uint32_t slot = findAllocation(ptr);
    
    if (allocTable[slot].occupied && !allocTable[slot].deleted && allocTable[slot].key == ptr) {
        uint32_t arrayIndex = allocTable[slot].index;
        
        free(ptr);
        allocations[arrayIndex] = NULL;
        
        allocTable[slot].deleted = true;
    }

    pthread_mutex_unlock(allocMutex);
}

ErrorState* initErrorReports(){
    ErrorState* initErrors = allocate(sizeof(ErrorState));

    initErrors->errorCapacity = 4;
    initErrors->errorCount = 0;
    initErrors->errorArray = allocate(sizeof(Error) * initErrors->errorCapacity);

    return initErrors;
}

void throwError(char* filename, char *context, size_t line, size_t column, size_t length, ErrorType errorType, ErrorClass errorClass) {
    pthread_mutex_lock(errorMutex);

    if(errors->errorCount == errors->errorCapacity) {
        errors->errorCapacity += 4;
        errors->errorArray = reallocate(errors->errorArray, errors->errorCapacity * sizeof(Error));
    }

    errors->errorArray[errors->errorCount].filename = filename;

    size_t contextLength = strlen(context);

    errors->errorArray[errors->errorCount].context = allocate(contextLength + 1);
    if (errors->errorArray[errors->errorCount].context == NULL) { fprintf(stderr, "\033Out of Memory"); exit(1);}
    memcpy(errors->errorArray[errors->errorCount].context, context, contextLength + 1);
    
    errors->errorArray[errors->errorCount].errorType = errorType;
    errors->errorArray[errors->errorCount].errorClass = errorClass;

    errors->errorArray[errors->errorCount].line = line;
    errors->errorArray[errors->errorCount].column = column;

    errors->errorArray[errors->errorCount].length = length;

    errors->errorCount++;

    pthread_mutex_unlock(errorMutex);
}

void displayErrors(){
    for(size_t i = 0; i < errors->errorCount; i++) {
        switch (errors->errorArray[i].errorClass) {
            case ERROR: printf("\n\033[0m\033[1;31m[ERROR] ");       break;
            case WARNING: printf("\n\033[0m\033[1;33m[WARNING] ");   break;
            case INFO: printf("\n\033[0m\033[1;34m[INFO] ");         break;
            }

            printf("%s\nIn file '%s' in line %zu column %zu:\n\n",
                   errors->errorArray[i].context,
                   errors->errorArray[i].filename,
                   errors->errorArray[i].line,
                   errors->errorArray[i].column);
    }
}

char* mmapInit(int fileDescriptor, size_t *fileSize){
    struct stat sb;
    if(fstat(fileDescriptor, &sb) == -1){fprintf(stderr, "Error finding file stats\n");
        exit(1);}
    *fileSize = sb.st_size;

    char* mappedFile = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fileDescriptor, 0);

    if (mappedFile == MAP_FAILED) {
        fprintf(stderr, "\033[31mError mapping file\0330m");
        close(fileDescriptor);
        finish(filesCount, 1);
    }

    madvise(mappedFile, sb.st_size, MADV_SEQUENTIAL);

    return mappedFile;
}

File* initFile(char* filename){
    File* newFile = allocate(sizeof(File));
    
    newFile->filename = filename;
    newFile->fd = open(filename, O_RDONLY);

    if (newFile->fd == -1) {
        fprintf(stderr, "\033[31mError opening file\n");
        free(newFile);
        exit(1);
    }

    newFile->file = mmapInit(newFile->fd, &newFile->fileSize);

    newFile->lexer = initLexer(newFile->filename, newFile->file, newFile->fileSize);
    newFile->parser = NULL;

    return newFile;
}

void* fileProcessor(void* file){
    lex((File*)file);

    ((File*)file)->parser = initParser(((File*)file)->lexer->tokenArray, ((File*)file)->lexer->count);
    // parse(((File*)file)->parser);

    return NULL;
}

int main(int argc, char *argv[]) {
    if(argc < 2){
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    filesCount = argc - 1;

    allocations = malloc(allocationsMaximum * sizeof(void*));
    if (allocations == NULL) {
        fprintf(stderr, "\033[91mOut of Memory: unable to allocate!\0330m");
        exit(1);
    }

    allocTable = calloc(allocTableCapacity, sizeof(AllocSlot));
    if (allocTable == NULL) {
        fprintf(stderr, "\033[91mOut of Memory: unable to allocate!\0330m");
        exit(1);
    }

    errorMutex = malloc(sizeof(pthread_mutex_t));
    if (errorMutex == NULL) {
        fprintf(stderr, "\033[91mOut of Memory: unable to allocate!\0330m");
        exit(1);
    }

    allocMutex = malloc(sizeof(pthread_mutex_t));
    if (allocMutex == NULL) {
        fprintf(stderr, "\033[91mOut of Memory: unable to allocate!\0330m");
        exit(1);
    }

    pthread_mutex_init(errorMutex, NULL);
    pthread_mutex_init(allocMutex, NULL);

    errors = initErrorReports();

    files = allocate(sizeof(File*) * (filesCount));

    pthread_t* threads = allocate(filesCount * sizeof(pthread_t));

    for(int i = 0; i < filesCount; i++) {
        File* currentFile = initFile(argv[i+1]);
        files[i] = currentFile;

        int status = pthread_create(&threads[i], NULL, fileProcessor, (void*)currentFile);
        if(status != 0) {
            printf("Error Spawning Thread");
            // TODO: remove all the threads and safely exit using finish()
        }
    }

    for(int i = 0; i < filesCount; i++) {
        pthread_join(threads[i],  NULL);
        showLex(files[i]->lexer);
    }

    displayErrors();

    return 0;
}
