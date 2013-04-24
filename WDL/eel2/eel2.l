%option reentrant
%option prefix="nseel"
%option bison-bridge
%option bison-locations
%option noyywrap
%option yylineno
%option never-interactive
%option batch
%option nounput
%option noinput

%{
#include <stdlib.h>
#include <stdio.h>

#define YY_USER_ACTION yylloc->first_line = yylineno;

#define YY_FATAL_ERROR(msg) { ((struct yyguts_t*)yyscanner)->yyextra_r->errVar=1; }
#define YY_INPUT(buf,result,max_size) { (result) = nseel_gets(yyextra,(buf),max_size); }

#define YY_EXTRA_TYPE compileContext *

#undef YY_BUF_SIZE
#define YY_BUF_SIZE (NSEEL_MAX_VARIABLE_NAMELEN*2)

#undef YY_READ_BUF_SIZE
#define YY_READ_BUF_SIZE (NSEEL_MAX_VARIABLE_NAMELEN)

#include "y.tab.h"

#ifdef _WIN32
#define YY_NO_UNISTD_H
#endif

#include "ns-eel-int.h"

int nseel_gets(compileContext *ctx, char *buf, size_t sz);

#define PARSENUM *yylval = nseel_translate(yyextra,yytext); return VALUE;
#define EEL_ACTION(x) return x;

#ifdef stdin
#undef stdin
#endif
#define stdin (0)

#ifdef stdout
#undef stdout
#endif
#define stdout (0)

static int g_fake_errno;
#ifdef errno
#undef errno
#endif

#define errno g_fake_errno

%}

%%

[0-9]+\.?[0-9]*               PARSENUM;
\.[0-9]+                      PARSENUM;
0[xX][0-9a-fA-F]+             PARSENUM; 

[a-zA-Z_][a-zA-Z0-9\._]*      do { int toktype=IDENTIFIER; *yylval = nseel_lookup(yyextra,&toktype, yytext); return toktype; } while (0);

[ \t\r\n]+      /* whitespace */

.       return (int)yytext[0];

%%
