#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <stdarg.h>

#include "share.h"

#define ALLOCATIONS_INITAL_MAXIMUM 1024

uint allocationsMaximum = ALLOCATIONS_INITAL_MAXIMUM;
uint allocationsAmount = 0;

void** allocations;

File** files;

pthread_t* threads;

pthread_t mainThread;

atomic_bool criticalFailure = false;

int filesCount;

ErrorState* errors = NULL;

pthread_mutex_t errorMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t allocMutex = PTHREAD_MUTEX_INITIALIZER;

short threadCount = 0;

typedef struct {
    void* key;
    uint32_t index;
    bool occupied;
    bool deleted;
} AllocSlot;

size_t allocTableCapacity = 0;
AllocSlot* allocTable;
size_t deletedCount = 0;

void finish(uint filesAmount, int code) {
    pthread_mutex_destroy(&errorMutex);
    pthread_mutex_destroy(&allocMutex);
    
    for(int i = 0; i < filesAmount; i++) {
        munmap((void*)files[i]->file, files[i]->fileSize);
        close(files[i]->fd);
    }

    for(int i = 0; i < allocationsAmount; i++) {
        free(allocations[i]);
    }

    free(allocations);
    free(allocTable);

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

uint32_t findAllocation(void* ptr) {
    if (allocTableCapacity == 0) return 0;

    uint64_t hashValue = hash(&ptr, sizeof(void*));
    uint32_t index = hashValue % allocTableCapacity;
    int firstDeletedSlot = -1;
    uint32_t probes = 0;

    while ((allocTable[index].occupied || allocTable[index].deleted) && probes < allocTableCapacity) {
        if (allocTable[index].key == ptr && !allocTable[index].deleted) {
            return index;
        }

        if (allocTable[index].deleted && firstDeletedSlot == -1) {
            firstDeletedSlot = index;
        }

        index = (index + 1) % allocTableCapacity;
        probes++;
    }
    
    if (firstDeletedSlot != -1) {
        return (uint32_t)firstDeletedSlot;
    }

    return index;
}

void rehashAllocationsTable() {
    size_t oldCapacity = allocTableCapacity;
    AllocSlot* oldTable = allocTable;
    allocTableCapacity = (allocTableCapacity == 0) ? ALLOCATIONS_INITAL_MAXIMUM : allocTableCapacity * 2;

    allocTable = calloc(allocTableCapacity, sizeof(AllocSlot));
    if (allocTable == NULL) {
        fprintf(stderr, "\033[91mOut of Memory: unable to rehash!\033[0m\n");
        finish(filesCount, 1);
    }

    deletedCount = 0;

    for (size_t i = 0; i < oldCapacity; i++) {
        if (oldTable[i].occupied && !oldTable[i].deleted) {
            uint32_t newSlot = findAllocation(oldTable[i].key);
            allocTable[newSlot].key = oldTable[i].key;
            allocTable[newSlot].index = oldTable[i].index;
            allocTable[newSlot].occupied = true;
            allocTable[newSlot].deleted = false;
        }
    }

    if (oldTable != NULL) {
        free(oldTable);
    }
}

void* allocate(size_t size) {
    pthread_mutex_lock(&allocMutex);

    if(criticalFailure) {
        pthread_mutex_unlock(&allocMutex);
        pthread_exit(NULL);
    }

    if (allocations == NULL) {
        allocations = malloc(allocationsMaximum * sizeof(void*));
    }

    void* mem = malloc(size);
    if (mem == NULL) {
        fprintf(stderr, "\033[91mOut of Memory: unable to allocate!\0330m");

        criticalFailure = true;
        pthread_mutex_unlock(&allocMutex);
        pthread_exit(NULL);
    }

    if (allocationsAmount >= allocationsMaximum) {
        allocationsMaximum += ALLOCATIONS_INITAL_MAXIMUM;
        void* temp = realloc(allocations, allocationsMaximum * sizeof(void*));
        if(temp == NULL) {
            fprintf(stderr, "\033[91mOut of Memory: unable to allocate!\0330m");
            finish(filesCount, 1);
        }
        allocations = temp;
    }

    if (allocTableCapacity == 0 || ((float)(allocationsAmount + deletedCount) / allocTableCapacity) >= 0.75) rehashAllocationsTable();

    allocations[allocationsAmount] = mem;
    allocationsAmount++;

    uint32_t slot = findAllocation(mem);
    allocTable[slot].key = mem;
    allocTable[slot].index = allocationsAmount - 1;
    allocTable[slot].occupied = true;
    allocTable[slot].deleted = false;

    pthread_mutex_unlock(&allocMutex);

    return mem;
}

void* reallocate(void* ptr, size_t size) {
    pthread_mutex_lock(&allocMutex);

    if(criticalFailure) {
        pthread_mutex_unlock(&allocMutex);
        if(pthread_self() != mainThread) pthread_exit(NULL);

        exit(1);
    }

    uint32_t slot = findAllocation(ptr);
    
    if (!allocTable[slot].occupied || allocTable[slot].deleted) {
        fprintf(stderr, "\033[91mAllocation not found. Unable to resize.\n\033[0m");
        pthread_mutex_unlock(&allocMutex);
        return NULL;
    }

    uint32_t arrayIndex = allocTable[slot].index;
    
    void* temp = realloc(ptr, size);
    if(temp == NULL) {
        fprintf(stderr, "\033[91mOut of Memory: unable to allocate!\033[0m");
        
        criticalFailure = true;
        pthread_mutex_unlock(&allocMutex);
        pthread_exit(NULL);
    }

    allocations[arrayIndex] = temp;

    if (temp != ptr) {
        allocTable[slot].deleted = true;
        deletedCount++;
        
        uint32_t newSlot = findAllocation(temp);
        allocTable[newSlot].key = temp;
        allocTable[newSlot].index = arrayIndex;
        allocTable[newSlot].occupied = true;
        allocTable[newSlot].deleted = false;
    }

    pthread_mutex_unlock(&allocMutex);

    return temp;
}

void* freeAllocation(void* ptr) {
    if (ptr == NULL) return NULL;

    pthread_mutex_lock(&allocMutex);

    uint32_t slot = findAllocation(ptr);
    
    if (allocTable[slot].occupied && !allocTable[slot].deleted && allocTable[slot].key == ptr) {
        uint32_t arrayIndex = allocTable[slot].index;
        
        free(ptr);

        allocationsAmount--;

        if (arrayIndex != allocationsAmount) {
            void* lastMem = allocations[allocationsAmount];
            allocations[arrayIndex] = lastMem;

            uint32_t movedSlot = findAllocation(lastMem);
            if (allocTable[movedSlot].occupied && !allocTable[movedSlot].deleted) {
                allocTable[movedSlot].index = arrayIndex;
            }
        }

        allocTable[slot].deleted = true;
        deletedCount++;
    }

    pthread_mutex_unlock(&allocMutex);
    return NULL;
}

char *tokenTypeLabels[] = {
    [TOKEN_IDENTIFIER] = "IDENTIFIER",
    [TOKEN_LITERAL] = "LITERAL",
    [TOKEN_TYPE] = "TYPE",

    [TOKEN_PIPE] = "PIPE",
    [TOKEN_ASSIGN_PIPE] = "ASSIGN_PIPE",
    [TOKEN_ASSIGN_OP] = "ASSIGN_OP (neutral)",
    [TOKEN_ASSIGN_OP_LEFT] = "ASSIGN_OP (left)",
    [TOKEN_ASSIGN_OP_RIGHT] = "ASSIGN_OP (right)",
    [TOKEN_DOT] = "DOT",
    [TOKEN_COMMA] = "COMMA",
    [TOKEN_COLON] = "COLON",
    [TOKEN_SEMICOLON] = "SEMICOLON",
    [TOKEN_BUCKET] = "BUCKET",
    [TOKEN_DOLLAR] = "DOLLAR",

    [TOKEN_BRACE] = "BRACE",
    [TOKEN_PAREN] = "PAREN",
    [TOKEN_BRACKET] = "BRACKET",
    [TOKEN_ANGLE] = "ANGLE",

    [TOKEN_OP] = "OP",

    [TOKEN_EQUAL] = "EQUAL",
    [TOKEN_QMARK] = "QMARK",
    [TOKEN_OCTOTHROPE] = "OCTOTHROPE",
    [TOKEN_AMPERSAND] = "AMPERSAND",
    [TOKEN_ELLIPSIS] = "ELLIPSIS",
    [TOKEN_SCOPE_RESOLVE] = "SCOPE_RESOLUTION",

    [TOKEN_EOF] = "EOF",
    [TOKEN_UNKNOWN] = "UNKNOWN"
};

char *stateTypeLabels[] = {
    [NONE] = "NONE",
    
    [IDENTIFIER_OBJ] = "OBJ",
    [IDENTIFIER_STRUCT] = "STRUCT",
    [IDENTIFIER_ENUM] = "ENUM",
    [IDENTIFIER_NAMESPACE] = "NAMESPACE",
    [IDENTIFIER_RETURN] = "RETURN",
    [IDENTIFIER_MACRO] = "MACRO",
    [IDENTIFIER_NOT] = "NOT",
    [IDENTIFIER_OR] = "OR",
    [IDENTIFIER_XOR] = "XOR",
    [IDENTIFIER_AND] = "AND",
    [IDENTIFIER_BITWISE] = "BITWISE",
    [IDENTIFIER_REFERENCE] = "REFERENCE",
    [IDENTIFIER_DEFERENCE] = "DEFERENCE",
    [IDENTIFIER_PRAGMA] = "PRAGMA",

    [PRAGMA_TARGET] = "TARGET",
    [PRAGMA_OPTIMIZE] = "OPTIMIZE",
    [PRAGMA_DIAGNOSTIC] = "DIAGNOSTIC",
    [PRAGMA_QUIET] = "QUIET",
    [PRAGMA_STRICT] = "STRICT",

    [TYPE_INT] = "INT",
    [TYPE_CHAR] = "CHAR",
    [TYPE_FLOAT] = "FLOAT",
    [TYPE_DOUBLE] = "DOUBLE",
    [TYPE_BOOL] = "BOOL",
    [TYPE_CONST] = "CONST",
    [TYPE_POINTER] = "POINTER",
    [TYPE_PRIVATE] = "PRIVATE",

    [LITERAL_NUMBER] = "NUMBER",
    [LITERAL_STRING] = "STRING",
    [LITERAL_CHAR] = "CHAR",

    [OP_PLUS] = "PLUS",
    [OP_MINUS] = "MINUS",
    [OP_MULTIPLICATION] = "MULTIPLICATION",
    [OP_DIVISION] = "DIVISION",
    [OP_EXPONENT] = "EXPONENT",
    [OP_MODULO] = "MODULO",
    [OP_INCREMENT] = "INCREMENT",
    [OP_DECREMENT] = "DECREMENT",

    [LEFT] = "LEFT",
    [RIGHT] = "RIGHT"
};

const char* fetchTokenTypeString(TokenType type) {
    if (type >= 0 && type <= TOKEN_UNKNOWN) {
        return tokenTypeLabels[type];
    }
    return "UNKNOWN_TOKEN";
}

const char* fetchTokenStateString(StateType type) {
    if (type >= 0 && type <= RIGHT) {
        return stateTypeLabels[type];
    }
    return "UNKNOWN_STATE";
}

ErrorState* initErrorReports(){
    ErrorState* initErrors = allocate(sizeof(ErrorState));

    initErrors->errorCapacity = 4;
    initErrors->errorCount = 0;
    initErrors->errorArray = allocate(sizeof(Error) * initErrors->errorCapacity);

    return initErrors;
}

const char* errorTemplates[] = {
    [ERROR_UNTERMINATED] = "Unterminated %s.",
    [ERROR_UNKNOWN_CHARACTER] = "\'%c\' is an unknown character and is not allowed.",
    [ERROR_EXPECTED] = "Expected %s, but received %s.",
    [ERROR_EMPTY] = "Empty %s. It should not be empty."
};

void throwError(char* filename, int fileSize, const char* file, TokenLocation location, ErrorType errorCode, int info, ErrorClass errorClass, ...) {
    pthread_mutex_lock(&errorMutex);

    if(errors->errorCount == errors->errorCapacity) {
        errors->errorCapacity += 4;
        errors->errorArray = reallocate(errors->errorArray, errors->errorCapacity * sizeof(Error));
    }

    errors->errorArray[errors->errorCount].filename = filename;

    errors->errorArray[errors->errorCount].file = file;
    errors->errorArray[errors->errorCount].fileSize = fileSize;

    const char* formatTemplate = errorTemplates[errorCode];

    va_list args;
    va_start(args, errorClass);

    char temp[512];

    vsnprintf(temp, sizeof(temp), formatTemplate, args);

    va_end(args);

    size_t contextLength = strlen(temp);

    errors->errorArray[errors->errorCount].context = allocate(contextLength + 1);
    memcpy(errors->errorArray[errors->errorCount].context, temp, contextLength + 1);
    
    errors->errorArray[errors->errorCount].errorType = errorCode;
    errors->errorArray[errors->errorCount].info = info;
    errors->errorArray[errors->errorCount].errorClass = errorClass;

    errors->errorArray[errors->errorCount].location = location;

    errors->errorCount++;

    pthread_mutex_unlock(&errorMutex);
}

void displayErrors(){
    for(size_t i = 0; i < errors->errorCount; i++) {
        switch (errors->errorArray[i].errorClass) {
            case ERROR: printf("\033[1;31m[ERROR] ");       break;
            case WARNING: printf("\033[1;33m[WARNING] ");   break;
            case INFO: printf("\033[1;34m[INFO] ");         break;
            }

            printf("%s\nIn file '%s' in line %d column %d:\n\n",
                   errors->errorArray[i].context,
                   errors->errorArray[i].filename,
                   errors->errorArray[i].location.line,
                   errors->errorArray[i].location.column);

            const char* file = errors->errorArray[i].file;
            int loc = errors->errorArray[i].location.loc;
            int col = errors->errorArray[i].location.column;
            int len = errors->errorArray[i].location.length;

            int fileSize = errors->errorArray[i].fileSize;

            int start = loc;
            while (start > 0 && file[start - 1] != '\n') start--;

            int end = loc;
            while (end < fileSize && file[end] != '\n') end++;

            printf("%.*s\n", end - start, &errors->errorArray[i].file[start]);

            for (int j = start; j < loc; j++) {
                if (file[j] == '\t') {
                    printf("\t");
                } else {
                    printf(" ");
                }
            }

            for(int j = 0; j < len && file[loc + j] != '\n'; j++) printf("^");

            printf("\033[0m\n\n");
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
        freeAllocation(newFile);
        exit(1);
    }

    newFile->file = mmapInit(newFile->fd, &newFile->fileSize);

    newFile->lexer = initLexer(newFile->filename, newFile->file, newFile->fileSize);
    newFile->parser = NULL;

    return newFile;
}

void* fileProcessor(void* file){
    lex((File*)file);

    ((File*)file)->parser = initParser(((File*)file)->filename, ((File*)file)->fileSize, ((File*)file)->file,((File*)file)->lexer->tokenArray, ((File*)file)->lexer->count);
    parse((File*)file);

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

    mainThread = pthread_self();

    errors = initErrorReports();

    files = allocate(sizeof(File*) * (filesCount));

    threads = allocate(filesCount * sizeof(pthread_t));


    for(int i = 0; i < filesCount; i++) {
        File* currentFile = initFile(argv[i+1]);
        files[i] = currentFile;

        int status = pthread_create(&threads[i], NULL, fileProcessor, (void*)currentFile);
        if(status != 0) {
            printf("Error Spawning Thread");
            finish(filesCount, 1);
        }
        threadCount++;
    }

    for(int i = 0; i < threadCount; i++) {
        pthread_join(threads[i], NULL);
    }

    if(criticalFailure) {
        finish(filesCount, 1);
    }

    for(int i = 0; i < filesCount; i++) {
        showLex(files[i]->lexer);
        showAST(files[i]->parser, (ASTNode*)(files[i]->parser->AST), 0);
    }

    displayErrors();
    finish(filesCount, 0);
}
