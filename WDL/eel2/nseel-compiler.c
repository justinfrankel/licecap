/*
  Expression Evaluator Library (NS-EEL) v2
  Copyright (C) 2004-2013 Cockos Incorporated
  Copyright (C) 1999-2003 Nullsoft, Inc.
  
  nseel-compiler.c

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


// for VirtualProtect

#include "ns-eel-int.h"

#include "../denormal.h"
#include "../wdlcstring.h"

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>

#ifdef __APPLE__
  #ifdef __LP64__
    #define EEL_USE_MPROTECT
  #endif
#endif

#ifdef EEL_USE_MPROTECT
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#endif

#define NSEEL_VARS_MALLOC_CHUNKSIZE 8

//#define LOG_OPT
//#define EEL_PPC_NOFREECODE
//#define EEL_PRINT_FAILS
//#define EEL_VALIDATE_WORKTABLE_USE
//#define EEL_VALIDATE_FSTUBS


#ifdef EEL_PRINT_FAILS
  #ifdef _WIN32
    #define RET_MINUS1_FAIL(x) { OutputDebugString(x); return -1; }
  #else
    #define RET_MINUS1_FAIL(x) { printf("%s\n",x); return -1; }
  #endif
#else
#define RET_MINUS1_FAIL(x) return -1;
#endif



#ifdef EEL_VALIDATE_WORKTABLE_USE
  #define MIN_COMPUTABLE_SIZE 0
  #define COMPUTABLE_EXTRA_SPACE 64 // safety buffer, if EEL_VALIDATE_WORKTABLE_USE set, used for magic-value-checking
#else
  #define MIN_COMPUTABLE_SIZE 32 // always use at least this big of a temp storage table (and reset the temp ptr when it goes past this boundary)
  #define COMPUTABLE_EXTRA_SPACE 16 // safety buffer, if EEL_VALIDATE_WORKTABLE_USE set, used for magic-value-checking
#endif


/*
  P1 is rightmost parameter
  P2 is second rightmost, if any
  P3 is third rightmost, if any
  registers on x86 are  (RAX etc on x86-64)
    P1(ret) EAX
    P2 EDI
    P3 ECX
    WTP RSI
    x86_64: r12 is a pointer to ram_state.blocks
    x86_64: r13 is a pointer to closenessfactor

  registers on PPC are:
    P1(ret) r3
    P2 r14 
    P3 r15
    WTP r16 (r17 has the original value)
    r13 is a pointer to ram_state.blocks

    ppc uses f31 and f30 and others for certain constants

  */

#ifdef EEL_TARGET_PORTABLE

#define EEL_DOESNT_NEED_EXEC_PERMS
#include "glue_port.h"

#elif defined(__ppc__)

#include "glue_ppc.h"

#elif defined(_WIN64) || defined(__LP64__)

#include "glue_x86_64.h"

#else

#include "glue_x86.h"

#endif

#ifndef GLUE_INVSQRT_NEEDREPL
#define GLUE_INVSQRT_NEEDREPL 0
#endif


// used by //#eel-no-optimize:xxx, in ctx->optimizeDisableFlags
#define OPTFLAG_NO_OPTIMIZE 1
#define OPTFLAG_NO_FPSTACK 2
#define OPTFLAG_NO_INLINEFUNC 4
#define OPTFLAG_FULL_DENORMAL_CHECKS 8 // if set, denormals/NaN are always filtered on assign
#define OPTFLAG_NO_DENORMAL_CHECKS 16 // if set and FULL not set, denormals/NaN are never filtered on assign




static int nseel_evallib_stats[5]; // source bytes, static code bytes, call code bytes, data bytes, segments
int *NSEEL_getstats()
{
  return nseel_evallib_stats;
}
EEL_F *NSEEL_getglobalregs()
{
  return nseel_globalregs;
}

// this stuff almost works
static int findByteOffsetInSource(compileContext *ctx, int byteoffs,int *destoffs)
{
	int x;
	if (!ctx->compileLineRecs || !ctx->compileLineRecs_size) return *destoffs=0;
	if (byteoffs < ctx->compileLineRecs[0].destByteCount) 
	{
		*destoffs=0;
		return 1;
	}
	for (x = 0; x < ctx->compileLineRecs_size-1; x ++)
	{
		if (byteoffs >= ctx->compileLineRecs[x].destByteCount &&
		    byteoffs < ctx->compileLineRecs[x+1].destByteCount) break;
	}
	*destoffs=ctx->compileLineRecs[(x&&x==ctx->compileLineRecs_size-1)?x-1:x].srcByteCount;

	return x+2;
}


static void onCompileNewLine(compileContext *ctx, int srcBytes, int destBytes)
{
	if (!ctx->compileLineRecs || ctx->compileLineRecs_size >= ctx->compileLineRecs_alloc)
	{
		ctx->compileLineRecs_alloc = ctx->compileLineRecs_size+1024;
		ctx->compileLineRecs = (lineRecItem *)realloc(ctx->compileLineRecs,sizeof(lineRecItem)*ctx->compileLineRecs_alloc);
	}
	if (ctx->compileLineRecs)
	{
		ctx->compileLineRecs[ctx->compileLineRecs_size].srcByteCount=srcBytes;
		ctx->compileLineRecs[ctx->compileLineRecs_size++].destByteCount=destBytes;
	}
}

static void *__newBlock(llBlock **start,int size, int wantMprotect);

#define OPCODE_IS_TRIVIAL(x) ((x)->opcodeType <= OPCODETYPE_VARPTRPTR)
enum {
  OPCODETYPE_DIRECTVALUE=0,
  OPCODETYPE_VALUE_FROM_NAMESPACENAME, // this.* are encoded this way
  OPCODETYPE_VARPTR,
  OPCODETYPE_VARPTRPTR,
  OPCODETYPE_FUNC1,
  OPCODETYPE_FUNC2,
  OPCODETYPE_FUNC3,
  OPCODETYPE_FUNCX,

  OPCODETYPE_MOREPARAMS,

  OPCODETYPE_INVALID,
};

struct opcodeRec
{
 int opcodeType; 
 int fntype;
 void *fn;
 
 union {
   struct opcodeRec *parms[3];
   struct {
     double directValue;
     EEL_F *valuePtr; // if direct value, valuePtr can be cached
   } dv;
 } parms;
  
 // allocate extra if using this field. used with:
 // OPCODETYPE_VALUE_FROM_NAMESPACENAME
 ///  or 
 // OPCODETYPE_FUNC* with fntype=FUNCTYPE_EELFUNC_THIS
 char relname[1]; 
};




static void *newTmpBlock(compileContext *ctx, int size)
{
  const int align = 8;
  const int a1=align-1;
  char *p=(char*)__newBlock(&ctx->tmpblocks_head,size+a1, 0);
  return p+((align-(((INT_PTR)p)&a1))&a1);
}

static void *__newBlock_align(compileContext *ctx, int size, int align, int isForCode) 
{
  const int a1=align-1;
  char *p=(char*)__newBlock(
                            (                            
                             isForCode < 0 ? (isForCode == -2 ? &ctx->pblocks : &ctx->tmpblocks_head) : 
                             isForCode > 0 ? &ctx->blocks_head : 
                             &ctx->blocks_head_data) ,size+a1, isForCode>0);
  return p+((align-(((INT_PTR)p)&a1))&a1);
}

static opcodeRec *newOpCode(compileContext *ctx)
{
  return (opcodeRec*)__newBlock_align(ctx,sizeof(opcodeRec),8, ctx->isSharedFunctions ? 0 : -1); 
}

#define newCodeBlock(x,a) __newBlock_align(ctx,x,a,1)
#define newDataBlock(x,a) __newBlock_align(ctx,x,a,0)
#define newCtxDataBlock(x,a) __newBlock_align(ctx,x,a,-2)

static void freeBlocks(llBlock **start);

#ifndef DECL_ASMFUNC
#define DECL_ASMFUNC(x)         \
  void nseel_asm_##x(void);        \
  void nseel_asm_##x##_end(void);


void _asm_megabuf(void);
void _asm_megabuf_end(void);
void _asm_gmegabuf(void);
void _asm_gmegabuf_end(void);

#endif


  DECL_ASMFUNC(booltofp)
  DECL_ASMFUNC(fptobool)
  DECL_ASMFUNC(sin)
  DECL_ASMFUNC(cos)
  DECL_ASMFUNC(tan)
  DECL_ASMFUNC(1pdd)
  DECL_ASMFUNC(2pdd)
  DECL_ASMFUNC(2pdds)
  DECL_ASMFUNC(1pp)
  DECL_ASMFUNC(2pp)
  DECL_ASMFUNC(sqr)
  DECL_ASMFUNC(sqrt)
  DECL_ASMFUNC(log)
  DECL_ASMFUNC(log10)
  DECL_ASMFUNC(abs)
  DECL_ASMFUNC(min)
  DECL_ASMFUNC(max)
  DECL_ASMFUNC(min_fp)
  DECL_ASMFUNC(max_fp)
  DECL_ASMFUNC(sig)
  DECL_ASMFUNC(sign)
  DECL_ASMFUNC(band)
  DECL_ASMFUNC(bor)
  DECL_ASMFUNC(bnot)
  DECL_ASMFUNC(if)
  DECL_ASMFUNC(fcall)
  DECL_ASMFUNC(repeat)
  DECL_ASMFUNC(repeatwhile)
  DECL_ASMFUNC(equal)
  DECL_ASMFUNC(notequal)
  DECL_ASMFUNC(below)
  DECL_ASMFUNC(above)
  DECL_ASMFUNC(beloweq)
  DECL_ASMFUNC(aboveeq)
  DECL_ASMFUNC(assign)
  DECL_ASMFUNC(assign_fromfp)
  DECL_ASMFUNC(assign_fast)
  DECL_ASMFUNC(assign_fast_fromfp)
  DECL_ASMFUNC(add)
  DECL_ASMFUNC(sub)
  DECL_ASMFUNC(add_op)
  DECL_ASMFUNC(sub_op)
  DECL_ASMFUNC(add_op_fast)
  DECL_ASMFUNC(sub_op_fast)
  DECL_ASMFUNC(mul)
  DECL_ASMFUNC(div)
  DECL_ASMFUNC(mul_op)
  DECL_ASMFUNC(div_op)
  DECL_ASMFUNC(mod)
  DECL_ASMFUNC(shl)
  DECL_ASMFUNC(shr)
  DECL_ASMFUNC(mod_op)
  DECL_ASMFUNC(or)
  DECL_ASMFUNC(or0)
  DECL_ASMFUNC(xor)
  DECL_ASMFUNC(xor_op)
  DECL_ASMFUNC(and)
  DECL_ASMFUNC(or_op)
  DECL_ASMFUNC(and_op)
  DECL_ASMFUNC(uplus)
  DECL_ASMFUNC(uminus)
  DECL_ASMFUNC(invsqrt)
  DECL_ASMFUNC(dbg_getstackptr)
  DECL_ASMFUNC(exec2)

  DECL_ASMFUNC(stack_push)
  DECL_ASMFUNC(stack_pop)
  DECL_ASMFUNC(stack_pop_fast) // just returns value, doesn't mod param
  DECL_ASMFUNC(stack_peek)
  DECL_ASMFUNC(stack_peek_int)
  DECL_ASMFUNC(stack_peek_top)
  DECL_ASMFUNC(stack_exch)

static void *NSEEL_PProc_GRAM(void *data, int data_size, compileContext *ctx)
{
  if (data_size>0) data=EEL_GLUE_set_immediate(data, (INT_PTR)ctx->gram_blocks);
  return data;
}

static void *NSEEL_PProc_Stack(void *data, int data_size, compileContext *ctx)
{
  codeHandleType *ch=ctx->tmpCodeHandle;

  if (data_size>0) 
  {
    UINT_PTR m1=(UINT_PTR)(NSEEL_STACK_SIZE * sizeof(EEL_F) - 1);
    UINT_PTR stackptr = ((UINT_PTR) (&ch->stack));

    ch->want_stack=1;
    if (!ch->stack) ch->stack = newDataBlock(NSEEL_STACK_SIZE*sizeof(EEL_F),NSEEL_STACK_SIZE*sizeof(EEL_F));

    data=EEL_GLUE_set_immediate(data, stackptr);
    data=EEL_GLUE_set_immediate(data, m1); // and
    data=EEL_GLUE_set_immediate(data, ((UINT_PTR)ch->stack&~m1)); //or
  }
  return data;
}

static void *NSEEL_PProc_Stack_PeekInt(void *data, int data_size, compileContext *ctx, INT_PTR offs)
{
  codeHandleType *ch=ctx->tmpCodeHandle;

  if (data_size>0) 
  {
    UINT_PTR m1=(UINT_PTR)(NSEEL_STACK_SIZE * sizeof(EEL_F) - 1);
    UINT_PTR stackptr = ((UINT_PTR) (&ch->stack));

    ch->want_stack=1;
    if (!ch->stack) ch->stack = newDataBlock(NSEEL_STACK_SIZE*sizeof(EEL_F),NSEEL_STACK_SIZE*sizeof(EEL_F));

    data=EEL_GLUE_set_immediate(data, stackptr);
    data=EEL_GLUE_set_immediate(data, offs);
    data=EEL_GLUE_set_immediate(data, m1); // and
    data=EEL_GLUE_set_immediate(data, ((UINT_PTR)ch->stack&~m1)); //or
  }
  return data;
}
static void *NSEEL_PProc_Stack_PeekTop(void *data, int data_size, compileContext *ctx)
{
  codeHandleType *ch=ctx->tmpCodeHandle;

  if (data_size>0) 
  {
    UINT_PTR stackptr = ((UINT_PTR) (&ch->stack));

    ch->want_stack=1;
    if (!ch->stack) ch->stack = newDataBlock(NSEEL_STACK_SIZE*sizeof(EEL_F),NSEEL_STACK_SIZE*sizeof(EEL_F));

    data=EEL_GLUE_set_immediate(data, stackptr);
  }
  return data;
}

#if defined(_MSC_VER) && _MSC_VER >= 1400
static double __floor(double a) { return floor(a); }
static double __ceil(double a) { return ceil(a); }
#define floor __floor
#define ceil __ceil
#endif


#ifdef NSEEL_EEL1_COMPAT_MODE
static double eel1band(double a, double b)
{
  return (fabs(a)>NSEEL_CLOSEFACTOR && fabs(b) > NSEEL_CLOSEFACTOR) ? 1.0 : 0.0;
}
static double eel1bor(double a, double b)
{
  return (fabs(a)>NSEEL_CLOSEFACTOR || fabs(b) > NSEEL_CLOSEFACTOR) ? 1.0 : 0.0;
}

static double eel1sigmoid(double x, double constraint)
{
  double t = (1+exp(-x * (constraint)));
  return fabs(t)>NSEEL_CLOSEFACTOR ? 1.0/t : 0;
}

#endif



#define FUNCTIONTYPE_PARAMETERCOUNTMASK 0xff

#define BIF_NPARAMS_MASK       0x7fff00
#define BIF_RETURNSONSTACK     0x000100
#define BIF_LASTPARMONSTACK    0x000200
#define BIF_RETURNSBOOL        0x000400 // this value is used in ns-eel.h in some macros, be sure to update it there if you change it here
#define BIF_LASTPARM_ASBOOL    0x000800
#define BIF_WONTMAKEDENORMAL   0x100000
#define BIF_CLEARDENORMAL      0x200000

#if defined(GLUE_HAS_FXCH) && GLUE_MAX_FPSTACK_SIZE > 0
  #define BIF_SECONDLASTPARMST 0x001000 // use with BIF_LASTPARMONSTACK only (last two parameters get passed on fp stack)
  #define BIF_LAZYPARMORDERING 0x002000 // allow optimizer to avoid fxch when using BIF_TWOPARMSONFPSTACK_LAZY etc
  #define BIF_REVERSEFPORDER   0x004000 // force a fxch (reverse order of last two parameters on fp stack, used by comparison functions)

  #ifndef BIF_FPSTACKUSE
    #define BIF_FPSTACKUSE(x) (((x)>=0&&(x)<8) ? ((7-(x))<<16):0)
  #endif
  #ifndef BIF_GETFPSTACKUSE
    #define BIF_GETFPSTACKUSE(x) (7 - (((x)>>16)&7))
  #endif
#else
  // do not support fp stack use unless GLUE_HAS_FXCH and GLUE_MAX_FPSTACK_SIZE>0
  #define BIF_SECONDLASTPARMST 0
  #define BIF_LAZYPARMORDERING 0
  #define BIF_REVERSEFPORDER   0
  #define BIF_FPSTACKUSE(x) 0
  #define BIF_GETFPSTACKUSE(x) 0
#endif

#define BIF_TWOPARMSONFPSTACK (BIF_SECONDLASTPARMST|BIF_LASTPARMONSTACK)
#define BIF_TWOPARMSONFPSTACK_LAZY (BIF_LAZYPARMORDERING|BIF_SECONDLASTPARMST|BIF_LASTPARMONSTACK)




EEL_F NSEEL_CGEN_CALL nseel_int_rand(EEL_F f);

#define FNPTR_HAS_CONDITIONAL_EXEC(op) (op->fntype == FUNCTYPE_FUNCTIONTYPEREC && (functionType*)op->fn >= fnTable1 && (functionType*)op->fn < fnTable1+5)

static functionType fnTable1[] = {
  { "_if",     nseel_asm_if,nseel_asm_if_end,    3|NSEEL_NPARAMS_FLAG_CONST|BIF_WONTMAKEDENORMAL, }, 
  { "_and",   nseel_asm_band,nseel_asm_band_end,  2|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSBOOL } ,
  { "_or",    nseel_asm_bor,nseel_asm_bor_end,   2|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSBOOL } ,
  { "loop", nseel_asm_repeat,nseel_asm_repeat_end, 2|NSEEL_NPARAMS_FLAG_CONST|BIF_WONTMAKEDENORMAL },
  { "while", nseel_asm_repeatwhile,nseel_asm_repeatwhile_end, 1|NSEEL_NPARAMS_FLAG_CONST|BIF_WONTMAKEDENORMAL },

  { "_not",   nseel_asm_bnot,nseel_asm_bnot_end,  1|NSEEL_NPARAMS_FLAG_CONST|BIF_LASTPARM_ASBOOL|BIF_RETURNSBOOL|BIF_FPSTACKUSE(1), } ,

  { "_equal",  nseel_asm_equal,nseel_asm_equal_end, 2|NSEEL_NPARAMS_FLAG_CONST|BIF_TWOPARMSONFPSTACK_LAZY|BIF_RETURNSBOOL|BIF_FPSTACKUSE(2), {0} },
  { "_noteq",  nseel_asm_notequal,nseel_asm_notequal_end, 2|NSEEL_NPARAMS_FLAG_CONST|BIF_TWOPARMSONFPSTACK_LAZY|BIF_RETURNSBOOL|BIF_FPSTACKUSE(2), {0} },

#ifdef GLUE_HAS_FXCH
  { "_above",  nseel_asm_above,nseel_asm_above_end, 2|NSEEL_NPARAMS_FLAG_CONST|BIF_TWOPARMSONFPSTACK|BIF_RETURNSBOOL|BIF_FPSTACKUSE(2) },
  { "_aboeq",  nseel_asm_beloweq,nseel_asm_beloweq_end, 2|NSEEL_NPARAMS_FLAG_CONST|BIF_TWOPARMSONFPSTACK|BIF_RETURNSBOOL|BIF_REVERSEFPORDER|BIF_FPSTACKUSE(2)  },
  { "_below",  nseel_asm_above,nseel_asm_above_end, 2|NSEEL_NPARAMS_FLAG_CONST|BIF_TWOPARMSONFPSTACK|BIF_RETURNSBOOL|BIF_REVERSEFPORDER|BIF_FPSTACKUSE(2)},
  { "_beleq",  nseel_asm_beloweq,nseel_asm_beloweq_end, 2|NSEEL_NPARAMS_FLAG_CONST|BIF_TWOPARMSONFPSTACK|BIF_RETURNSBOOL|BIF_FPSTACKUSE(2) },
#else
  { "_above",  nseel_asm_above,nseel_asm_above_end, 2|NSEEL_NPARAMS_FLAG_CONST|BIF_LASTPARMONSTACK|BIF_RETURNSBOOL },
  { "_aboeq",  nseel_asm_aboveeq,nseel_asm_aboveeq_end, 2|NSEEL_NPARAMS_FLAG_CONST|BIF_LASTPARMONSTACK|BIF_RETURNSBOOL },
  { "_below",  nseel_asm_below,nseel_asm_below_end, 2|NSEEL_NPARAMS_FLAG_CONST|BIF_TWOPARMSONFPSTACK|BIF_RETURNSBOOL },
  { "_beleq",  nseel_asm_beloweq,nseel_asm_beloweq_end, 2|NSEEL_NPARAMS_FLAG_CONST|BIF_TWOPARMSONFPSTACK|BIF_RETURNSBOOL },
#endif


#ifndef GLUE_HAS_NATIVE_TRIGSQRTLOG
   { "sin",   nseel_asm_1pdd,nseel_asm_1pdd_end,   1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK, {&sin} },
   { "cos",    nseel_asm_1pdd,nseel_asm_1pdd_end,   1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK, {&cos} },
   { "tan",    nseel_asm_1pdd,nseel_asm_1pdd_end,   1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK, {&tan}  },
   { "sqrt",   nseel_asm_1pdd,nseel_asm_1pdd_end,  1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK, {&sqrt}, },
   { "log",    nseel_asm_1pdd,nseel_asm_1pdd_end,   1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK, {&log} },
   { "log10",  nseel_asm_1pdd,nseel_asm_1pdd_end, 1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK, {&log10} },
#else
   { "sin",   nseel_asm_sin,nseel_asm_sin_end,   1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(1) },
   { "cos",    nseel_asm_cos,nseel_asm_cos_end,   1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(1) },
   { "tan",    nseel_asm_tan,nseel_asm_tan_end,   1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(1) },
   { "sqrt",   nseel_asm_sqrt,nseel_asm_sqrt_end,  1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(1) },
   { "log",    nseel_asm_log,nseel_asm_log_end,   1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(3), },
   { "log10",  nseel_asm_log10,nseel_asm_log10_end, 1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(3), },
#endif

  { "_set",nseel_asm_assign,nseel_asm_assign_end,2|BIF_FPSTACKUSE(1)|BIF_CLEARDENORMAL, }, // if denormal flag set, we'll use assign which will take care of the denormal
  { "_mod",nseel_asm_mod,nseel_asm_mod_end,2 | NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_TWOPARMSONFPSTACK|BIF_FPSTACKUSE(1)|BIF_CLEARDENORMAL },
  { "_shr",nseel_asm_shr,nseel_asm_shr_end,2 | NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_TWOPARMSONFPSTACK|BIF_FPSTACKUSE(2)|BIF_CLEARDENORMAL },
  { "_shl",nseel_asm_shl,nseel_asm_shl_end,2 | NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_TWOPARMSONFPSTACK|BIF_FPSTACKUSE(2)|BIF_CLEARDENORMAL },

  { "_mulop",nseel_asm_mul_op,nseel_asm_mul_op_end,2|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(2)|BIF_CLEARDENORMAL}, // mulop/divop clear denormals manually
  { "_divop",nseel_asm_div_op,nseel_asm_div_op_end,2|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(2)|BIF_CLEARDENORMAL},

  { "_orop",nseel_asm_or_op,nseel_asm_or_op_end,2|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(2)|BIF_CLEARDENORMAL},  // these go to int so they clear denormals too
  { "_andop",nseel_asm_and_op,nseel_asm_and_op_end,2|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(2)|BIF_CLEARDENORMAL}, 
  { "_xorop",nseel_asm_xor_op,nseel_asm_xor_op_end,2|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(2)|BIF_CLEARDENORMAL}, 
  { "_modop",nseel_asm_mod_op,nseel_asm_mod_op_end,2|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(2)|BIF_CLEARDENORMAL}, 

  { "_addop",nseel_asm_add_op,nseel_asm_add_op_end,2|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(2)|BIF_CLEARDENORMAL},  // default versions of these clear denormals, but we can shortcut to non-denorm check versions if input is known non-denormal
  { "_subop",nseel_asm_sub_op,nseel_asm_sub_op_end,2|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(2)|BIF_CLEARDENORMAL}, 


   { "asin",   nseel_asm_1pdd,nseel_asm_1pdd_end,  1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK, {&asin}, },
   { "acos",   nseel_asm_1pdd,nseel_asm_1pdd_end,  1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK, {&acos}, },
   { "atan",   nseel_asm_1pdd,nseel_asm_1pdd_end,  1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK, {&atan}, },
   { "atan2",  nseel_asm_2pdd,nseel_asm_2pdd_end, 2|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_TWOPARMSONFPSTACK, {&atan2}, },
   { "pow",    nseel_asm_2pdd,nseel_asm_2pdd_end,   2|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_TWOPARMSONFPSTACK, {&pow}, },
   { "_powop",    nseel_asm_2pdds,nseel_asm_2pdds_end,   2|BIF_LASTPARMONSTACK|BIF_CLEARDENORMAL, {&pow}, },
   { "exp",    nseel_asm_1pdd,nseel_asm_1pdd_end,   1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK, {&exp}, },
   { "abs",    nseel_asm_abs,nseel_asm_abs_end,   1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(0)|BIF_WONTMAKEDENORMAL },
   { "sqr",    nseel_asm_sqr,nseel_asm_sqr_end,   1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(1) },
   { "min",    nseel_asm_min,nseel_asm_min_end,   2|NSEEL_NPARAMS_FLAG_CONST|BIF_FPSTACKUSE(3)|BIF_WONTMAKEDENORMAL },
   { "max",    nseel_asm_max,nseel_asm_max_end,   2|NSEEL_NPARAMS_FLAG_CONST|BIF_FPSTACKUSE(3)|BIF_WONTMAKEDENORMAL },
   { "sign",   nseel_asm_sign,nseel_asm_sign_end,  1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(2)|BIF_CLEARDENORMAL, },
   { "rand",   nseel_asm_1pdd,nseel_asm_1pdd_end,  1|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK|BIF_WONTMAKEDENORMAL, {&nseel_int_rand}, },

   { "floor",  nseel_asm_1pdd,nseel_asm_1pdd_end, 1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK|BIF_WONTMAKEDENORMAL, {&floor} },
   { "ceil",   nseel_asm_1pdd,nseel_asm_1pdd_end,  1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK|BIF_WONTMAKEDENORMAL, {&ceil} },

   { "invsqrt",   nseel_asm_invsqrt,nseel_asm_invsqrt_end,  1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(3), {GLUE_INVSQRT_NEEDREPL} },

   { "__dbg_getstackptr",   nseel_asm_dbg_getstackptr,nseel_asm_dbg_getstackptr_end,  1|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(1),  },

  { "_xor",    nseel_asm_xor,nseel_asm_xor_end,   2|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_TWOPARMSONFPSTACK_LAZY|BIF_FPSTACKUSE(2)|BIF_CLEARDENORMAL } ,

#ifdef NSEEL_EEL1_COMPAT_MODE
  { "sigmoid", nseel_asm_2pdd,nseel_asm_2pdd_end, 2|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_TWOPARMSONFPSTACK, {&eel1sigmoid}, },

  // these differ from _and/_or, they always evaluate both...
  { "band",  nseel_asm_2pdd,nseel_asm_2pdd_end, 2|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_TWOPARMSONFPSTACK|BIF_CLEARDENORMAL , {&eel1band}, },
  { "bor",  nseel_asm_2pdd,nseel_asm_2pdd_end, 2|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_TWOPARMSONFPSTACK|BIF_CLEARDENORMAL , {&eel1bor}, },

  {"exec2",nseel_asm_exec2,nseel_asm_exec2_end,2|NSEEL_NPARAMS_FLAG_CONST|BIF_WONTMAKEDENORMAL},
  {"exec3",nseel_asm_exec2,nseel_asm_exec2_end,3|NSEEL_NPARAMS_FLAG_CONST|BIF_WONTMAKEDENORMAL},
#endif // end EEL1 compat


  {"_mem",_asm_megabuf,_asm_megabuf_end,1|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(1)|BIF_CLEARDENORMAL,{&__NSEEL_RAMAlloc}, 
    #ifdef GLUE_MEM_NEEDS_PPROC
      NSEEL_PProc_RAM,
    #else
      NULL
    #endif
  },

  {"_gmem",_asm_gmegabuf,_asm_gmegabuf_end,1|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(1)|BIF_CLEARDENORMAL,{&__NSEEL_RAMAllocGMEM},NSEEL_PProc_GRAM},
  {"freembuf",_asm_generic1parm,_asm_generic1parm_end,1,{&__NSEEL_RAM_MemFree},NSEEL_PProc_RAM},
  {"memcpy",_asm_generic3parm,_asm_generic3parm_end,3,{&__NSEEL_RAM_MemCpy},NSEEL_PProc_RAM},
  {"memset",_asm_generic3parm,_asm_generic3parm_end,3,{&__NSEEL_RAM_MemSet},NSEEL_PProc_RAM},

  {"stack_push",nseel_asm_stack_push,nseel_asm_stack_push_end,1|BIF_FPSTACKUSE(0),{0,},NSEEL_PProc_Stack},
  {"stack_pop",nseel_asm_stack_pop,nseel_asm_stack_pop_end,1|BIF_FPSTACKUSE(1),{0,},NSEEL_PProc_Stack},
  {"stack_peek",nseel_asm_stack_peek,nseel_asm_stack_peek_end,1|NSEEL_NPARAMS_FLAG_CONST|BIF_LASTPARMONSTACK|BIF_FPSTACKUSE(0),{0,},NSEEL_PProc_Stack},
  {"stack_exch",nseel_asm_stack_exch,nseel_asm_stack_exch_end,1|BIF_FPSTACKUSE(1), {0,},NSEEL_PProc_Stack_PeekTop},
};

static functionType *fnTableUser;
static int fnTableUser_size;

functionType *nseel_getFunctionFromTable(int idx)
{
  if (idx<0) return 0;
  if (idx>=sizeof(fnTable1)/sizeof(fnTable1[0]))
  {
    idx -= sizeof(fnTable1)/sizeof(fnTable1[0]);
    if (!fnTableUser || idx >= fnTableUser_size) return 0;
    return fnTableUser+idx;
  }
  return fnTable1+idx;
}

int NSEEL_init() // returns 0 on success
{

#ifdef EEL_VALIDATE_FSTUBS
  int a;
  for (a=0;a < sizeof(fnTable1)/sizeof(fnTable1[0]);a++)
  {
    char *code_startaddr = (char*)fnTable1[a].afunc;
    char *endp = (char *)fnTable1[a].func_e;
    // validate
    int sz=0;
    char *f=(char *)GLUE_realAddress(code_startaddr,endp,&sz);

    if (f+sz > endp) 
    {
#ifdef _WIN32
      OutputDebugString("bad eel function stub\n");
#else
      printf("bad eel function stub\n");
#endif
      *(char *)NULL = 0;
    }
  }
#ifdef _WIN32
      OutputDebugString("eel function stub (builtin) validation complete\n");
#else
      printf("eel function stub (builtin) validation complete\n");
#endif
#endif

  NSEEL_quit();
  return 0;
}

void NSEEL_addfunctionex2(const char *name, int nparms, char *code_startaddr, int code_len, NSEEL_PPPROC pproc, void *fptr, void *fptr2)
{
  if (!fnTableUser || !(fnTableUser_size&7))
  {
    fnTableUser=(functionType *)realloc(fnTableUser,(fnTableUser_size+8)*sizeof(functionType));
  }
  if (fnTableUser)
  {

#ifdef EEL_VALIDATE_FSTUBS
    {
      char *endp = code_startaddr+code_len;
      // validate
      int sz=0;
      char *f=(char *)GLUE_realAddress(code_startaddr,endp,&sz);

      if (f+sz > endp) 
      {
#ifdef _WIN32
        OutputDebugString("bad eel function stub\n");
#else
        printf("bad eel function stub\n");
#endif
        *(char *)NULL = 0;
      }
#ifdef _WIN32
      OutputDebugString(name);
      OutputDebugString(" - validated eel function stub\n");
#else
      printf("eel function stub validation complete for %s\n",name);
#endif
    }
#endif

    memset(&fnTableUser[fnTableUser_size],0,sizeof(functionType));

    if (!(nparms & BIF_RETURNSBOOL)) 
    {
      if (code_startaddr == (void *)&_asm_generic1parm_retd || 
          code_startaddr == (void *)&_asm_generic2parm_retd ||
          code_startaddr == (void *)&_asm_generic3parm_retd)
      {
        nparms |= BIF_RETURNSONSTACK;
      }
    }
    fnTableUser[fnTableUser_size].nParams = nparms;
    fnTableUser[fnTableUser_size].name = name;
    fnTableUser[fnTableUser_size].afunc = code_startaddr;
    fnTableUser[fnTableUser_size].func_e = code_startaddr + code_len;
    fnTableUser[fnTableUser_size].pProc = pproc;
    fnTableUser[fnTableUser_size].replptrs[0]=fptr;
    fnTableUser[fnTableUser_size].replptrs[1]=fptr2;
    fnTableUser_size++;
  }
}

void NSEEL_quit()
{
  free(fnTableUser);
  fnTableUser_size=0;
  fnTableUser=0;
}

//---------------------------------------------------------------------------------------------------------------
static void freeBlocks(llBlock **start)
{
  llBlock *s=*start;
  *start=0;
  while (s)
  {
    llBlock *llB = s->next;
    free(s);
    s=llB;
  }
}

//---------------------------------------------------------------------------------------------------------------
static void *__newBlock(llBlock **start, int size, int wantMprotect)
{
#if !defined(EEL_DOESNT_NEED_EXEC_PERMS) && defined(_WIN32)
  DWORD ov;
  UINT_PTR offs,eoffs;
#endif
  llBlock *llb;
  int alloc_size;
  if (*start && (LLB_DSIZE - (*start)->sizeused) >= size)
  {
    void *t=(*start)->block+(*start)->sizeused;
    (*start)->sizeused+=(size+7)&~7;
    return t;
  }

  alloc_size=sizeof(llBlock);
  if ((int)size > LLB_DSIZE) alloc_size += size - LLB_DSIZE;
  llb = (llBlock *)malloc(alloc_size); // grab bigger block if absolutely necessary (heh)
  if (!llb) return NULL;
  
#ifndef EEL_DOESNT_NEED_EXEC_PERMS
  if (wantMprotect) 
  {
  #ifdef _WIN32
    offs=((UINT_PTR)llb)&~4095;
    eoffs=((UINT_PTR)llb + alloc_size + 4095)&~4095;
    VirtualProtect((LPVOID)offs,eoffs-offs,PAGE_EXECUTE_READWRITE,&ov);
  //  MessageBox(NULL,"vprotecting, yay\n","a",0);
  #elif defined(EEL_USE_MPROTECT)
    {
      static int pagesize = 0;
      if (!pagesize)
      {
        pagesize=sysconf(_SC_PAGESIZE);
        if (!pagesize) pagesize=4096;
      }
      uintptr_t offs,eoffs;
      offs=((uintptr_t)llb)&~(pagesize-1);
      eoffs=((uintptr_t)llb + alloc_size + pagesize-1)&~(pagesize-1);
      mprotect((void*)offs,eoffs-offs,PROT_WRITE|PROT_READ|PROT_EXEC);
    }
  #endif
  }
#endif
  llb->sizeused=(size+7)&~7;
  llb->next = *start;  
  *start = llb;
  return llb->block;
}


//---------------------------------------------------------------------------------------------------------------
opcodeRec *nseel_createCompiledValue(compileContext *ctx, EEL_F value)
{
  opcodeRec *r=newOpCode(ctx);
  if (r)
  {
    r->opcodeType = OPCODETYPE_DIRECTVALUE;
    r->parms.dv.directValue = value; 
    r->parms.dv.valuePtr = NULL;
  }
  return r;
}

opcodeRec *nseel_createCompiledValueFromNamespaceName(compileContext *ctx, const char *relName)
{
  int n=strlen(relName);
  opcodeRec *r=(opcodeRec*)__newBlock_align(ctx,sizeof(opcodeRec)+NSEEL_MAX_VARIABLE_NAMELEN,8, ctx->isSharedFunctions ? 0 : -1); 
  if (!r) return 0;
  r->opcodeType=OPCODETYPE_VALUE_FROM_NAMESPACENAME;
  if (n > NSEEL_MAX_VARIABLE_NAMELEN) n=NSEEL_MAX_VARIABLE_NAMELEN;
  memcpy(r->relname,relName,n);
  r->relname[n]=0;
  return r;
}

opcodeRec *nseel_createCompiledValuePtr(compileContext *ctx, EEL_F *addrValue)
{
  opcodeRec *r=newOpCode(ctx);
  if (r)
  {
    r->opcodeType = OPCODETYPE_VARPTR;
    r->parms.dv.valuePtr=addrValue;
    r->parms.dv.directValue=0.0;
  }
  return r;
}

opcodeRec *nseel_createCompiledValuePtrPtr(compileContext *ctx, EEL_F **addrValue)
{
  opcodeRec *r=newOpCode(ctx);
  if (r)
  {
    r->opcodeType = OPCODETYPE_VARPTRPTR;
    r->parms.dv.valuePtr=(EEL_F *)addrValue;
    r->parms.dv.directValue=0.0;
  }
  return r;
}

opcodeRec *nseel_createCompiledFunctionCallEELThis(compileContext *ctx, _codeHandleFunctionRec *fnp, const char *relName)
{
  int n=strlen(relName);
  opcodeRec *r=(opcodeRec*)__newBlock_align(ctx,sizeof(opcodeRec)+NSEEL_MAX_VARIABLE_NAMELEN,8, ctx->isSharedFunctions ? 0 : -1); 
  if (!r) return 0;
  r->fntype=FUNCTYPE_EELFUNC_THIS;
  r->fn = fnp;
  if (fnp->num_params > 3) r->opcodeType = OPCODETYPE_FUNCX;
  else if (fnp->num_params == 3) r->opcodeType = OPCODETYPE_FUNC3;
  else if (fnp->num_params==2) r->opcodeType = OPCODETYPE_FUNC2;
  else r->opcodeType = OPCODETYPE_FUNC1;
  if (n > NSEEL_MAX_VARIABLE_NAMELEN) n=NSEEL_MAX_VARIABLE_NAMELEN;
  memcpy(r->relname,relName,n);
  r->relname[n]=0;

  return r;
}

opcodeRec *nseel_createCompiledFunctionCall(compileContext *ctx, int np, int fntype, void *fn)
{
  opcodeRec *r=newOpCode(ctx);
  if (!r) return 0;
  r->fntype=fntype;
  r->fn = fn;

  if (np > 3) r->opcodeType = OPCODETYPE_FUNCX;
  else if (np == 3) r->opcodeType = OPCODETYPE_FUNC3;
  else if (np==2) r->opcodeType = OPCODETYPE_FUNC2;
  else r->opcodeType = OPCODETYPE_FUNC1;

  return r;
}

opcodeRec *nseel_setCompiledFunctionCallParameters(opcodeRec *fn, opcodeRec *code1, opcodeRec *code2, opcodeRec *code3)
{
  if (fn)
  {
    opcodeRec *r = fn;
    r->parms.parms[0] = code1;
    r->parms.parms[1] = code2;
    r->parms.parms[2] = code3;
  }
  return fn;
}


opcodeRec *nseel_createMoreParametersOpcode(compileContext *ctx, opcodeRec *code1, opcodeRec *code2)
{
  opcodeRec *r=code1 && code2 ? newOpCode(ctx) : NULL;
  if (r)
  {
    r->opcodeType = OPCODETYPE_MOREPARAMS;
    r->parms.parms[0] = code1;
    r->parms.parms[1] = code2;
    r->parms.parms[2] = 0;
  }
  return r;
}

opcodeRec *nseel_createSimpleCompiledFunction(compileContext *ctx, int fn, int np, opcodeRec *code1, opcodeRec *code2)
{
  opcodeRec *r=code1 && (np<2 || code2) ? newOpCode(ctx) : NULL;
  if (r)
  {
    r->opcodeType = np>=2 ? OPCODETYPE_FUNC2:OPCODETYPE_FUNC1;
    r->fntype = fn;
    r->fn = fn == FN_JOIN_STATEMENTS ? r : NULL; // for joins, fn is temporarily used for tail pointers
    if (code1 && fn == FN_JOIN_STATEMENTS && code1->opcodeType == OPCODETYPE_FUNC2 && code1->fntype == fn)
    {
      opcodeRec *t = (opcodeRec *)code1->fn;
      // keep joins in the form of dosomething->morestuff. 
      // in this instance, code1 is previous stuff to do, code2 is new stuff to do
      r->parms.parms[0] = t->parms.parms[1];
      r->parms.parms[1] = code2;

      code1->fn = (t->parms.parms[1] = r);
      return code1;
    }
    r->parms.parms[0] = code1;
    r->parms.parms[1] = code2;
  }
  return r;  
}


// these are bitmasks; on request you can tell what is supported, and compileOpcodes will return one of them
#define RETURNVALUE_IGNORE 0 // ignore return value
#define RETURNVALUE_NORMAL 1 // pointer
#define RETURNVALUE_FPSTACK 2
#define RETURNVALUE_BOOL 4 // P1 is nonzero if true




static int compileOpcodes(compileContext *ctx, opcodeRec *op, unsigned char *bufOut, int bufOut_len, int *computTable, const char *namespacePathToThis, 
                          int supportedReturnValues, int *rvType, int *fpStackUsage, int *canHaveDenormalOutput);


static unsigned char *compileCodeBlockWithRet(compileContext *ctx, opcodeRec *rec, int *computTableSize, const char *namespacePathToThis, 
                                              int supportedReturnValues, int *rvType, int *fpStackUse, int *canHaveDenormalOutput);

_codeHandleFunctionRec *eel_createFunctionNamespacedInstance(compileContext *ctx, _codeHandleFunctionRec *fr, const char *nameptr)
{
  int n;
  _codeHandleFunctionRec *subfr = fr->isCommonFunction ? newDataBlock(sizeof(_codeHandleFunctionRec),8) : newTmpBlock(ctx,sizeof(_codeHandleFunctionRec));
  if (!subfr) return 0;
  // fr points to functionname()'s rec, nameptr to blah.functionname()

  *subfr = *fr;
  n = strlen(nameptr);
  if (n > sizeof(subfr->fname)-1) n=sizeof(subfr->fname)-1;
  memcpy(subfr->fname,nameptr,n);
  subfr->fname[n]=0;

  subfr->next = NULL;
  subfr->startptr=0; // make sure this code gets recompiled (with correct member ptrs) for this instance!

  // subfr->derivedCopies already points to the right place
  fr->derivedCopies = subfr; 
  
  return subfr;

}
static void combineNamespaceFields(char *nm, const char *prefix, const char *relname) // nm must be NSEEL_MAX_VARIABLE_NAMELEN+1 bytes
{
  int lfp = prefix ? strlen(prefix) : 0, lrn=strlen(relname);

  while (*relname == '.') // if relname begins with ., then remove a chunk of context from prefix
  {
    relname++;
    while (lfp>0 && prefix[lfp-1] != '.') lfp--;
    if (lfp>0) lfp--;       
  }

  if (lfp > NSEEL_MAX_VARIABLE_NAMELEN-3) lfp=NSEEL_MAX_VARIABLE_NAMELEN-3;
  if (lfp>0) memcpy(nm,prefix,lfp);

  if (lrn > NSEEL_MAX_VARIABLE_NAMELEN - lfp - (lfp>0)) lrn=NSEEL_MAX_VARIABLE_NAMELEN - lfp - (lfp>0);
  if (lrn > 0)
  {
    if (lfp>0) nm[lfp++] = '.';
    memcpy(nm+lfp,relname,lrn);
    lfp+=lrn;
  }
  nm[lfp++]=0;
}


//---------------------------------------------------------------------------------------------------------------
static void *nseel_getBuiltinFunctionAddress(compileContext *ctx, 
      int fntype, void *fn, 
      NSEEL_PPPROC *pProc, void ***replList, 
      void **endP, int *abiInfo, int preferredReturnValues)
{
  switch (fntype)
  {
#define RF(x) *endP = nseel_asm_##x##_end; return (void*)nseel_asm_##x
    case FN_ADD: *abiInfo = BIF_RETURNSONSTACK|BIF_TWOPARMSONFPSTACK_LAZY|BIF_FPSTACKUSE(2)|BIF_WONTMAKEDENORMAL; RF(add);
    case FN_SUB: *abiInfo = BIF_RETURNSONSTACK|BIF_TWOPARMSONFPSTACK|BIF_FPSTACKUSE(2)|BIF_WONTMAKEDENORMAL; RF(sub);
    case FN_MULTIPLY: *abiInfo = BIF_RETURNSONSTACK|BIF_TWOPARMSONFPSTACK_LAZY|BIF_FPSTACKUSE(2); RF(mul);
    case FN_DIVIDE: *abiInfo = BIF_RETURNSONSTACK|BIF_TWOPARMSONFPSTACK|BIF_FPSTACKUSE(2); RF(div);
#ifndef EEL_TARGET_PORTABLE
    case FN_JOIN_STATEMENTS: *abiInfo = BIF_WONTMAKEDENORMAL; RF(exec2); // shouldn't ever be used anyway, but scared to remove
#endif
    case FN_AND: *abiInfo = BIF_RETURNSONSTACK|BIF_TWOPARMSONFPSTACK_LAZY|BIF_FPSTACKUSE(2)|BIF_CLEARDENORMAL; RF(and);
    case FN_OR: *abiInfo = BIF_RETURNSONSTACK|BIF_TWOPARMSONFPSTACK_LAZY|BIF_FPSTACKUSE(2)|BIF_CLEARDENORMAL; RF(or);
#ifndef EEL_TARGET_PORTABLE
    case FN_UPLUS: *abiInfo = BIF_WONTMAKEDENORMAL; RF(uplus);   // shouldn't ever be used anyway, but scared to remove
#endif
    case FN_UMINUS: *abiInfo = BIF_RETURNSONSTACK|BIF_LASTPARMONSTACK|BIF_WONTMAKEDENORMAL; RF(uminus);
#undef RF

    case FUNCTYPE_FUNCTIONTYPEREC:
      if (fn)
      {
        functionType *p=(functionType *)fn;

        // if prefers fpstack or bool, or ignoring value, then use fp-stack versions
        if ((preferredReturnValues&(RETURNVALUE_BOOL|RETURNVALUE_FPSTACK)) || !preferredReturnValues)
        {
          static functionType min2={ "min",    nseel_asm_min_fp,nseel_asm_min_fp_end,   2|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_TWOPARMSONFPSTACK_LAZY|BIF_FPSTACKUSE(2)|BIF_WONTMAKEDENORMAL };
          static functionType max2={ "max",    nseel_asm_max_fp,nseel_asm_max_fp_end,   2|NSEEL_NPARAMS_FLAG_CONST|BIF_RETURNSONSTACK|BIF_TWOPARMSONFPSTACK_LAZY|BIF_FPSTACKUSE(2)|BIF_WONTMAKEDENORMAL };

          if (p->afunc == (void*)nseel_asm_min) p = &min2;
          else if (p->afunc == (void*)nseel_asm_max) p = &max2;
        }

        *replList=p->replptrs;
        *pProc=p->pProc;
        *endP = p->func_e;
        *abiInfo = p->nParams & BIF_NPARAMS_MASK;
        return p->afunc; 
      }
    break;
  }
  
  return 0;
}



static void *nseel_getEELFunctionAddress(compileContext *ctx, 
      opcodeRec *op,
      int *customFuncParmSize, int *customFuncLocalStorageSize,
      EEL_F ***customFuncLocalStorage, int *computTableTop, 
      void **endP, int *isRaw, int wantCodeGenerated,
      const char *namespacePathToThis, int *rvMode, int *fpStackUse, int *canHaveDenormalOutput) // if wantCodeGenerated is false, can return bogus pointers in raw mode
{
  _codeHandleFunctionRec *fn = (_codeHandleFunctionRec*)op->fn;
  switch (op->fntype)
  {
    case FUNCTYPE_EELFUNC_THIS:
      if (fn)
      {
        _codeHandleFunctionRec *fr_base = fn;
        char nm[NSEEL_MAX_VARIABLE_NAMELEN+1];
        combineNamespaceFields(nm,namespacePathToThis,op->relname);

        fn = 0; // if this gets re-set, it will be the new function

        // find resolved function
        if (!fn)
        {
          _codeHandleFunctionRec *fr = fr_base;
          // scan for function
          while (fr && !fn)
          {
            if (!strcasecmp(fr->fname,nm)) fn = fr;
            fr=fr->derivedCopies;
          }
        }

        if (!fn) // generate copy of function
        {
          fn = eel_createFunctionNamespacedInstance(ctx,fr_base,nm);
        }
      }

      // fall through!
    case FUNCTYPE_EELFUNC:
      if (fn)
      {
        const char *fPrefix=NULL;
        char prefix_buf[NSEEL_MAX_VARIABLE_NAMELEN+1];

        _codeHandleFunctionRec *fr = (_codeHandleFunctionRec *) fn;

        if (fr->usesThisPointer)
        {
          char *p=fr->fname;
          while (*p) p++;
          while (p >= fr->fname && *p != '.') p--;
          if (p >= fr->fname)
          {
            int l = p-fr->fname;
            memcpy(prefix_buf,fr->fname,l);
            prefix_buf[l]=0;
            fPrefix = prefix_buf;
          }
          else
          {
            fPrefix = fr->fname; // default prefix is function name if no other context
          }
        }


        if (!fr->startptr && fr->opcodes && fr->startptr_size > 0)
        {
          int sz;
          fr->tmpspace_req=0;
          fr->rvMode = RETURNVALUE_IGNORE;
          fr->canHaveDenormalOutput=0;

          sz=compileOpcodes(ctx,fr->opcodes,NULL,128*1024*1024,&fr->tmpspace_req,fPrefix,RETURNVALUE_NORMAL|RETURNVALUE_FPSTACK,&fr->rvMode,&fr->fpStackUsage,&fr->canHaveDenormalOutput);

          if (!wantCodeGenerated)
          {
            // don't compile anything for now, just give stats
            if (computTableTop) *computTableTop += fr->tmpspace_req;
            *customFuncParmSize = fr->num_params;
            *customFuncLocalStorage = fr->localstorage;
            *customFuncLocalStorageSize = fr->localstorage_size;
            *rvMode = fr->rvMode;
            *fpStackUse = fr->fpStackUsage;
            if (canHaveDenormalOutput) *canHaveDenormalOutput=fr->canHaveDenormalOutput;

            if (sz <= NSEEL_MAX_FUNCTION_SIZE_FOR_INLINE && !(ctx->optimizeDisableFlags&OPTFLAG_NO_INLINEFUNC))
            {
              *isRaw = 1;
              *endP = ((char *)1) + sz;
              return (char *)1;
            }
            *endP = (void*)nseel_asm_fcall_end;
            return (void*)nseel_asm_fcall;
          }

          if (sz <= NSEEL_MAX_FUNCTION_SIZE_FOR_INLINE && !(ctx->optimizeDisableFlags&OPTFLAG_NO_INLINEFUNC))
          {
            void *p=newTmpBlock(ctx,sz);
            fr->tmpspace_req=0;
            if (p)
            {
              fr->canHaveDenormalOutput=0;
              sz=compileOpcodes(ctx,fr->opcodes,(unsigned char*)p,sz,&fr->tmpspace_req,fPrefix,RETURNVALUE_NORMAL|RETURNVALUE_FPSTACK,&fr->rvMode,&fr->fpStackUsage,&fr->canHaveDenormalOutput);
              // recompile function with native context pointers
              if (sz>0)
              {
                fr->startptr_size=sz;
                fr->startptr=p;
              }
            }
          }
          else
          {
            unsigned char *codeCall;
            fr->tmpspace_req=0;
            fr->fpStackUsage=0;
            fr->canHaveDenormalOutput=0;
            codeCall=compileCodeBlockWithRet(ctx,fr->opcodes,&fr->tmpspace_req,fPrefix,RETURNVALUE_NORMAL|RETURNVALUE_FPSTACK,&fr->rvMode,&fr->fpStackUsage,&fr->canHaveDenormalOutput);
            if (codeCall)
            {
              void *f=GLUE_realAddress(nseel_asm_fcall,nseel_asm_fcall_end,&sz);
              fr->startptr = newTmpBlock(ctx,sz);
              if (fr->startptr)
              {
                memcpy(fr->startptr,f,sz);
                EEL_GLUE_set_immediate(fr->startptr,(INT_PTR)codeCall);
                fr->startptr_size = sz;
              }
            }
          }
        }
        if (fr->startptr)
        {
          if (computTableTop) *computTableTop += fr->tmpspace_req;
          *customFuncParmSize = fr->num_params;
          *customFuncLocalStorage = fr->localstorage;
          *customFuncLocalStorageSize = fr->localstorage_size;
          *rvMode = fr->rvMode;
          *fpStackUse = fr->fpStackUsage;
          if (canHaveDenormalOutput) *canHaveDenormalOutput= fr->canHaveDenormalOutput;
          *endP = (char*)fr->startptr + fr->startptr_size;
          *isRaw=1;
          return fr->startptr;
        }
      }
    break;
  }
  
  return 0;
}



// returns true if does something (other than calculating and throwing away a value)
static char optimizeOpcodes(compileContext *ctx, opcodeRec *op, int needsResult)
{
  opcodeRec *lastJoinOp=NULL;
  char retv, retv_parm[3], joined_retv=0;
  while (op && op->opcodeType == OPCODETYPE_FUNC2 && op->fntype == FN_JOIN_STATEMENTS)
  {
    if (!optimizeOpcodes(ctx,op->parms.parms[0], 0) || OPCODE_IS_TRIVIAL(op->parms.parms[0]))
    {
      // direct value, can skip ourselves
      memcpy(op,op->parms.parms[1],sizeof(*op));
    }
    else
    {
      joined_retv |= 1;
      lastJoinOp = op;
      op = op->parms.parms[1];
    }
  }
  
start_over: // when an opcode changed substantially in optimization, goto here to reprocess it

  retv = retv_parm[0]=retv_parm[1]=retv_parm[2]=0;

  if (!op || // should never really happen
      OPCODE_IS_TRIVIAL(op) || // should happen often (vars)
      op->opcodeType < 0 || op->opcodeType >= OPCODETYPE_INVALID // should never happen (assert would be appropriate heh)
      ) return joined_retv;
  
  if (!needsResult)
  {
    if (op->fntype == FUNCTYPE_EELFUNC || op->fntype == FUNCTYPE_EELFUNC_THIS) 
    {
      needsResult=1; // assume eel functions are non-const for now
    }
    else if (op->fntype == FUNCTYPE_FUNCTIONTYPEREC)
    {
      functionType  *pfn = (functionType *)op->fn;
      if (!pfn || !(pfn->nParams&NSEEL_NPARAMS_FLAG_CONST)) needsResult=1;
    }
  }

  if (op->opcodeType>=OPCODETYPE_FUNC2) retv_parm[1] = optimizeOpcodes(ctx,op->parms.parms[1], needsResult);
  if (op->opcodeType>=OPCODETYPE_FUNC3) retv_parm[2] = optimizeOpcodes(ctx,op->parms.parms[2], needsResult);

  retv_parm[0] = optimizeOpcodes(ctx,op->parms.parms[0], needsResult || 
      (FNPTR_HAS_CONDITIONAL_EXEC(op) && (retv_parm[1] || retv_parm[2] || op->opcodeType <= OPCODETYPE_FUNC1)) );

  if (op->opcodeType != OPCODETYPE_MOREPARAMS)
  {
    if (op->fntype >= 0 && op->fntype < FUNCTYPE_SIMPLEMAX)
    {
      if (op->opcodeType == OPCODETYPE_FUNC1) // within FUNCTYPE_SIMPLE
      {
        if (op->parms.parms[0]->opcodeType == OPCODETYPE_DIRECTVALUE)
        {
          switch (op->fntype)
          {
            case FN_UMINUS:
              op->opcodeType = OPCODETYPE_DIRECTVALUE;
              op->parms.dv.directValue = - op->parms.parms[0]->parms.dv.directValue;
              op->parms.dv.valuePtr=NULL;
            goto start_over;
            case FN_UPLUS:
              op->opcodeType = OPCODETYPE_DIRECTVALUE;
              op->parms.dv.directValue = op->parms.parms[0]->parms.dv.directValue;
              op->parms.dv.valuePtr=NULL;
            goto start_over;
          }
        }
      }
      else if (op->opcodeType == OPCODETYPE_FUNC2)  // within FUNCTYPE_SIMPLE
      {
        int dv0 = op->parms.parms[0]->opcodeType == OPCODETYPE_DIRECTVALUE;
        int dv1 = op->parms.parms[1]->opcodeType == OPCODETYPE_DIRECTVALUE;
        if (dv0 && dv1)
        {
          switch (op->fntype)
          {
            case FN_DIVIDE:
              op->opcodeType = OPCODETYPE_DIRECTVALUE;
              op->parms.dv.directValue = op->parms.parms[0]->parms.dv.directValue / op->parms.parms[1]->parms.dv.directValue;
              op->parms.dv.valuePtr=NULL;
            goto start_over;
            case FN_MULTIPLY:
              op->opcodeType = OPCODETYPE_DIRECTVALUE;
              op->parms.dv.directValue = op->parms.parms[0]->parms.dv.directValue * op->parms.parms[1]->parms.dv.directValue;
              op->parms.dv.valuePtr=NULL;
            goto start_over;
            case FN_ADD:
              op->opcodeType = OPCODETYPE_DIRECTVALUE;
              op->parms.dv.directValue = op->parms.parms[0]->parms.dv.directValue + op->parms.parms[1]->parms.dv.directValue;
              op->parms.dv.valuePtr=NULL;
            goto start_over;
            case FN_SUB:
              op->opcodeType = OPCODETYPE_DIRECTVALUE;
              op->parms.dv.directValue = op->parms.parms[0]->parms.dv.directValue - op->parms.parms[1]->parms.dv.directValue;
              op->parms.dv.valuePtr=NULL;
            goto start_over;
            case FN_AND:
              op->opcodeType = OPCODETYPE_DIRECTVALUE;
              op->parms.dv.directValue = (double) (((WDL_INT64)op->parms.parms[0]->parms.dv.directValue) & ((WDL_INT64)op->parms.parms[1]->parms.dv.directValue));
              op->parms.dv.valuePtr=NULL;
            goto start_over;
            case FN_OR:
              op->opcodeType = OPCODETYPE_DIRECTVALUE;
              op->parms.dv.directValue = (double) (((WDL_INT64)op->parms.parms[0]->parms.dv.directValue) | ((WDL_INT64)op->parms.parms[1]->parms.dv.directValue));
              op->parms.dv.valuePtr=NULL;
            goto start_over;
          }
        }
        else if (dv0 || dv1)
        {
          double dvalue = op->parms.parms[!dv0]->parms.dv.directValue;
          switch (op->fntype)
          {
            case FN_OR:
              if (!(WDL_INT64)dvalue)
              {
                // replace with or0
                static functionType fr={"or0",nseel_asm_or0, nseel_asm_or0_end, 1|NSEEL_NPARAMS_FLAG_CONST|BIF_LASTPARMONSTACK|BIF_RETURNSONSTACK|BIF_CLEARDENORMAL, {0}, NULL};

                op->opcodeType = OPCODETYPE_FUNC1;
                op->fntype = FUNCTYPE_FUNCTIONTYPEREC;
                op->fn = &fr;
                if (dv0) op->parms.parms[0] = op->parms.parms[1];
                goto start_over;
              }
            break;
            case FN_SUB:
              if (dv0) 
              {
                if (dvalue == 0.0)
                {
                  op->opcodeType = OPCODETYPE_FUNC1;
                  op->fntype = FN_UMINUS;
                  op->parms.parms[0] = op->parms.parms[1];
                  goto start_over;
                }
                break;
              }
              // fall through, if dv1 we can remove +0.0

            case FN_ADD:
              if (dvalue == 0.0) 
              {
                memcpy(op,op->parms.parms[!!dv0],sizeof(*op));
                goto start_over;
              }
            break;
            case FN_AND:
              if ((WDL_INT64)dvalue) break;
              dvalue = 0.0; // treat x&0 as x*0, which optimizes to 0
            
              // fall through
            case FN_MULTIPLY:
              if (dvalue == 0.0) // remove multiply by 0.0 (using 0.0 direct value as replacement), unless the nonzero side did something
              {
                if (!retv_parm[!!dv0]) 
                {
                  memcpy(op,op->parms.parms[!dv0],sizeof(*op)); // set to 0 if other action wouldn't do anything
                  goto start_over;
                }
                else
                {
                  // this is 0.0 * oldexpressionthatmustbeprocessed or oldexpressionthatmustbeprocessed*0.0
                  op->fntype = FN_JOIN_STATEMENTS;

                  if (dv0) // 0.0*oldexpression, reverse the order so that 0 is returned
                  {
                    // set to (oldexpression;0)
                    opcodeRec *tmp = op->parms.parms[1];
                    op->parms.parms[1] = op->parms.parms[0];
                    op->parms.parms[0] = tmp;
                  }
                  goto start_over;
                }
              }
              else if (dvalue == 1.0) // remove multiply by 1.0 (using non-1.0 value as replacement)
              {
                memcpy(op,op->parms.parms[!!dv0],sizeof(*op));
                goto start_over;
              }
            break;
            case FN_DIVIDE:
              if (dv1)
              {
                if (dvalue == 1.0)  // remove divide by 1.0  (using non-1.0 value as replacement)
                {
                  memcpy(op,op->parms.parms[!!dv0],sizeof(*op));
                  goto start_over;
                }
                else
                {
                  // change to a multiply
                  if (op->parms.parms[1]->parms.dv.directValue == 0.0)
                  {
                    op->fntype = FN_MULTIPLY;
                    goto start_over;
                  }
                  else
                  {
                    double d = 1.0/op->parms.parms[1]->parms.dv.directValue;

                    WDL_DenormalDoubleAccess *p = (WDL_DenormalDoubleAccess*)&d;
                    // allow conversion to multiply if reciprocal is exact
                    // we could also just look to see if the last few digits of the mantissa were 0, which would probably be good
                    // enough, but if the user really wants it they should do * (1/x) instead to force precalculation of reciprocal.
                    if (!p->w.lw && !(p->w.hw & 0xfffff)) 
                    {
                      op->fntype = FN_MULTIPLY;
                      op->parms.parms[1]->parms.dv.directValue = d;
                      op->parms.parms[1]->parms.dv.valuePtr=NULL;
                      goto start_over;
                    }
                  }
                }
              }
              else if (dvalue == 0.0)
              {
                if (!retv_parm[!!dv0])
                {
                  // if 0/x set to always 0.
                  // this is 0.0 / (oldexpression that can be eliminated)
                  memcpy(op,op->parms.parms[!dv0],sizeof(*op)); // set to 0 if other action wouldn't do anything
                }
                else
                {
                  opcodeRec *tmp;
                  // this is 0.0 / oldexpressionthatmustbeprocessed
                  op->fntype = FN_JOIN_STATEMENTS;
                  tmp = op->parms.parms[1];
                  op->parms.parms[1] = op->parms.parms[0];
                  op->parms.parms[0] = tmp;
                  // set to (oldexpression;0)
                }
                goto start_over;
              }
            break;
          }
        }
      }
      // FUNCTYPE_SIMPLE
    }   
    else if (op->fntype == FUNCTYPE_FUNCTIONTYPEREC && op->fn)
    {

      /*
      probably worth doing reduction on:
      _divop (constant change to multiply)
      _and
      _or
      _not
      _mod
      _shr
      _shl
      _xor
      abs

      maybe:
      min
      max
      _equal
      _noteq
      _below
      _above
      _beleq
      _aboeq


      also, optimize should (recursively or maybe iteratively?) search transitive functions (mul/div) for more constant reduction possibilities


      */


      functionType  *pfn = (functionType *)op->fn;

      if (!(pfn->nParams&NSEEL_NPARAMS_FLAG_CONST)) retv|=1;

      if (op->opcodeType==OPCODETYPE_FUNC1) // within FUNCTYPE_FUNCTIONTYPEREC
      {
        if (op->parms.parms[0]->opcodeType == OPCODETYPE_DIRECTVALUE)
        {
          int suc=1;
          EEL_F v = op->parms.parms[0]->parms.dv.directValue;
  #define DOF(x) if (!strcmp(pfn->name,#x)) v = x(v); else
          DOF(sin)
          DOF(cos)
          DOF(tan)
          DOF(asin)
          DOF(acos)
          DOF(atan)
          DOF(sqrt)
          DOF(exp)
          DOF(log)
          DOF(log10)
          /*else*/ suc=0;
  #undef DOF
          if (suc)
          {
            op->opcodeType = OPCODETYPE_DIRECTVALUE;
            op->parms.dv.directValue = v;
            op->parms.dv.valuePtr=NULL;
            goto start_over;
          }


        }
      }
      else if (op->opcodeType==OPCODETYPE_FUNC2)  // within FUNCTYPE_FUNCTIONTYPEREC
      {
        if (op->parms.parms[0]->opcodeType == OPCODETYPE_DIRECTVALUE &&
            op->parms.parms[1]->opcodeType == OPCODETYPE_DIRECTVALUE)
        {
          if (pfn->replptrs[0] == &pow || 
              pfn->replptrs[0] == &atan2) 
          {
            op->opcodeType = OPCODETYPE_DIRECTVALUE;
            op->parms.dv.directValue = pfn->replptrs[0]==pow ? 
              pow(op->parms.parms[0]->parms.dv.directValue, op->parms.parms[1]->parms.dv.directValue) :
              atan2(op->parms.parms[0]->parms.dv.directValue, op->parms.parms[1]->parms.dv.directValue);
            op->parms.dv.valuePtr=NULL;
            goto start_over;
          }
        }
        else if (pfn->replptrs[0] == &pow)
        {
          opcodeRec *first_parm = op->parms.parms[0];
          if (first_parm->opcodeType == op->opcodeType && first_parm->fn == op->fn && first_parm->fntype == op->fntype)
          {            
            // since first_parm is a pow too, we can multiply the exponents.

            // set our base to be the base of the inner pow
            op->parms.parms[0] = first_parm->parms.parms[0];

            // make the old extra pow be a multiply of the exponents
            first_parm->fntype = FN_MULTIPLY;
            first_parm->parms.parms[0] = op->parms.parms[1];

            // put that as the exponent
            op->parms.parms[1] = first_parm;

            goto start_over;
          }
        }
      }
      else if (op->opcodeType==OPCODETYPE_FUNC3)  // within FUNCTYPE_FUNCTIONTYPEREC
      {
        if (op->parms.parms[0]->opcodeType == OPCODETYPE_DIRECTVALUE)
        {
          if (!strcmp(pfn->name,"_if"))
          {
            int s = fabs(op->parms.parms[0]->parms.dv.directValue) >= NSEEL_CLOSEFACTOR;
            memcpy(op,op->parms.parms[s ? 1 : 2],sizeof(opcodeRec));
            goto start_over;
          }
        }
      }
      // FUNCTYPE_FUNCTIONTYPEREC
    }
    else
    {
      // unknown or eel func, assume non-const
      retv |= 1;
    }
  }

  // if we need results, or our function has effects itself, then finish
  if (retv || needsResult)
  {
    return retv || joined_retv || retv_parm[0] || retv_parm[1] || retv_parm[2];
  }

  // we don't need results here, and our function is const, which means we can remove it
  {
    int cnt=0, idx1=0, idx2=0, x;
    for (x=0;x<3;x++) if (retv_parm[x]) { if (!cnt++) idx1=x; else idx2=x; }

    if (!cnt) // none of the parameters do anything, remove this opcode
    {
      if (lastJoinOp)
      {
        // replace previous join with its first linked opcode, removing this opcode completely
        memcpy(lastJoinOp,lastJoinOp->parms.parms[0],sizeof(*lastJoinOp));
      }
      else if (op->opcodeType != OPCODETYPE_DIRECTVALUE)
      {
        // allow caller to easily detect this as trivial and remove it
        op->opcodeType = OPCODETYPE_DIRECTVALUE;
        op->parms.dv.valuePtr=NULL;
        op->parms.dv.directValue=0.0;
      }
      // return joined_retv below
    }
    else
    {
      // if parameters are non-const, and we're a conditional, preserve our function
      if (FNPTR_HAS_CONDITIONAL_EXEC(op)) return 1;

      // otherwise, condense into either the non-const statement, or a join
      if (cnt==1)
      {
        memcpy(op,op->parms.parms[idx1],sizeof(*op));
      }
      else if (cnt == 2)
      {
        op->opcodeType = OPCODETYPE_FUNC2;
        op->fntype = FN_JOIN_STATEMENTS;
        op->fn = op;
        op->parms.parms[0] = op->parms.parms[idx1];
        op->parms.parms[1] = op->parms.parms[idx2];
        op->parms.parms[2] = NULL;
      }
      else
      {
        // todo need to create a new opcodeRec here, for now just leave as is 
        // (non-conditional const 3 parameter functions are rare anyway)
      }
      return 1;
    }
  }
  return joined_retv;
}


static int generateValueToReg(compileContext *ctx, opcodeRec *op, unsigned char *bufOut, int whichReg, const char *functionPrefix, int allowCache)
{
  EEL_F *b=NULL;
  if (op->opcodeType==OPCODETYPE_VALUE_FROM_NAMESPACENAME)
  {
    char nm[NSEEL_MAX_VARIABLE_NAMELEN+1];

    combineNamespaceFields(nm,functionPrefix,op->relname);
    
    b = nseel_int_register_var(ctx,nm,0);
    if (!b) RET_MINUS1_FAIL("error registering var")
  }
  else
  {
    if (op->opcodeType != OPCODETYPE_DIRECTVALUE) allowCache=0;

    b=op->parms.dv.valuePtr;
    if (b && op->opcodeType == OPCODETYPE_VARPTRPTR) b = *(EEL_F **)b;
    if (!b && allowCache)
    {
      int n=50; // only scan last X items
      opcodeRec *r = ctx->directValueCache;
      while (r && n--)
      {
        if (r->parms.dv.directValue == op->parms.dv.directValue && (b=r->parms.dv.valuePtr)) break;
        r=(opcodeRec*)r->fn;
      }
    }
    if (!b)
    {
      ctx->l_stats[3]++;
      b = newDataBlock(sizeof(EEL_F),sizeof(EEL_F));
      if (!b) RET_MINUS1_FAIL("error allocating data block")

      if (op->opcodeType != OPCODETYPE_VARPTRPTR) op->parms.dv.valuePtr = b;
      #if EEL_F_SIZE == 8
        *b = denormal_filter_double2(op->parms.dv.directValue);
      #else
        *b = denormal_filter_float2(op->parms.dv.directValue);
      #endif

      if (allowCache)
      {
        op->fn = ctx->directValueCache;
        ctx->directValueCache = op;
      }
    }
  }

  GLUE_MOV_PX_DIRECTVALUE_GEN(bufOut,(INT_PTR)b,whichReg);
  return GLUE_MOV_PX_DIRECTVALUE_SIZE;
}


unsigned char *compileCodeBlockWithRet(compileContext *ctx, opcodeRec *rec, int *computTableSize, const char *namespacePathToThis, 
                                       int supportedReturnValues, int *rvType, int *fpStackUsage, int *canHaveDenormalOutput)
{
  unsigned char *p, *newblock2;
  // generate code call
  int funcsz=compileOpcodes(ctx,rec,NULL,1024*1024*128,NULL,namespacePathToThis,supportedReturnValues, rvType,fpStackUsage, NULL);
  if (funcsz<0) return NULL;
 
  p = newblock2 = newCodeBlock(funcsz+ sizeof(GLUE_RET)+GLUE_FUNC_ENTER_SIZE+GLUE_FUNC_LEAVE_SIZE,32);
  if (!newblock2) return NULL;
  #if GLUE_FUNC_ENTER_SIZE > 0
    memcpy(p,&GLUE_FUNC_ENTER,GLUE_FUNC_ENTER_SIZE); 
    p += GLUE_FUNC_ENTER_SIZE;
  #endif
  *fpStackUsage=0;
  funcsz=compileOpcodes(ctx,rec,p, funcsz, computTableSize,namespacePathToThis,supportedReturnValues, rvType,fpStackUsage, canHaveDenormalOutput);         
  if (funcsz<0) return NULL;
  p+=funcsz;

  #if GLUE_FUNC_LEAVE_SIZE > 0
    memcpy(p,&GLUE_FUNC_LEAVE,GLUE_FUNC_LEAVE_SIZE); 
    p+=GLUE_FUNC_LEAVE_SIZE;
  #endif
  memcpy(p,&GLUE_RET,sizeof(GLUE_RET)); p+=sizeof(GLUE_RET);
  
  ctx->l_stats[2]+=funcsz+2;
  return newblock2;
}      


static int compileNativeFunctionCall(compileContext *ctx, opcodeRec *op, unsigned char *bufOut, int bufOut_len, int *computTableSize, const char *namespacePathToThis, 
                                     int *rvMode, int *fpStackUsage, int preferredReturnValues, int *canHaveDenormalOutput)
{
  // builtin function generation
  int cfunc_abiinfo=0;
  int local_fpstack_use=0; // how many items we have pushed onto the fp stack
  int parm_size=0;
  int need_fxch=0;
  int pn;
  int last_nt_parm=-1, last_nt_parm_type;
  void *func_e=NULL;
  int n_params= 1 + op->opcodeType - OPCODETYPE_FUNC1;
  NSEEL_PPPROC preProc=0;
  void **repl=NULL;
  void *func = nseel_getBuiltinFunctionAddress(ctx, op->fntype, op->fn, &preProc,&repl,&func_e,&cfunc_abiinfo,preferredReturnValues);

  if (!func) RET_MINUS1_FAIL("error getting funcaddr")

  if (op->opcodeType == OPCODETYPE_FUNCX)
  {
    // this is not yet supported (calling conventions will need to be sorted, among other things)
    RET_MINUS1_FAIL("funcx not supported for native functions")
  }
  *fpStackUsage=BIF_GETFPSTACKUSE(cfunc_abiinfo);

  *rvMode = RETURNVALUE_NORMAL;

  if (op->parms.parms[0]->opcodeType == OPCODETYPE_DIRECTVALUE)
  {
    if (func == nseel_asm_stack_pop)
    {
      int func_size=0;
      func = GLUE_realAddress(nseel_asm_stack_pop_fast,nseel_asm_stack_pop_fast_end,&func_size);
      if (!func || bufOut_len < func_size) RET_MINUS1_FAIL(func?"failed on popfast size":"failed on popfast addr")

      if (bufOut) 
      {
        memcpy(bufOut,func,func_size);
        NSEEL_PProc_Stack(bufOut,func_size,ctx);
      }
      return func_size;            
    }
    else if (func == nseel_asm_stack_peek)
    {
      int f = (int) op->parms.parms[0]->parms.dv.directValue;
      if (!f)
      {
        int func_size=0;
        func = GLUE_realAddress(nseel_asm_stack_peek_top,nseel_asm_stack_peek_top_end,&func_size);
        if (!func || bufOut_len < func_size) RET_MINUS1_FAIL(func?"failed on peek size":"failed on peek addr")

        if (bufOut) 
        {
          memcpy(bufOut,func,func_size);
          NSEEL_PProc_Stack_PeekTop(bufOut,func_size,ctx);
        }
        return func_size;
      }
      else
      {
        int func_size=0;
        func = GLUE_realAddress(nseel_asm_stack_peek_int,nseel_asm_stack_peek_int_end,&func_size);
        if (!func || bufOut_len < func_size) RET_MINUS1_FAIL(func?"failed on peekint size":"failed on peekint addr")

        if (bufOut)
        {
          memcpy(bufOut,func,func_size);
          NSEEL_PProc_Stack_PeekInt(bufOut,func_size,ctx,f*sizeof(EEL_F));
        }
        return func_size;
      }
    }
  }
  // end of built-in function specific special casing


  // first pass, calculate any non-trivial parameters
  for (pn=0; pn < n_params; pn++)
  { 
    if (!OPCODE_IS_TRIVIAL(op->parms.parms[pn]))
    {
      int canHaveDenorm=0;
      int subfpstackuse=0;
      int lsz=0; 
      int rvt=RETURNVALUE_NORMAL;
      int may_need_fppush=-1;
      if (last_nt_parm>=0)
      {
        if (last_nt_parm_type==RETURNVALUE_FPSTACK)
        {          
          may_need_fppush= parm_size;
        }
        else
        {
          // push last result
          if (bufOut_len < parm_size + (int)sizeof(GLUE_PUSH_P1)) RET_MINUS1_FAIL("failed on size, pushp1")
          if (bufOut) memcpy(bufOut + parm_size, &GLUE_PUSH_P1, sizeof(GLUE_PUSH_P1));
          parm_size += sizeof(GLUE_PUSH_P1);
        }
      }         

      if (pn == n_params - 1)
      {
        if (cfunc_abiinfo&BIF_LASTPARMONSTACK) rvt=RETURNVALUE_FPSTACK;
        else if (cfunc_abiinfo&BIF_LASTPARM_ASBOOL) rvt=RETURNVALUE_BOOL;
        else if (func == nseel_asm_assign) rvt=RETURNVALUE_FPSTACK|RETURNVALUE_NORMAL;
      }
      else if (pn == n_params -2 && (cfunc_abiinfo&BIF_SECONDLASTPARMST))
      {
        rvt=RETURNVALUE_FPSTACK;
      }

      lsz = compileOpcodes(ctx,op->parms.parms[pn],bufOut ? bufOut + parm_size : NULL,bufOut_len - parm_size, computTableSize, namespacePathToThis, rvt,&rvt, &subfpstackuse, &canHaveDenorm);

      if (canHaveDenorm && canHaveDenormalOutput) *canHaveDenormalOutput = 1;

      if (lsz<0) RET_MINUS1_FAIL("call coc failed")

      parm_size += lsz;            

      if (may_need_fppush>=0)
      {
        if (local_fpstack_use+subfpstackuse >= (GLUE_MAX_FPSTACK_SIZE-1) || (ctx->optimizeDisableFlags&OPTFLAG_NO_FPSTACK))
        {
          if (bufOut_len < parm_size + (int)sizeof(GLUE_POP_FPSTACK_TOSTACK)) 
            RET_MINUS1_FAIL("failed on size, popfpstacktostack")

          if (bufOut) 
          {
            memmove(bufOut + may_need_fppush + sizeof(GLUE_POP_FPSTACK_TOSTACK), bufOut + may_need_fppush, parm_size - may_need_fppush);
            memcpy(bufOut + may_need_fppush, &GLUE_POP_FPSTACK_TOSTACK, sizeof(GLUE_POP_FPSTACK_TOSTACK));

          }
          parm_size += sizeof(GLUE_POP_FPSTACK_TOSTACK);
        }
        else
        {
          local_fpstack_use++;
        }
      }

      if (subfpstackuse+local_fpstack_use > *fpStackUsage) *fpStackUsage = subfpstackuse+local_fpstack_use;

      last_nt_parm = pn;
      last_nt_parm_type = rvt;

      if (pn == n_params - 1 && func == nseel_asm_assign)
      {
        if (!(ctx->optimizeDisableFlags & OPTFLAG_FULL_DENORMAL_CHECKS) && 
            (!canHaveDenorm || (ctx->optimizeDisableFlags & OPTFLAG_NO_DENORMAL_CHECKS)))
        {
          if (rvt == RETURNVALUE_FPSTACK)
          {
            cfunc_abiinfo |= BIF_LASTPARMONSTACK;
            func = nseel_asm_assign_fast_fromfp;
            func_e = nseel_asm_assign_fast_fromfp_end;
          }
          else
          {
            func = nseel_asm_assign_fast;
            func_e = nseel_asm_assign_fast_end;
          }
        }
        else
        {
          if (rvt == RETURNVALUE_FPSTACK)
          {
            cfunc_abiinfo |= BIF_LASTPARMONSTACK;
            func = nseel_asm_assign_fromfp;
            func_e = nseel_asm_assign_fromfp_end;
          }
        }
        
      }
    }
  }

  pn = last_nt_parm;
  
  if (pn >= 0) // if the last thing executed doesn't go to the last parameter, move it there
  {
    if ((cfunc_abiinfo&BIF_SECONDLASTPARMST) && pn == n_params-2)
    {
      // do nothing, things are in the right place
    }
    else if (pn != n_params-1)
    {
      // generate mov p1->pX
      if (bufOut_len < parm_size + GLUE_SET_PX_FROM_P1_SIZE) RET_MINUS1_FAIL("size, pxfromp1")
      if (bufOut) GLUE_SET_PX_FROM_P1(bufOut + parm_size,n_params - 1 - pn);
      parm_size += GLUE_SET_PX_FROM_P1_SIZE;
    }
  }

  // pop any pushed parameters
  while (--pn >= 0)
  { 
    if (!OPCODE_IS_TRIVIAL(op->parms.parms[pn]))
    {
      if ((cfunc_abiinfo&BIF_SECONDLASTPARMST) && pn == n_params-2)
      {
        if (!local_fpstack_use)
        {
          if (bufOut_len < parm_size + sizeof(GLUE_POP_STACK_TO_FPSTACK)) RET_MINUS1_FAIL("size, popstacktofpstack 2")
          if (bufOut) memcpy(bufOut+parm_size,GLUE_POP_STACK_TO_FPSTACK,sizeof(GLUE_POP_STACK_TO_FPSTACK));
          parm_size += sizeof(GLUE_POP_STACK_TO_FPSTACK);
          need_fxch = 1;
        }
        else
        {
          local_fpstack_use--;
        }
      }
      else
      {
        if (bufOut_len < parm_size + GLUE_POP_PX_SIZE) RET_MINUS1_FAIL("size, poppx")
        if (bufOut) GLUE_POP_PX(bufOut + parm_size,n_params - 1 - pn);
        parm_size += GLUE_POP_PX_SIZE;
      }
    }
  }

  // finally, set trivial pointers
  for (pn=0; pn < n_params; pn++)
  { 
    if (OPCODE_IS_TRIVIAL(op->parms.parms[pn]))
    {
      if (pn == n_params-2 && (cfunc_abiinfo&(BIF_SECONDLASTPARMST)))  // second to last parameter
      {
        int a = compileOpcodes(ctx,op->parms.parms[pn],bufOut ? bufOut+parm_size : NULL,bufOut_len - parm_size,computTableSize,namespacePathToThis,
                                RETURNVALUE_FPSTACK,NULL,NULL,canHaveDenormalOutput);
        if (a<0) RET_MINUS1_FAIL("coc call here 2")
        parm_size+=a;
        need_fxch = 1;
      }
      else if (pn == n_params-1)  // last parameter, but we should call compileOpcodes to get it in the right format (compileOpcodes can optimize that process if it needs to)
      {
        int rvt=0, a;
        int wantFpStack = func == nseel_asm_assign;
#ifdef GLUE_PREFER_NONFP_DV_ASSIGNS // x86-64, and maybe others, prefer to avoid the fp stack for a simple copy
        if (wantFpStack &&
            (op->parms.parms[pn]->opcodeType != OPCODETYPE_DIRECTVALUE ||
            (op->parms.parms[pn]->parms.dv.directValue != 1.0 && op->parms.parms[pn]->parms.dv.directValue != 0.0)))
        {
          wantFpStack=0;
        }
#endif

        a = compileOpcodes(ctx,op->parms.parms[pn],bufOut ? bufOut+parm_size : NULL,bufOut_len - parm_size,computTableSize,namespacePathToThis,
          (cfunc_abiinfo & BIF_LASTPARMONSTACK) ? RETURNVALUE_FPSTACK : 
          (cfunc_abiinfo & BIF_LASTPARM_ASBOOL) ? RETURNVALUE_BOOL : 
          wantFpStack ? (RETURNVALUE_FPSTACK|RETURNVALUE_NORMAL) : 
          RETURNVALUE_NORMAL,&rvt, NULL,canHaveDenormalOutput);
        
        if (a<0) RET_MINUS1_FAIL("coc call here 3")
        parm_size+=a;
        need_fxch = 0;

        if (func == nseel_asm_assign)
        {
          if (rvt == RETURNVALUE_FPSTACK)
          {           
            if (!(ctx->optimizeDisableFlags & OPTFLAG_FULL_DENORMAL_CHECKS))
            {
              func = nseel_asm_assign_fast_fromfp;
              func_e = nseel_asm_assign_fast_fromfp_end;
            }
            else
            {
              func = nseel_asm_assign_fromfp;
              func_e = nseel_asm_assign_fromfp_end;
            }
          }
          else if (!(ctx->optimizeDisableFlags & OPTFLAG_FULL_DENORMAL_CHECKS))
          {
             // assigning a value (from a variable or other non-computer), can use a fast assign (no denormal/result checking)
            func = nseel_asm_assign_fast;
            func_e = nseel_asm_assign_fast_end;
          }
        }
      }
      else
      {
        if (bufOut_len < parm_size + GLUE_MOV_PX_DIRECTVALUE_SIZE) RET_MINUS1_FAIL("size, pxdvsz")
        if (bufOut) 
        {
          if (generateValueToReg(ctx,op->parms.parms[pn],bufOut + parm_size,n_params - 1 - pn,namespacePathToThis, 0/*nocaching, function gets pointer*/)<0) RET_MINUS1_FAIL("gvtr")
        }
        parm_size += GLUE_MOV_PX_DIRECTVALUE_SIZE;
      }
    }
  }

#ifdef GLUE_HAS_FXCH
  if ((cfunc_abiinfo&(BIF_SECONDLASTPARMST)) && !(cfunc_abiinfo&(BIF_LAZYPARMORDERING))&&
      ((!!need_fxch)^!!(cfunc_abiinfo&BIF_REVERSEFPORDER)) 
      )
  {
    // emit fxch
    if (bufOut_len < sizeof(GLUE_FXCH)) RET_MINUS1_FAIL("len,fxch")
    if (bufOut) 
    { 
      memcpy(bufOut+parm_size,GLUE_FXCH,sizeof(GLUE_FXCH));
    }
    parm_size+=sizeof(GLUE_FXCH);
  }
#endif
  
  if (!*canHaveDenormalOutput)
  {
    // if add_op or sub_op, and non-denormal input, safe to omit denormal checks
    if (func == (void*)nseel_asm_add_op)
    {
      func = nseel_asm_add_op_fast;
      func_e = nseel_asm_add_op_fast_end;
    }
    else if (func == (void*)nseel_asm_sub_op)
    {
      func = nseel_asm_sub_op_fast;
      func_e = nseel_asm_sub_op_fast_end;
    }
  }


  if (cfunc_abiinfo & (BIF_CLEARDENORMAL | BIF_RETURNSBOOL) ) *canHaveDenormalOutput=0;
  else if (!(cfunc_abiinfo & BIF_WONTMAKEDENORMAL)) *canHaveDenormalOutput=1;

  {
    int func_size=0;
    func = GLUE_realAddress(func,func_e,&func_size);
    if (!func) RET_MINUS1_FAIL("failrealladdrfunc")
                     
    if (bufOut_len < parm_size + func_size) RET_MINUS1_FAIL("funcsz")
  
    if (bufOut)
    {
      unsigned char *p=bufOut + parm_size;
      memcpy(p, func, func_size);
      if (preProc) p=preProc(p,func_size,ctx);
      if (repl)
      {
        if (repl[0]) p=EEL_GLUE_set_immediate(p,(INT_PTR)repl[0]);
        if (repl[1]) p=EEL_GLUE_set_immediate(p,(INT_PTR)repl[1]);
        if (repl[2]) p=EEL_GLUE_set_immediate(p,(INT_PTR)repl[2]);
        if (repl[3]) p=EEL_GLUE_set_immediate(p,(INT_PTR)repl[3]);
      }
    }

    if (cfunc_abiinfo&BIF_RETURNSONSTACK) *rvMode = RETURNVALUE_FPSTACK;
    else if (cfunc_abiinfo&BIF_RETURNSBOOL) *rvMode=RETURNVALUE_BOOL;

    return parm_size + func_size;
  }
  // end of builtin function generation
}

static int compileEelFunctionCall(compileContext *ctx, opcodeRec *op, unsigned char *bufOut, int bufOut_len, int *computTableSize, const char *namespacePathToThis, 
                                  int *rvMode, int *fpStackUse, int *canHaveDenormalOutput)
{
  int func_size=0, parm_size=0;
  int pn;
  int last_nt_parm=-1,last_nt_parm_mode=0;
  void *func_e=NULL;
  int n_params;
  opcodeRec *parmptrs[NSEEL_MAX_EELFUNC_PARAMETERS];
  int cfp_numparams=-1;
  int cfp_statesize=0;
  EEL_F **cfp_ptrs=NULL;
  int func_raw=0;
  int do_parms;
  int x;

  void *func;

  for (x=0; x < 3; x ++) parmptrs[x] = op->parms.parms[x];

  if (op->opcodeType == OPCODETYPE_FUNCX)
  {
    n_params=0;
    for (x=0;x<3;x++)
    {
      opcodeRec *prni=op->parms.parms[x];
      while (prni && n_params < NSEEL_MAX_EELFUNC_PARAMETERS)
      {
        const int isMP = prni->opcodeType == OPCODETYPE_MOREPARAMS;
        parmptrs[n_params++] = isMP ? prni->parms.parms[0] : prni;
        if (!isMP) break;
        prni = prni->parms.parms[1];
      }
    }
  }
  else 
  {
    n_params = 1 + op->opcodeType - OPCODETYPE_FUNC1;
  }
          
  *fpStackUse = 0;
  func = nseel_getEELFunctionAddress(ctx, op,
                                      &cfp_numparams,&cfp_statesize,&cfp_ptrs, 
                                      computTableSize, 
                                      &func_e, &func_raw,                                              
                                      !!bufOut,namespacePathToThis,rvMode,fpStackUse,canHaveDenormalOutput);

  if (func_raw) func_size = (char*)func_e  - (char*)func;
  else if (func) func = GLUE_realAddress(func,func_e,&func_size);
  
  if (!func) RET_MINUS1_FAIL("eelfuncaddr")

  *fpStackUse += 1;

  if (cfp_numparams>0 && n_params != cfp_numparams)
  {
    _codeHandleFunctionRec *fn = (_codeHandleFunctionRec*)op->fn;
    snprintf(ctx->last_error_string,sizeof(ctx->last_error_string),"Function '%s' takes %d parameters, passed %d\n",fn->fname,cfp_numparams,n_params);
    RET_MINUS1_FAIL("eelfuncnp")
  }

  // user defined function
  do_parms = cfp_numparams>0 && cfp_ptrs && cfp_statesize>0;

  // if function local/parameter state is zero, we need to allocate storage for it
  if (cfp_statesize>0 && cfp_ptrs && !cfp_ptrs[0])
  {
    EEL_F *pstate = newDataBlock(sizeof(EEL_F)*cfp_statesize,8);
    if (!pstate) RET_MINUS1_FAIL("eelfuncdb")

    for (pn=0;pn<cfp_statesize;pn++)
    {
      pstate[pn]=0;
      cfp_ptrs[pn] = pstate + pn;
    }
  }


  // first process parameters that are non-trivial
  for (pn=0; pn < n_params; pn++)
  { 
    int needDenorm=0;
    int lsz,sUse=0;                      
    
    if (OPCODE_IS_TRIVIAL(parmptrs[pn])) continue; // skip and process after

    if (last_nt_parm >= 0 && do_parms)
    {
      if (last_nt_parm_mode == RETURNVALUE_FPSTACK)
      {
        if (bufOut_len < parm_size + (int)sizeof(GLUE_POP_FPSTACK_TOSTACK)) RET_MINUS1_FAIL("eelfunc_size popfpstacktostack")
        if (bufOut) memcpy(bufOut + parm_size,GLUE_POP_FPSTACK_TOSTACK,sizeof(GLUE_POP_FPSTACK_TOSTACK));
        parm_size+=sizeof(GLUE_POP_FPSTACK_TOSTACK);
      }
      else
      {
        if (bufOut_len < parm_size + (int)sizeof(GLUE_PUSH_P1PTR_AS_VALUE)) RET_MINUS1_FAIL("eelfunc_size pushp1ptrasval")
    
        // push
        if (bufOut) memcpy(bufOut + parm_size,&GLUE_PUSH_P1PTR_AS_VALUE,sizeof(GLUE_PUSH_P1PTR_AS_VALUE));
        parm_size+=sizeof(GLUE_PUSH_P1PTR_AS_VALUE);
      }
    }

    last_nt_parm_mode=0;
    lsz = compileOpcodes(ctx,parmptrs[pn],bufOut ? bufOut + parm_size : NULL,bufOut_len - parm_size, computTableSize, namespacePathToThis,
      do_parms ? (RETURNVALUE_FPSTACK|RETURNVALUE_NORMAL) : RETURNVALUE_IGNORE,&last_nt_parm_mode,&sUse, &needDenorm);

    // todo: if needDenorm, denorm convert when copying parameter

    if (lsz<0) RET_MINUS1_FAIL("eelfunc, coc fail")

    if (last_nt_parm_mode == RETURNVALUE_FPSTACK) sUse++;
    if (sUse > *fpStackUse) *fpStackUse=sUse;
    parm_size += lsz;

    last_nt_parm = pn;
  }
  // pop non-trivial results into place
  if (last_nt_parm >=0 && do_parms)
  {
    while (--pn >= 0)
    { 
      if (OPCODE_IS_TRIVIAL(parmptrs[pn])) continue; // skip and process after
      if (pn == last_nt_parm)
      {
        if (last_nt_parm_mode == RETURNVALUE_FPSTACK)
        {
          // pop to memory directly
          const int cpsize = GLUE_POP_FPSTACK_TO_PTR(NULL,NULL);
          if (bufOut_len < parm_size + cpsize) RET_MINUS1_FAIL("eelfunc size popfpstacktoptr")

          if (bufOut) GLUE_POP_FPSTACK_TO_PTR((unsigned char *)bufOut + parm_size,cfp_ptrs[pn]);
          parm_size += cpsize;
        }
        else
        {
          // copy direct p1ptr to mem
          const int cpsize = GLUE_COPY_VALUE_AT_P1_TO_PTR(NULL,NULL);
          if (bufOut_len < parm_size + cpsize) RET_MINUS1_FAIL("eelfunc size copyvalueatp1toptr")

          if (bufOut) GLUE_COPY_VALUE_AT_P1_TO_PTR((unsigned char *)bufOut + parm_size,cfp_ptrs[pn]);
          parm_size += cpsize;
        }
      }
      else
      {
        const int popsize =  GLUE_POP_VALUE_TO_ADDR(NULL,NULL);
        if (bufOut_len < parm_size + popsize) RET_MINUS1_FAIL("eelfunc size pop value to addr")

        if (bufOut) GLUE_POP_VALUE_TO_ADDR((unsigned char *)bufOut + parm_size,cfp_ptrs[pn]);
        parm_size+=popsize;

      }
    }
  }

  // finally, set any trivial parameters
  if (do_parms)
  {
    const int cpsize = GLUE_MOV_PX_DIRECTVALUE_SIZE + GLUE_COPY_VALUE_AT_P1_TO_PTR(NULL,NULL);
    for (pn=0; pn < n_params; pn++)
    { 
      if (!OPCODE_IS_TRIVIAL(parmptrs[pn])) continue; // set trivial values, we already set nontrivials

      if (bufOut_len < parm_size + cpsize) RET_MINUS1_FAIL("eelfunc size trivial set")

      if (bufOut) 
      {
        if (generateValueToReg(ctx,parmptrs[pn],bufOut + parm_size,0,namespacePathToThis, 1)<0) RET_MINUS1_FAIL("eelfunc gvr fail")
        GLUE_COPY_VALUE_AT_P1_TO_PTR(bufOut + parm_size + GLUE_MOV_PX_DIRECTVALUE_SIZE,cfp_ptrs[pn]);
      }
      parm_size += cpsize;

    }
  }

  if (bufOut_len < parm_size + func_size) RET_MINUS1_FAIL("eelfunc size combined")
  
  if (bufOut) memcpy(bufOut + parm_size, func, func_size);

  return parm_size + func_size;
  // end of EEL function generation
}

#ifdef DUMP_OPS_DURING_COMPILE
void dumpOp(compileContext *ctx, opcodeRec *op, int start);
#endif

#ifdef GLUE_MAX_JMPSIZE
#define CHECK_SIZE_FORJMP(x,y) if ((x)<0 || (x)>=GLUE_MAX_JMPSIZE) goto y;
#define RET_MINUS1_FAIL_FALLBACK(err,j) goto j;
#else
#define CHECK_SIZE_FORJMP(x,y)
#define RET_MINUS1_FAIL_FALLBACK(err,j) RET_MINUS1_FAIL(err)
#endif
static int compileOpcodesInternal(compileContext *ctx, opcodeRec *op, unsigned char *bufOut, int bufOut_len, int *computTableSize, const char *namespacePathToThis, int *calledRvType, int preferredReturnValues, int *fpStackUse, int *canHaveDenormalOutput)
{
  int rv_offset=0;
  if (!op) RET_MINUS1_FAIL("coi !op")

  *fpStackUse=0;
  // special case: statement delimiting means we can process the left side into place, and iteratively do the second parameter without recursing
  // also we don't need to save/restore anything to the stack (which the normal 2 parameter function processing does)
  while (op->opcodeType == OPCODETYPE_FUNC2 && op->fntype == FN_JOIN_STATEMENTS)
  {
    int fUse;
    int parm_size = compileOpcodes(ctx,op->parms.parms[0],bufOut,bufOut_len, computTableSize, namespacePathToThis, RETURNVALUE_IGNORE, NULL,&fUse,NULL);
    if (parm_size < 0) RET_MINUS1_FAIL("coc join fail")
    op = op->parms.parms[1];
    if (!op) RET_MINUS1_FAIL("join got to null")

    if (fUse>*fpStackUse) *fpStackUse=fUse;
    if (bufOut) bufOut += parm_size;
    bufOut_len -= parm_size;
    rv_offset += parm_size;
#ifdef DUMP_OPS_DURING_COMPILE
    if (op->opcodeType != OPCODETYPE_FUNC2 || op->fntype != FN_JOIN_STATEMENTS) dumpOp(ctx,op,0);
#endif
  }

  if (op->fntype == FUNCTYPE_FUNCTIONTYPEREC)
  {
    // special case: while
    functionType *fn_ptr = (functionType *)op->fn;
    if (op->opcodeType == OPCODETYPE_FUNC1 && fn_ptr == fnTable1 + 4)
    {
      *calledRvType = RETURNVALUE_BOOL;

#ifndef GLUE_INLINE_LOOPS
      // todo: PPC looping support when loop length is small enough
      {
        unsigned char *pwr=bufOut;
        unsigned char *newblock2;
        int stubsz;
        void *stubfunc = GLUE_realAddress(nseel_asm_repeatwhile,nseel_asm_repeatwhile_end,&stubsz);
        if (!stubfunc || bufOut_len < stubsz) RET_MINUS1_FAIL(stubfunc ? "repeatwhile size fail" :"repeatwhile addr fail")

        if (bufOut)
        {
          newblock2=compileCodeBlockWithRet(ctx,op->parms.parms[0],computTableSize,namespacePathToThis, RETURNVALUE_BOOL, NULL, fpStackUse, NULL);
          if (!newblock2) RET_MINUS1_FAIL("repeatwhile ccbwr fail")
      
          memcpy(pwr,stubfunc,stubsz);
          pwr=EEL_GLUE_set_immediate(pwr,(INT_PTR)newblock2); 
        }
      
        return rv_offset+stubsz;
      }
#else
      {
        unsigned char *looppt, *jzoutpt;
        int parm_size=0,subsz;
        if (bufOut_len < parm_size + sizeof(GLUE_WHILE_SETUP) + sizeof(GLUE_WHILE_BEGIN)) RET_MINUS1_FAIL("while size fail 1")
        if (bufOut) memcpy(bufOut + parm_size,GLUE_WHILE_SETUP,sizeof(GLUE_WHILE_SETUP));
        parm_size+=sizeof(GLUE_WHILE_SETUP);
        looppt = bufOut + parm_size;
        if (bufOut) memcpy(bufOut + parm_size,GLUE_WHILE_BEGIN,sizeof(GLUE_WHILE_BEGIN));
        parm_size+=sizeof(GLUE_WHILE_BEGIN);

        subsz = compileOpcodes(ctx,op->parms.parms[0],bufOut ? (bufOut + parm_size) : NULL,bufOut_len - parm_size, computTableSize, namespacePathToThis, RETURNVALUE_BOOL, NULL,fpStackUse, NULL);
        if (subsz<0) RET_MINUS1_FAIL("while coc fail")

        if (bufOut_len < parm_size + sizeof(GLUE_WHILE_END) + sizeof(GLUE_WHILE_CHECK_RV)) RET_MINUS1_FAIL("which size fial 2")

        parm_size+=subsz;
        if (bufOut) memcpy(bufOut + parm_size, GLUE_WHILE_END, sizeof(GLUE_WHILE_END));
        parm_size+=sizeof(GLUE_WHILE_END);
        jzoutpt = bufOut + parm_size;

        if (bufOut) memcpy(bufOut + parm_size, GLUE_WHILE_CHECK_RV, sizeof(GLUE_WHILE_CHECK_RV));
        parm_size+=sizeof(GLUE_WHILE_CHECK_RV);
        if (bufOut) 
        {
          GLUE_JMP_SET_OFFSET(bufOut + parm_size,(looppt - (bufOut+parm_size)) );
          GLUE_JMP_SET_OFFSET(jzoutpt, (bufOut + parm_size) - jzoutpt);
        }
        return rv_offset+parm_size;
      }

#endif
    }

    // special case: loop
    if (op->opcodeType == OPCODETYPE_FUNC2 && fn_ptr == fnTable1+3)
    {    
      int fUse;
      int parm_size = compileOpcodes(ctx,op->parms.parms[0],bufOut,bufOut_len, computTableSize, namespacePathToThis, RETURNVALUE_FPSTACK, NULL,&fUse, NULL);
      if (parm_size < 0) RET_MINUS1_FAIL("loop coc fail")
      
      *calledRvType = RETURNVALUE_BOOL;
      if (fUse > *fpStackUse) *fpStackUse=fUse;
           
#ifndef GLUE_INLINE_LOOPS
      // todo: PPC looping support when loop length is small enough
      {
        void *stub;
        int stubsize;        
        unsigned char *newblock2, *p;
        stub = GLUE_realAddress(nseel_asm_repeat,nseel_asm_repeat_end,&stubsize);
        if (bufOut_len < parm_size + stubsize) RET_MINUS1_FAIL("loop size fail")
        if (bufOut)
        {
          newblock2 = compileCodeBlockWithRet(ctx,op->parms.parms[1],computTableSize,namespacePathToThis, RETURNVALUE_IGNORE, NULL,fpStackUse, NULL);
      
          p = bufOut + parm_size;
          memcpy(p, stub, stubsize);
      
          p=EEL_GLUE_set_immediate(p,(INT_PTR)newblock2);
        }
        return rv_offset + parm_size + stubsize;
      }
#else
      {
        int subsz;
        int fUse=0;
        unsigned char *skipptr1,*loopdest;
        if (bufOut_len < parm_size + sizeof(GLUE_LOOP_LOADCNT) + sizeof(GLUE_LOOP_CLAMPCNT) + sizeof(GLUE_LOOP_BEGIN)) RET_MINUS1_FAIL("loop size fail")

        // store, convert to int, compare against 1, if less than, skip to end
        if (bufOut) memcpy(bufOut+parm_size,GLUE_LOOP_LOADCNT,sizeof(GLUE_LOOP_LOADCNT));
        parm_size += sizeof(GLUE_LOOP_LOADCNT);
        skipptr1 = bufOut+parm_size; 

        // compare aginst max loop length, jump to loop start if not above it
        if (bufOut) memcpy(bufOut+parm_size,GLUE_LOOP_CLAMPCNT,sizeof(GLUE_LOOP_CLAMPCNT));
        parm_size += sizeof(GLUE_LOOP_CLAMPCNT);

        // loop code:
        loopdest = bufOut + parm_size;
        if (bufOut) memcpy(bufOut+parm_size,GLUE_LOOP_BEGIN,sizeof(GLUE_LOOP_BEGIN));
        parm_size += sizeof(GLUE_LOOP_BEGIN);

        subsz = compileOpcodes(ctx,op->parms.parms[1],bufOut ? (bufOut + parm_size) : NULL,bufOut_len - parm_size, computTableSize, namespacePathToThis, RETURNVALUE_IGNORE, NULL, &fUse, NULL);
        if (subsz<0) RET_MINUS1_FAIL("loop coc fail")
        if (fUse > *fpStackUse) *fpStackUse=fUse;

        parm_size += subsz;

        if (bufOut_len < parm_size + sizeof(GLUE_LOOP_END)) RET_MINUS1_FAIL("loop size fail 2")

        if (bufOut) memcpy(bufOut+parm_size,GLUE_LOOP_END,sizeof(GLUE_LOOP_END));
        parm_size += sizeof(GLUE_LOOP_END);
        
        if (bufOut) 
        {
          GLUE_JMP_SET_OFFSET(bufOut + parm_size,loopdest - (bufOut+parm_size));
          GLUE_JMP_SET_OFFSET(skipptr1, (bufOut+parm_size) - skipptr1);
        }

        return rv_offset + parm_size;

      }
#endif
    }
    
    // special case: BAND/BOR
    if (op->opcodeType == OPCODETYPE_FUNC2 && (fn_ptr == fnTable1+1 || fn_ptr == fnTable1+2))
    {
      int fUse=0;
      int parm_size,parm_size_pre;
      int retType=RETURNVALUE_IGNORE;
      if (preferredReturnValues != RETURNVALUE_IGNORE) retType = RETURNVALUE_BOOL;

      *calledRvType = retType;
      
      parm_size = compileOpcodes(ctx,op->parms.parms[0],bufOut,bufOut_len, computTableSize, namespacePathToThis, RETURNVALUE_BOOL, NULL, &fUse, NULL);
      if (parm_size < 0) RET_MINUS1_FAIL("loop band/bor coc fail")
      
      if (fUse > *fpStackUse) *fpStackUse=fUse;


      parm_size_pre=parm_size;

      {
        int sz2, fUse=0;
        unsigned char *destbuf;
        const int testsz=(fn_ptr == fnTable1+2) ? sizeof(GLUE_JMP_IF_P1_NZ) : sizeof(GLUE_JMP_IF_P1_Z);
        if (bufOut_len < parm_size+testsz) RET_MINUS1_FAIL_FALLBACK("band/bor size fail",doNonInlinedAndOr_)

        if (bufOut)  memcpy(bufOut+parm_size,(fn_ptr == fnTable1+2) ? GLUE_JMP_IF_P1_NZ : GLUE_JMP_IF_P1_Z,testsz); 
        parm_size += testsz;
        destbuf = bufOut + parm_size;

        sz2= compileOpcodes(ctx,op->parms.parms[1],bufOut?bufOut+parm_size:NULL,bufOut_len-parm_size, computTableSize, namespacePathToThis, retType, NULL,&fUse, NULL);

        CHECK_SIZE_FORJMP(sz2,doNonInlinedAndOr_)
        if (sz2<0) RET_MINUS1_FAIL("band/bor coc fail")

        parm_size+=sz2;
        if (bufOut) GLUE_JMP_SET_OFFSET(destbuf, (bufOut + parm_size) - destbuf);

        if (fUse > *fpStackUse) *fpStackUse=fUse;
        return rv_offset + parm_size;
      }
#ifdef GLUE_MAX_JMPSIZE
      if (0) 
      {
        void *stub;
        int stubsize;        
        unsigned char *newblock2, *p;
      
        // encode as function call
doNonInlinedAndOr_:
        parm_size = parm_size_pre;

        if (fn_ptr == fnTable1+1) 
        {
          stub = GLUE_realAddress(nseel_asm_band,nseel_asm_band_end,&stubsize);
        }
        else 
        {
          stub = GLUE_realAddress(nseel_asm_bor,nseel_asm_bor_end,&stubsize);
        }
      
        if (bufOut_len < parm_size + stubsize) RET_MINUS1_FAIL("band/bor len fail")
      
        if (bufOut)
        {
          fUse=0;
          newblock2 = compileCodeBlockWithRet(ctx,op->parms.parms[1],computTableSize,namespacePathToThis, retType, NULL, &fUse, NULL);
          if (!newblock2) RET_MINUS1_FAIL("band/bor ccbwr fail")

          if (fUse > *fpStackUse) *fpStackUse=fUse;
      
          p = bufOut + parm_size;
          memcpy(p, stub, stubsize);
      
          p=EEL_GLUE_set_immediate(p,(INT_PTR)newblock2);
        }
        return rv_offset + parm_size + stubsize;
      }
#endif
    }  
    
    if (op->opcodeType == OPCODETYPE_FUNC3 && fn_ptr == fnTable1 + 0) // special case: IF
    {
      int fUse=0;
      int parm_size_pre;
      int use_rv = RETURNVALUE_IGNORE;
      int parm_size = compileOpcodes(ctx,op->parms.parms[0],bufOut,bufOut_len, computTableSize, namespacePathToThis, RETURNVALUE_BOOL, NULL,&fUse, NULL);
      if (parm_size < 0) RET_MINUS1_FAIL("if coc fail")
      if (fUse > *fpStackUse) *fpStackUse=fUse;

      if (preferredReturnValues & RETURNVALUE_NORMAL) use_rv=RETURNVALUE_NORMAL;
      else if (preferredReturnValues & RETURNVALUE_FPSTACK) use_rv=RETURNVALUE_FPSTACK;
      else if (preferredReturnValues & RETURNVALUE_BOOL) use_rv=RETURNVALUE_BOOL;
      
      *calledRvType = use_rv;
      parm_size_pre = parm_size;

      {
        int csz,hasSecondHalf;
        if (bufOut_len < parm_size + sizeof(GLUE_JMP_IF_P1_Z)) RET_MINUS1_FAIL_FALLBACK("if size fail",doNonInlineIf_)
        if (bufOut) memcpy(bufOut+parm_size,GLUE_JMP_IF_P1_Z,sizeof(GLUE_JMP_IF_P1_Z));
        parm_size += sizeof(GLUE_JMP_IF_P1_Z);
        csz=compileOpcodes(ctx,op->parms.parms[1],bufOut ? bufOut+parm_size : NULL,bufOut_len - parm_size, computTableSize, namespacePathToThis, use_rv, NULL,&fUse, canHaveDenormalOutput);
        if (fUse > *fpStackUse) *fpStackUse=fUse;
        hasSecondHalf = preferredReturnValues || !OPCODE_IS_TRIVIAL(op->parms.parms[2]);

        CHECK_SIZE_FORJMP(csz,doNonInlineIf_)
        if (csz<0) RET_MINUS1_FAIL("if coc fial")

        if (bufOut) GLUE_JMP_SET_OFFSET(bufOut + parm_size, csz + (hasSecondHalf?sizeof(GLUE_JMP_NC):0));
        parm_size+=csz;

        if (hasSecondHalf)
        {
          if (bufOut_len < parm_size + sizeof(GLUE_JMP_NC)) RET_MINUS1_FAIL_FALLBACK("if len fail",doNonInlineIf_)
          if (bufOut) memcpy(bufOut+parm_size,GLUE_JMP_NC,sizeof(GLUE_JMP_NC));
          parm_size+=sizeof(GLUE_JMP_NC);

          csz=compileOpcodes(ctx,op->parms.parms[2],bufOut ? bufOut+parm_size : NULL,bufOut_len - parm_size, computTableSize, namespacePathToThis, use_rv, NULL, &fUse, canHaveDenormalOutput);

          CHECK_SIZE_FORJMP(csz,doNonInlineIf_)
          if (csz<0) RET_MINUS1_FAIL("if coc 2 fail")

          // update jump address
          if (bufOut) GLUE_JMP_SET_OFFSET(bufOut + parm_size,csz); 
          parm_size+=csz;       
          if (fUse > *fpStackUse) *fpStackUse=fUse;
        }
        return rv_offset + parm_size;
      }
#ifdef GLUE_MAX_JMPSIZE
      if (0)
      {
        unsigned char *newblock2,*newblock3,*ptr;
        void *stub;
        int stubsize;
doNonInlineIf_:
        parm_size = parm_size_pre;
        stub = GLUE_realAddress(nseel_asm_if,nseel_asm_if_end,&stubsize);
      
        if (!stub || bufOut_len < parm_size + stubsize) RET_MINUS1_FAIL(stub ? "if sz fail" : "if addr fail")
      
        if (bufOut)
        {
          fUse=0;
          newblock2 = compileCodeBlockWithRet(ctx,op->parms.parms[1],computTableSize,namespacePathToThis, use_rv, NULL,&fUse, canHaveDenormalOutput); 
          if (fUse > *fpStackUse) *fpStackUse=fUse;
          newblock3 = compileCodeBlockWithRet(ctx,op->parms.parms[2],computTableSize,namespacePathToThis, use_rv, NULL,&fUse, canHaveDenormalOutput);
          if (fUse > *fpStackUse) *fpStackUse=fUse;
          if (!newblock2 || !newblock3) RET_MINUS1_FAIL("if subblock gen fail")
      
          ptr = bufOut + parm_size;
          memcpy(ptr, stub, stubsize);
           
          ptr=EEL_GLUE_set_immediate(ptr,(INT_PTR)newblock2);
          EEL_GLUE_set_immediate(ptr,(INT_PTR)newblock3);
        }
        return rv_offset + parm_size + stubsize;
      }
#endif
    }    
  }   
 
  switch (op->opcodeType)
  {
    case OPCODETYPE_DIRECTVALUE:
        if (preferredReturnValues == RETURNVALUE_BOOL)
        {
          int w = fabs(op->parms.dv.directValue) >= NSEEL_CLOSEFACTOR;
          int wsz=(w?sizeof(GLUE_SET_P1_NZ):sizeof(GLUE_SET_P1_Z));

          *calledRvType = RETURNVALUE_BOOL;
          if (bufOut_len < wsz) RET_MINUS1_FAIL("direct bool size fail3")
          if (bufOut) memcpy(bufOut,w?GLUE_SET_P1_NZ:GLUE_SET_P1_Z,wsz);
          return rv_offset+wsz;
        }
        else if (preferredReturnValues & RETURNVALUE_FPSTACK)
        {
#ifdef GLUE_HAS_FLDZ
          if (op->parms.dv.directValue == 0.0)
          {
            *fpStackUse = 1;
            *calledRvType = RETURNVALUE_FPSTACK;
            if (bufOut_len < sizeof(GLUE_FLDZ)) RET_MINUS1_FAIL("direct fp fail 1")
            if (bufOut) memcpy(bufOut,GLUE_FLDZ,sizeof(GLUE_FLDZ));
            return rv_offset+sizeof(GLUE_FLDZ);
          }
#endif
#ifdef GLUE_HAS_FLD1
          if (op->parms.dv.directValue == 1.0)
          {
            *fpStackUse = 1;
            *calledRvType = RETURNVALUE_FPSTACK;
            if (bufOut_len < sizeof(GLUE_FLD1)) RET_MINUS1_FAIL("direct fp fail 1")
            if (bufOut) memcpy(bufOut,GLUE_FLD1,sizeof(GLUE_FLD1));
            return rv_offset+sizeof(GLUE_FLD1);
          }
#endif
        }
        // fall through

    case OPCODETYPE_VALUE_FROM_NAMESPACENAME:
    case OPCODETYPE_VARPTR:
    case OPCODETYPE_VARPTRPTR:


      #ifdef GLUE_MOV_PX_DIRECTVALUE_TOSTACK_SIZE
        if (OPCODE_IS_TRIVIAL(op))
        {
          if (preferredReturnValues & RETURNVALUE_FPSTACK)
          {
            *fpStackUse = 1;
            if (bufOut_len < GLUE_MOV_PX_DIRECTVALUE_TOSTACK_SIZE) RET_MINUS1_FAIL("direct fp fail 2")
            if (bufOut)
            {
              if (generateValueToReg(ctx,op,bufOut,-1,namespacePathToThis, 1 /*allow caching*/)<0) RET_MINUS1_FAIL("direct fp fail gvr")
            }
            *calledRvType = RETURNVALUE_FPSTACK;
            return rv_offset+GLUE_MOV_PX_DIRECTVALUE_TOSTACK_SIZE;
          }
        }
      #endif

      if (bufOut_len < GLUE_MOV_PX_DIRECTVALUE_SIZE) 
      {
        RET_MINUS1_FAIL("direct value fail 1")
      }
      if (bufOut) 
      {
        if (generateValueToReg(ctx,op,bufOut,0,namespacePathToThis, !!(preferredReturnValues&RETURNVALUE_FPSTACK)/*cache if going to the fp stack*/)<0) RET_MINUS1_FAIL("direct value gvr fail3")
      }
    return rv_offset + GLUE_MOV_PX_DIRECTVALUE_SIZE;

    case OPCODETYPE_FUNCX:
    case OPCODETYPE_FUNC1:
    case OPCODETYPE_FUNC2:
    case OPCODETYPE_FUNC3:
      
      if (op->fntype == FUNCTYPE_EELFUNC_THIS || op->fntype == FUNCTYPE_EELFUNC)
      {
        int a;
        
        a = compileEelFunctionCall(ctx,op,bufOut,bufOut_len,computTableSize,namespacePathToThis, calledRvType,fpStackUse,canHaveDenormalOutput);
        if (a<0) return a;
        rv_offset += a;
      }
      else
      {
        int a;
        a = compileNativeFunctionCall(ctx,op,bufOut,bufOut_len,computTableSize,namespacePathToThis, calledRvType,fpStackUse,preferredReturnValues,canHaveDenormalOutput);
        if (a<0)return a;
        rv_offset += a;
      }        
    return rv_offset;
  }

  RET_MINUS1_FAIL("default opcode fail")
}

#ifdef DUMP_OPS_DURING_COMPILE
FILE *g_debugfp;
int g_debugfp_indent;
int g_debugfp_histsz=0;

void dumpOp(compileContext *ctx, opcodeRec *op, int start)
{
  if (start>=0)
  {
    if (g_debugfp)
    {
      static opcodeRec **hist;
      
      int x;
      int hit=0;
      if (!hist) hist = (opcodeRec**) calloc(1024,1024*sizeof(opcodeRec*));
      for(x=0;x<g_debugfp_histsz;x++)
      {
        if (hist[x] == op) { hit=1; break; }
      }
      if (x ==g_debugfp_histsz && g_debugfp_histsz<1024*1024) hist[g_debugfp_histsz++] = op;

      if (!start) 
      {
        g_debugfp_indent-=2;
        fprintf(g_debugfp,"%*s}(join)\n",g_debugfp_indent," ");
      }
      if (g_debugfp_indent>=100) *(char *)1=0;
      fprintf(g_debugfp,"%*s{ %p : %d%s: ",g_debugfp_indent," ",op,op->opcodeType, hit ? " -- DUPLICATE" : "");
      switch (op->opcodeType)
      {
        case OPCODETYPE_DIRECTVALUE:
          fprintf(g_debugfp,"dv %f",op->parms.dv.directValue);
        break;
        case OPCODETYPE_VARPTR:
          {
            int wb; 
            for (wb = 0; wb < ctx->varTable_numBlocks; wb ++)
            {
              char **plist=ctx->varTable_Names[wb];
              if (!plist) break;
    
              if (op->parms.dv.valuePtr >= ctx->varTable_Values[wb] && op->parms.dv.valuePtr < ctx->varTable_Values[wb] + NSEEL_VARS_PER_BLOCK)
              {
                fprintf(g_debugfp,"var %s",plist[op->parms.dv.valuePtr - ctx->varTable_Values[wb]]);
                break;
              }
            }        
          }
        break;
        case OPCODETYPE_FUNC1:
        case OPCODETYPE_FUNC2:
        case OPCODETYPE_FUNC3:
        case OPCODETYPE_FUNCX:
          if (op->fntype == FUNCTYPE_FUNCTIONTYPEREC)
          {
            functionType *p=(functionType*)op->fn;
            fprintf(g_debugfp,"func %d: %s",p->nParams&0xff,p->name);
          }
          else
            fprintf(g_debugfp,"sf %d",op->fntype);
        break;

      }
      fprintf(g_debugfp,"\n");
      g_debugfp_indent+=2;
    }
  }
  else
  {
    if (g_debugfp)
    {
      g_debugfp_indent-=2;
      fprintf(g_debugfp,"%*s}%p\n",g_debugfp_indent," ",op);
    }
  }
}
#endif

int compileOpcodes(compileContext *ctx, opcodeRec *op, unsigned char *bufOut, int bufOut_len, int *computTableSize, const char *namespacePathToThis, 
                   int supportedReturnValues, int *rvType, int *fpStackUse, int *canHaveDenormalOutput)
{
  int code_returns=RETURNVALUE_NORMAL;
  int fpsu=0;
  int codesz;
  int denorm=0;

#ifdef DUMP_OPS_DURING_COMPILE
  dumpOp(ctx,op,1);
#endif
  
  codesz = compileOpcodesInternal(ctx,op,bufOut,bufOut_len,computTableSize,namespacePathToThis,&code_returns, supportedReturnValues,&fpsu,&denorm);
  if (denorm && canHaveDenormalOutput) *canHaveDenormalOutput=1;

#ifdef DUMP_OPS_DURING_COMPILE
  dumpOp(ctx,op,-1);
#endif

  if (codesz < 0) return codesz;


  /*
  {
    char buf[512];
    sprintf(buf,"opcode %d %d (%s): fpu use: %d\n",op->opcodeType,op->fntype,
      op->opcodeType >= OPCODETYPE_FUNC1 && op->fntype == FUNCTYPE_FUNCTIONTYPEREC ? (
      ((functionType *)op->fn)->name
      ) : "",
      fpsu);
    OutputDebugString(buf);
  }
  */

  if (fpStackUse) *fpStackUse=fpsu;

  if (bufOut) bufOut += codesz;
  bufOut_len -= codesz;


  if (code_returns == RETURNVALUE_BOOL && !(supportedReturnValues & RETURNVALUE_BOOL) && supportedReturnValues)
  {
    int stubsize;
    void *stub = GLUE_realAddress(nseel_asm_booltofp,nseel_asm_booltofp_end,&stubsize);
    if (!stub || bufOut_len < stubsize) RET_MINUS1_FAIL(stub?"booltofp size":"booltfp addr")
    if (bufOut) 
    {
      unsigned char *p=bufOut;
      memcpy(bufOut,stub,stubsize);
      bufOut += stubsize;
    }
    codesz+=stubsize;
    bufOut_len -= stubsize;
    
    code_returns = RETURNVALUE_FPSTACK;
  }


  // default processing of code_returns to meet return value requirements
  if (supportedReturnValues & code_returns) 
  {
    if (rvType) *rvType = code_returns;
    return codesz;
  }


  if (rvType) *rvType = RETURNVALUE_IGNORE;


  if (code_returns == RETURNVALUE_NORMAL)
  {
    if (supportedReturnValues & (RETURNVALUE_FPSTACK|RETURNVALUE_BOOL))
    {
      if (bufOut_len < GLUE_PUSH_VAL_AT_PX_TO_FPSTACK_SIZE) RET_MINUS1_FAIL("pushvalatpxtofpstack,size")
      if (bufOut) 
      {
        GLUE_PUSH_VAL_AT_PX_TO_FPSTACK(bufOut,0); // always fld qword [eax] but we might change that later
        bufOut += GLUE_PUSH_VAL_AT_PX_TO_FPSTACK_SIZE;
      }
      codesz += GLUE_PUSH_VAL_AT_PX_TO_FPSTACK_SIZE;  
      bufOut_len -= GLUE_PUSH_VAL_AT_PX_TO_FPSTACK_SIZE;

      if (supportedReturnValues & RETURNVALUE_BOOL) 
      {
        code_returns = RETURNVALUE_FPSTACK;
      }
      else
      {
        if (rvType) *rvType = RETURNVALUE_FPSTACK;
      }
    }
  }

  if (code_returns == RETURNVALUE_FPSTACK)
  {
    if (supportedReturnValues & RETURNVALUE_BOOL)
    {
      int stubsize;
      void *stub = GLUE_realAddress(nseel_asm_fptobool,nseel_asm_fptobool_end,&stubsize);
      if (!stub || bufOut_len < stubsize) RET_MINUS1_FAIL(stub?"fptobool size":"fptobool addr")
      if (bufOut) 
      {
        memcpy(bufOut,stub,stubsize);
        bufOut += stubsize;
      }
      codesz+=stubsize;
      bufOut_len -= stubsize;

      if (rvType) *rvType = RETURNVALUE_BOOL;
    }
    else if (supportedReturnValues & RETURNVALUE_NORMAL)
    {
      if (computTableSize) (*computTableSize) ++;

      if (bufOut_len < GLUE_POP_FPSTACK_TO_WTP_TO_PX_SIZE) RET_MINUS1_FAIL("popfpstacktowtptopxsize")

      // generate fp-pop to temp space
      if (bufOut) GLUE_POP_FPSTACK_TO_WTP_TO_PX(bufOut,0);
      codesz+=GLUE_POP_FPSTACK_TO_WTP_TO_PX_SIZE;
      if (rvType) *rvType = RETURNVALUE_NORMAL;
    }
    else
    {
      // toss return value that will be ignored
      if (bufOut_len < GLUE_POP_FPSTACK_SIZE) RET_MINUS1_FAIL("popfpstack size")
      if (bufOut) memcpy(bufOut,GLUE_POP_FPSTACK,GLUE_POP_FPSTACK_SIZE);   
      codesz+=GLUE_POP_FPSTACK_SIZE;
    }
  }

  return codesz;
}




static char *preprocessCode(compileContext *ctx, char *expression, int src_offset_bytes, int dest_offset_bytes)
{
  char *expression_start=expression;
  int len=0;
  int alloc_len=strlen(expression)+1+64;
  char *buf=(char *)malloc(alloc_len);
  int semicnt=0;
  // we need to call onCompileNewLine for each new line we get
 
  //onCompileNewLine(ctx, 

  while (*expression)
  {
    if (len > alloc_len-64)
    {
      alloc_len = len+128;
      buf=(char*)realloc(buf,alloc_len);
    }

    if (expression[0] == '/')
    {
      if (expression[1] == '/')
      {
        expression+=2;
        if (!strncasecmp(expression,"#eel-no-optimize:",17))
        {
          ctx->optimizeDisableFlags = atoi(expression+17);
        }

        while (expression[0] && expression[0] != '\n') expression++;
	      continue;
      }
      else if (expression[1] == '*')
      {
        expression+=2;
        while (expression[0] && (expression[0] != '*' || expression[1] != '/')) 
	      {
		      if (expression[0] == '\n') onCompileNewLine(ctx,expression+1-expression_start + src_offset_bytes,dest_offset_bytes+len);
		      expression++;
	      }
        if (expression[0]) expression+=2; // at this point we KNOW expression[0]=* and expression[1]=/
	      continue;
      }
    }
    
    if (expression[0] == '(' && expression[1]==')')
    {
      expression+=2;
      memcpy(buf+len,"(0)",3);
      len+=3;
      ctx->l_stats[0]+=3;
      continue;
    }
    if (expression[0] == '$')
    {
      if (toupper(expression[1]) == 'X'||expression[1] == '~')
      {
        char isBits = expression[1] == '~';
        char *p=expression+2;
        unsigned int v=strtoul(expression+2,&p,isBits ? 10 : 16);
        char tmp[256];
        expression=p;

        if (isBits)
        {
          if (v>53) v=53;
          sprintf(tmp,"%.1f",(double) ((((WDL_INT64)1) << v) - 1));
        }
        else
        {
          sprintf(tmp,"%u",v);
        }
        memcpy(buf+len,tmp,strlen(tmp));
        len+=strlen(tmp);
        ctx->l_stats[0]+=strlen(tmp);
        continue;

      }
      if (expression[1]=='\'' && expression[2] && expression[3]=='\'')
      {
        char tmp[64];
        sprintf(tmp,"%u",((unsigned char *)expression)[2]);
        expression+=4;

        memcpy(buf+len,tmp,strlen(tmp));
        len+=strlen(tmp);
        ctx->l_stats[0]+=strlen(tmp);
        continue;
      }
      if (toupper(expression[1]) == 'P' && toupper(expression[2]) == 'I')
      {
        static char *str="3.141592653589793";
        expression+=3;
        memcpy(buf+len,str,17);
        len+=17; //strlen(str);
        ctx->l_stats[0]+=17;
	      continue;
      }
      if (toupper(expression[1]) == 'E')
      {
        static char *str="2.71828183";
        expression+=2;
        memcpy(buf+len,str,10);
        len+=10; //strlen(str);
        ctx->l_stats[0]+=10;
  	    continue;
      }
      if (toupper(expression[1]) == 'P' && toupper(expression[2]) == 'H' && toupper(expression[3]) == 'I')
      {
        static char *str="1.61803399";
        expression+=4;
        memcpy(buf+len,str,10);
        len+=10; //strlen(str);
        ctx->l_stats[0]+=10;
	      continue;
      }
      
    }

    {
      char c=*expression++;

      if (c == '\n') onCompileNewLine(ctx,expression-expression_start + src_offset_bytes,len + dest_offset_bytes);
      if (isspace(c)) c=' ';

      if (c == '(') semicnt++;
      else if (c == ')') { semicnt--; if (semicnt < 0) semicnt=0; }
      else if (c == ';' && semicnt > 0)
      {
        // convert ; to % if next nonwhitespace char is alnum, otherwise convert to space
        int p=0;
        int nc;
		int commentstate=0;
        	while ((nc=expression[p]))
		{
			if (!commentstate && nc == '/')
			{
				if (expression[p+1] == '/') commentstate=1;
				else if (expression[p+1] == '*') commentstate=2;
			}

			if (commentstate == 1 && nc == '\n') commentstate=0;
			else if (commentstate == 2 && nc == '*' && expression[p+1]=='/')
			{
				p++; // skip *
				commentstate=0;
			}
			else if (!commentstate && !isspace(nc)) break;

			p++;
		}
		// fucko, we should look for even more chars, me thinks
        if (nc && (isalnum(nc) 
#if 1
				|| nc == '(' || nc == '_' || nc == '!' || nc == '$' || nc == '-' || nc == '+' /* unary +, -, !, symbols, etc, mean new statement */
#endif
				)) c='%';
        else c = ' '; // stray ;
      }
#if 0
      else if (semicnt > 0 && c == ',')
      {
        int p=0;
        int nc;
        while ((nc=expression[p]) && isspace(nc)) p++;
		if (nc == ',' || nc == ')') 
		{
			expression += p+1;
			buf[len++]=',';
			buf[len++]='0';
			c=nc; // append this char
		}
      }
#endif
	  // list of operators

	  else if (!isspace(c) && !isalnum(c)) // check to see if this operator is ours
	  {

			static char *symbollists[]=
			{
				"", // stop at any control char that is not parenthed
				":(,;?%", 
				",):?;", // or || or &&
				",);", // jf> removed :? from this, for =
				",);",
        "",  // rscan=5, only scans for a negative ] level
        "", // rscan=6, like rscan==0 but lower precedence -- stop at any non-^ control char that is not parenthed
			};


			static const struct 
			{
			  char op[2];
			  char lscan,rscan;
			  char *func;
			} preprocSymbols[] = 
			{
				{{'+','='}, 0, 3, "_addop" },
				{{'-','='}, 0, 3, "_subop" },
				{{'%','='}, 0, 3, "_modop" },
				{{'|','='}, 0, 3, "_orop" },
				{{'&','='}, 0, 3, "_andop"},
				{{'~','='}, 0, 3, "_xorop" },

				{{'/','='}, 0, 3, "_divop"},
				{{'*','='}, 0, 3, "_mulop"},
				{{'^','='}, 0, 3, "_powop"},

				{{'=','='}, 1, 2, "_equal" },
				{{'<','='}, 1, 2, "_beleq" },
				{{'>','='}, 1, 2, "_aboeq" },
				{{'<','<'}, 0, 6, "_shl" },
				{{'>','>'}, 0, 6, "_shr" },
				{{'<',0  }, 1, 2, "_below" },
				{{'>',0  }, 1, 2, "_above" },
				{{'!','='}, 1, 2, "_noteq" },
				{{'|','|'}, 1, 2, "_or" },
				{{'&','&'}, 1, 2, "_and" },
				{{'=',0  }, 0, 3, "_set" },
				{{'~',0},   0, 6, "_xor" },
				{{'%',0},   0, 6, "_mod" },
				{{'^',0},   0, 0, "pow" },


        {{'[',0  }, 0, 5, },
				{{'!',0  },-1, 0, }, // this should also ignore any leading +-
				{{'?',0  }, 1, 4, },

			};


			int n;
			int ns=sizeof(preprocSymbols)/sizeof(preprocSymbols[0]);
			for (n = 0; n < ns; n++)
			{
				if (c == preprocSymbols[n].op[0] && (!preprocSymbols[n].op[1] || expression[0] == preprocSymbols[n].op[1])) 
				{
					break;
				}
			}
			if (n < ns)
			{

				int lscan=preprocSymbols[n].lscan;
				int rscan=preprocSymbols[n].rscan;

	      // parse left side of =, scanning back for an unparenthed nonwhitespace nonalphanumeric nonparenth?
	      // so megabuf(x+y)= would be fine, x=, but +x= would do +set(x,)
       	char *l_ptr=0;
				char *r_ptr=0;
	      if (lscan >= 0)
				{
					char *scan=symbollists[lscan];
	       	int l_semicnt=0;
					l_ptr=buf + len - 1;
					while (l_ptr >= buf)
					{
						if (*l_ptr == ')') l_semicnt++;
						else if (*l_ptr == '(')
						{
							l_semicnt--;
							if (l_semicnt < 0) break;
						}
						else if (!l_semicnt) 
						{
							if (!*scan)
							{
								if (!isspace(*l_ptr) && !isalnum(*l_ptr) && *l_ptr != '_' && *l_ptr != '.') break;
							}
							else
							{
								char *sc=scan;
								if (lscan == 2 && ( // not currently used, even
									(l_ptr[0]=='|' && l_ptr[1] == '|')||
									(l_ptr[0]=='&' && l_ptr[1] == '&')
									)
								   ) break;
								while (*sc && *l_ptr != *sc) sc++;
								if (*sc) break;
							}
						}
						l_ptr--;
					}
					buf[len]=0;

					l_ptr++;

					len = l_ptr - buf;

					l_ptr = strdup(l_ptr); // doesn't need to be preprocessed since it just was
        }
				if (preprocSymbols[n].op[1]) expression++;

				r_ptr=expression;
				{ 
					// scan forward to an uncommented,  unparenthed semicolon, comma, or ), or ]
					int r_semicnt=0,r_semicnt2=0;
					int r_qcnt=0;
					char *scan=symbollists[rscan];
					int commentstate=0;
					int hashadch=0;
					while (*r_ptr)
					{
						if (!commentstate && *r_ptr == '/')
						{
							if (r_ptr[1] == '/') commentstate=1;
							else if (r_ptr[1] == '*') commentstate=2;
						}
						if (commentstate == 1 && *r_ptr == '\n') commentstate=0;
						else if (commentstate == 2 && *r_ptr == '*' && r_ptr[1]=='/')
						{
							r_ptr++; // skip *
							commentstate=0;
						}
						else if (!commentstate)
						{
              if (*r_ptr == '(') { hashadch=1; r_semicnt++; }
              else if (*r_ptr == '[') { hashadch=1; r_semicnt2++; }
							else if (*r_ptr == ')') 
							{
								r_semicnt--;
								if (r_semicnt < 0 && r_semicnt2<=0) break;
							}
							else if (*r_ptr == ']') 
							{
								r_semicnt2--;
								if (r_semicnt2 < 0 && r_semicnt<=0) break;
							}
							else if (!r_semicnt && !r_semicnt2)
							{
								char *sc=scan;
								if (*r_ptr == ';' || *r_ptr == ',') break;
							
								if (!rscan || rscan == 6)
								{
									if (*r_ptr == ':') break;
									if (!isspace(*r_ptr) && !isalnum(*r_ptr) && *r_ptr != '_' && *r_ptr != '.' && 
                      (rscan != 6  || *r_ptr != '^' || r_ptr[1] == '=') && hashadch) break;
									if (isalnum(*r_ptr) || *r_ptr == '_')hashadch=1;
								}								
								else if (rscan == 2 &&
									((r_ptr[0]=='|' && r_ptr[1] == '|')||
									(r_ptr[0]=='&' && r_ptr[1] == '&')
									)
								   ) break;

								else if (rscan == 3 || rscan == 4)
								{
									if (*r_ptr == ':') r_qcnt--;
									else if (*r_ptr == '?') r_qcnt++;

									if (r_qcnt < 3-rscan) break;
								}

								while (*sc && *r_ptr != *sc) sc++;
								if (*sc) break;
							}
						}
						r_ptr++;
					}
					// expression -> r_ptr is our string (not including r_ptr)

					{
						char *orp=r_ptr;

						char rps=*orp;
						*orp=0; // temporarily terminate

						r_ptr=preprocessCode(ctx,expression,src_offset_bytes + (expression-expression_start),dest_offset_bytes + len);
						expression=orp;

						*orp = rps; // fix termination(restore string)
					}

				}

				if (r_ptr)
				{
					int thisl = strlen(l_ptr?l_ptr:"") + strlen(r_ptr) + 32;

	    			if (len+thisl > alloc_len-64)
    				{
      					alloc_len = len+thisl+128;
      					buf=(char*)realloc(buf,alloc_len);
    				}


          if (n == ns-3)
          {
            char *lp = l_ptr;
            char *rp = r_ptr;
      	    while (lp && *lp && isspace(*lp)) lp++;
      	    while (rp && *rp && isspace(*rp)) rp++;
            if (lp && !strncasecmp(lp,"gmem",4) && (!lp[4] || isspace(lp[4])))
            {
              len+=sprintf(buf+len,"_gmem(%s",r_ptr && *r_ptr ? r_ptr : "0");
              ctx->l_stats[0]+=strlen(l_ptr)+4;
            }
            else if (rp && *rp && strcmp(rp,"0"))
            {
  	          len+=sprintf(buf+len,"_mem((%s)+(%s)",lp,rp);
              ctx->l_stats[0]+=strlen(lp)+strlen(rp)+8;
            }
            else 
            {
	            len+=sprintf(buf+len,"_mem(%s",lp);
              ctx->l_stats[0]+=strlen(lp)+4;
            }

            // skip the ]
            if (*expression == ']') expression++;

          }
					else if (n == ns-2)
					{
						len+=sprintf(buf+len,"_not(%s",r_ptr);

						ctx->l_stats[0]+=4;
					}
					else if (n == ns-1)// if (l_ptr,r_ptr1,r_ptr2)
					{
						char *rptr2=r_ptr;
						char *tmp=r_ptr;
						int parcnt=0;
						int qcnt=1;
						while (*rptr2)
						{
							if (*rptr2 == '?') qcnt++;
							else if (*rptr2 == ':') qcnt--;
							else if (*rptr2 == '(') parcnt++;
							else if (*rptr2 == ')') parcnt--;
							if (parcnt < 0) break;
							if (!parcnt && !qcnt && *rptr2 == ':') break;
							rptr2++;
						}
						if (*rptr2) *rptr2++=0;
						while (isspace(*rptr2)) rptr2++;

						while (isspace(*tmp)) tmp++;

						len+=sprintf(buf+len,"_if(%s,%s,%s",l_ptr,*tmp?tmp:"0",*rptr2?rptr2:"0");
						ctx->l_stats[0]+=6;
					}
					else
					{
						len+=sprintf(buf+len,"%s(%s,%s",preprocSymbols[n].func,l_ptr?l_ptr:"",r_ptr);
						ctx->l_stats[0]+=strlen(preprocSymbols[n].func)+2;
					}

				}

				free(r_ptr);
				free(l_ptr);


				c = ')'; // close parenth below
		  }
	  }

//      if (c != ' ' || (len && buf[len-1] != ' ')) // don't bother adding multiple spaces
      {
      	buf[len++]=c;
      	if (c != ' ') ctx->l_stats[0]++;
      }
    }
  }
  buf[len]=0;

  return buf;
}

#ifdef PPROC_TEST

int main(int argc, char* argv[])
{
	compileContext ctx={0};
	char *p=preprocessCode(&ctx,argv[1]);
	if (p)printf("%s\n",p);
	free(p);
	return 0;
}

#endif

#if 0
static void movestringover(char *str, int amount)
{
  char tmp[1024+8];

  int l=(int)strlen(str);
  l=min(1024-amount-1,l);

  memcpy(tmp,str,l+1);

  while (l >= 0 && tmp[l]!='\n') l--;
  l++;

  tmp[l]=0;//ensure we null terminate

  memcpy(str+amount,tmp,l+1);
}
#endif

//------------------------------------------------------------------------------
NSEEL_CODEHANDLE NSEEL_code_compile(NSEEL_VMCTX _ctx, const char *__expression, int lineoffs)
{
  return NSEEL_code_compile_ex(_ctx,__expression,lineoffs,0);
}

typedef struct topLevelCodeSegmentRec {
  struct topLevelCodeSegmentRec *_next;
  void *code;
  int codesz;
  int tmptable_use;
} topLevelCodeSegmentRec;

NSEEL_CODEHANDLE NSEEL_code_compile_ex(NSEEL_VMCTX _ctx, const char *__expression, int lineoffs, int compile_flags)
{
  char *_expression;
  compileContext *ctx = (compileContext *)_ctx;
  char *expression,*expression_start;
  codeHandleType *handle;
  topLevelCodeSegmentRec *startpts_tail=NULL;
  topLevelCodeSegmentRec *startpts=NULL;
  _codeHandleFunctionRec *oldCommonFunctionList;
  int curtabptr_sz=0;
  void *curtabptr=NULL;
  int had_err=0;

  if (!ctx) return 0;

  ctx->directValueCache=0;
  ctx->optimizeDisableFlags=0;

  if (compile_flags & NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS_RESET)
  {
    ctx->functions_common=NULL; // reset common function list
  }
  else
  {
    // reset common compiled function code, forcing a recompile if shared
    _codeHandleFunctionRec *a = ctx->functions_common;
    while (a)
    {
      _codeHandleFunctionRec *b = a->derivedCopies;

      if (a->localstorage) 
      {
        // force local storage actual values to be reallocated if used again
        memset(a->localstorage,0,sizeof(EEL_F *) * a->localstorage_size);
      }

      a->startptr = NULL; // force this copy to be recompiled

      while (b)
      {
        b->startptr = NULL; // force derived copies to get recompiled
        // no need to reset b->localstorage, since it points to a->localstorage
        b=b->derivedCopies;
      }

      a=a->next;
    }
  }
  
  ctx->last_error_string[0]=0;

  if (!__expression || !*__expression) return 0;


  _expression = strdup(__expression);
  if (!_expression) return 0;

  oldCommonFunctionList = ctx->functions_common;
  {
    // do in place replace of "$'x'" to "56  " or whatnot
    // we avoid changing the length of the string here, due to wanting to know where errors occur
    char *p=_expression;
    while (*p)
    {
      if (p[0] == '$' && p[1]=='\'' && p[2] && p[3]=='\'')
      {
        char tmp[64];
        int a,tl;
        sprintf(tmp,"%d",(int)((unsigned char *)p)[2]);
        tl=strlen(tmp);
        if (tl>3) tl=3;
        for (a=0;a<tl;a++) p[a]=tmp[a];
        for (;a<4;a++) p[a]=' ';
        p+=4;
      }
      else
      {
        p++;
      }
    }
  }

  ctx->isSharedFunctions = !!(compile_flags & NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);
  ctx->functions_local = NULL;

  freeBlocks(&ctx->tmpblocks_head);  // free blocks
  freeBlocks(&ctx->blocks_head);  // free blocks
  freeBlocks(&ctx->blocks_head_data);  // free blocks
  memset(ctx->l_stats,0,sizeof(ctx->l_stats));
  free(ctx->compileLineRecs); 
  ctx->compileLineRecs=0; 
  ctx->compileLineRecs_size=0; 
  ctx->compileLineRecs_alloc=0;

  handle = (codeHandleType*)newDataBlock(sizeof(codeHandleType),8);

  if (!handle) 
  {
    free(_expression);
    return 0;
  }

  
  memset(handle,0,sizeof(codeHandleType));

  ctx->tmpCodeHandle = handle;
  expression_start=expression=preprocessCode(ctx,_expression,0,0);

  while (*expression)
  {
    int computTableTop = 0;
    int startptr_size=0;
    void *startptr=NULL;
    opcodeRec *start_opcode=NULL;
    char *expr;
    int function_numparms=0;
    char is_fname[NSEEL_MAX_VARIABLE_NAMELEN+1];
    is_fname[0]=0;

    memset(ctx->function_localTable_Size,0,sizeof(ctx->function_localTable_Size));
    memset(ctx->function_localTable_Names,0,sizeof(ctx->function_localTable_Names));
    ctx->function_localTable_ValuePtrs=0;
    ctx->function_usesThisPointer=0;
    ctx->function_curName=NULL;
    
#ifdef NSEEL_USE_OLD_PARSER
    ctx->colCount=0;
#endif
    
    // single out segment
    while (*expression == ';' || isspace(*expression)) expression++;
    if (!*expression) break;
    expr=expression;

    while (*expression && *expression != ';') expression++;
    if (*expression) *expression++ = 0;

    // parse   

    if (!strncasecmp(expr,"function",8) && isspace(expr[8]))
    {
      char *p = expr+8;
      while (isspace(p[0])) p++;
      if (isalpha(p[0]) || p[0] == '_') 
      {
        int had_parms_locals=0;
        char *sp=p;
        int l;
        while (isalnum(p[0]) || p[0] == '_') p++;
        l=min(p-sp, sizeof(is_fname)-1);
        memcpy(is_fname, sp, l);
        is_fname[l]=0;
        ctx->function_curName = is_fname; // only assigned for the duration of the loop, cleared later //-V507

        expr = p;

        while (*expr)
        {
          const char *tn;
          int tn_len;
          p=expr;
          while (isspace(*p)) p++;

          tn = p;
          while (*p && !isspace(*p) && *p != '(') p++;
          tn_len = p - tn;

          while (isspace(*p)) p++;
        
          if (*p == '(' && 
              (
                !tn_len ||
                (tn_len == 5 && !strncasecmp(tn,"local",tn_len))  ||
                (tn_len == 6 && !strncasecmp(tn,"static",tn_len))  ||
                (tn_len == 8 && !strncasecmp(tn,"instance",tn_len))
              )
             )
          {
            int maxcnt=0,state=0;
            int is_parms = 0;
            int localTableContext = 0;

            if (tn_len == 0) 
            {
              if (had_parms_locals) break; // formal parameters must be before instance() static() or local(), otherwise it is assumed to be the body of the function
              is_parms = 1;
            }
            else 
            {
              localTableContext = (tn_len == 8 && !strncasecmp(tn,"instance",tn_len)); //adding to "implied this" table
            }
            had_parms_locals=1;

            // skip past opening paren
            p++;

            sp=p;
            while (*p && *p != ')') 
            {
              if (isspace(*p) || *p == ',')
              {
                if (state) maxcnt++;
                state=0;
              }
              else state=1;
              p++;
            }
            if (state) maxcnt++;
            if (*p)
            {
              expr=p+1;
          
              if (maxcnt > 0)
              {
                char **ot = ctx->function_localTable_Names[localTableContext];
                int osz = ctx->function_localTable_Size[localTableContext];

                maxcnt += osz;

                ctx->function_localTable_Names[localTableContext] = (char **)newTmpBlock(ctx,sizeof(char *) * maxcnt);

                if (ctx->function_localTable_Names[localTableContext])
                {
                  int i=osz;
                  if (osz && ot) memcpy(ctx->function_localTable_Names[localTableContext],ot,sizeof(char *) * osz);
                  p=sp;
                  while (p < expr-1 && i < maxcnt)
                  {
                    while (p < expr && (isspace(*p) || *p == ',')) p++;
                    sp=p;
                    while (p < expr-1 && (!isspace(*p) && *p != ',')) p++;
                    
                    if (isalpha(*sp) || *sp == '_')
                    {
                      char *newstr;
                      int l = (p-sp);
                      if (l > NSEEL_MAX_VARIABLE_NAMELEN) l = NSEEL_MAX_VARIABLE_NAMELEN;
                      newstr = newTmpBlock(ctx,l+1);
                      if (newstr)
                      {
                        memcpy(newstr,sp,l);
                        newstr[l]=0;
                        ctx->function_localTable_Names[localTableContext][i++] = newstr;
                      }
                    }
                  }

                  ctx->function_localTable_Size[localTableContext]=i;

                  if (is_parms) function_numparms = i;
                }
              }
            }         
          }
          else break;
        }
      }
    }
    if (ctx->function_localTable_Size>0)
    {
      ctx->function_localTable_ValuePtrs = 
          ctx->isSharedFunctions ? newDataBlock(ctx->function_localTable_Size[0] * sizeof(EEL_F *),8) : 
                                   newTmpBlock(ctx,ctx->function_localTable_Size[0] * sizeof(EEL_F *)); 
      if (!ctx->function_localTable_ValuePtrs)
      {
        ctx->function_localTable_Size[0]=0;
        function_numparms=0;
      }
      else
      {
        memset(ctx->function_localTable_ValuePtrs,0,sizeof(EEL_F *) * ctx->function_localTable_Size[0]); // force values to be allocated
      }
    }

    ctx->errVar=0;

#ifdef NSEEL_USE_OLD_PARSER
    nseel_llinit(ctx);
    if (!nseel_yyparse(ctx,expr) && !ctx->errVar)
    {
      start_opcode = ctx->result;
    }
#else
   {
     int nseelparse(compileContext* context);

#ifdef NSEEL_SUPER_MINIMAL_LEXER
     ctx->rdbuf_start = ctx->rdbuf = expr;
     if (!nseelparse(ctx) && !ctx->errVar)
     {
       start_opcode = ctx->result;
     }
     ctx->rdbuf = NULL;
#else

     void nseelrestart (void *input_file ,void *yyscanner );

     nseelrestart(NULL,ctx->scanner);
     ctx->inputbufferptr = expr;

     if (!nseelparse(ctx) && !ctx->errVar)
     {
       start_opcode = ctx->result;
     }
     if (ctx->errVar && ctx->errVar_l>0)
     {
       const char *p=expr;
       while (*p && ctx->errVar_l-->0)
       {
         while (*p && *p != '\n') { p++; ctx->errVar++; }
         if (*p) { ctx->errVar++; p++; }
       }
     }
     ctx->inputbufferptr=NULL;
#endif

   }
#endif
           
    if (start_opcode)
    {
      int rvMode=0, fUse=0;

#ifdef LOG_OPT
      char buf[512];
      int sd=0;
      sprintf(buf,"pre opt sz=%d (tsackDepth=%d)\n",compileOpcodes(ctx,start_opcode,NULL,1024*1024*256,NULL, NULL,RETURNVALUE_IGNORE,NULL,&sd,NULL),sd);
#ifdef _WIN32
      OutputDebugString(buf);
#else
      printf("%s\n",buf);
#endif
#endif
      if (!(ctx->optimizeDisableFlags&OPTFLAG_NO_OPTIMIZE)) optimizeOpcodes(ctx,start_opcode,is_fname[0] ? 1 : 0);
#ifdef LOG_OPT
      sprintf(buf,"post opt sz=%d, stack depth=%d\n",compileOpcodes(ctx,start_opcode,NULL,1024*1024*256,NULL,NULL, RETURNVALUE_IGNORE,NULL,&sd,NULL),sd);
#ifdef _WIN32
      OutputDebugString(buf);
#else
      printf("%s\n",buf);
#endif
#endif

#ifdef DUMP_OPS_DURING_COMPILE
      g_debugfp_indent=0;
      g_debugfp_histsz=0;
      g_debugfp = fopen("C:/temp/foo.txt","w");
#endif
      startptr_size = compileOpcodes(ctx,start_opcode,NULL,1024*1024*256,NULL, NULL, 
        is_fname[0] ? (RETURNVALUE_NORMAL|RETURNVALUE_FPSTACK) : RETURNVALUE_IGNORE, &rvMode, &fUse, NULL); // if not a function, force return value as address (avoid having to pop it ourselves
                                          // if a function, allow the code to decide how return values are generated

#ifdef DUMP_OPS_DURING_COMPILE
      if (g_debugfp) fclose(g_debugfp);
      g_debugfp=0;
#endif

      if (is_fname[0])
      {
        _codeHandleFunctionRec *fr = ctx->isSharedFunctions ? newDataBlock(sizeof(_codeHandleFunctionRec),8) : 
                                        newTmpBlock(ctx,sizeof(_codeHandleFunctionRec)); 
        if (fr)
        {
          memset(fr,0,sizeof(_codeHandleFunctionRec));
          fr->startptr_size = startptr_size;
          fr->opcodes = start_opcode;
          fr->rvMode = rvMode;
     
          fr->fpStackUsage=fUse;
          fr->tmpspace_req = computTableTop;

          if (ctx->function_localTable_Size[0] > 0 && ctx->function_localTable_ValuePtrs)
          {
            fr->num_params=function_numparms;
            fr->localstorage = ctx->function_localTable_ValuePtrs;
            fr->localstorage_size = ctx->function_localTable_Size[0];
          }

          fr->usesThisPointer = ctx->function_usesThisPointer;
          fr->isCommonFunction = ctx->isSharedFunctions;

          strcpy(fr->fname,is_fname);

          if (ctx->isSharedFunctions)
          {
            fr->next = ctx->functions_common;
            ctx->functions_common = fr;
          }
          else
          {
            fr->next = ctx->functions_local;
            ctx->functions_local = fr;
          }         
        }
        continue;
      }

      if (!startptr_size) continue; // optimized away
      if (startptr_size>0)
      {
        startptr = newTmpBlock(ctx,startptr_size);
        if (startptr)
        {
          startptr_size=compileOpcodes(ctx,start_opcode,(unsigned char*)startptr,startptr_size,&computTableTop, NULL, RETURNVALUE_IGNORE, NULL,NULL, NULL);
          if (startptr_size<=0) startptr = NULL;
          
        }
      }
    }

    if (!startptr) 
    { 
      int byteoffs = expr - expression_start;
      int destoffs,linenumber;
      char buf[50], *p;
      int x,le;
      
#ifdef NSEEL_EEL1_COMPAT_MODE
      if (!startptr) continue;
#endif

      if (ctx->errVar > 0) byteoffs += ctx->errVar;
      linenumber=findByteOffsetInSource(ctx,byteoffs,&destoffs);
      if (destoffs < 0) destoffs=0;

      le=strlen(_expression);
      if (destoffs >= le) destoffs=le;
      p= _expression + destoffs;
      x=0;
      while (x < sizeof(buf)-1)
      {
	      if (!*p) break;
        if (x && (*p == '\r' || *p == '\n')) break;

        if (!isspace(*p) || (x && !isspace(p[-1]))) buf[x++]=*p;
        
        p++;
      }
      buf[x]=0;

      if (!ctx->last_error_string[0])
        snprintf(ctx->last_error_string,sizeof(ctx->last_error_string),"Around line %d '%s'",linenumber+lineoffs,buf);

      startpts=NULL;
      startpts_tail=NULL; 
      had_err=1;
      break; 
    }
    
    if (!is_fname[0]) // redundant check (if is_fname[0] is set and we succeeded, it should continue)
                      // but we'll be on the safe side
    {
      topLevelCodeSegmentRec *p = newTmpBlock(ctx,sizeof(topLevelCodeSegmentRec));
      p->_next=0;
      p->code = startptr;
      p->codesz = startptr_size;
      p->tmptable_use = computTableTop;
                  
      if (!startpts_tail) startpts_tail=startpts=p;
      else
      {
        startpts_tail->_next=p;
        startpts_tail=p;
      }

      if (curtabptr_sz < computTableTop)
      {
        curtabptr_sz=computTableTop;
      }
    }
  }
  free(ctx->compileLineRecs); 
  ctx->compileLineRecs=0; 
  ctx->compileLineRecs_size=0; 
  ctx->compileLineRecs_alloc=0;

  memset(ctx->function_localTable_Size,0,sizeof(ctx->function_localTable_Size));
  memset(ctx->function_localTable_Names,0,sizeof(ctx->function_localTable_Names));
  ctx->function_localTable_ValuePtrs=0;
  ctx->function_usesThisPointer=0;
  ctx->function_curName=NULL;

  ctx->tmpCodeHandle = NULL;
    
  if (handle->want_stack)
  {
    if (!handle->stack) startpts=NULL;
  }

  if (startpts) 
  {
    curtabptr_sz += 2; // many functions use the worktable for temporary storage of up to 2 EEL_F's

    handle->workTable_size = curtabptr_sz;
    handle->workTable = curtabptr = newDataBlock((curtabptr_sz+MIN_COMPUTABLE_SIZE + COMPUTABLE_EXTRA_SPACE) * sizeof(EEL_F),32);

#ifdef EEL_VALIDATE_WORKTABLE_USE
    if (curtabptr) memset(curtabptr,0x3a,(curtabptr_sz+MIN_COMPUTABLE_SIZE + COMPUTABLE_EXTRA_SPACE) * sizeof(EEL_F));
#endif
    if (!curtabptr) startpts=NULL;
  }


  if (startpts || (!had_err && (compile_flags & NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS)))
  {
    unsigned char *writeptr;
    topLevelCodeSegmentRec *p=startpts;
    int size=sizeof(GLUE_RET)+GLUE_FUNC_ENTER_SIZE+GLUE_FUNC_LEAVE_SIZE; // for ret at end :)
    int wtpos=0;

    // now we build one big code segment out of our list of them, inserting a mov esi, computable before each item as necessary
    while (p)
    {
      if (wtpos <= 0)
      {
        wtpos=MIN_COMPUTABLE_SIZE;
        size += GLUE_RESET_WTP(NULL,0);
      }
      size+=p->codesz;
      wtpos -= p->tmptable_use;
      p=p->_next;
    }
    handle->code = newCodeBlock(size,32);
    if (handle->code)
    {
      writeptr=(unsigned char *)handle->code;
      #if GLUE_FUNC_ENTER_SIZE > 0
        memcpy(writeptr,&GLUE_FUNC_ENTER,GLUE_FUNC_ENTER_SIZE); 
        writeptr += GLUE_FUNC_ENTER_SIZE;
      #endif
      p=startpts;
      wtpos=0;
      while (p)
      {
        if (wtpos <= 0)
        {
          wtpos=MIN_COMPUTABLE_SIZE;
          writeptr+=GLUE_RESET_WTP(writeptr,curtabptr);
        }
        memcpy(writeptr,(char*)p->code,p->codesz);
        writeptr += p->codesz;
        wtpos -= p->tmptable_use;
      
        p=p->_next;
      }
      #if GLUE_FUNC_LEAVE_SIZE > 0
        memcpy(writeptr,&GLUE_FUNC_LEAVE,GLUE_FUNC_LEAVE_SIZE); 
        writeptr += GLUE_FUNC_LEAVE_SIZE;
      #endif
      memcpy(writeptr,&GLUE_RET,sizeof(GLUE_RET)); writeptr += sizeof(GLUE_RET);
      ctx->l_stats[1]=size;
      handle->code_size = writeptr - (unsigned char *)handle->code;
    }
    
    handle->blocks = ctx->blocks_head;
    handle->blocks_data = ctx->blocks_head_data;
    ctx->blocks_head=0;
    ctx->blocks_head_data=0;
  }
  else
  {
    // failed compiling, or failed calloc()
    handle=NULL;              // return NULL (after resetting blocks_head)
  }


  ctx->directValueCache=0;
  ctx->functions_local = NULL;
  
  ctx->isSharedFunctions=0;

  freeBlocks(&ctx->tmpblocks_head);  // free blocks
  freeBlocks(&ctx->blocks_head);  // free blocks of code (will be nonzero only on error)
  freeBlocks(&ctx->blocks_head_data);  // free blocks of data (will be nonzero only on error)

  if (handle)
  {
    handle->ramPtr = ctx->ram_state.blocks;
    memcpy(handle->code_stats,ctx->l_stats,sizeof(ctx->l_stats));
    nseel_evallib_stats[0]+=ctx->l_stats[0];
    nseel_evallib_stats[1]+=ctx->l_stats[1];
    nseel_evallib_stats[2]+=ctx->l_stats[2];
    nseel_evallib_stats[3]+=ctx->l_stats[3];
    nseel_evallib_stats[4]++;
  }
  else
  {
    ctx->functions_common = oldCommonFunctionList; // failed compiling, remove any added common functions from the list

    // remove any derived copies of functions due to error, since we may have added some that have been freed
    while (oldCommonFunctionList)
    {
      oldCommonFunctionList->derivedCopies=NULL;
      oldCommonFunctionList=oldCommonFunctionList->next;
    }
  }
  memset(ctx->l_stats,0,sizeof(ctx->l_stats));

  free(expression_start);
  free(_expression);

  return (NSEEL_CODEHANDLE)handle;
}

//------------------------------------------------------------------------------
void NSEEL_code_execute(NSEEL_CODEHANDLE code)
{
  INT_PTR tabptr;
  INT_PTR codeptr;
  codeHandleType *h = (codeHandleType *)code;
  if (!h || !h->code) return;

  codeptr = (INT_PTR) h->code;
#if 0
  {
	unsigned int *p=(unsigned int *)codeptr;
	while (*p != GLUE_RET[0])
	{
		printf("instr:%04X:%04X\n",*p>>16,*p&0xffff);
		p++;
	}
  }
#endif

  tabptr=(INT_PTR)h->workTable;
  //printf("calling code!\n");
  GLUE_CALL_CODE(tabptr,codeptr,(INT_PTR)h->ramPtr);

}


char *NSEEL_code_getcodeerror(NSEEL_VMCTX ctx)
{
  compileContext *c=(compileContext *)ctx;
  if (ctx && c->last_error_string[0]) return c->last_error_string;
  return 0;
}

//------------------------------------------------------------------------------
void NSEEL_code_free(NSEEL_CODEHANDLE code)
{
  codeHandleType *h = (codeHandleType *)code;
  if (h != NULL)
  {
#ifdef EEL_VALIDATE_WORKTABLE_USE
    if (h->workTable)
    {
      char *p = ((char*)h->workTable) + h->workTable_size*sizeof(EEL_F);
      int x;
      for(x=COMPUTABLE_EXTRA_SPACE*sizeof(EEL_F) - 1;x >= 0; x --)
        if (p[x] != 0x3a)
        {
          char buf[512];
          sprintf(buf,"worktable overrun at byte %d (wts=%d), value = %f\n",x,h->workTable_size, *(EEL_F*)(p+(x&~(sizeof(EEL_F)-1))));
          OutputDebugString(buf);
          break;
        }
    }
#endif

    nseel_evallib_stats[0]-=h->code_stats[0];
    nseel_evallib_stats[1]-=h->code_stats[1];
    nseel_evallib_stats[2]-=h->code_stats[2];
    nseel_evallib_stats[3]-=h->code_stats[3];
    nseel_evallib_stats[4]--;
    
#ifdef EEL_PPC_NOFREECODE
  #pragma warn leaky-code mode, not freeing code, will leak, fixme!!!
#else
  freeBlocks(&h->blocks);
#endif
    
    freeBlocks(&h->blocks_data);


  }

}


//------------------------------------------------------------------------------
static void NSEEL_VM_freevars(NSEEL_VMCTX _ctx)
{
  if (_ctx)
  {
    compileContext *ctx=(compileContext *)_ctx;

    free(ctx->varTable_Values);
    free(ctx->varTable_Names);
    ctx->varTable_Values=0;
    ctx->varTable_Names=0;

    ctx->varTable_numBlocks=0;
  }
}


NSEEL_VMCTX NSEEL_VM_alloc() // return a handle
{
  compileContext *ctx=calloc(1,sizeof(compileContext));
#ifndef NSEEL_USE_OLD_PARSER

  #ifdef NSEEL_SUPER_MINIMAL_LEXER
    ctx->scanner = ctx;
  #else
    if (ctx)
    {
      int nseellex_init(void ** ptr_yy_globals);
      void nseelset_extra(void *user_defined , void *yyscanner);
      if (nseellex_init(&ctx->scanner))
      {
        free(ctx);
        return NULL;
      }
      nseelset_extra(ctx,ctx->scanner);
    }
  #endif

#endif

  if (ctx) ctx->ram_state.closefact = NSEEL_CLOSEFACTOR;
  return ctx;
}

void NSEEL_VM_free(NSEEL_VMCTX _ctx) // free when done with a VM and ALL of its code have been freed, as well
{

  if (_ctx)
  {
    compileContext *ctx=(compileContext *)_ctx;
    NSEEL_VM_freevars(_ctx);
    NSEEL_VM_freeRAM(_ctx);

    freeBlocks(&ctx->pblocks);

    // these should be 0 normally but just in case
    freeBlocks(&ctx->tmpblocks_head);  // free blocks
    freeBlocks(&ctx->blocks_head);  // free blocks
    freeBlocks(&ctx->blocks_head_data);  // free blocks


    free(ctx->compileLineRecs);

#ifndef NSEEL_USE_OLD_PARSER
    #ifndef NSEEL_SUPER_MINIMAL_LEXER
      if (ctx->scanner)
      {
       int nseellex_destroy(void *yyscanner);
       nseellex_destroy(ctx->scanner);
      }
    #endif
    ctx->scanner=0;
#endif
    free(ctx);
  }

}

int *NSEEL_code_getstats(NSEEL_CODEHANDLE code)
{
  codeHandleType *h = (codeHandleType *)code;
  if (h)
  {
    return h->code_stats;
  }
  return 0;
}

void NSEEL_VM_SetCustomFuncThis(NSEEL_VMCTX ctx, void *thisptr)
{
  if (ctx)
  {
    compileContext *c=(compileContext*)ctx;
    c->caller_this=thisptr;
  }
}





void *NSEEL_PProc_RAM(void *data, int data_size, compileContext *ctx)
{
  if (data_size>0) data=EEL_GLUE_set_immediate(data, (INT_PTR)ctx->ram_state.blocks); 
  return data;
}

void *NSEEL_PProc_THIS(void *data, int data_size, compileContext *ctx)
{
  if (data_size>0) data=EEL_GLUE_set_immediate(data, (INT_PTR)ctx->caller_this);
  return data;
}

void NSEEL_VM_remove_unused_vars(NSEEL_VMCTX _ctx)
{
  compileContext *ctx = (compileContext *)_ctx;
  int wb;
  if (ctx) for (wb = 0; wb < ctx->varTable_numBlocks; wb ++)
  {
    int ti;
    char **plist=ctx->varTable_Names[wb];
    if (!plist) break;

    for (ti = 0; ti < NSEEL_VARS_PER_BLOCK; ti ++)
    {        
      if (plist[ti])
      {
        varNameHdr *v = ((varNameHdr*)plist[ti])-1;
        if (!v->refcnt && !v->isreg) 
        {
          plist[ti]=NULL;
        }
      }
    }
  }
}

void NSEEL_VM_remove_all_nonreg_vars(NSEEL_VMCTX _ctx)
{
  compileContext *ctx = (compileContext *)_ctx;
  int wb;
  if (ctx) for (wb = 0; wb < ctx->varTable_numBlocks; wb ++)
  {
    int ti;
    char **plist=ctx->varTable_Names[wb];
    if (!plist) break;

    for (ti = 0; ti < NSEEL_VARS_PER_BLOCK; ti ++)
    {        
      if (plist[ti])
      {
        varNameHdr *v = ((varNameHdr*)plist[ti])-1;
        if (!v->isreg) 
        {
          plist[ti]=NULL;
        }
      }
    }
  }
}

void NSEEL_VM_clear_var_refcnts(NSEEL_VMCTX _ctx)
{
  compileContext *ctx = (compileContext *)_ctx;
  int wb;
  if (ctx) for (wb = 0; wb < ctx->varTable_numBlocks; wb ++)
  {
    int ti;
    char **plist=ctx->varTable_Names[wb];
    if (!plist) break;

    for (ti = 0; ti < NSEEL_VARS_PER_BLOCK; ti ++)
    {        
      if (plist[ti])
      {
        varNameHdr *v = ((varNameHdr*)plist[ti])-1;
        v->refcnt=0;
      }
    }
  }
}

EEL_F *nseel_int_register_var(compileContext *ctx, const char *name, int isReg)
{
  int match_wb = -1, match_ti=-1;
  int wb;
  int ti=0;
  for (wb = 0; wb < ctx->varTable_numBlocks; wb ++)
  {
    char **plist=ctx->varTable_Names[wb];
    if (!plist) return NULL; // error!

    for (ti = 0; ti < NSEEL_VARS_PER_BLOCK; ti ++)
    { 
      if (!plist[ti])
      {
        if (match_wb < 0)
        {
          match_wb=wb;
          match_ti=ti;
        }
      }
      else if (!strncasecmp(plist[ti],name,NSEEL_MAX_VARIABLE_NAMELEN))
      {
        varNameHdr *v = ((varNameHdr*)plist[ti])-1;
        v->refcnt++;
        if (isReg) v->isreg=isReg;
        break;
      }
    }
    if (ti < NSEEL_VARS_PER_BLOCK) break;
  }

  if (wb == ctx->varTable_numBlocks && match_wb >=0 && match_ti >= 0)
  {
    wb = match_wb;
    ti = match_ti;
  }

  if (wb == ctx->varTable_numBlocks)
  {
    ti=0;
    // add new block
    if (!(ctx->varTable_numBlocks&(NSEEL_VARS_MALLOC_CHUNKSIZE-1)) || !ctx->varTable_Values || !ctx->varTable_Names )
    {
      ctx->varTable_Values = (EEL_F **)realloc(ctx->varTable_Values,(ctx->varTable_numBlocks+NSEEL_VARS_MALLOC_CHUNKSIZE) * sizeof(EEL_F *));
      ctx->varTable_Names = (char ***)realloc(ctx->varTable_Names,(ctx->varTable_numBlocks+NSEEL_VARS_MALLOC_CHUNKSIZE) * sizeof(char **));

      if (!ctx->varTable_Names || !ctx->varTable_Values) return NULL;
    }
    ctx->varTable_numBlocks++;

    ctx->varTable_Values[wb] = (EEL_F *)newCtxDataBlock(sizeof(EEL_F)*NSEEL_VARS_PER_BLOCK,8);
    ctx->varTable_Names[wb] = (char **)newCtxDataBlock(sizeof(char *)*NSEEL_VARS_PER_BLOCK,1);
    if (ctx->varTable_Values[wb])
    {
      memset(ctx->varTable_Values[wb],0,sizeof(EEL_F)*NSEEL_VARS_PER_BLOCK);
    }
    if (ctx->varTable_Names[wb])
    {
      memset(ctx->varTable_Names[wb],0,sizeof(char *)*NSEEL_VARS_PER_BLOCK);
    }
  }

  if (!ctx->varTable_Names[wb] || !ctx->varTable_Values[wb]) return NULL;

  if (!ctx->varTable_Names[wb][ti])
  {
    int l = strlen(name);
    char *b;
    varNameHdr *vh;
    if (l > NSEEL_MAX_VARIABLE_NAMELEN) l = NSEEL_MAX_VARIABLE_NAMELEN;
    b=newCtxDataBlock( sizeof(varNameHdr) + l+1,1);
    if (!b) return NULL; // malloc fail
    vh=(varNameHdr *)b;
    vh->refcnt=1;
    vh->isreg=isReg;

    b+=sizeof(varNameHdr);

    memcpy(b,name,l);
    b[l] = 0;

    ctx->varTable_Names[wb][ti] = b;
    ctx->varTable_Values[wb][ti]=0.0;
  }
  return ctx->varTable_Values[wb] + ti;
}



EEL_F nseel_globalregs[100];


//------------------------------------------------------------------------------

void NSEEL_VM_enumallvars(NSEEL_VMCTX ctx, int (*func)(const char *name, EEL_F *val, void *ctx), void *userctx)
{
  compileContext *tctx = (compileContext *) ctx;
  int wb;
  if (!tctx) return;
  
  for (wb = 0; wb < tctx->varTable_numBlocks; wb ++)
  {
    int ti;
    char **plist=tctx->varTable_Names[wb];
    if (!plist) break;
    
    for (ti = 0; ti < NSEEL_VARS_PER_BLOCK; ti ++)
    {              
      if (plist[ti] && !func(plist[ti],tctx->varTable_Values[wb] + ti,userctx)) break;
    }
    if (ti < NSEEL_VARS_PER_BLOCK)
      break;
  }
}


//------------------------------------------------------------------------------
EEL_F *NSEEL_VM_regvar(NSEEL_VMCTX _ctx, const char *var)
{
  compileContext *ctx = (compileContext *)_ctx;
  if (!ctx) return 0;
  
  if (!strncasecmp(var,"reg",3) && strlen(var) == 5 && isdigit(var[3]) && isdigit(var[4]))
  {
    int x=atoi(var+3);
    if (x < 0 || x > 99) x=0;
    return nseel_globalregs + x;
  }
  
  return nseel_int_register_var(ctx,var,1);
}

int  NSEEL_VM_get_var_refcnt(NSEEL_VMCTX _ctx, const char *name)
{
  compileContext *ctx = (compileContext *)_ctx;
  int wb;
  if (!ctx) return -1;

  for (wb = 0; wb < ctx->varTable_numBlocks; wb ++)
  {
    int ti;
    if (!ctx->varTable_Values[wb] || !ctx->varTable_Names[wb]) break;

    for (ti = 0; ti < NSEEL_VARS_PER_BLOCK; ti ++)
    {        
      if (ctx->varTable_Names[wb][ti] && !strcasecmp(ctx->varTable_Names[wb][ti],name)) 
      {
        varNameHdr *h = ((varNameHdr *)ctx->varTable_Names[wb][ti])-1;
        return h->refcnt;
      }
    }
  }

  return -1;
}



//------------------------------------------------------------------------------
opcodeRec *nseel_lookup(compileContext *ctx, int *typeOfObject, const char *sname)
{
  char tmp[NSEEL_MAX_VARIABLE_NAMELEN*2];
  int i;
  *typeOfObject = IDENTIFIER;
  
  lstrcpyn_safe(tmp,sname,sizeof(tmp));
  
  if (!strncasecmp(tmp,"reg",3) && strlen(tmp) == 5 && isdigit(tmp[3]) && isdigit(tmp[4]) && (i=atoi(tmp+3))>=0 && i<100)
  {
    return nseel_createCompiledValuePtr(ctx,nseel_globalregs+i);
  }
  
  // scan for parameters/local variables before user functions   
  if (strncasecmp(tmp,"this.",5) && 
      ctx->function_localTable_Size[0] > 0 &&
      ctx->function_localTable_Names[0] && 
      ctx->function_localTable_ValuePtrs)
  {
    char **namelist = ctx->function_localTable_Names[0];
    for (i=0; i < ctx->function_localTable_Size[0]; i++)
    {
      if (namelist[i] && !strncasecmp(namelist[i],tmp,NSEEL_MAX_VARIABLE_NAMELEN))
      {
        return nseel_createCompiledValuePtrPtr(ctx, ctx->function_localTable_ValuePtrs+i);
      }
    }
  }
  
  // if instance name set, translate tmp or tmp.* into "this.tmp.*"
  if (strncasecmp(tmp,"this.",5) && 
      ctx->function_localTable_Size[1] > 0 && 
      ctx->function_localTable_Names[1])
  {
    char **namelist = ctx->function_localTable_Names[1];
    for (i=0; i < ctx->function_localTable_Size[1]; i++)
    {
      int tl = namelist[i] ? strlen(namelist[i]) : 0;
      
      if (tl && !strncasecmp(namelist[i],tmp,tl) && (tmp[tl] == 0 || tmp[tl] == '.'))
      {
        strcpy(tmp,"this.");
        lstrcpyn_safe(tmp + 5, sname, sizeof(tmp) - 5); // update tmp with "this.tokenname"
        break;
      }
    }
  }
  
  
  if (strncasecmp(tmp,"this.",5))
  {
    const char *nptr = tmp;
    
#ifdef NSEEL_EEL1_COMPAT_MODE
    if (!strcasecmp(nptr,"if")) nptr="_if";
    else if (!strcasecmp(nptr,"bnot")) nptr="_not";
    else if (!strcasecmp(nptr,"assign")) nptr="_set";
    else if (!strcasecmp(nptr,"equal")) nptr="_equal";
    else if (!strcasecmp(nptr,"below")) nptr="_below";
    else if (!strcasecmp(nptr,"above")) nptr="_above";
    else if (!strcasecmp(nptr,"megabuf")) nptr="_mem";
    else if (!strcasecmp(nptr,"gmegabuf")) nptr="_gmem";
#endif
    
    for (i=0;nseel_getFunctionFromTable(i);i++)
    {
      functionType *f=nseel_getFunctionFromTable(i);
      if (!strcasecmp(f->name, nptr))
      {
        int np=f->nParams&FUNCTIONTYPE_PARAMETERCOUNTMASK;
        switch (np)
        {
          case 0:
          case 1: *typeOfObject = FUNCTION1; break;
          case 2: *typeOfObject = FUNCTION2; break;
          case 3: *typeOfObject = FUNCTION3; break;
          default: 
#ifndef NSEEL_USE_OLD_PARSER
            *typeOfObject = FUNCTIONX; // newly supported X-parameter functions
#else
            *typeOfObject = FUNCTION1;  // should never happen, unless the caller was silly
#endif
            break;
        }
        return nseel_createCompiledFunctionCall(ctx,np,FUNCTYPE_FUNCTIONTYPEREC,(void *) f);
      }
    }
  } 
  
  {
    _codeHandleFunctionRec *fr = NULL;
    
    char *postName = tmp;
    while (*postName) postName++;
    while (postName >= tmp && *postName != '.') postName--;
    if (++postName <= tmp) postName=0;
    
    if (!fr)
    {
      fr = ctx->functions_local;
      while (fr)
      {
        if (!strcasecmp(fr->fname,postName?postName:tmp)) break;
        fr=fr->next;
      }
    }
    if (!fr)
    {
      fr = ctx->functions_common;
      while (fr)
      {
        if (!strcasecmp(fr->fname,postName?postName:tmp)) break;
        fr=fr->next;
      }
    }
    
    if (fr)
    {
      *typeOfObject=
#ifndef NSEEL_USE_OLD_PARSER
        fr->num_params>3?FUNCTIONX :
#endif       
        fr->num_params>=3?FUNCTION3 : fr->num_params==2?FUNCTION2 : FUNCTION1;
      
      if (!strncasecmp(tmp,"this.",5) && tmp[5]) // relative scoped call
      {
        // we're calling this. something, defer lookup of derived version to code generation
        ctx->function_usesThisPointer = 1;
        return nseel_createCompiledFunctionCallEELThis(ctx,fr,tmp+5);
      }
      
      if (postName && fr->usesThisPointer) // if has context and calling an eel function that needs context
      {
        _codeHandleFunctionRec *scan=fr;
        while (scan)
        {
          if (!strcasecmp(scan->fname,tmp)) break;
          scan=scan->derivedCopies;
        }
        
        // if didn't find a cached instance, create our fully qualified function instance
        if (!scan) scan = eel_createFunctionNamespacedInstance(ctx,fr,tmp);
        
        
        // use our derived/cached version if we didn't fail
        if (scan) fr=scan; 
      }
      
      return nseel_createCompiledFunctionCall(ctx,fr->num_params,FUNCTYPE_EELFUNC,(void *)fr);     
    }
    
  }
  
  // instance variables
  if (!strncasecmp(tmp,"this.",5) && tmp[5])
  {
    ctx->function_usesThisPointer=1;
    return nseel_createCompiledValueFromNamespaceName(ctx,tmp+5); 
  }
  
  {
    EEL_F *p=nseel_int_register_var(ctx,tmp,0);
    if (p) return nseel_createCompiledValuePtr(ctx,p); 
  }
  return nseel_createCompiledValue(ctx,0.0);
}




//------------------------------------------------------------------------------
opcodeRec *nseel_translate(compileContext *ctx, const char *tmp)
{
  if (tmp[0] == '0' && toupper(tmp[1])=='X')
  {
    char *p;
    return nseel_createCompiledValue(ctx,(EEL_F)strtoul(tmp+2,&p,16));
  }
  if (strstr(tmp,".")) return nseel_createCompiledValue(ctx,(EEL_F)atof(tmp));
  return nseel_createCompiledValue(ctx,(EEL_F)atoi(tmp)); // todo: this could be atof()  too, eventually, but that might break things
}

