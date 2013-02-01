/*
  Expression Evaluator Library (NS-EEL) v2
  Copyright (C) 2004-2013 Cockos Incorporated
  Copyright (C) 1999-2003 Nullsoft, Inc.
  
  nseel-caltab.c

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

#define YYERROR(x) nseel_yyerror(ctx)
       
#define	YYFINAL		51
#define	YYFLAG		-32768
#define	YYNTBASE	21

#define YYTRANSLATE(x) ((unsigned)(x) <= 264 ? yytranslate[x] : 26)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,    14,     9,     2,    18,
    19,    12,    10,    20,    11,     2,    13,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
    17,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     8,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,    15,    16
};


static const short yyr1[] = {     0,
    21,    21,    22,    23,    23,    23,    24,    24,    24,    24,
    24,    24,    24,    24,    24,    24,    24,    25,    25,    25
};

static const short yyr2[] = {     0,
     1,     3,     1,     1,     1,     3,     1,     3,     3,     3,
     3,     3,     3,     3,     2,     2,     1,     4,     6,     8
};

static const short yydefact[] = {     0,
     3,     4,     0,     0,     0,     0,     0,     0,     5,     7,
     1,    17,     0,     0,     0,     0,     4,    16,    15,     0,
     0,     0,     0,     0,     0,     0,     0,     2,     0,     0,
     0,     6,    14,    13,    11,    12,     8,     9,    10,    18,
     0,     0,     0,     0,    19,     0,     0,    20,     0,     0,
     0
};

static const short yydefgoto[] = {    49,
     9,    10,    11,    12
};

static const short yypact[] = {    19,
-32768,   -11,    -7,    -5,    -4,    38,    38,    38,-32768,-32768,
   136,-32768,    38,    38,    38,    38,-32768,-32768,-32768,    88,
    38,    38,    38,    38,    38,    38,    38,   136,   100,    49,
    62,-32768,    41,    54,    -9,    -9,-32768,-32768,-32768,-32768,
    38,    38,   112,    75,-32768,    38,   124,-32768,    12,    27,
-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,    -6,-32768
};


#define	YYLAST		150


static const short yytable[] = {    18,
    19,    20,    25,    26,    27,    13,    28,    29,    30,    31,
    14,    50,    15,    16,    33,    34,    35,    36,    37,    38,
    39,     1,     2,     3,     4,     5,    51,     0,     6,     7,
     0,     0,     0,     0,    43,    44,     8,     0,     0,    47,
     1,    17,     3,     4,     5,     0,     0,     6,     7,    22,
    23,    24,    25,    26,    27,     8,    21,    22,    23,    24,
    25,    26,    27,    23,    24,    25,    26,    27,    41,    21,
    22,    23,    24,    25,    26,    27,     0,     0,     0,     0,
     0,    42,    21,    22,    23,    24,    25,    26,    27,     0,
     0,     0,     0,     0,    46,    21,    22,    23,    24,    25,
    26,    27,     0,     0,     0,     0,    32,    21,    22,    23,
    24,    25,    26,    27,     0,     0,     0,     0,    40,    21,
    22,    23,    24,    25,    26,    27,     0,     0,     0,     0,
    45,    21,    22,    23,    24,    25,    26,    27,     0,     0,
     0,     0,    48,    21,    22,    23,    24,    25,    26,    27
};

static const short yycheck[] = {     6,
     7,     8,    12,    13,    14,    17,    13,    14,    15,    16,
    18,     0,    18,    18,    21,    22,    23,    24,    25,    26,
    27,     3,     4,     5,     6,     7,     0,    -1,    10,    11,
    -1,    -1,    -1,    -1,    41,    42,    18,    -1,    -1,    46,
     3,     4,     5,     6,     7,    -1,    -1,    10,    11,     9,
    10,    11,    12,    13,    14,    18,     8,     9,    10,    11,
    12,    13,    14,    10,    11,    12,    13,    14,    20,     8,
     9,    10,    11,    12,    13,    14,    -1,    -1,    -1,    -1,
    -1,    20,     8,     9,    10,    11,    12,    13,    14,    -1,
    -1,    -1,    -1,    -1,    20,     8,     9,    10,    11,    12,
    13,    14,    -1,    -1,    -1,    -1,    19,     8,     9,    10,
    11,    12,    13,    14,    -1,    -1,    -1,    -1,    19,     8,
     9,    10,    11,    12,    13,    14,    -1,    -1,    -1,    -1,
    19,     8,     9,    10,    11,    12,    13,    14,    -1,    -1,
    -1,    -1,    19,     8,     9,    10,    11,    12,    13,    14
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(ctx->yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	return(0)
#define YYABORT 	return(1)

#define YYTERROR	1
#define YYERRCODE	256

#define YYLEX		nseel_yylex(ctx,&exp)

/* If nonreentrant, generate the variables here */

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#define YYINITDEPTH 5000
#define YYMAXDEPTH 5000

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
#define __yy_bcopy(from,to,count) memcpy(to,from,(count)>0?(count):0)

//#ln 131 "bison.simple"
int nseel_yyparse(compileContext *ctx, char *exp)
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

  int yystacksize = YYINITDEPTH;

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

  ctx->yylval = 0;
  yystate = 0;
  yyerrstatus = 0;
  ctx->yynerrs = 0;
  ctx->yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
//      YYSTYPE *yyvs1 = yyvs;
  //    short *yyss1 = yyss;

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

      if (yystacksize >= YYMAXDEPTH)
    	{
	      YYERROR("internal error: parser stack overflow");
	      return 2;
	    }

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;


      if (yyssp >= yyss + yystacksize - 1) YYABORT;
    }


// yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (ctx->yychar == YYEMPTY)
    {
//	yyStackSize = yyssp - (yyss - 1);
	ctx->yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (ctx->yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      ctx->yychar = YYEOF;		/* Don't call YYLEX any more */

    }
  else
    {
      yychar1 = YYTRANSLATE(ctx->yychar);

    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */


  /* Discard the token being shifted unless it is eof.  */
  if (ctx->yychar != YYEOF)
    ctx->yychar = YYEMPTY;

  *++yyvsp = ctx->yylval;

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  yyval = yyvsp[1-yylen]; /* implement default value of the action */


  switch (yyn) {

case 1:
//#ln 32 "cal.y"
{       yyval = yyvsp[0]; ctx->result = yyvsp[0];     ;
    break;}
case 2:
//#ln 34 "cal.y"
{                                {
                                 ctx->result = yyval = yyvsp[0]; // unused!
                                 }
                       ;
    break;}
case 3:
//#ln 50 "cal.y"
{ yyval = yyvsp[0] ;
    break;}
case 4:
//#ln 55 "cal.y"
{       yyval = yyvsp[0];;
    break;}
case 5:
//#ln 57 "cal.y"
{       yyval = yyvsp[0];;
    break;}
case 6:
//#ln 59 "cal.y"
{       yyval = yyvsp[-1];;
    break;}
case 7:
//#ln 64 "cal.y"
{ yyval = yyvsp[0];              ;
    break;}
case 8:
//#ln 66 "cal.y"
{                                 yyval = nseel_createSimpleCompiledFunction(ctx,FN_MULTIPLY, 2, yyvsp[-2], yyvsp[0]);
    break;}
case 9:
//#ln 72 "cal.y"
{                                 yyval = nseel_createSimpleCompiledFunction(ctx,FN_DIVIDE, 2, yyvsp[-2], yyvsp[0]);
    break;}
case 10:
//#ln 78 "cal.y"
{                                  yyval = nseel_createSimpleCompiledFunction(ctx,FN_JOIN_STATEMENTS, 2, yyvsp[-2], yyvsp[0]);
    break;}
case 11:
//#ln 84 "cal.y"
{                                 yyval = nseel_createSimpleCompiledFunction(ctx,FN_ADD, 2, yyvsp[-2], yyvsp[0]);
    break;}
case 12:
//#ln 90 "cal.y"
{                                 yyval = nseel_createSimpleCompiledFunction(ctx,FN_SUB, 2, yyvsp[-2], yyvsp[0]);
    break;}
case 13:
//#ln 96 "cal.y"
{                                  yyval = nseel_createSimpleCompiledFunction(ctx,FN_AND, 2, yyvsp[-2], yyvsp[0]);
    break;}
case 14:
//#ln 102 "cal.y"
{                                  yyval = nseel_createSimpleCompiledFunction(ctx,FN_OR, 2, yyvsp[-2], yyvsp[0]);
    break;}
case 15:
//#ln 108 "cal.y"
{                                  yyval = nseel_createSimpleCompiledFunction(ctx,FN_UMINUS, 1, yyvsp[0], 0);
    break;}
case 16:
//#ln 114 "cal.y"
{                                 yyval = nseel_createSimpleCompiledFunction(ctx,FN_UPLUS, 1, yyvsp[0], 0);
    break;}
case 17:
//#ln 120 "cal.y"
{       yyval = yyvsp[0];     
    break;}
case 18:
//#ln 125 "cal.y"
{                                  yyval = nseel_setCompiledFunctionCallParameters(yyvsp[-3], yyvsp[-1], 0, 0);
    break;}
case 19:
//#ln 131 "cal.y"
{                                 yyval = nseel_setCompiledFunctionCallParameters(yyvsp[-5], yyvsp[-3], yyvsp[-1], 0);
    break;}
case 20:
//#ln 137 "cal.y"
{                                 yyval = nseel_setCompiledFunctionCallParameters(yyvsp[-7], yyvsp[-5], yyvsp[-3], yyvsp[-1]);
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
//#ln 362 "bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;

  *++yyvsp = yyval;


  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++ctx->yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  for (x = 0; x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
#error this should not compile
      msg = (char *) xmalloc(size + 15);
	  strcpy(msg, "syntax error");

	  if (count < 5)
	    {
	      count = 0;
	      for (x = 0; x < (sizeof(yytname) / sizeof(char *)); x++)
		if (yycheck[x + yyn] == x)
		  {
		    strcat(msg, count == 0 ? ", expecting `" : " or `");
		    strcat(msg, yytname[x]);
		    strcat(msg, "'");
		    count++;
		  }
	    }
	  YYERROR(msg);
	  free(msg);
	}
      else
#endif /* YYERROR_VERBOSE */
	YYERROR("syntax error");
    }

//yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (ctx->yychar == YYEOF) YYABORT;

      ctx->yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;


yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = ctx->yylval;

  yystate = yyn;
  goto yynewstate;
}

#endif