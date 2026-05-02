#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"

Token *iTk; // the iterator in the tokens list
Token *consumedTk; // the last consumed token

void tkerr(const char *fmt, ...);

bool consume(int code);

//unit: ( structDef | fnDef | varDef )* END
bool unit();

//structDef: STRUCT ID LACC varDef* RACC SEMICOLON
bool structDef();

//varDef: typeBase ID arrayDecl? SEMICOLON
bool varDef();

//typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
bool typeBase();

//arrayDecl: LBRACKET INT? RBRACKET
bool arrayDecl();

//fnDef: ( typeBase | VOID ) ID
//				LPAR ( fnParam ( COMMA fnParam )* )? RPAR
//				stmCompound
bool fnDef();

//fnParam: typeBase ID arrayDecl?
bool fnParam();

// stm: stmCompound
//		| IF LPAR expr RPAR stm ( ELSE stm )?
//		| WHILE LPAR expr RPAR stm
//		| RETURN expr? SEMICOLON
//		| expr? SEMICOLON
bool stm();

// stmCompound: LACC ( varDef | stm )* RACC
bool stmCompound();

// expr: exprAssign
bool expr();

// exprAssign: exprUnary ASSIGN exprAssign | exprOr
bool exprAssign();

//exprOrPrim: OR exprAnd exprOrPrim | E
bool exprOrPrim();

// exprOr: exprAnd exprOrPrim
bool exprOr();

// exprAndPrim: AND exprEq exprAndPrim | E
bool exprAndPrim();

// exprAnd: exprEq exprAndPrim
bool exprAnd();

// exprEqPrim: ( EQUAL | NOTEQ ) exprRel exprEqPrim | E
bool exprEqPrim();

// exprEq: exprRel exprEqPrim
bool exprEq();

// exprRelPrim: ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd exprRelPrim | E
bool exprRelPrim();

// exprRel: exprAdd exprRelPrim
bool exprRel();

// exprAddPrim: ( ADD | SUB ) exprMul exprAddPrim | E
bool exprAddPrim();

// exprAdd: exprMul exprAddPrim
bool exprAdd();

// exprMulPrim: ( MUL | DIV ) exprCast exprMulPrim | E
bool exprMulPrim();

// exprMul: exprCast exprMulPrim
bool exprMul();

// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast();

// exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
bool exprUnary();

// exprPostfixPrim: LBRACKET expr RBRACKET exprPostfixPrim
// 				 | DOT ID exprPostfixPrim
// 				 | E
bool exprPostfixPrim();

// exprPostfix: exprPrimary exprPostfixPrim
bool exprPostfix();

// exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )?
//		| INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
bool exprPrimary();

void tkerr(const char *fmt, ...) {
	fprintf(stderr, "error in line %d: ", iTk->line);
	va_list va;
	va_start(va, fmt);
	vfprintf(stderr, fmt, va);
	va_end(va);
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

bool consume(int code) {
	if (iTk->code == code) {
		consumedTk = iTk;
		iTk = iTk->next;
		return true;
	}
	return false;
}

bool unit() {
	for (;;) {
		if (structDef()) {
		} else if (fnDef()) {
		} else if (varDef()) {
		} else break;
	}
	if (consume(END)) {
		return true;
	}
	return false;
}

bool structDef() {
	Token *start = iTk;
	if (consume(STRUCT)) {
		if (consume(ID)) {
			if (consume(LACC)) {
				for (;;) {
					if (varDef()) {
					} else break;
				}
				if (consume(RACC)) {
					if (consume(SEMICOLON)) return true;
					else tkerr("The semicolon \';\' after \'}\' is missing");
				} else tkerr("The \'}\' is missing");
			}
		}
	}
	iTk = start;
	return false;
}

bool varDef() {
	Token *start = iTk;
	if (typeBase()) {
		if (consume(ID)) {
			arrayDecl();
			if (consume(SEMICOLON)) return true;
			else tkerr("The semicolon \';\' is missing");
		}
	}
	iTk = start;
	return false;
}

bool typeBase() {
	Token *start = iTk;
	if (consume(TYPE_INT)) return true;
	if (consume(TYPE_DOUBLE)) return true;
	if (consume(TYPE_CHAR)) return true;
	if (consume(STRUCT)) {
		if (consume(ID)) return true;
	}
	iTk = start;
	return false;
}

bool arrayDecl() {
	Token *start = iTk;
	if (consume(LBRACKET)) {
		consume(INT);
		if (consume(RBRACKET)) { return true; } else tkerr("The \']\' is missing");
	}
	iTk = start;
	return false;
}

bool fnDef() {
	Token *start = iTk;
	bool hasType = false;
	if (typeBase()) {
		hasType = true;
	} else if (consume(VOID)) {
		hasType = true;
	}
	if (hasType) {
		if (consume(ID)) {
			if (consume(LPAR)) {
				if (fnParam()) {
					while (consume(COMMA)) {
						if (fnParam()) {
						} else tkerr("Param after\',\'  is missing");
					}
				}
				if (consume(RPAR)) {
					if (stmCompound()) {
						return true;
					} else tkerr("Body of the function is missing");
				} else tkerr("Fct declaration si not closed \')\' is missing");
			}
		}
	}
	iTk = start;
	return false;
}

bool fnParam() {
	Token *start = iTk;
	if (typeBase()) {
		if (consume(ID)) {
			arrayDecl();
			return true;
		} else tkerr("ID is missing");
	}
	iTk = start;
	return false;
}

bool stm() {
	Token *start = iTk;
	if (stmCompound()) return true;
	if (consume(IF)) {
		if (consume(LPAR)) {
			if (expr()) {
				if (consume(RPAR)) {
					if (stm()) {
						if (consume(ELSE)) {
							if (stm()) {
							} else tkerr("ELSE instructions are missing");
						}
						return true;
					} else tkerr("IF instructions are missing");
				} else tkerr("IF expression is not closed \')\' is missing");
			} else tkerr("IF expression is missing");
		} else tkerr("\'(\' is missing");
	}
	if (consume(WHILE)) {
		if (consume(LPAR)) {
			if (expr()) {
				if (consume(RPAR)) {
					if (stm())return true;
					else tkerr("WHILE instructions are missing");
				} else tkerr("WHILE expression is not closed \')\' is missing");
			} else tkerr("WHILE expression is missing");
		} else tkerr("\'(\' is missing");
	}
	if (consume(RETURN)) {
		expr();
		if (consume(SEMICOLON)) return true;
		else tkerr("\';\' is missing");
	}
	if (expr()) {
		if (consume(SEMICOLON)) return true;
		else tkerr("\';\' is missing");
	} else if (consume(SEMICOLON)) return true;
	iTk = start;
	return false;
}

bool stmCompound() {
	Token *start = iTk;
	if (consume(LACC)) {
		for (;;) {
			if (varDef()) {
			} else if (stm()) {
			} else break;
		}
		if (consume(RACC)) return true;
		else tkerr("\'}\' is missing");
	}
	iTk = start;
	return false;
}

bool expr() { return exprAssign(); }

bool exprAssign() {
	Token *start = iTk;
	if (exprUnary()) {
		if (consume(ASSIGN)) {
			if (exprAssign()) {
				return true;
			}
		}
	}
	iTk = start;
	if (exprOr()) return true;
	iTk = start;
	return false;
}

bool exprOrPrim() {
	if (consume(OR)) {
		if (exprAnd()) return exprOrPrim();
		else tkerr("expression is missing ||");
	}
	return true;
}

bool exprOr() {
	if (exprAnd()) return exprOrPrim();
	return false;
}

bool exprAndPrim() {
	if (consume(AND)) {
		if (exprEq()) return exprAndPrim();
		else tkerr("expression is missing after &&");
	}
	return true;
}

bool exprAnd() {
	if (exprEq()) return exprAndPrim();
	return false;
}

bool exprEqPrim() {
	if (consume(EQUAL) || consume(NOTEQ)) {
		if (exprRel()) return exprEqPrim();
		else tkerr("expression is missing ==\\!=");
	}
	return true;
}

bool exprEq() {
	if (exprRel()) return exprEqPrim();
	return false;
}

bool exprRelPrim() {
	if (consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)) {
		if (exprAdd()) return exprRelPrim();
		else tkerr("expression is missing after < \\ <= \\ > \\ >=");
	}
	return true;
}

bool exprRel() {
	if (exprAdd()) return exprRelPrim();
	return false;
}

bool exprAddPrim() {
	if (consume(ADD) || consume(SUB)) {
		if (exprMul()) return exprAddPrim();
		else tkerr("expression is missing after + \\ -");
	}
	return true;
}

bool exprAdd() {
	if (exprMul()) return exprAddPrim();
	return false;
}

bool exprMulPrim() {
	if (consume(MUL) || consume(DIV)) {
		if (exprCast()) return exprMulPrim();
		else tkerr("expression is missing after * \\ \\");
	}
	return true;
}

bool exprMul() {
	if (exprCast()) return exprMulPrim();
	return false;
}

bool exprCast() {
	Token *start = iTk;
	if (consume(LPAR)) {
		if (typeBase()) {
			arrayDecl();
			if (consume(RPAR)) {
				if (exprCast()) {
					return true;
				} else tkerr("expression is missing after cast");
			} else tkerr("cast expression is not closed \')\' is missing");
		}
		iTk = start;
	}
	return exprUnary();
}

bool exprUnary() {
	Token *start = iTk;
	if (consume(SUB) || consume(NOT)) {
		if (exprUnary()) return true;
		else tkerr("expression is missing after unarOp");
	}
	iTk = start;
	if (exprPostfix()) return true;
	iTk = start;
	return false;
}

bool exprPostfixPrim() {
	if (consume(LBRACKET)) {
		if (expr()) {
			if (consume(RBRACKET)) return exprPostfixPrim();
			else tkerr("expression is not closed \']\' is missing");
		}else tkerr("expression is missing in []");
	}
	if (consume(DOT)) {
		if (consume(ID)) return exprPostfixPrim();
		else tkerr("after \'.\' the identifier is missing");
	}
	return true;
}

bool exprPostfix() {
	if (exprPrimary()) return exprPostfixPrim();
	return false;
}

bool exprPrimary() {
	Token *start = iTk;
	if (consume(ID)) {
		if (consume(LPAR)) {
			if (expr()) {
				for (; consume(COMMA);) {
					expr();
				}
			}
			if (consume(RPAR)) {
				return true;
			} else tkerr("function call is not closed \')\' is missing");
		}
		return true;
	}
	if (consume(INT)) return true;
	if (consume(DOUBLE)) return true;
	if (consume(CHAR)) return true;
	if (consume(STRING)) return true;
	if (consume(LPAR)) {
		if (expr()) {
			if (consume(RPAR)) {
				return true;
			} else tkerr("expression is not closed \')\' is missing");
		} else tkerr("expression is missing");
	}
	iTk = start;
	return false;
}

void parse(Token *tokens) {
	iTk = tokens;
	if (!unit())tkerr("syntax error");
}
