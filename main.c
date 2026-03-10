#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "utils.h"



int main(int argc, char *argv[]){
    if(argc<2){
        printf("Usage: %s <input_file>\n",argv[0]);
        return 1;
    }
    char *buf=loadFile(argv[1]);
    Token *tokens=tokenize(buf);
    showTokens(tokens);
    free(buf);
    return 0;
}