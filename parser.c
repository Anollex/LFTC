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

// exprOrPrim: OR exprAnd exprOrPrim | E
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

const char *opName(int code) {
	switch (code) {
		case LESS: return "<";
		case LESSEQ: return "<=";
		case GREATER: return ">";
		case GREATEREQ: return ">=";
		case EQUAL: return "==";
		case NOTEQ: return "!=";
		case ADD: return "+";
		case SUB: return "-";
		case MUL: return "*";
		case DIV: return "/";
		case OR: return "||";
		case AND: return "&&";
		case NOT: return "!";
		case ASSIGN: return "=";
		default: return "?";
	}
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
					else tkerr("missing ; after struct definition");
				} else tkerr("missing } in struct definition");
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
			else tkerr("missing ; after variable declaration");
		} else tkerr("missing identifier, missing { in struct definition");
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
		if (consume(RBRACKET)) return true;
		else tkerr("missing ] in array declaration");
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
						} else tkerr("missing parameter after ,");
					}
				}
				if (consume(RPAR)) {
					if (stmCompound()) {
						return true;
					} else tkerr("missing function body");
				} else tkerr("missing ) in function declaration");
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
		} else tkerr("missing parameter name");
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
							} else tkerr("missing statement after else");
						}
						return true;
					} else tkerr("missing statement after if");
				} else tkerr("invalid if condition or missing )");
			} else tkerr("missing if condition");
		} else tkerr("missing ( after if");
	}
	if (consume(WHILE)) {
		if (consume(LPAR)) {
			if (expr()) {
				if (consume(RPAR)) {
					if (stm()) return true;
					else tkerr("missing statement after while");
				} else tkerr("invalid while condition or missing )");
			} else tkerr("missing while condition");
		} else tkerr("missing ( after while");
	}
	if (consume(RETURN)) {
		expr(); // optional
		if (consume(SEMICOLON)) return true;
		else tkerr("missing ; after return");
	}
	if (expr()) {
		if (consume(SEMICOLON)) return true;
		else tkerr("missing ; after expression");
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
		else tkerr("missing }");
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

// exprOrPrim: OR exprAnd exprOrPrim | E
bool exprOrPrim() {
	if (consume(OR)) {
		int op = consumedTk->code;
		if (exprAnd()) return exprOrPrim();
		else tkerr("missing expression after %s", opName(op));
	}
	return true; // epsilon
}

// exprOr: exprAnd exprOrPrim
bool exprOr() {
	if (exprAnd()) return exprOrPrim();
	return false;
}

// exprAndPrim: AND exprEq exprAndPrim | E
bool exprAndPrim() {
	if (consume(AND)) {
		int op = consumedTk->code;
		if (exprEq()) return exprAndPrim();
		else tkerr("missing expression after %s", opName(op));
	}
	return true; // epsilon
}

// exprAnd: exprEq exprAndPrim
bool exprAnd() {
	if (exprEq()) return exprAndPrim();
	return false;
}

// exprEqPrim: ( EQUAL | NOTEQ ) exprRel exprEqPrim | E
bool exprEqPrim() {
	if (consume(EQUAL) || consume(NOTEQ)) {
		int op = consumedTk->code;
		if (exprRel()) return exprEqPrim();
		else tkerr("missing expression after %s", opName(op));
	}
	return true; // epsilon
}

// exprEq: exprRel exprEqPrim
bool exprEq() {
	if (exprRel()) return exprEqPrim();
	return false;
}

// exprRelPrim: ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd exprRelPrim | E
bool exprRelPrim() {
	if (consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)) {
		int op = consumedTk->code;
		if (exprAdd()) return exprRelPrim();
		else tkerr("missing expression after %s", opName(op));
	}
	return true; // epsilon
}

// exprRel: exprAdd exprRelPrim
bool exprRel() {
	if (exprAdd()) return exprRelPrim();
	return false;
}

// exprAddPrim: ( ADD | SUB ) exprMul exprAddPrim | E
bool exprAddPrim() {
	if (consume(ADD) || consume(SUB)) {
		int op = consumedTk->code;
		if (exprMul()) return exprAddPrim();
		else tkerr("missing expression after %s", opName(op));
	}
	return true; // epsilon
}

// exprAdd: exprMul exprAddPrim
bool exprAdd() {
	if (exprMul()) return exprAddPrim();
	return false;
}

// exprMulPrim: ( MUL | DIV ) exprCast exprMulPrim | E
bool exprMulPrim() {
	if (consume(MUL) || consume(DIV)) {
		int op = consumedTk->code;
		if (exprCast()) return exprMulPrim();
		else tkerr("missing expression after %s", opName(op));
	}
	return true; // epsilon
}

// exprMul: exprCast exprMulPrim
bool exprMul() {
	if (exprCast()) return exprMulPrim();
	return false;
}

// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast() {
	Token *start = iTk;
	if (consume(LPAR)) {
		if (typeBase()) {
			arrayDecl();
			if (consume(RPAR)) {
				if (exprCast()) return true;
				else tkerr("missing expression after cast");
			} else tkerr("missing ) after cast type");
		}
		iTk = start;
	}
	return exprUnary();
}

// exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
bool exprUnary() {
	Token *start = iTk;
	if (consume(SUB) || consume(NOT)) {
		int op = consumedTk->code;
		if (exprUnary()) return true;
		else tkerr("missing expression after %s", opName(op));
	}
	iTk = start;
	if (exprPostfix()) return true;
	iTk = start;
	return false;
}

// exprPostfixPrim: LBRACKET expr RBRACKET exprPostfixPrim | DOT ID exprPostfixPrim | E
bool exprPostfixPrim() {
	if (consume(LBRACKET)) {
		if (expr()) {
			if (consume(RBRACKET)) return exprPostfixPrim();
			else tkerr("missing ] after array index");
		} else tkerr("missing expression in array index");
	}
	if (consume(DOT)) {
		if (consume(ID)) return exprPostfixPrim();
		else tkerr("missing field name after .");
	}
	return true; // epsilon
}

// exprPostfix: exprPrimary exprPostfixPrim
bool exprPostfix() {
	if (exprPrimary()) return exprPostfixPrim();
	return false;
}

// exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )? | INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
bool exprPrimary() {
	Token *start = iTk;
	if (consume(ID)) {
		if (consume(LPAR)) {
			if (expr()) {
				for (; consume(COMMA);) {
					if (!expr()) tkerr("missing argument after , in function call");
				}
			}
			if (consume(RPAR)) {
				return true;
			} else tkerr("missing ) in function call");
		}
		return true;
	}
	if (consume(INT)) return true;
	if (consume(DOUBLE)) return true;
	if (consume(CHAR)) return true;
	if (consume(STRING)) return true;
	if (consume(LPAR)) {
		if (expr())
		{
			if (consume(RPAR)) {
				return true;
			} else tkerr("missing ) after expression");
		}
		// } else tkerr("invalid or missing expression after (");
	}
	iTk = start;
	return false;
}

void parse(Token *tokens) {
	iTk = tokens;
	if (!unit()) tkerr("unexpected token at global level");
}
