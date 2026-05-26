// struct S{
// 	int n;
// 	char text[16];
// 	};
//
// struct S a;
// struct S v[10];
//
// void f(char text[],int i,char ch){
// 	text[i]=ch;
// 	int x;
// 	}
//
// int h(int x,int y){
// 	if(x>0&&x<y){
// 		f(v[x].text,y,'#');
// 		return 1;
// 		}
// 	return 0;
// 	}

struct S{
    int n;
    char text[16];
};
struct S a;
struct S v[10];
int x;
char ch;
double d;

int h(int x, int y){
    return x + y;
}

void f(char text[], int i, char ch){

    // AT: the if condition must be a scalar value
    // if(v) x=1;

    // AT: the while condition must be a scalar value
    // while(v) x=1;

    // AT: a void function cannot return a value
    // return 1;

    // AT: the assign destination must be a left-value
    // 3 = x;

    // AT: the assign destination cannot be constant
    // v = v;

    // AT: the assign source cannot be converted to destination
    // x = a;

    // AT: invalid operand type for || (struct cu ||)
    // x = a || 1;

    // AT: invalid operand type for && (struct cu &&)
    // x = a && 1;

    // AT: invalid operand type for == (struct cu ==)
    // x = a == 1;

    // AT: invalid operand type for < (struct cu <)
    // x = a < 1;

    // AT: invalid operand type for + (struct cu +)
    // x = a + 1;

    // AT: invalid operand type for * (struct cu *)
    // x = a * 1;

    // AT: unary - must have a scalar operand (array cu -)
    // x = -v;

    // AT: unary ! must have a scalar operand (array cu !)
    // x = !v;

    // AT: only an array can be indexed
    // x = ch[0];

    // AT: the index is not convertible to int
    // x = text[a];

    // AT: a field can only be selected from a struct
    // x = i.n;

    // AT: the structure S does not have a field z
    // x = a.z;

    // AT: undefined id
    // x = nedefinit;

    // AT: only a function can be called
    // x();

    // AT: a function can only be called
    // x = h;

    // AT: too many arguments in function call
    // h(1, 2, 3);

    // AT: too few arguments in function call
    // h(1);

    // AT: in call, cannot convert argument type to parameter type
    // h(a, 1);

    // AT: cannot convert to a struct type
    // x = (struct S)1;

    // AT: cannot convert a struct
    // x = (int)a;

    // AT: an array can be converted only to another array
    // x = (int)v;

    // AT: a scalar can be converted only to another scalar
    // d = (double[])d;
}

int g(){
    // AT: a non-void function must return a value
    // return;

    return 0;
}