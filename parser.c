#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"

Token *iTk;		// the iterator in the tokens list
Token *consumedTk;		// the last consumed token

void tkerr(const char *fmt,...){
	fprintf(stderr,"error in line %d: ",iTk->line);
	va_list va;
	va_start(va,fmt);
	vfprintf(stderr,fmt,va);
	va_end(va);
	fprintf(stderr,"\n");
	exit(EXIT_FAILURE);
	}

bool consume(int code){
	if(iTk->code==code){
		consumedTk=iTk;
		iTk=iTk->next;
		return true;
		}
	return false;
	}

// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
bool typeBase(){
	Token *start=iTk;
	if(consume(TYPE_INT)) return true;
	if(consume(TYPE_DOUBLE)) return true;
	if(consume(TYPE_CHAR)) return true;
	if(consume(STRUCT)){
		if(consume(ID)) return true;
		else tkerr("Struct ID is missing at line %d\n", iTk->line);
		}
	iTk=start;
	return false;
	}

bool arrayDecl() {
	Token *start=iTk;
	if(consume(LBRACKET)) {
		consume(INT);
		if (consume(RBRACKET)){ return true; }
		else tkerr("The \']\' is missing at line %d\n");
	}
	iTk=start;
	return false;
}

bool varDef() {
	Token *start=iTk;
	if(typeBase()) {
		if(consume(ID)) {
			arrayDecl();
			if(consume(SEMICOLON)) return true;
			else tkerr("The semicolon \';\' is missing at line %d\n", iTk->line);
		}else tkerr("ID is missing at line %d\n", iTk->line);
	}
	iTk=start;
	return false;
}

bool structDef(){
	Token * start = iTk;
	if(consume(STRUCT)) {
		if(consume(ID)) {
			if(consume(LACC)) {
				for (;;) {
					if (varDef()){}
					else break;
				}
				if(consume(RBRACKET)) {
					if(consume(SEMICOLON)) return true;
					else tkerr("The semicolon \';\' after \'}\' is missing at line %d\n", iTk->line);
				}else tkerr("The \'}\' is missing at line %d\n", iTk->line);
			}
		}
	}
	iTk=start;
	return false;
}

bool fnParam() {
	Token *start=iTk;
	if(typeBase()){
		if (consume(ID)) {
			arrayDecl();
			return true;
		}else tkerr("ID is missing at line %d\n", iTk->line);
	}
	iTk=start;
	return false;
}

bool stmCompound();

bool fnDef() {
	Token *start=iTk;
	bool hasType=false;
	if(typeBase()) {
		hasType=true;
	}else if(consume(VOID)) {
		hasType=true;
	}

	if(hasType) {
		if (consume(LPAR)) {
			if (fnParam()) {
				while (consume(COMMA)) {
					if (fnParam()){}
					else tkerr("Param is missing after\',\' at line %d", iTk->line);
				}
			}
			if (consume(RPAR)) {
				if (stmCompound()) {
					return true;
				}else tkerr("Fct is missing the body at line %d", iTk->line);
			}else tkerr("Fct declaration si not closed missing \')\' at line");
		}
	}
}


// unit: ( structDef | fnDef | varDef )* END
bool unit(){
	for(;;){
		if(structDef()){}
		else if(fnDef()){}
		else if(varDef()){}
		else break;
		}
	if(consume(END)){
		return true;
		}
	return false;
	}

void parse(Token *tokens){
	iTk=tokens;
	if(!unit())tkerr("syntax error");
	}
