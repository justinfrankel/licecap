/*
  Expression Evaluator Library (NS-EEL)
  Copyright (C) 2004-2013 Cockos Incorporated
  Copyright (C) 1999-2003 Nullsoft, Inc.
  
  nseel-yylex.c

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


#ifndef NSEEL_USE_OLD_PARSER

#   define YYMALLOC malloc
#   define YYFREE free

int nseellex(void * yylval_param,void * yylloc_param ,void *yyscanner);
void nseelerror(void *pos,compileContext *ctx, const char *str);

#include <stdlib.h>
#include <string.h>

#include "y.tab.c"

#else

#define NBPW		 16
#define EOF			(-1)


#define YYERRORVAL   256     /* yacc's value */

static int llset(compileContext *ctx);
static int llinp(compileContext *ctx, char **exp);
static int lexgetc(char **exp)
{
  char c= **exp;
  if (c) (*exp)++;
  return( c != 0 ? c : -1);
}
static int tst__b(register int c, char tab[])
{
  return (tab[(c >> 3) & 037] & (1 << (c & 07)) );
}


int nseel_gettokenlen(compileContext *ctx, int lltbsiz)
{
  char *lp;
  int tp=0;
  for (lp = ctx->llbuf; lp < ctx->llend && tp < lltbsiz; tp++, lp++);
  return tp;

}

int nseel_gettoken(compileContext *ctx, char *lltb, int lltbsiz)
{
  char *lp, *tp, *ep;

  tp = lltb;
  ep = tp+lltbsiz-1;
  for (lp = ctx->llbuf; lp < ctx->llend && tp < ep;) *tp++ = *lp++;
  *tp = 0;
  return(tp-lltb);
}


int nseel_yylex(compileContext *ctx, char **exp)
{
  register int c, st;
  int final, l, llk, i;
  register struct lextab *lp;
  char *cp;

  while (1)
  {
    llk = 0;
    if (llset(ctx)) return(0);
    st = 0;
    final = -1;
    lp = &nseel_lextab;

    do {
            if (lp->lllook && (l = lp->lllook[st])) {
                    for (c=0; c<NBPW; c++)
                            if (l&(1<<c))
                                    ctx->llsave[c] = ctx->llp1;
                    llk++;
            }
            if ((i = lp->llfinal[st]) != -1) {
                    final = i;
                    ctx->llend = ctx->llp1;
            }
            if ((c = llinp(ctx,exp)) < 0)
                    break;
            if ((cp = lp->llbrk) && llk==0 && tst__b(c, cp)) {
                    ctx->llp1--;
                    break;
            }
    } while ((st = (*lp->llmove)(lp, c, st)) != -1);


    if (ctx->llp2 < ctx->llp1)
            ctx->llp2 = ctx->llp1;
    if (final == -1) {
            ctx->llend = ctx->llp1;
            if (st == 0 && c < 0)
                    return(0);
            if ((cp = lp->llill) && tst__b(c, cp)) {
                    continue;
            }
            return(YYERRORVAL);
    }
    if ((c = (final >> 11) & 037))
            ctx->llend = ctx->llsave[c-1];
    if ((c = (*lp->llactr)(ctx,final&03777)) >= 0)
            return(c);
  }
}

void nseel_llinit(compileContext *ctx)
{
   ctx->llp1 = ctx->llp2 = ctx->llend = ctx->llbuf;
   ctx->llebuf = ctx->llbuf + sizeof(ctx->llbuf);
   ctx->lleof = 0;
}


static int llinp(compileContext *ctx, char **exp)
{
        register int c;
        register struct lextab *lp;
        register char *cp;

        lp = &nseel_lextab;
        cp = lp->llign;                         /* Ignore class         */
        for (;;) {
                /*
                 * Get the next character from the save buffer (if possible)
                 * If the save buffer's empty, then return EOF or the next
                 * input character.  Ignore the character if it's in the
                 * ignore class.
                 */
                c = (ctx->llp1 < ctx->llp2) ? *ctx->llp1 & 0377 : (ctx->lleof) ? EOF : lexgetc(exp);
                if (c >= 0) {                   /* Got a character?     */
                        if (cp && tst__b(c, cp))
                                continue;       /* Ignore it            */
                        if (ctx->llp1 >= ctx->llebuf) {   /* No, is there room?   */
                                return -1;
                        }
                        *ctx->llp1++ = c;            /* Store in token buff  */
                } else
                        ctx->lleof = 1;              /* Set EOF signal       */
                return(c);
        }
}

static int llset(compileContext *ctx)
/*
 * Return TRUE if EOF and nothing was moved in the look-ahead buffer
 */
{
        register char *lp1, *lp2;

        for (lp1 = ctx->llbuf, lp2 = ctx->llend; lp2 < ctx->llp2;)
                *lp1++ = *lp2++;
        ctx->llend = ctx->llp1 = ctx->llbuf;
        ctx->llp2 = lp1;
        return(ctx->lleof && lp1 == ctx->llbuf);
}

#endif