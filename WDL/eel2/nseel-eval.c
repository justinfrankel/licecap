/*
  Expression Evaluator Library (NS-EEL) v2
  Copyright (C) 2004-2008 Cockos Incorporated
  Copyright (C) 1999-2003 Nullsoft, Inc.
  
  nseel-eval.c

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include <string.h>
#include <ctype.h>
#include "ns-eel-int.h"
#include "../wdlcstring.h"

#ifndef NSEEL_USE_OLD_PARSER

  #ifdef NSEEL_SUPER_MINIMAL_LEXER

  int nseellex(opcodeRec **output, YYLTYPE * yylloc_param, compileContext *scctx)
  {
    int rv,toklen=0;
    *output = 0;
    while ((rv=scctx->rdbuf[0]) && (rv== ' ' || rv=='\t' || rv == '\r' || rv == '\n')) scctx->rdbuf++;

    if (rv)
    {
      char buf[NSEEL_MAX_VARIABLE_NAMELEN*2];
      int l;
      char *ss = scctx->rdbuf++;
      if (isalpha(rv) || rv == '_')
      {
        while ((rv=scctx->rdbuf[0]) && (isalnum(rv) || rv == '_' || rv == '.')) scctx->rdbuf++;
        l = scctx->rdbuf - ss + 1;
        if (l > sizeof(buf)) l=sizeof(buf);
        lstrcpyn_safe(buf,ss,l);      

        rv=0;
        *output = nseel_lookup(scctx,&rv,buf);
      }
      else if ((rv >= '0' && rv <= '9') || (rv == '.' && (scctx->rdbuf[0] >= '0' && scctx->rdbuf[0] <= '9')))
      {
        if (rv == '0' && (scctx->rdbuf[0] == 'x' || scctx->rdbuf[0] == 'X'))
        {
          scctx->rdbuf++;
          while ((rv=scctx->rdbuf[0]) && ((rv>='0' && rv<='9') || (rv>='a' && rv<='f') || (rv>='A' && rv<='F'))) scctx->rdbuf++;
          // this allows 0x, whereas the lex version will not parse that as a token
        }
        else
        {
          int pcnt=rv == '.';
          while ((rv=scctx->rdbuf[0]) && ((rv>='0' && rv<='9') || (rv == '.' && !pcnt++))) scctx->rdbuf++;       
        }
        l = scctx->rdbuf - ss + 1;
        if (l > sizeof(buf)) l=sizeof(buf);
        lstrcpyn_safe(buf,ss,l);
        *output = nseel_translate(scctx,buf);
        rv=VALUE;
      }
      toklen = scctx->rdbuf - ss;
    }
  
    yylloc_param->first_column = scctx->rdbuf - scctx->rdbuf_start - toklen;
    return rv;
  }
  void nseelerror(YYLTYPE *pos,compileContext *ctx, const char *str)
  {
    ctx->errVar=pos->first_column>0?pos->first_column:1;
  }
#else

  int nseel_gets(compileContext *ctx, char *buf, size_t sz)
  {
    int n=0;
    if (ctx->inputbufferptr) while (n < sz)
    {
      char c=ctx->inputbufferptr[0];
      if (!c) break;
      if (c == '/' && ctx->inputbufferptr[1] == '*')
      {
        ctx->inputbufferptr+=2; // skip /*

        while (ctx->inputbufferptr[0] && (ctx->inputbufferptr[0]  != '*' || ctx->inputbufferptr[1] != '/'))  ctx->inputbufferptr++;
        if (ctx->inputbufferptr[0]) ctx->inputbufferptr+=2; // skip */
        continue;
      }

      ctx->inputbufferptr++;
      buf[n++] = c;
    }
    return n;

  }


  //#define EEL_TRACE_LEX

  #ifdef EEL_TRACE_LEX
  #define nseellex nseellex2

  #endif
  #include "lex.nseel.c"

  #ifdef EEL_TRACE_LEX

  #undef nseellex

  int nseellex(YYSTYPE * yylval_param, YYLTYPE * yylloc_param , yyscan_t yyscanner)
  {
    int a=nseellex2(yylval_param,yylloc_param,yyscanner);

    char buf[512];
    sprintf(buf,"tok: %c (%d)\n",a,a);
    OutputDebugString(buf);
    return a;
  }
  #endif//EEL_TRACE_LEX


  void nseelerror(YYLTYPE *pos,compileContext *ctx, const char *str)
  {
    ctx->errVar=pos->first_column>0?pos->first_column:1;
    ctx->errVar_l = pos->first_line;
  }
#endif // !NSEEL_SUPER_MINIMAL_LEXER

#else

//---------------------------------------------------------------------------
int nseel_yyerror(compileContext *ctx)
{
  ctx->errVar = ctx->colCount;
  return 0;
}


#endif