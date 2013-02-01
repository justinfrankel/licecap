/*
  Expression Evaluator Library (NS-EEL) v2
  Copyright (C) 2004-2013 Cockos Incorporated
  Copyright (C) 1999-2003 Nullsoft, Inc.
  
  nseel-lextab.c

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

#include "ns-eel-int.h"

#ifdef NSEEL_USE_OLD_PARSER

#define LEXSKIP		(-1)

static int _lmovb(struct lextab *lp, int c, int st)
{
        int base;
        if (c == '.' && st >= 15 && st < 30) c='_'; // if in a token, treat . as _ (should probably modify llnext instead)

        while ((base = lp->llbase[st]+c) > lp->llnxtmax ||
                        (lp->llcheck[base] & 0377) != st) {

                if (st != lp->llendst) {
                        base = lp->lldefault[st] & 0377;
                        st = base;
                }
                else
                        return(-1);
        }
        return(lp->llnext[base]&0377);
}

static int _Alextab(compileContext *ctx, int __na__)		 
{
	// fucko: JF> 17 -> 19?
  
   if (__na__ >= 0 && __na__ <= 17) 
   {     
     ctx->colCount+=nseel_gettokenlen(ctx,256);
   }
   switch (__na__)
   {
      case 0:           
        {
          char tmp[NSEEL_MAX_VARIABLE_NAMELEN*2];
          nseel_gettoken(ctx,tmp, sizeof(tmp));
          if (tmp[0] < '0' || tmp[0] > '9') // not sure where this logic came from
          {
            ctx->yylval = nseel_lookup(ctx,&__na__,tmp); 
            return __na__;
          }
        }
      case 1:
      case 2:
      case 3:
        {
          char tmp[NSEEL_MAX_VARIABLE_NAMELEN*2];
          nseel_gettoken(ctx,tmp, sizeof(tmp));
          ctx->yylval = nseel_translate(ctx,tmp); 
          return VALUE; 
        }
      case 4:
      case 5:   
        {
          char tmp[NSEEL_MAX_VARIABLE_NAMELEN*2];
          nseel_gettoken(ctx,tmp, sizeof(tmp));
          ctx->yylval = nseel_lookup(ctx,&__na__,tmp); 
          return __na__;
        }
      case 6:   return '+';
      case 7:   return '-';
      case 8:   return '*'; 
      case 9:   return '/'; 
      case 10:  return '%'; 
      case 11:  return '&'; 
      case 12:  return '|'; 
      case 13:  return '('; 
      case 14:  return ')'; 
      case 15:  return '='; 
      case 16:  return ','; 
      case 17:  return ';'; 
	}
	return (LEXSKIP);
}


static char _Flextab[] =
   {
   1, 18, 17, 16, 15, 14, 13, 12,
   11, 10, 9, 8, 7, 6, 4, 5,
   5, 4, 4, 3, 3, 3, 3, 4,
   0, 4, 5, 0, 5, 4, 1, 3,
   0, 2, -1, 1, -1,
   };


static char _Nlextab[] =
   {
   36, 36, 36, 36, 36, 36, 36, 36,
   36, 1, 36, 36, 36, 36, 36, 36,
   36, 36, 36, 36, 36, 36, 36, 36,
   36, 36, 36, 36, 36, 36, 36, 36,
   1, 36, 36, 36, 36, 9, 8, 36,
   6, 5, 11, 13, 3, 12, 19, 10,
   30, 30, 30, 30, 30, 30, 30, 30,
   30, 30, 36, 2, 36, 4, 36, 36,
   36, 29, 29, 29, 29, 29, 29, 18,
   18, 18, 18, 18, 18, 18, 18, 18,
   18, 18, 18, 18, 18, 18, 18, 18,
   18, 18, 18, 36, 36, 36, 36, 18,
   36, 29, 29, 29, 29, 29, 23, 18,
   18, 18, 18, 18, 18, 18, 18, 18,
   18, 18, 18, 18, 18, 18, 14, 18,
   18, 18, 18, 36, 7, 15, 15, 15,
   15, 15, 15, 15, 15, 15, 15, 36,
   36, 36, 36, 36, 36, 36, 17, 17,
   17, 17, 17, 17, 17, 17, 17, 17,
   17, 17, 17, 17, 17, 17, 17, 17,
   17, 17, 17, 17, 17, 17, 17, 17,
   36, 36, 36, 36, 17, 36, 17, 17,
   17, 17, 17, 17, 17, 17, 17, 17,
   17, 17, 17, 17, 17, 17, 17, 17,
   17, 17, 17, 17, 17, 17, 17, 17,
   15, 15, 15, 15, 15, 15, 15, 15,
   15, 15, 36, 36, 36, 36, 36, 36,
   36, 16, 16, 16, 16, 16, 16, 16,
   16, 16, 16, 16, 16, 16, 16, 16,
   16, 16, 16, 16, 16, 16, 16, 16,
   16, 16, 16, 36, 36, 36, 36, 16,
   36, 16, 16, 16, 16, 16, 16, 16,
   16, 16, 16, 16, 16, 16, 16, 16,
   16, 16, 16, 16, 16, 16, 16, 16,
   16, 16, 16, 22, 22, 22, 22, 22,
   22, 22, 22, 22, 22, 21, 21, 21,
   21, 21, 21, 21, 21, 21, 21, 36,
   20, 26, 26, 26, 26, 26, 26, 26,
   26, 26, 26, 36, 36, 36, 36, 36,
   36, 36, 25, 25, 25, 25, 25, 25,
   36, 24, 36, 36, 36, 36, 36, 36,
   20, 36, 36, 36, 36, 36, 36, 36,
   36, 36, 36, 36, 36, 36, 36, 36,
   36, 36, 25, 25, 25, 25, 25, 25,
   36, 24, 26, 26, 26, 26, 26, 26,
   26, 26, 26, 26, 36, 36, 36, 36,
   36, 36, 36, 28, 28, 28, 28, 28,
   28, 36, 27, 36, 36, 36, 36, 36,
   36, 36, 36, 36, 36, 36, 36, 36,
   36, 36, 36, 36, 36, 36, 36, 36,
   36, 36, 36, 28, 28, 28, 28, 28,
   28, 31, 27, 35, 35, 35, 35, 35,
   35, 35, 35, 35, 35, 36, 36, 36,
   36, 36, 36, 36, 34, 34, 34, 33,
   34, 34, 36, 32, 36, 36, 36, 36,
   36, 36, 36, 36, 36, 36, 36, 36,
   36, 36, 36, 36, 36, 36, 36, 36,
   36, 36, 36, 36, 34, 34, 34, 33,
   34, 34, 36, 32, 34, 34, 34, 34,
   34, 34, 34, 34, 34, 34, 36, 36,
   36, 36, 36, 36, 36, 34, 34, 34,
   34, 34, 34, 36, 32, 36, 36, 36,
   36, 36, 36, 36, 36, 36, 36, 36,
   36, 36, 36, 36, 36, 36, 36, 36,
   36, 36, 36, 36, 36, 34, 34, 34,
   34, 34, 34, 36, 32,
   };

static char _Clextab[] =
   {
   -1, -1, -1, -1, -1, -1, -1, -1,
   -1, 0, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1,
   0, -1, -1, -1, -1, 0, 0, -1,
   0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, -1, 0, -1, 0, -1, -1,
   -1, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, -1, -1, -1, -1, 0,
   -1, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, -1, 0, 14, 14, 14,
   14, 14, 14, 14, 14, 14, 14, -1,
   -1, -1, -1, -1, -1, -1, 14, 14,
   14, 14, 14, 14, 14, 14, 14, 14,
   14, 14, 14, 14, 14, 14, 14, 14,
   14, 14, 14, 14, 14, 14, 14, 14,
   -1, -1, -1, -1, 14, -1, 14, 14,
   14, 14, 14, 14, 14, 14, 14, 14,
   14, 14, 14, 14, 14, 14, 14, 14,
   14, 14, 14, 14, 14, 14, 14, 14,
   15, 15, 15, 15, 15, 15, 15, 15,
   15, 15, -1, -1, -1, -1, -1, -1,
   -1, 15, 15, 15, 15, 15, 15, 15,
   15, 15, 15, 15, 15, 15, 15, 15,
   15, 15, 15, 15, 15, 15, 15, 15,
   15, 15, 15, -1, -1, -1, -1, 15,
   -1, 15, 15, 15, 15, 15, 15, 15,
   15, 15, 15, 15, 15, 15, 15, 15,
   15, 15, 15, 15, 15, 15, 15, 15,
   15, 15, 15, 19, 19, 19, 19, 19,
   19, 19, 19, 19, 19, 20, 20, 20,
   20, 20, 20, 20, 20, 20, 20, -1,
   19, 23, 23, 23, 23, 23, 23, 23,
   23, 23, 23, -1, -1, -1, -1, -1,
   -1, -1, 23, 23, 23, 23, 23, 23,
   -1, 23, -1, -1, -1, -1, -1, -1,
   19, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, 23, 23, 23, 23, 23, 23,
   -1, 23, 26, 26, 26, 26, 26, 26,
   26, 26, 26, 26, -1, -1, -1, -1,
   -1, -1, -1, 26, 26, 26, 26, 26,
   26, -1, 26, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, 26, 26, 26, 26, 26,
   26, 30, 26, 30, 30, 30, 30, 30,
   30, 30, 30, 30, 30, -1, -1, -1,
   -1, -1, -1, -1, 30, 30, 30, 30,
   30, 30, -1, 30, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, 30, 30, 30, 30,
   30, 30, -1, 30, 33, 33, 33, 33,
   33, 33, 33, 33, 33, 33, -1, -1,
   -1, -1, -1, -1, -1, 33, 33, 33,
   33, 33, 33, -1, 33, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, 33, 33, 33,
   33, 33, 33, -1, 33,
   };

static char _Dlextab[] =
   {
   36, 36, 36, 36, 36, 36, 36, 36,
   36, 36, 36, 36, 36, 36, 36, 36,
   15, 14, 14, 36, 36, 20, 19, 14,
   14, 23, 15, 15, 26, 23, 36, 19,
   36, 36, 33, 30,
   };

static int _Blextab[] =
   {
   0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 77, 152,
   0, 0, 0, 227, 237, 0, 0, 249,
   0, 0, 306, 0, 0, 0, 363, 0,
   0, 420, 0, 0, 0,
   };

struct lextab nseel_lextab =	{
			36,		 
			_Dlextab,	 
			_Nlextab,	 
			_Clextab,	 
			_Blextab,	 
			524,		 
			_lmovb,		 
			_Flextab,	 
			_Alextab,	 

			0,   	 
			0,		 
			0,		 
			0,		 
			};

 

#endif