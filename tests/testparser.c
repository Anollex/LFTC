struct Pt{
	int x;
	int y;
	};
struct Pt points[10];

double max(double a,double b){
	if(a>=b)return a;
		else return b;
	}

int len(char s[]){
	int i;
	i=0;
	while(s[i])i=i+1;
	return i;
	}

void main(){
	int i;
	i=10;
	while(i!=0){
		puti(i);
		i=i/2;
		}
	}


// ============================================================
// ERROR TESTS - uncomment ONE at a time
// Each line shows: error trigger -> expected error message
// ============================================================

// --- structDef errors ---
// ERR01: missing ; after struct -> "missing ; after struct definition"
// struct Err1{ int x; }

// ERR02: missing } in struct -> "invalid declaration or missing } in struct definition"
// struct Err2{ int x; int y;

// ERR03: invalid content in struct -> "invalid declaration or missing } in struct definition"
// struct Err3{ 123; };

// --- varDef errors ---
// ERR04: missing ; after var -> "missing ; after variable declaration"
// int noSemicolon

// ERR05: missing identifier after type -> "missing identifier after type (...)"
// int ;

// ERR06: struct without { (ambiguous) -> "missing identifier after type (...)"
// struct Pt }

// --- arrayDecl errors ---
// ERR07: missing ] in array -> "missing ] in array declaration"
// int arr2[10;

// --- fnDef errors ---
// ERR08: missing function body -> "missing function body"
// void noBody()

// ERR09: missing ) in function decl -> "invalid parameter or missing ) in function declaration"
// void badFn(int a, int b {  }

// ERR10: missing parameter after , -> "missing parameter after ,"
// void badParams(int a, ){}

// ERR11: missing parameter name -> "missing parameter name"
// void badParam(int ){}

// --- stm errors (IF) ---
// ERR12: missing ( after if -> "missing ( after if"
// void errIf1(){ if x > 0; }

// ERR13: missing if condition -> "missing if condition"
// void errIf2(){ if(); }

// ERR14: invalid if condition or missing ) -> "invalid if condition or missing )"
// void errIf3(){ if(x > 0 x = 1; }

// ERR15: missing statement after if -> "missing statement after if"
// void errIf4(){ if(1) }

// ERR16: missing statement after else -> "missing statement after else"
// void errIf5(){ if(1) i=0; else }

// --- stm errors (WHILE) ---
// ERR17: missing ( after while -> "missing ( after while"
// void errW1(){ while x > 0; }

// ERR18: missing while condition -> "missing while condition"
// void errW2(){ while(); }

// ERR19: invalid while condition or missing ) -> "invalid while condition or missing )"
// void errW3(){ while(x > 0 x = 1; }

// ERR20: missing statement after while -> "missing statement after while"
// void errW4(){ while(1) }

// --- stm errors (RETURN) ---
// ERR21: missing ; after return -> "missing ; after return"
// int errRet(){ return 0 }

// --- stm errors (expression) ---
// ERR22: missing ; after expression -> "missing ; after expression"
// void errExpr(){ x + 1 }

// --- stmCompound errors ---
// ERR23: invalid statement or missing } -> "invalid statement or missing }"
// void errBlock(){ int x; x = 1;

// --- exprPrimary errors ---
// ERR24: missing ) in function call -> "missing ) in function call"
// void errCall(){ max(1.0, 2.0; }

// ERR25: missing argument after , -> "missing argument after , in function call"
// void errArg(){ max(1.0, ); }

// ERR26: missing ) after expression -> "missing ) after expression"
// void errParen(){ int x; x = (1 + 2; }

// --- exprPostfix errors ---
// ERR27: missing ] after array index -> "missing ] after array index"
// void errIdx(){ int x; x = arr[0; }

// ERR28: missing expression in array index -> "missing expression in array index"
// void errIdx2(){ int x; x = arr[]; }

// ERR29: missing field name after . -> "missing field name after ."
// void errDot(){ int x; x = points[0].; }

// --- exprCast errors ---
// ERR30: missing ) after cast type -> "missing ) after cast type"
// void errCast(){ int x; x = (int d; }

// ERR31: missing expression after cast -> "missing expression after cast"
// void errCast2(){ int x; x = (int); }

// --- exprUnary errors ---
// ERR32: missing expression after - -> "missing expression after -"
// void errUn1(){ int x; x = -; }

// ERR33: missing expression after ! -> "missing expression after !"
// void errUn2(){ int x; x = !; }

// --- exprBinary errors (operator Prim functions) ---
// ERR34: missing expression after + -> "missing expression after +"
// void errAdd(){ int x; x = 1 +; }

// ERR35: missing expression after - -> "missing expression after -"
// void errSub(){ int x; x = 1 -; }

// ERR36: missing expression after * -> "missing expression after *"
// void errMul(){ int x; x = 1 *; }

// ERR37: missing expression after / -> "missing expression after /"
// void errDiv(){ int x; x = 1 /; }

// ERR38: missing expression after < -> "missing expression after <"
// void errLt(){ int x; x = 1 <; }

// ERR39: missing expression after <= -> "missing expression after <="
// void errLe(){ int x; x = 1 <=; }

// ERR40: missing expression after > -> "missing expression after >"
// void errGt(){ int x; x = 1 >; }

// ERR41: missing expression after >= -> "missing expression after >="
// void errGe(){ int x; x = 1 >=; }

// ERR42: missing expression after == -> "missing expression after =="
// void errEq(){ int x; x = 1 ==; }

// ERR43: missing expression after != -> "missing expression after !="
// void errNe(){ int x; x = 1 !=; }

// ERR44: missing expression after && -> "missing expression after &&"
// void errAnd(){ int x; x = 1 &&; }

// ERR45: missing expression after || -> "missing expression after ||"
// void errOr(){ int x; x = 1 ||; }