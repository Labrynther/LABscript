cmake_minimum_required(VERSION 4.4.0)
project(LABscript)

set(SOURCE_FILES
    src/main.c
    src/tokens.h
    src/share.h
    src/keywordHash.h
    src/lexer.c
    src/parser.c
    src/preprocess.c)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
