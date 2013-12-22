/*
  Expression Evaluator Library (NS-EEL) v2
  Copyright (C) 2004-2013 Cockos Incorporated
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

#ifdef NSEEL_SUPER_MINIMAL_LEXER

  int nseellex(opcodeRec **output, YYLTYPE * yylloc_param, compileContext *scctx)
  {
    const char *rdptr = scctx->rdbuf;
    const char *endptr = scctx->rdbuf_end;
    int rv=0,toklen=0;
    *output = 0;

start_over:
    while (rdptr < endptr && (!(rv=rdptr[0]) || rv== ' ' || rv=='\t' || rv == '\r' || rv == '\n')) rdptr++;
    if (rdptr >= endptr) rv=0;

    if (rv == '/' && rdptr < endptr-1)
    {
      if (rdptr[1] == '/')
      {
        rdptr+=2;
        while (rdptr < endptr && *rdptr != '\n' && *rdptr != '\r') rdptr++;
        goto start_over;
      }
      else if (rdptr[1] == '*')
      {
        rdptr+=2;
        while (rdptr < endptr && (rdptr[0]  != '*' || rdptr >= endptr-1 || rdptr[1] != '/'))  rdptr++;
        if (rdptr < endptr-1) rdptr+=2; // skip */
        goto start_over;
      }
    }

    if (rdptr < endptr)
    {
      int l;
      char buf[NSEEL_MAX_VARIABLE_NAMELEN*2];
      const char *ss = rdptr++;
      if (isalpha(rv) || rv == '_')
      {
        while (rdptr < endptr && (rv=rdptr[0]) && (isalnum(rv) || rv == '_' || rv == '.')) rdptr++;
        l = rdptr - ss + 1;
        if (l > sizeof(buf)) l=sizeof(buf);
        lstrcpyn_safe(buf,ss,l);      

        rv=0;
        *output = nseel_lookup(scctx,&rv,buf);
      }
      else if ((rv >= '0' && rv <= '9') || (rv == '.' && (rdptr < endptr && rdptr[0] >= '0' && rdptr[0] <= '9')))
      {
        if (rv == '0' && rdptr < endptr && (rdptr[0] == 'x' || rdptr[0] == 'X'))
        {
          rdptr++;
          while (rdptr < endptr && (rv=rdptr[0]) && ((rv>='0' && rv<='9') || (rv>='a' && rv<='f') || (rv>='A' && rv<='F'))) rdptr++;
        }
        else
        {
          int pcnt=rv == '.';
          while (rdptr < endptr && (rv=rdptr[0]) && ((rv>='0' && rv<='9') || (rv == '.' && !pcnt++))) rdptr++;       
        }
        l = rdptr - ss + 1;
        if (l > sizeof(buf)) l=sizeof(buf);
        lstrcpyn_safe(buf,ss,l);
        *output = nseel_translate(scctx,buf);
        rv=VALUE;
      }
      else if (rdptr < endptr)
      {
        if (rv == '$')
        {
          switch (rdptr[0])
          {
            case 'x':
            case 'X':
              rdptr++;
              while (rdptr < endptr && (rv=rdptr[0]) && ((rv>='0' && rv<='9') || (rv>='a' && rv<='f') || (rv>='A' && rv<='F'))) rdptr++;
            break;
            case '~':
              rdptr++;
              while (rdptr < endptr && (rv=rdptr[0]) && (rv>='0' && rv<='9')) rdptr++;
            break;
            case '\'':
              if (rdptr+2 < endptr && rdptr[1] && rdptr[2] == '\'') rdptr += 3;
            break;
            case 'e':
            case 'E':
              rdptr++;
            break;
            case 'p':
            case 'P':
              if (rdptr+1 < endptr && toupper(rdptr[1]) == 'I') rdptr+=2;
              else if (rdptr+2 < endptr && toupper(rdptr[1]) == 'H' && toupper(rdptr[2]) == 'I') rdptr+=3;
            break;
          }
          if (rdptr != ss+1)
          {
            l = rdptr - ss + 1;
            if (l > sizeof(buf)) l=sizeof(buf);
            lstrcpyn_safe(buf,ss,l);
            *output = nseel_translate(scctx,buf);
            rv=VALUE;
          }
        }
        else if (rv == '<')
        {
          const char nc=*rdptr;
          if (nc == '<')
          {
            rdptr++;
            rv=TOKEN_SHL;
          }
          else if (nc == '=')
          {
            rdptr++;
            rv=TOKEN_LTE;
          }
        }
        else if (rv == '>')
        {
          const char nc=*rdptr;
          if (nc == '>')
          {
            rdptr++;
            rv=TOKEN_SHR;
          }
          else if (nc == '=')
          {
            rdptr++;
            rv=TOKEN_GTE;
          }
        }
        else if (rv == '&' && *rdptr == '&')
        {
          rdptr++;
          rv = TOKEN_LOGICAL_AND;
        }      
        else if (rv == '|' && *rdptr == '|')
        {
          rdptr++;
          rv = TOKEN_LOGICAL_OR;
        }
        else if (*rdptr == '=')
        {         
          switch (rv)
          {
            case '+': rv=TOKEN_ADD_OP; rdptr++; break;
            case '-': rv=TOKEN_SUB_OP; rdptr++; break;
            case '%': rv=TOKEN_MOD_OP; rdptr++; break;
            case '|': rv=TOKEN_OR_OP;  rdptr++; break;
            case '&': rv=TOKEN_AND_OP; rdptr++; break;
            case '~': rv=TOKEN_XOR_OP; rdptr++; break;
            case '/': rv=TOKEN_DIV_OP; rdptr++; break;
            case '*': rv=TOKEN_MUL_OP; rdptr++; break;
            case '^': rv=TOKEN_POW_OP; rdptr++; break;
            case '!':
              rdptr++;
              if (rdptr < endptr && *rdptr == '=')
              {
                rdptr++;
                rv=TOKEN_NE_EXACT;
              }
              else
                rv=TOKEN_NE;
            break;
            case '=':
              rdptr++;
              if (rdptr < endptr && *rdptr == '=')
              {
                rdptr++;
                rv=TOKEN_EQ_EXACT;
              }
              else
                rv=TOKEN_EQ;
            break;
          }
        }
      }
      toklen = rdptr - ss;
    }

    scctx->rdbuf = rdptr;
    yylloc_param->first_column = rdptr - scctx->rdbuf_start - toklen;
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
    const char *endptr = ctx->rdbuf_end;
    const char *rdptr = ctx->rdbuf;
    if (!rdptr) return 0;
    
    while (n < sz && rdptr < endptr) buf[n++] = *rdptr++;
    ctx->rdbuf=rdptr;
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
  }
#endif // !NSEEL_SUPER_MINIMAL_LEXER
