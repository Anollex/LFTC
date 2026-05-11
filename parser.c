#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"

#include "ad.h"
#include "at.h"
#include "utils.h"

Token *iTk; // the iterator in the tokens list
Token *consumedTk; // the last consumed token
Symbol *owner = NULL;

void tkerr(const char *fmt, ...);

bool consume(int code);

//unit: ( structDef | fnDef | varDef )* END
bool unit();

//structDef: STRUCT ID LACC varDef* RACC SEMICOLON
bool structDef();

//varDef: typeBase ID arrayDecl? SEMICOLON
bool varDef();

//typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
bool typeBase(Type *t);

//arrayDecl: LBRACKET INT? RBRACKET
bool arrayDecl(Type *t);

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
bool stmCompound(bool newDomain);

// expr: exprAssign
bool expr(Ret *r);

// exprAssign: exprUnary ASSIGN exprAssign | exprOr
bool exprAssign(Ret *r);

// exprOrPrim: OR exprAnd exprOrPrim | E
bool exprOrPrim(Ret *r);

// exprOr: exprAnd exprOrPrim
bool exprOr(Ret *r);

// exprAndPrim: AND exprEq exprAndPrim | E
bool exprAndPrim(Ret *r);

// exprAnd: exprEq exprAndPrim
bool exprAnd(Ret *r);

// exprEqPrim: ( EQUAL | NOTEQ ) exprRel exprEqPrim | E
bool exprEqPrim(Ret *r);

// exprEq: exprRel exprEqPrim
bool exprEq(Ret *r);

// exprRelPrim: ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd exprRelPrim | E
bool exprRelPrim(Ret *r);

// exprRel: exprAdd exprRelPrim
bool exprRel(Ret *r);

// exprAddPrim: ( ADD | SUB ) exprMul exprAddPrim | E
bool exprAddPrim(Ret *r);

// exprAdd: exprMul exprAddPrim
bool exprAdd(Ret *r);

// exprMulPrim: ( MUL | DIV ) exprCast exprMulPrim | E
bool exprMulPrim(Ret *r);

// exprMul: exprCast exprMulPrim
bool exprMul(Ret *r);

// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast(Ret *r);

// exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
bool exprUnary(Ret *r);

// exprPostfixPrim: LBRACKET expr RBRACKET exprPostfixPrim
// 				 | DOT ID exprPostfixPrim
// 				 | E
bool exprPostfixPrim(Ret *r);

// exprPostfix: exprPrimary exprPostfixPrim
bool exprPostfix(Ret *r);

// exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )?
//		| INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
bool exprPrimary(Ret *r);

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
			Token *tkName = consumedTk;
			if (consume(LACC)) {
				Symbol *s = findSymbolInDomain(symTable, tkName->text);
				if (s) tkerr("SD: symbol redefinition: %s", tkName->text);
				s = addSymbolToDomain(symTable, newSymbol(tkName->text, SK_STRUCT));
				s->type.tb = TB_STRUCT;
				s->type.s = s;
				s->type.n = -1;
				pushDomain();
				owner = s;
				for (;;) {
					if (varDef()) {
					} else break;
				}
				if (consume(RACC)) {
					if (consume(SEMICOLON)) {
						owner = NULL;
						dropDomain();
						return true;
					} else tkerr("missing ; after struct definition");
				} else tkerr("missing } in struct definition/invalid declaration");
			}
		}
	}
	iTk = start;
	return false;
}

bool varDef() {
	Token *start = iTk;
	Type t;
	if (typeBase(&t)) {
		if (consume(ID)) {
			Token *tkName = consumedTk;
			arrayDecl(&t);
			if (t.n == 0) tkerr("AD: a vector variable must have a specified dimension");
			if (consume(SEMICOLON)) {
				Symbol *var = findSymbolInDomain(symTable, tkName->text);
				if (var) tkerr("SD: symbol redefinition: %s", tkName->text);
				var = newSymbol(tkName->text, SK_VAR);
				var->type = t;
				var->owner = owner;
				addSymbolToDomain(symTable, var);
				if (owner) {
					switch (owner->kind) {
						case SK_FN:
							var->varIdx = symbolsLen(owner->fn.locals);
							addSymbolToList(&owner->fn.locals, dupSymbol(var));
							break;
						case SK_STRUCT:
							var->varIdx = typeSize(&owner->type);
							addSymbolToList(&owner->structMembers, dupSymbol(var));
							break;
					}
				} else {
					var->varMem = safeAlloc(typeSize(&t));
				}
				return true;
			} else tkerr("missing ; after variable declaration");
		} else tkerr("missing identifier of a variable/function, missing { in struct definition");
	}
	iTk = start;
	return false;
}

bool typeBase(Type *t) {
	t->n = -1;
	Token *start = iTk;
	if (consume(TYPE_INT)) {
		t->tb = TB_INT;
		return true;
	}
	if (consume(TYPE_DOUBLE)) {
		t->tb = TB_DOUBLE;
		return true;
	}
	if (consume(TYPE_CHAR)) {
		t->tb = TB_CHAR;
		return true;
	}
	if (consume(STRUCT)) {
		if (consume(ID)) {
			Token *tkName = consumedTk;
			t->tb = TB_STRUCT;
			t->s = findSymbol(tkName->text);
			if (!t->s) tkerr("AD: undefined struct: %s", tkName->text);
			return true;
		} else tkerr("missing identifier, invalid struct or function definition");
	}
	iTk = start;
	return false;
}

bool arrayDecl(Type *t) {
	Token *start = iTk;
	if (consume(LBRACKET)) {
		if (consume(INT)) {
			Token *tkSize = consumedTk;
			t->n = tkSize->i;
		} else {
			t->n = 0;
		}
		if (consume(RBRACKET)) return true;
		else tkerr("missing ] in array declaration");
	}
	iTk = start;
	return false;
}

bool fnDef() {
	Token *start = iTk;
	Type t;
	bool hasType = false;
	if (typeBase(&t)) {
		hasType = true;
	} else if (consume(VOID)) {
		t.tb = TB_VOID;
		t.n = -1;
		hasType = true;
	}
	if (hasType) {
		if (consume(ID)) {
			Token *tkName = consumedTk;
			if (consume(LPAR)) {
				Symbol *fn = findSymbolInDomain(symTable, tkName->text);
				if (fn) tkerr("AD: symbol redefinition: %s", tkName->text);
				fn = newSymbol(tkName->text, SK_FN);
				fn->type = t;
				addSymbolToDomain(symTable, fn);
				owner = fn;
				pushDomain();
				if (fnParam()) {
					while (consume(COMMA)) {
						if (fnParam()) {
						} else tkerr("missing/invalid parameter after ,");
					}
				}
				if (consume(RPAR)) {
					if (stmCompound(false)) {
						dropDomain();
						owner = NULL;
						return true;
					} else tkerr("missing function body");
				} else tkerr("missing/invalid parameter/missing ) in function declaration");
			}
		}
	}
	iTk = start;
	return false;
}

bool fnParam() {
	Token *start = iTk;
	Type t;
	if (typeBase(&t)) {
		if (consume(ID)) {
			Token *tkName = consumedTk;
			if (arrayDecl(&t)) { t.n = 0; }
			Symbol *param = findSymbolInDomain(symTable, tkName->text);
			if (param) tkerr("AD: symbol redefinition: %s", tkName->text);
			param = newSymbol(tkName->text, SK_PARAM);
			param->type = t;
			param->owner = owner;
			param->paramIdx = symbolsLen(owner->fn.params);
			addSymbolToDomain(symTable, param);
			addSymbolToList(&owner->fn.params, dupSymbol(param));
			return true;
		} else tkerr("missing parameter name");
	}
	iTk = start;
	return false;
}

bool stm() {
	Token *start = iTk;
	if (stmCompound(true)) return true;
	if (consume(IF)) {
		if (consume(LPAR)) {
			Ret rCond;
			if (expr(&rCond)) {
				if (!canBeScalar(&rCond)) tkerr("AT: the if condition must be a scalar value");
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
			Ret rCond;
			if (expr(&rCond)) {
				if (!canBeScalar(&rCond)) tkerr("AT: the while condition must be a scalar value");
				if (consume(RPAR)) {
					if (stm()) return true;
					else tkerr("missing statement after while");
				} else tkerr("invalid while condition or missing )");
			} else tkerr("missing while condition");
		} else tkerr("missing ( after while");
	}
	if (consume(RETURN)) {
		Ret rExpr;
		if (expr(&rExpr)) {
			if (owner->type.tb == TB_VOID) tkerr("AT: a void function cannot return a value");
			if (!canBeScalar(&rExpr)) tkerr("AT: the return value must be a scalar value");
			if (!convTo(&rExpr.type, &owner->type))
				tkerr("AT: cannot convert the return expression type to the function return type");
		} else {
			if (owner->type.tb != TB_VOID) tkerr("AT: a non-void function must return a value");
		}
		if (consume(SEMICOLON)) return true;
		else tkerr("missing ; after return");
	}
	Ret rExpr;
	if (expr(&rExpr)) {
		if (consume(SEMICOLON)) return true;
		else tkerr("missing ; after expression");
	} else if (consume(SEMICOLON)) return true;
	iTk = start;
	return false;
}

bool stmCompound(bool newDomain) {
	Token *start = iTk;
	if (consume(LACC)) {
		if (newDomain) pushDomain();
		for (;;) {
			if (varDef()) {
			} else if (stm()) {
			} else break;
		}
		if (consume(RACC)) {
			if (newDomain) dropDomain();
			return true;
		} else tkerr("missing }");
	}
	iTk = start;
	return false;
}

bool expr(Ret *r) { return exprAssign(r); }

bool exprAssign(Ret *r) {
	Token *start = iTk;
	Ret rDst;
	if (exprUnary(&rDst)) {
		if (consume(ASSIGN)) {
			if (exprAssign(r)) {
				if (!rDst.lval) tkerr("AT: the assign destination must be a left-value");
				if (rDst.ct) tkerr("AT: the assign destination cannot be constant");
				if (!canBeScalar(&rDst)) tkerr("AT: the assign destination must be scalar");
				if (!canBeScalar(r)) tkerr("AT: the assign source must be scalar");
				if (!convTo(&r->type, &rDst.type))
					tkerr("AT: the assign source cannot be converted to destination");
				r->lval = false;
				r->ct = true;
				return true;
			}
		}
	}
	iTk = start;
	if (exprOr(r)) return true;
	iTk = start;
	return false;
}

// exprOrPrim: OR exprAnd exprOrPrim | E
bool exprOrPrim(Ret *r) {
	if (consume(OR)) {
		int op = consumedTk->code;
		Ret right;
		if (exprAnd(&right)) {
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst))
				tkerr("AT: invalid operand type for %s", opName(op));
			*r = (Ret){{TB_INT, NULL, -1}, false, true};
			return exprOrPrim(r);
		} else tkerr("missing expression after %s", opName(op));
	}
	return true; // epsilon
}

// exprOr: exprAnd exprOrPrim
bool exprOr(Ret *r) {
	if (exprAnd(r)) return exprOrPrim(r);
	return false;
}

// exprAndPrim: AND exprEq exprAndPrim | E
bool exprAndPrim(Ret *r) {
	if (consume(AND)) {
		int op = consumedTk->code;
		Ret right;
		if (exprEq(&right)) {
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst))
				tkerr("AT: invalid operand type for %s", opName(op));
			*r = (Ret){{TB_INT, NULL, -1}, false, true};
			return exprAndPrim(r);
		} else tkerr("missing expression after %s", opName(op));
	}
	return true; // epsilon
}

// exprAnd: exprEq exprAndPrim
bool exprAnd(Ret *r) {
	if (exprEq(r)) return exprAndPrim(r);
	return false;
}

// exprEqPrim: ( EQUAL | NOTEQ ) exprRel exprEqPrim | E
bool exprEqPrim(Ret *r) {
	if (consume(EQUAL) || consume(NOTEQ)) {
		int op = consumedTk->code;
		Ret right;
		if (exprRel(&right)) {
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst))
				tkerr("AT: invalid operand type for %s", opName(op));
			*r = (Ret){{TB_INT, NULL, -1}, false, true};
			return exprEqPrim(r);
		} else tkerr("missing expression after %s", opName(op));
	}
	return true;
}

// exprEq: exprRel exprEqPrim
bool exprEq(Ret *r) {
	if (exprRel(r)) return exprEqPrim(r);
	return false;
}

// exprRelPrim: ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd exprRelPrim | E
bool exprRelPrim(Ret *r) {
	if (consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)) {
		int op = consumedTk->code;
		Ret right;
		if (exprAdd(&right)) {
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst))
				tkerr("AT: invalid operand type for %s", opName(op));
			*r = (Ret){{TB_INT, NULL, -1}, false, true};
			return exprRelPrim(r);
		} else tkerr("missing expression after %s", opName(op));
	}
	return true;
}

// exprRel: exprAdd exprRelPrim
bool exprRel(Ret *r) {
	if (exprAdd(r)) return exprRelPrim(r);
	return false;
}

// exprAddPrim: ( ADD | SUB ) exprMul exprAddPrim | E
bool exprAddPrim(Ret *r) {
	if (consume(ADD) || consume(SUB)) {
		int op = consumedTk->code;
		Ret right;
		if (exprMul(&right)) {
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst))
				tkerr("AT: invalid operand type for %s", opName(op));
			*r = (Ret){tDst, false, true};
			return exprAddPrim(r);
		} else tkerr("missing expression after %s", opName(op));
	}
	return true;
}

// exprAdd: exprMul exprAddPrim
bool exprAdd(Ret *r) {
	if (exprMul(r)) return exprAddPrim(r);
	return false;
}

// exprMulPrim: ( MUL | DIV ) exprCast exprMulPrim | E
bool exprMulPrim(Ret *r) {
	if (consume(MUL) || consume(DIV)) {
		int op = consumedTk->code;
		Ret right;
		if (exprCast(&right)) {
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst))
				tkerr("AT: invalid operand type for %s", opName(op));
			*r = (Ret){tDst, false, true};
			return exprMulPrim(r);
		} else tkerr("missing expression after %s", opName(op));
	}
	return true;
}

// exprMul: exprCast exprMulPrim
bool exprMul(Ret *r) {
	if (exprCast(r)) return exprMulPrim(r);
	return false;
}

// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast(Ret *r) {
	Token *start = iTk;
	if (consume(LPAR)) {
		Type t;
		if (typeBase(&t)) {
			arrayDecl(&t);
			if (consume(RPAR)) {
				Ret op;
				if (exprCast(&op)) {
					if (t.tb == TB_STRUCT) tkerr("AT: cannot convert to a struct type");
					if (op.type.tb == TB_STRUCT) tkerr("AT: cannot convert a struct");
					if (op.type.n >= 0 && t.n < 0) tkerr("AT: an array can be converted only to another array");
					if (op.type.n < 0 && t.n >= 0) tkerr("AT: a scalar can be converted only to another scalar");
					*r = (Ret){t, false, true};
					return true;
				} else tkerr("missing expression after cast");
			} else tkerr("missing ) after cast type");
		}
		iTk = start;
	}
	return exprUnary(r);
}

// exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
bool exprUnary(Ret *r) {
	Token *start = iTk;
	if (consume(SUB) || consume(NOT)) {
		int op = consumedTk->code;
		if (exprUnary(r)) {
			if (!canBeScalar(r)) tkerr("AT: unary %s must have a scalar operand", opName(op));
			r->lval = false;
			r->ct = true;
			return true;
		} else tkerr("missing expression after %s", opName(op));
	}
	iTk = start;
	if (exprPostfix(r)) return true;
	iTk = start;
	return false;
}

// exprPostfixPrim: LBRACKET expr RBRACKET exprPostfixPrim | DOT ID exprPostfixPrim | E
bool exprPostfixPrim(Ret *r) {
	if (consume(LBRACKET)) {
		Ret idx;
		if (expr(&idx)) {
			if (consume(RBRACKET)) {
				if (r->type.n < 0) tkerr("AT: only an array can be indexed");
				Type tInt = {TB_INT, NULL, -1};
				if (!convTo(&idx.type, &tInt)) tkerr("AT: the index is not convertible to int");
				r->type.n = -1;
				r->lval = true;
				r->ct = false;
				return exprPostfixPrim(r);
			} else tkerr("missing ] after array index");
		} else tkerr("missing expression in array index");
	}
	if (consume(DOT)) {
		if (consume(ID)) {
			Token *tkName = consumedTk;
			if (r->type.tb != TB_STRUCT) tkerr("AT: a field can only be selected from a struct");
			Symbol *s = findSymbolInList(r->type.s->structMembers, tkName->text);
			if (!s) tkerr("AT: the structure %s does not have a field %s", r->type.s->name, tkName->text);
			*r = (Ret){s->type, true, s->type.n >= 0};
			return exprPostfixPrim(r);
		} else tkerr("missing field name after .");
	}
	return true; // epsilon
}

// exprPostfix: exprPrimary exprPostfixPrim
bool exprPostfix(Ret *r) {
	if (exprPrimary(r)) return exprPostfixPrim(r);
	return false;
}

// exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )? | INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
bool exprPrimary(Ret *r) {
	Token *start = iTk;
	if (consume(ID)) {
		Token *tkName = consumedTk;
		Symbol *s = findSymbol(tkName->text);
		if (!s) tkerr("AT: undefined id: %s", tkName->text);
		if (consume(LPAR)) {
			if (s->kind != SK_FN) tkerr("AT: only a function can be called");
			Ret rArg;
			Symbol *param = s->fn.params;
			if (expr(&rArg)) {
				if (!param) tkerr("AT: too many arguments in function call");
				if (!convTo(&rArg.type, &param->type))
					tkerr("AT: in call, cannot convert the argument type to the parameter type");
				param = param->next;
				for (; consume(COMMA);) {
					if (!expr(&rArg)) tkerr("missing argument after , in function call");
					if (!param) tkerr("AT: too many arguments in function call");
					if (!convTo(&rArg.type, &param->type))
						tkerr("AT: in call, cannot convert the argument type to the parameter type");
					param = param->next;
				}
			}
			if (consume(RPAR)) {
				if (param) tkerr("AT: too few arguments in function call");
				*r = (Ret){s->type, false, true};
				return true;
			} else tkerr("missing ) in function call");
		} else {
			// doar variabilă/parametru
			if (s->kind == SK_FN) tkerr("AT: a function can only be called");
			*r = (Ret){s->type, true, s->type.n >= 0};
		}
		return true;
	}
	if (consume(INT)) { *r = (Ret){{TB_INT, NULL, -1}, false, true}; return true; }
	if (consume(DOUBLE)) { *r = (Ret){{TB_DOUBLE, NULL, -1}, false, true}; return true; }
	if (consume(CHAR)) { *r = (Ret){{TB_CHAR, NULL, -1}, false, true}; return true; }
	if (consume(STRING)) { *r = (Ret){{TB_CHAR, NULL, 0}, false, true}; return true; }
	if (consume(LPAR)) {
		if (expr(r)) {
			if (consume(RPAR)) {
				return true;
			} else tkerr("missing ) after expression");
		}
	}
	iTk = start;
	return false;
}

void parse(Token *tokens) {
	iTk = tokens;
	if (!unit()) tkerr("unexpected token at global level");
}
