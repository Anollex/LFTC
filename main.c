#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "utils.h"

const char *tkCodeName(int code){
    switch(code){
        case ID: return "ID";
        case TYPE_CHAR: return "TYPE_CHAR";
        case TYPE_DOUBLE: return "TYPE_DOUBLE";
        case ELSE: return "ELSE";
        case IF: return "IF";
        case TYPE_INT: return "TYPE_INT";
        case RETURN: return "RETURN";
        case STRUCT: return "STRUCT";
        case VOID: return "VOID";
        case WHILE: return "WHILE";
        case INT: return "INT";
        case DOUBLE: return "DOUBLE";
        case CHAR: return "CHAR";
        case STRING: return "STRING";
        case COMMA: return "COMMA";
        case SEMICOLON: return "SEMICOLON";
        case LPAR: return "LPAR";
        case RPAR: return "RPAR";
        case LBRACKET: return "LBRACKET";
        case RBRACKET: return "RBRACKET";
        case LACC: return "LACC";
        case RACC: return "RACC";
        case END: return "END";
        case ADD: return "ADD";
        case SUB: return "SUB";
        case MUL: return "MUL";
        case DIV: return "DIV";
        case DOT: return "DOT";
        case AND: return "AND";
        case OR: return "OR";
        case NOT: return "NOT";
        case ASSIGN: return "ASSIGN";
        case EQUAL: return "EQUAL";
        case NOTEQ: return "NOTEQ";
        case LESS: return "LESS";
        case LESSEQ: return "LESSEQ";
        case GREATER: return "GREATER";
        case GREATEREQ: return "GREATEREQ";
        default: return "UNKNOWN";
    }
}

void showAtoms(const Token *tokens){
    for(const Token *tk=tokens;tk;tk=tk->next){
        printf("%d\t%s",tk->line,tkCodeName(tk->code));
        switch(tk->code){
            case ID:
                printf(":%s",tk->text);
                break;
            case INT:
                printf(":%d",tk->i);
                break;
            case DOUBLE:
                printf(":%g",tk->d);
                break;
            case CHAR:
                printf(":%c",tk->c);
                break;
            case STRING:
                printf(":%s",tk->text);
                break;
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]){
    if(argc<2){
        printf("Usage: %s <input_file>\n",argv[0]);
        return 1;
    }
    char *buf=loadFile(argv[1]);
    Token *tokens=tokenize(buf);
    showAtoms(tokens);
    free(buf);
    return 0;
}