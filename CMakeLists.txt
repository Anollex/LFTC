cmake_minimum_required(VERSION 3.20)
project(LTFC C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(LTFC
        main.c
        lexer.c
        lexer.h
        utils.c
        utils.h)