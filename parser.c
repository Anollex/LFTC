#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"

#include "ad.h"
#include "at.h"
#include "gc.h"
#include "utils.h"

Token *iTk; // the iterator in the tokens list
Token *consumedTk; // the last consumed token
Symbol *owner = NULL;

void tkerr(const char *fmt, ...);

bool consume(int code);

bool unit();
bool structDef();
bool varDef();
bool typeBase(Type *t);
bool arrayDecl(Type *t);
bool fnDef();
bool fnParam();
bool stm();
bool stmCompound(bool newDomain);
bool expr(Ret *r);
bool exprAssign(Ret *r);
bool exprOrPrim(Ret *r);
bool exprOr(Ret *r);
bool exprAndPrim(Ret *r);
bool exprAnd(Ret *r);
bool exprEqPrim(Ret *r);
bool exprEq(Ret *r);
bool exprRelPrim(Ret *r);
bool exprRel(Ret *r);
bool exprAddPrim(Ret *r);
bool exprAdd(Ret *r);
bool exprMulPrim(Ret *r);
bool exprMul(Ret *r);
bool exprCast(Ret *r);
bool exprUnary(Ret *r);
bool exprPostfixPrim(Ret *r);
bool exprPostfix(Ret *r);
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
	Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
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
					// GC: ENTER (nb_locals set after body)
					addInstr(&fn->fn.instr, OP_ENTER);
					if (stmCompound(false)) {
						// GC: set nb_locals, add RET_VOID for void functions
						fn->fn.instr->arg.i = symbolsLen(fn->fn.locals);
						if (fn->type.tb == TB_VOID)
							addInstrWithInt(&fn->fn.instr, OP_RET_VOID, symbolsLen(fn->fn.params));
						dropDomain();
						owner = NULL;
						return true;
					} else tkerr("missing function body");
				} else tkerr("missing/invalid parameter/missing ) in function declaration");
			}
		}
	}
	iTk = start;
	if (owner) delInstrAfter(startInstr);
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
	Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
	Token *start = iTk;
	if (stmCompound(true)) return true;
	if (consume(IF)) {
		if (consume(LPAR)) {
			Ret rCond;
			if (expr(&rCond)) {
				if (!canBeScalar(&rCond)) tkerr("AT: the if condition must be a scalar value");
				// GC
				addRVal(&owner->fn.instr, rCond.lval, &rCond.type);
				Type intType = {TB_INT, NULL, -1};
				insertConvIfNeeded(lastInstr(owner->fn.instr), &rCond.type, &intType);
				Instr *ifJF = addInstr(&owner->fn.instr, OP_JF);
				if (consume(RPAR)) {
					if (stm()) {
						if (consume(ELSE)) {
							// GC: jump over else
							Instr *ifJMP = addInstr(&owner->fn.instr, OP_JMP);
							ifJF->arg.instr = addInstr(&owner->fn.instr, OP_NOP);
							if (stm()) {
								ifJMP->arg.instr = addInstr(&owner->fn.instr, OP_NOP);
							} else tkerr("missing statement after else");
						} else {
							ifJF->arg.instr = addInstr(&owner->fn.instr, OP_NOP);
						}
						return true;
					} else tkerr("missing statement after if");
				} else tkerr("invalid if condition or missing )");
			} else tkerr("missing if condition");
		} else tkerr("missing ( after if");
	}
	if (consume(WHILE)) {
		// GC: position before condition
		Instr *beforeWhileCond = lastInstr(owner->fn.instr);
		if (consume(LPAR)) {
			Ret rCond;
			if (expr(&rCond)) {
				if (!canBeScalar(&rCond)) tkerr("AT: the while condition must be a scalar value");
				// GC
				addRVal(&owner->fn.instr, rCond.lval, &rCond.type);
				Type intType = {TB_INT, NULL, -1};
				insertConvIfNeeded(lastInstr(owner->fn.instr), &rCond.type, &intType);
				Instr *whileJF = addInstr(&owner->fn.instr, OP_JF);
				if (consume(RPAR)) {
					if (stm()) {
						// GC: jump back, set exit
						addInstr(&owner->fn.instr, OP_JMP)->arg.instr = beforeWhileCond->next;
						whileJF->arg.instr = addInstr(&owner->fn.instr, OP_NOP);
						return true;
					} else tkerr("missing statement after while");
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
			// GC
			addRVal(&owner->fn.instr, rExpr.lval, &rExpr.type);
			insertConvIfNeeded(lastInstr(owner->fn.instr), &rExpr.type, &owner->type);
			addInstrWithInt(&owner->fn.instr, OP_RET, symbolsLen(owner->fn.params));
		} else {
			if (owner->type.tb != TB_VOID) tkerr("AT: a non-void function must return a value");
			// GC
			addInstrWithInt(&owner->fn.instr, OP_RET_VOID, symbolsLen(owner->fn.params));
		}
		if (consume(SEMICOLON)) return true;
		else tkerr("missing ; after return");
	}
	Ret rExpr;
	if (expr(&rExpr)) {
		// GC: drop unused result
		if (rExpr.type.tb != TB_VOID) addInstr(&owner->fn.instr, OP_DROP);
		if (consume(SEMICOLON)) return true;
		else tkerr("missing ; after expression");
	} else if (consume(SEMICOLON)) return true;
	iTk = start;
	if (owner) delInstrAfter(startInstr);
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
	Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
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
				// GC
				addRVal(&owner->fn.instr, r->lval, &r->type);
				insertConvIfNeeded(lastInstr(owner->fn.instr), &r->type, &rDst.type);
				switch (rDst.type.tb) {
					case TB_INT: addInstr(&owner->fn.instr, OP_STORE_I); break;
					case TB_DOUBLE: addInstr(&owner->fn.instr, OP_STORE_F); break;
				}
				r->lval = false;
				r->ct = true;
				return true;
			}
		}
	}
	iTk = start;
	if (owner) delInstrAfter(startInstr);
	if (exprOr(r)) return true;
	iTk = start;
	if (owner) delInstrAfter(startInstr);
	return false;
}

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
	return true;
}

bool exprOr(Ret *r) {
	if (exprAnd(r)) return exprOrPrim(r);
	return false;
}

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
	return true;
}

bool exprAnd(Ret *r) {
	if (exprEq(r)) return exprAndPrim(r);
	return false;
}

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

bool exprEq(Ret *r) {
	if (exprRel(r)) return exprEqPrim(r);
	return false;
}

bool exprRelPrim(Ret *r) {
	if (consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)) {
		int op = consumedTk->code;
		// GC
		Instr *lastLeft = lastInstr(owner->fn.instr);
		addRVal(&owner->fn.instr, r->lval, &r->type);
		Ret right;
		if (exprAdd(&right)) {
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst))
				tkerr("AT: invalid operand type for %s", opName(op));
			// GC
			addRVal(&owner->fn.instr, right.lval, &right.type);
			insertConvIfNeeded(lastLeft, &r->type, &tDst);
			insertConvIfNeeded(lastInstr(owner->fn.instr), &right.type, &tDst);
			switch (op) {
				case LESS:
					switch (tDst.tb) {
						case TB_INT: addInstr(&owner->fn.instr, OP_LESS_I); break;
						case TB_DOUBLE: addInstr(&owner->fn.instr, OP_LESS_F); break;
					} break;
			}
			*r = (Ret){{TB_INT, NULL, -1}, false, true};
			return exprRelPrim(r);
		} else tkerr("missing expression after %s", opName(op));
	}
	return true;
}

bool exprRel(Ret *r) {
	if (exprAdd(r)) return exprRelPrim(r);
	return false;
}

bool exprAddPrim(Ret *r) {
	if (consume(ADD) || consume(SUB)) {
		int op = consumedTk->code;
		// GC
		Instr *lastLeft = lastInstr(owner->fn.instr);
		addRVal(&owner->fn.instr, r->lval, &r->type);
		Ret right;
		if (exprMul(&right)) {
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst))
				tkerr("AT: invalid operand type for %s", opName(op));
			// GC
			addRVal(&owner->fn.instr, right.lval, &right.type);
			insertConvIfNeeded(lastLeft, &r->type, &tDst);
			insertConvIfNeeded(lastInstr(owner->fn.instr), &right.type, &tDst);
			switch (op) {
				case ADD:
					switch (tDst.tb) {
						case TB_INT: addInstr(&owner->fn.instr, OP_ADD_I); break;
						case TB_DOUBLE: addInstr(&owner->fn.instr, OP_ADD_F); break;
					} break;
				case SUB:
					switch (tDst.tb) {
						case TB_INT: addInstr(&owner->fn.instr, OP_SUB_I); break;
						case TB_DOUBLE: addInstr(&owner->fn.instr, OP_SUB_F); break;
					} break;
			}
			*r = (Ret){tDst, false, true};
			return exprAddPrim(r);
		} else tkerr("missing expression after %s", opName(op));
	}
	return true;
}

bool exprAdd(Ret *r) {
	if (exprMul(r)) return exprAddPrim(r);
	return false;
}

bool exprMulPrim(Ret *r) {
	if (consume(MUL) || consume(DIV)) {
		int op = consumedTk->code;
		// GC
		Instr *lastLeft = lastInstr(owner->fn.instr);
		addRVal(&owner->fn.instr, r->lval, &r->type);
		Ret right;
		if (exprCast(&right)) {
			Type tDst;
			if (!arithTypeTo(&r->type, &right.type, &tDst))
				tkerr("AT: invalid operand type for %s", opName(op));
			// GC
			addRVal(&owner->fn.instr, right.lval, &right.type);
			insertConvIfNeeded(lastLeft, &r->type, &tDst);
			insertConvIfNeeded(lastInstr(owner->fn.instr), &right.type, &tDst);
			switch (op) {
				case MUL:
					switch (tDst.tb) {
						case TB_INT: addInstr(&owner->fn.instr, OP_MUL_I); break;
						case TB_DOUBLE: addInstr(&owner->fn.instr, OP_MUL_F); break;
					} break;
				case DIV:
					switch (tDst.tb) {
						case TB_INT: addInstr(&owner->fn.instr, OP_DIV_I); break;
						case TB_DOUBLE: addInstr(&owner->fn.instr, OP_DIV_F); break;
					} break;
			}
			*r = (Ret){tDst, false, true};
			return exprMulPrim(r);
		} else tkerr("missing expression after %s", opName(op));
	}
	return true;
}

bool exprMul(Ret *r) {
	if (exprCast(r)) return exprMulPrim(r);
	return false;
}

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
	return true;
}

bool exprPostfix(Ret *r) {
	if (exprPrimary(r)) return exprPostfixPrim(r);
	return false;
}

bool exprPrimary(Ret *r) {
	Instr *startInstr = owner ? lastInstr(owner->fn.instr) : NULL;
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
				// GC
				addRVal(&owner->fn.instr, rArg.lval, &rArg.type);
				insertConvIfNeeded(lastInstr(owner->fn.instr), &rArg.type, &param->type);
				param = param->next;
				for (; consume(COMMA);) {
					if (!expr(&rArg)) tkerr("missing argument after , in function call");
					if (!param) tkerr("AT: too many arguments in function call");
					if (!convTo(&rArg.type, &param->type))
						tkerr("AT: in call, cannot convert the argument type to the parameter type");
					// GC
					addRVal(&owner->fn.instr, rArg.lval, &rArg.type);
					insertConvIfNeeded(lastInstr(owner->fn.instr), &rArg.type, &param->type);
					param = param->next;
				}
			}
			if (consume(RPAR)) {
				if (param) tkerr("AT: too few arguments in function call");
				// GC: call
				if (s->fn.extFnPtr) {
					addInstr(&owner->fn.instr, OP_CALL_EXT)->arg.extFnPtr = s->fn.extFnPtr;
				} else {
					addInstr(&owner->fn.instr, OP_CALL)->arg.instr = s->fn.instr;
				}
				*r = (Ret){s->type, false, true};
				return true;
			} else tkerr("missing ) in function call");
		} else {
			if (s->kind == SK_FN) tkerr("AT: a function can only be called");
			// GC: push address of variable/parameter
			if (s->kind == SK_VAR) {
				if (s->owner == NULL) {
					addInstr(&owner->fn.instr, OP_ADDR)->arg.p = s->varMem;
				} else {
					switch (s->type.tb) {
						case TB_INT: addInstrWithInt(&owner->fn.instr, OP_FPADDR_I, s->varIdx + 1); break;
						case TB_DOUBLE: addInstrWithInt(&owner->fn.instr, OP_FPADDR_F, s->varIdx + 1); break;
					}
				}
			}
			if (s->kind == SK_PARAM) {
				switch (s->type.tb) {
					case TB_INT:
						addInstrWithInt(&owner->fn.instr, OP_FPADDR_I,
							s->paramIdx - symbolsLen(s->owner->fn.params) - 1); break;
					case TB_DOUBLE:
						addInstrWithInt(&owner->fn.instr, OP_FPADDR_F,
							s->paramIdx - symbolsLen(s->owner->fn.params) - 1); break;
				}
			}
			*r = (Ret){s->type, true, s->type.n >= 0};
		}
		return true;
	}
	if (consume(INT)) {
		Token *ct = consumedTk;
		addInstrWithInt(&owner->fn.instr, OP_PUSH_I, ct->i);
		*r = (Ret){{TB_INT, NULL, -1}, false, true};
		return true;
	}
	if (consume(DOUBLE)) {
		Token *ct = consumedTk;
		addInstrWithDouble(&owner->fn.instr, OP_PUSH_F, ct->d);
		*r = (Ret){{TB_DOUBLE, NULL, -1}, false, true};
		return true;
	}
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
	if (owner) delInstrAfter(startInstr);
	return false;
}

void parse(Token *tokens) {
	iTk = tokens;
	if (!unit()) tkerr("unexpected token at global level");
}