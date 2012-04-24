%option reentrant
%option prefix="nseel"
%option bison-bridge
%option bison-locations
%option noyywrap
%option yylineno

%{
#include <stdlib.h>
#include <stdio.h>

#define YY_USER_ACTION yylloc->first_line = yylineno;

#define YY_INPUT(buf,result,max_size) { (result) = nseel_gets(yyextra,(buf),max_size); }

#define YY_EXTRA_TYPE compileContext *

#include "y.tab.h"

#ifdef _WIN32
#define YY_NO_UNISTD_H
#endif

#include "ns-eel-int.h"

int nseel_gets(compileContext *ctx, char *buf, size_t sz);

#define PARSENUM *yylval = nseel_translate((compileContext*)yyextra,yytext); return VALUE;
#define EEL_ACTION(x) return x;

%}

%%

[0-9]+\.?[0-9]*               PARSENUM;
\.[0-9]+                      PARSENUM;

# // eventually: 0[xX][0-9a-fA-F]+             PARSENUM; 

[a-zA-Z_][a-zA-Z0-9\._]*      do { int toktype=IDENTIFIER; *yylval = nseel_lookup((compileContext*)yyextra,&toktype, yytext); return toktype; } while (0);

[ \t\r\n]+      /* whitespace */

.       return (int)yytext[0];

%%
