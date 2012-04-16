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



//------------------------------------------------------------------------------
opcodeRec *nseel_translate(compileContext *ctx, int type)
{
  int v;
  int n;
  char tmp[256];
  nseel_gettoken(ctx,tmp, sizeof(tmp));

  switch (type)
  {
    case INTCONST: return nseel_createCompiledValue(ctx,(EEL_F)atoi(tmp)); // todo: this could be atof() eventually
    case DBLCONST: return nseel_createCompiledValue(ctx,(EEL_F)atof(tmp));
    case HEXCONST:
      v=0;
      n=0;
      while (1)
      {
        int a=tmp[n++];
        if (a >= '0' && a <= '9') v=(v<<4)+a-'0';
        else if (a >= 'A' && a <= 'F') v=(v<<4)+10+a-'A';
        else if (a >= 'a' && a <= 'f') v=(v<<4)+10+a-'a';
        else break;
      }
		return nseel_createCompiledValue(ctx,(EEL_F)v);
  }
  return 0;
}



//---------------------------------------------------------------------------
int nseel_yyerror(compileContext *ctx)
{
  ctx->errVar = ctx->colCount;
  return 0;
}
