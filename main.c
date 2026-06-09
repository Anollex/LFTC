#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ad.h"
#include "lexer.h"
#include "parser.h"
#include "utils.h"
#include "vm.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }
    char *buf = loadFile(argv[1]);
    Token *tokens = tokenize(buf);
    // showTokens(tokens);
    pushDomain();
    vmInit();
    parse(tokens);
    // showDomain(symTable, "global");
    Symbol *symMain = findSymbolInDomain(symTable, "main");
    if (!symMain) err("missing main function");
    Instr *entryCode = NULL;
    addInstr(&entryCode, OP_CALL)->arg.instr = symMain->fn.instr;
    addInstr(&entryCode, OP_HALT);
    run(entryCode);
    dropDomain();
    free(buf);
    return 0;
}