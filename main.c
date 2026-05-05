#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ad.h"
#include "lexer.h"
#include "parser.h"
#include "utils.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("\a.Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    char *buf = loadFile(argv[1]);
    Token *tokens = tokenize(buf);
    //showTokens(tokens);
    pushDomain();               // creează domeniul global
    parse(tokens);
    showDomain(symTable, "global");  // afișează simbolurile
    dropDomain();
    free(buf);
    return 0;
}
