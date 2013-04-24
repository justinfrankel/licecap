%pure-parser
%name-prefix="nseel"
%parse-param { compileContext* context }
%lex-param { void* scanner  }


/* this will prevent y.tab.c from ever calling yydestruct(), since we do not use it and it is a waste */
%destructor {
 #define yydestruct(a,b,c,d,e) 0
} VALUE


%{
#ifdef _WIN32
#include <windows.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "y.tab.h"
#include "ns-eel-int.h"
  
#define scanner context->scanner
#define YY_(x) ("")

%}

%token VALUE IDENTIFIER FUNCTION1 FUNCTION2 FUNCTION3 FUNCTIONX


%start program

%%

more_params:
	expression
	| expression ',' more_params
	{
	  $$ = nseel_createMoreParametersOpcode(context,$1,$3);
	}
	;

value_thing:
	VALUE
	| IDENTIFIER
	| '(' expression ')'
	{
	  $$ = $2;
	}
	| FUNCTION1 '(' expression ')'
	{
  	  $$ = nseel_setCompiledFunctionCallParameters($1, $3, 0, 0);
	}
	| FUNCTION2 '(' expression ',' expression ')'
	{
  	  $$ = nseel_setCompiledFunctionCallParameters($1, $3, $5, 0);
	}
	| FUNCTION3 '(' expression ',' expression ',' expression ')' 
	{
  	  $$ = nseel_setCompiledFunctionCallParameters($1, $3, $5, $7);
	}
	| FUNCTIONX '(' expression ',' expression ',' more_params ')' 
	{
  	  $$ = nseel_setCompiledFunctionCallParameters($1, $3, $5, $7);
	}
	;

unary_expr:
	value_thing
	| '+' unary_expr
	{
	  $$ = $2;
	}
	| '-' unary_expr
	{
	  $$ = nseel_createSimpleCompiledFunction(context,FN_UMINUS,1,$2,0);
	}
	;


div_expr:
	unary_expr
	| div_expr '/' unary_expr
	{
	  $$ = nseel_createSimpleCompiledFunction(context,FN_DIVIDE,2,$1,$3);
	}
	;


mul_expr:
	div_expr
	| mul_expr '*' div_expr
	{
	  $$ = nseel_createSimpleCompiledFunction(context,FN_MULTIPLY,2,$1,$3);
	}
	;


sub_expr:
	mul_expr
	| sub_expr '-' mul_expr
	{
	  $$ = nseel_createSimpleCompiledFunction(context,FN_SUB,2,$1,$3);
	}
	;

add_expr:
	sub_expr
	| add_expr '+' sub_expr
	{
	  $$ = nseel_createSimpleCompiledFunction(context,FN_ADD,2,$1,$3);
	}
	;

andor_expr:
	add_expr
	| andor_expr '&' add_expr
	{
	  $$ = nseel_createSimpleCompiledFunction(context,FN_AND,2,$1,$3);
	}
	| andor_expr '|' add_expr
	{
	  $$ = nseel_createSimpleCompiledFunction(context,FN_OR,2,$1,$3);
	}
	;

expression: 
	andor_expr
	| expression '%' andor_expr
	{
	  $$ = nseel_createSimpleCompiledFunction(context,FN_JOIN_STATEMENTS,2,$1,$3);
	}
	;


program:
	expression
	{ 
                int a = @1.first_line;
                context->result = $1;
	}
	;


%%
