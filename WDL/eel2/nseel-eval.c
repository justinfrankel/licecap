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

#ifndef NSEEL_USE_OLD_PARSER


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


#include "lex.nseel.c"

void nseelerror(YYLTYPE *pos,compileContext *ctx, const char *str)
{
  ctx->errVar=pos->first_column>0?pos->first_column:1;
  ctx->errVar_l = pos->first_line;
}

#else

//---------------------------------------------------------------------------
int nseel_yyerror(compileContext *ctx)
{
  ctx->errVar = ctx->colCount;
  return 0;
}


#endif