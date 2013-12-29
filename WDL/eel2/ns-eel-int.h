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


enum { 

  // these ignore fn in opcodes, just use fntype to determine function
  FN_MULTIPLY=0,
  FN_DIVIDE,
  FN_JOIN_STATEMENTS,
  FN_ADD,
  FN_SUB,
  FN_AND,
  FN_OR,
  FN_UMINUS,
  FN_UPLUS,
  FUNCTYPE_SIMPLEMAX,


  FUNCTYPE_FUNCTIONTYPEREC, // fn is a functionType *
  FUNCTYPE_EELFUNC, // fn is a _codeHandleFunctionRec *
};



#define YYSTYPE opcodeRec *

#define NSEEL_CLOSEFACTOR 0.00001

typedef struct
{
	int srcByteCount;
	int destByteCount;
} lineRecItem;
  
  
typedef struct opcodeRec opcodeRec;

typedef struct _codeHandleFunctionRec 
{
  struct _codeHandleFunctionRec *next; // main linked list (only used for high level functions)
  struct _codeHandleFunctionRec *derivedCopies; // separate linked list, head being the main function, other copies being derived versions

  void *startptr; // compiled code (may be cleared + recompiled when shraed)
  opcodeRec *opcodes;

  int startptr_size; 
  int tmpspace_req;
    
  int num_params;

  int rvMode; // RETURNVALUE_*
  int fpStackUsage; // 0-8, usually
  int canHaveDenormalOutput;

  // local storage's first items are the parameters, then locals. Note that the opcodes will reference localstorage[] via VARPTRPTR, but 
  // the values localstorage[x] points are reallocated from context-to-context, if it is a common function.

  // separately allocated list of pointers, the contents of the list should be zeroed on context changes if a common function
  // note that when making variations on a function (context), it is shared, but since it is zeroed on context changes, it is context-local
  int localstorage_size;
  EEL_F **localstorage; 

  int isCommonFunction;
  int usesNamespaces;
  unsigned int parameterAsNamespaceMask;

  char fname[NSEEL_MAX_FUNCSIG_NAME+1];
} _codeHandleFunctionRec;  
  
#define LLB_DSIZE (65536-64)
typedef struct _llBlock {
  struct _llBlock *next;
  int sizeused;
  char block[LLB_DSIZE];
} llBlock;

typedef struct {
  llBlock *blocks, 
          *blocks_data;
  void *workTable; // references a chunk in blocks_data

  void *code;
  int code_size; // in case the caller wants to write it out
  int code_stats[4];

  int want_stack;
  void *stack;  // references a chunk in blocks_data, somewhere within the complete NSEEL_STACK_SIZE aligned at NSEEL_STACK_SIZE

  void *ramPtr;

  int workTable_size; // size (minus padding/extra space) of workTable -- only used if EEL_VALIDATE_WORKTABLE_USE set, but might be handy to have around too
} codeHandleType;



typedef struct _compileContext
{
  EEL_F **varTable_Values;
  char   ***varTable_Names;
  int varTable_numBlocks;

  int errVar,gotEndOfInput;
  opcodeRec *result;
  char last_error_string[256];

#ifdef NSEEL_USE_OLD_PARSER
  int colCount;
  YYSTYPE yylval;
  int	yychar;			/*  the lookahead symbol		*/
  int yynerrs;			/*  number of parse errors so far       */

  char    *llsave[16];             /* Look ahead buffer            */
  char    llbuf[NSEEL_MAX_VARIABLE_NAMELEN*2+128];             /* work buffer                          */
  char    *llp1;//   = &llbuf[0];    /* pointer to next avail. in token      */
  char    *llp2;//   = &llbuf[0];    /* pointer to end of lookahead          */
  char    *llend;//  = &llbuf[0];    /* pointer to end of token              */
  char    *llebuf;// = &llbuf[sizeof llbuf];
  int     lleof;
#else
  void *scanner;
  #ifdef NSEEL_SUPER_MINIMAL_LEXER
    char *rdbuf, *rdbuf_start;
  #else
    const char *inputbufferptr;
    int errVar_l;
  #endif
#endif

  llBlock *tmpblocks_head, // used while compiling, and freed after compiling

          *blocks_head,  // used while compiling, transferred to code context (these are pages marked as executable)
          *blocks_head_data, // used while compiling, transferred to code context

          *pblocks; // persistent blocks, stores data used by varTable_Names, varTable_Values, etc.

  int l_stats[4]; // source bytes, static code bytes, call code bytes, data bytes

  lineRecItem *compileLineRecs;
  int compileLineRecs_size;
  int compileLineRecs_alloc;

  _codeHandleFunctionRec *functions_local, *functions_common;

  // state used while generating functions
  int optimizeDisableFlags;
  struct opcodeRec *directValueCache; // linked list using fn as next

  int isSharedFunctions;
  int isGeneratingCommonFunction;
  int function_usesNamespaces;
  // [0] is parameter+local symbols (combined space)
  // [1] is symbols which get implied "this." if used
  int function_localTable_Size[2]; // for parameters only
  char **function_localTable_Names[2]; // lists of pointers
  EEL_F **function_localTable_ValuePtrs;
  const char *function_curName; // name of current function

  codeHandleType *tmpCodeHandle;
  
  struct
  {
    int needfree;
    int __pad;
    double closefact;
    EEL_F *blocks[NSEEL_RAM_BLOCKS];
  } ram_state
#ifdef __GNUC__
    __attribute__ ((aligned (8)))
#endif
   ;

  void *gram_blocks;

  void *caller_this;
}
compileContext;

#define NSEEL_VARS_PER_BLOCK 64

#define NSEEL_NPARAMS_FLAG_CONST 0x80000
typedef struct {
      const char *name;
      void *afunc;
      void *func_e;
      int nParams;
      void *replptrs[4];
      NSEEL_PPPROC pProc;
} functionType;


typedef struct
{
  int refcnt;
  char isreg;
} varNameHdr;

extern functionType *nseel_getFunctionFromTable(int idx);

opcodeRec *nseel_createCompiledValue(compileContext *ctx, EEL_F value);
opcodeRec *nseel_createCompiledValuePtr(compileContext *ctx, EEL_F *addrValue, const char *namestr);
opcodeRec *nseel_createCompiledValuePtrPtr(compileContext *ctx, EEL_F **addrValue);

opcodeRec *nseel_createMoreParametersOpcode(compileContext *ctx, opcodeRec *code1, opcodeRec *code2);
opcodeRec *nseel_createSimpleCompiledFunction(compileContext *ctx, int fn, int np, opcodeRec *code1, opcodeRec *code2);
opcodeRec *nseel_createCompiledFunctionCall(compileContext *ctx, int np, int ftype, void *fn);
opcodeRec *nseel_createCompiledEELFunctionCall(compileContext *ctx, _codeHandleFunctionRec *fn, const char *thistext, int namespaceidx);
opcodeRec *nseel_setCompiledFunctionCallParameters(opcodeRec *fn, opcodeRec *code1, opcodeRec *code2, opcodeRec *code3);

opcodeRec *nseel_createCompiledValueFromNamespaceName(compileContext *ctx, const char *relName, int thisctx);
EEL_F *nseel_int_register_var(compileContext *ctx, const char *name, int isReg);
_codeHandleFunctionRec *eel_createFunctionNamespacedInstance(compileContext *ctx, _codeHandleFunctionRec *fr, const char *nameptr);

typedef struct nseel_globalVarItem
{
  EEL_F data;
  struct nseel_globalVarItem *_next;
  char name[1]; // varlen, does not include _global. prefix
} nseel_globalVarItem;

extern nseel_globalVarItem *nseel_globalreg_list; // if NSEEL_EEL1_COMPAT_MODE, must use NSEEL_getglobalregs() for regxx values

#ifdef NSEEL_USE_OLD_PARSER
  #define	VALUE	258
  #define	IDENTIFIER	259
  #define	FUNCTION1	260
  #define	FUNCTION2	261
  #define	FUNCTION3	262
  #define UMINUS  263
  #define UPLUS   264

  #define INTCONST 1
  #define DBLCONST 2
  #define HEXCONST 3
  #define VARIABLE 4
  #define OTHER    5
#else
  #include "y.tab.h"
#endif

opcodeRec *nseel_translate(compileContext *ctx, const char *tmp);
int nseel_gettokenlen(compileContext *ctx, int maxlen);
opcodeRec *nseel_lookup(compileContext *ctx, int *typeOfObject, const char *sname);
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

EEL_F * NSEEL_CGEN_CALL __NSEEL_RAMAlloc(EEL_F **blocks, unsigned int w);
EEL_F * NSEEL_CGEN_CALL __NSEEL_RAMAllocGMEM(EEL_F ***blocks, unsigned int w);
EEL_F * NSEEL_CGEN_CALL __NSEEL_RAM_MemSet(EEL_F **blocks,EEL_F *dest, EEL_F *v, EEL_F *lenptr);
EEL_F * NSEEL_CGEN_CALL __NSEEL_RAM_MemFree(void *blocks, EEL_F *which);
EEL_F * NSEEL_CGEN_CALL __NSEEL_RAM_MemCpy(EEL_F **blocks,EEL_F *dest, EEL_F *src, EEL_F *lenptr);



#ifndef max
#define max(x,y) ((x)<(y)?(y):(x))
#define min(x,y) ((x)<(y)?(x):(y))
#endif

#ifdef __cplusplus
}
#endif

#endif//__NS_EELINT_H__
