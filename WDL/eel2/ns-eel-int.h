/*
  Nullsoft Expression Evaluator Library (NS-EEL)
  Copyright (C) 1999-2003 Nullsoft, Inc.
  
  ns-eel-int.h: internal code definition header.

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

#ifndef __NS_EELINT_H__
#define __NS_EELINT_H__

#ifdef _WIN32
#include <windows.h>
#else
#include "../wdltypes.h"
#endif

#include "ns-eel.h"
#include "ns-eel-addfuncs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FN_ASSIGN   0
#define FN_MULTIPLY 1
#define FN_DIVIDE   2
#define FN_MODULO   3
#define FN_ADD      4
#define FN_SUB      5
#define FN_AND      6
#define FN_OR       7
#define FN_UMINUS   8
#define FN_UPLUS    9

#define MATH_SIMPLE 0
#define MATH_FN     1

#define YYSTYPE INT_PTR

#define NSEEL_CLOSEFACTOR 0.00001

typedef struct
{
	int srcByteCount;
	int destByteCount;
} lineRecItem;

typedef struct _compileContext
{
  EEL_F **varTable_Values;
  char   **varTable_Names;
  int varTable_numBlocks;

  int errVar;
  int colCount;
  INT_PTR result;
  char last_error_string[256];
  YYSTYPE yylval;
  int	yychar;			/*  the lookahead symbol		*/
  int yynerrs;			/*  number of parse errors so far       */
  char yytext[256];
  char lastVar[256];

  char    *llsave[16];             /* Look ahead buffer            */
  char    llbuf[100];             /* work buffer                          */
  char    *llp1;//   = &llbuf[0];    /* pointer to next avail. in token      */
  char    *llp2;//   = &llbuf[0];    /* pointer to end of lookahead          */
  char    *llend;//  = &llbuf[0];    /* pointer to end of token              */
  char    *llebuf;// = &llbuf[sizeof llbuf];
  int     lleof;
  int     yyline;//  = 0;

  void *tmpblocks_head,*blocks_head;
  int computTableTop; // make it abort on potential overflow =)
  int l_stats[4]; // source bytes, static code bytes, call code bytes, data bytes

  lineRecItem *compileLineRecs;
  int compileLineRecs_size;
  int compileLineRecs_alloc;

  void *ram_blocks; // this needs to be immediately followed by
  int ram_needfree;

  void *gram_blocks;

  void *caller_this;
}
compileContext;

#define NSEEL_VARS_PER_BLOCK 64

typedef struct {
      const char *name;
      void *afunc;
      void *func_e;
      int nParams;
      void *replptrs[4];
      NSEEL_PPPROC pProc;
} functionType;


extern functionType *nseel_getFunctionFromTable(int idx);

INT_PTR nseel_createCompiledValue(compileContext *ctx, EEL_F value, EEL_F *addrValue);
INT_PTR nseel_createCompiledFunction1(compileContext *ctx, int fntype, INT_PTR fn, INT_PTR code);
INT_PTR nseel_createCompiledFunction2(compileContext *ctx, int fntype, INT_PTR fn, INT_PTR code1, INT_PTR code2);
INT_PTR nseel_createCompiledFunction3(compileContext *ctx, int fntype, INT_PTR fn, INT_PTR code1, INT_PTR code2, INT_PTR code3);

extern EEL_F nseel_globalregs[100];

void nseel_resetVars(compileContext *ctx);
EEL_F *nseel_getVarPtr(compileContext *ctx, char *varName);
EEL_F *nseel_registerVar(compileContext *ctx, char *varName);

INT_PTR *EEL_GLUE_set_immediate(void *_p, void *newv);

// other shat



INT_PTR nseel_setVar(compileContext *ctx, INT_PTR varNum);
INT_PTR nseel_getVar(compileContext *ctx, INT_PTR varNum);
void *nseel_compileExpression(compileContext *ctx, char *txt);

#define	VALUE	258
#define	IDENTIFIER	259
#define	FUNCTION1	260
#define	FUNCTION2	261
#define	FUNCTION3	262
#define	UMINUS	263
#define	UPLUS	264

INT_PTR nseel_translate(compileContext *ctx, int type);
void nseel_count(compileContext *ctx);
void nseel_setLastVar(compileContext *ctx);
INT_PTR nseel_lookup(compileContext *ctx, int *typeOfObject);
int nseel_yyerror(compileContext *ctx);
int nseel_yylex(compileContext *ctx, char **exp);
int nseel_yyparse(compileContext *ctx, char *exp);
void nseel_llinit(compileContext *ctx);
int nseel_gettoken(compileContext *ctx, char *lltb, int lltbsiz);

struct  lextab {
        int     llendst;                /* Last state number            */
        char    *lldefault;             /* Default state table          */
        char    *llnext;                /* Next state table             */
        char    *llcheck;               /* Check table                  */
        int     *llbase;                /* Base table                   */
        int     llnxtmax;               /* Last in next table           */
        int     (*llmove)();            /* Move between states          */
        char     *llfinal;               /* Final state descriptions     */
        int     (*llactr)();            /* Action routine               */
        int     *lllook;                /* Look ahead vector if != NULL */
        char    *llign;                 /* Ignore char vec if != NULL   */
        char    *llbrk;                 /* Break char vec if != NULL    */
        char    *llill;                 /* Illegal char vec if != NULL  */
};
extern struct lextab nseel_lextab;

EEL_F * NSEEL_CGEN_CALL __NSEEL_RAMAlloc(EEL_F ***blocks, int w);
EEL_F * NSEEL_CGEN_CALL __NSEEL_RAMAllocGMEM(EEL_F ***blocks, int w);
EEL_F * NSEEL_CGEN_CALL __NSEEL_RAM_MemSet(EEL_F ***blocks,EEL_F *dest, EEL_F *v, EEL_F *lenptr);
EEL_F * NSEEL_CGEN_CALL __NSEEL_RAM_MemFree(EEL_F ***blocks, EEL_F *which);
EEL_F * NSEEL_CGEN_CALL __NSEEL_RAM_MemCpy(EEL_F ***blocks,EEL_F *dest, EEL_F *src, EEL_F *lenptr);



#ifndef max
#define max(x,y) ((x)<(y)?(y):(x))
#define min(x,y) ((x)<(y)?(x):(y))
#endif



#ifdef __ppc__

  #define EEL_F2int(x) ((int)(x))

#elif defined (_WIN64)

  // todo: AMD64 version?
  #define EEL_F2int(x) ((int)(x))

#elif defined(_MSC_VER)

static __inline int EEL_F2int(EEL_F d)
{
  int tmp;
  __asm {
    fld d
    fistp tmp
  }
  return tmp;
}

#else

static inline int EEL_F2int(EEL_F d)
{
	  int tmp;
    __asm__ __volatile__ ("fistpl %0" : "=m" (tmp) : "t" (d) : "st") ;
    return tmp;
}

#endif

#ifdef __cplusplus
}
#endif

#endif//__NS_EELINT_H__
