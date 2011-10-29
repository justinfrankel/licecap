/*
  Expression Evaluator Library (NS-EEL) v2
  Copyright (C) 2004-2008 Cockos Incorporated
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


#include <string.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>

#ifndef _WIN64
  #ifndef __ppc__
    #include <float.h>
  #endif
#endif

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

#ifdef NSEEL_EEL1_COMPAT_MODE

#ifndef EEL_NO_CHANGE_FPFLAGS
#define EEL_NO_CHANGE_FPFLAGS
#endif

#endif

#ifndef _WIN64
#if !defined(_RC_CHOP) && !defined(EEL_NO_CHANGE_FPFLAGS)

#include <fpu_control.h>
#define _RC_CHOP _FPU_RC_ZERO
#define _MCW_RC _FPU_RC_ZERO
static unsigned int _controlfp(unsigned int val, unsigned int mask)
{
   unsigned int ret;
   _FPU_GETCW(ret);
   if (mask)
   {
     ret&=~mask;
     ret|=val;
     _FPU_SETCW(ret);
   }
   return ret;
}

#endif
#endif


#ifdef __ppc__

#define GLUE_MOV_EAX_DIRECTVALUE_SIZE 8
static void GLUE_MOV_EAX_DIRECTVALUE_GEN(void *b, INT_PTR v) 
{   
    	unsigned int uv=(unsigned int)v;
	unsigned short *p=(unsigned short *)b;

        *p++ = 0x3C60; // addis r3, r0, hw
	*p++ = (uv>>16)&0xffff;
        *p++ = 0x6063; // ori r3, r3, lw
	*p++ = uv&0xffff;
}


// mflr r5
// stwu r5, -4(r1)
const static unsigned int GLUE_FUNC_ENTER[2] = { 0x7CA802A6, 0x94A1FFFC };

// lwz r5, 0(r1)
// addi r1, r1, 4
// mtlr r5
const static unsigned int GLUE_FUNC_LEAVE[3] = { 0x80A10000, 0x38210004, 0x7CA803A6 };
#define GLUE_FUNC_ENTER_SIZE sizeof(GLUE_FUNC_ENTER)
#define GLUE_FUNC_LEAVE_SIZE sizeof(GLUE_FUNC_LEAVE)

const static unsigned int GLUE_RET[]={0x4E800020}; // blr

const static unsigned int GLUE_MOV_ESI_EDI=0x7E308B78; // mr r16, r17

static int GLUE_RESET_ESI(char *out, void *ptr)
{
  if (out) memcpy(out,&GLUE_MOV_ESI_EDI,sizeof(GLUE_MOV_ESI_EDI));
  return sizeof(GLUE_MOV_ESI_EDI);

}



// stwu r3, -4(r1)
const static unsigned int GLUE_PUSH_EAX[1]={ 0x9461FFFC};

// lwz r14, 0(r1)
// addi r1, r1, 4
const static unsigned int GLUE_POP_EBX[2]={ 0x81C10000, 0x38210004, };

// lwz r15, 0(r1)
// addi r1, r1, 4
const static unsigned int GLUE_POP_ECX[2]={ 0x81E10000, 0x38210004 };


static void GLUE_CALL_CODE(INT_PTR bp, INT_PTR cp) 
{
  __asm__(
          "stmw r14, -80(r1)\n"
          "mtctr %0\n"
          "mr r17, %1\n" 
	  "subi r17, r17, 8\n"
          "mflr r5\n" 
          "stw r5, -84(r1)\n"
          "subi r1, r1, 88\n"
          "bctrl\n"
          "addi r1, r1, 88\n"
          "lwz r5, -84(r1)\n"
          "lmw r14, -80(r1)\n"
          "mtlr r5\n"
            ::"r" (cp), "r" (bp));
};

INT_PTR *EEL_GLUE_set_immediate(void *_p, void *newv)
{
// todo 64 bit ppc will take some work
  unsigned int *p=(unsigned int *)_p;
  while ((p[0]&0x0000FFFF) != 0x0000dead && 
         (p[1]&0x0000FFFF) != 0x0000beef) p++;
  p[0] = (p[0]&0xFFFF0000) | (((unsigned int)newv)>>16);
  p[1] = (p[1]&0xFFFF0000) | (((unsigned int)newv)&0xFFFF);

  return (INT_PTR*)++p;
}


#else

//x86 specific code

#define GLUE_FUNC_ENTER_SIZE 0
#define GLUE_FUNC_LEAVE_SIZE 0
const static unsigned int GLUE_FUNC_ENTER[1];
const static unsigned int GLUE_FUNC_LEAVE[1];

#if defined(_WIN64) || defined(__LP64__)
#define GLUE_MOV_EAX_DIRECTVALUE_SIZE 10
static void GLUE_MOV_EAX_DIRECTVALUE_GEN(void *b, INT_PTR v) {   
  unsigned short *bb = (unsigned short *)b;
  *bb++ =0xB848; 
  *(INT_PTR *)bb = v; 
}
const static unsigned char  GLUE_PUSH_EAX[2]={	   0x50,0x50}; // push rax ; push rax (push twice to preserve alignment)
const static unsigned char  GLUE_POP_EBX[2]={0x5F, 0x5f}; //pop rdi ; twice
const static unsigned char  GLUE_POP_ECX[2]={0x59, 0x59 }; // pop rcx ; twice
#else
#define GLUE_MOV_EAX_DIRECTVALUE_SIZE 5
static void GLUE_MOV_EAX_DIRECTVALUE_GEN(void *b, int v) 
{   
  *((unsigned char *)b) =0xB8; 
  b= ((unsigned char *)b)+1;
  *(int *)b = v; 
}
const static unsigned char  GLUE_PUSH_EAX[4]={0x83, 0xEC, 12,   0x50}; // sub esp, 12, push eax
const static unsigned char  GLUE_POP_EBX[4]={0x5F, 0x83, 0xC4, 12}; //pop ebx, add esp, 12 // DI=5F, BX=0x5B;
const static unsigned char  GLUE_POP_ECX[4]={0x59, 0x83, 0xC4, 12}; // pop ecx, add esp, 12

#endif

//const static unsigned short GLUE_MOV_ESI_EDI=0xF78B;
const static unsigned char  GLUE_RET=0xC3;

static int GLUE_RESET_ESI(unsigned char *out, void *ptr)
{
#if defined(_WIN64) || defined(__LP64__)
  if (out)
  {
	  *out++ = 0x48;
    *out++ = 0xBE; // mov rsi, constant64
  	*(void **)out = ptr;
    out+=sizeof(void *);
  }
  return 2+sizeof(void *);
#else
  if (out)
  {
    *out++ = 0xBE; // mov esi, constant
    memcpy(out,&ptr,sizeof(void *));
    out+=sizeof(void *);
  }
  return 1+sizeof(void *);
#endif
}


static void GLUE_CALL_CODE(INT_PTR bp, INT_PTR cp) 
{
  #if defined(_WIN64) || defined(__LP64__)
    extern void win64_callcode(INT_PTR code);
    win64_callcode(cp);
  #else // non-64 bit
    #ifdef _MSC_VER
      #ifndef EEL_NO_CHANGE_FPFLAGS
        unsigned int old_v=_controlfp(0,0); 
        _controlfp(_RC_CHOP,_MCW_RC);
      #endif

      __asm
      {
        mov eax, cp
        pushad 
        call eax
        popad
      };

      #ifndef EEL_NO_CHANGE_FPFLAGS
        _controlfp(old_v,_MCW_RC);
      #endif
    #else // gcc x86
      #ifndef EEL_NO_CHANGE_FPFLAGS
        unsigned int old_v=_controlfp(0,0); 
        _controlfp(_RC_CHOP,_MCW_RC);
      #endif
      __asm__("call *%%eax"::"a" (cp): "%ecx","%edx","%esi","%edi");
      #ifndef EEL_NO_CHANGE_FPFLAGS
        _controlfp(old_v,_MCW_RC);
      #endif
    #endif //gcc x86
  #endif // 32bit
}

INT_PTR *EEL_GLUE_set_immediate(void *_p, void *newv)
{
  char *p=(char*)_p;
  while (*(INT_PTR *)p != ~(INT_PTR)0) p++;
  *(INT_PTR *)p = (INT_PTR)newv;
  return ((INT_PTR*)p)+1;
}

#endif


static void *GLUE_realAddress(void *fn, void *fn_e, int *size)
{
#if defined(_MSC_VER) || defined(__LP64__)

  unsigned char *p;

#if defined(_DEBUG) && !defined(__LP64__)
  if (*(unsigned char *)fn == 0xE9) // this means jump to the following address
  {
    fn = ((unsigned char *)fn) + *(int *)((char *)fn+1) + 5;
  }
#endif

  // this may not work in debug mode?
  p=(unsigned char *)fn;
  for (;;)
  {
    int a;
    for (a=0;a<12;a++)
    {
      if (p[a] != (a?0x90:0x89)) break;
    }
    if (a>=12)
    {
      *size = (char *)p - (char *)fn;
    //  if (*size<0) MessageBox(NULL,"expect poof","a",0);
      return fn;
    }
    p++;
  }
#else

  char *p=(char *)fn_e - sizeof(GLUE_RET);
  if (p <= (char *)fn) *size=0;
  else
  {
    while (p > (char *)fn && memcmp(p,&GLUE_RET,sizeof(GLUE_RET))) p-=sizeof(GLUE_RET);
    *size = p - (char *)fn;
  }
  return fn;

#endif
}



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



#define LLB_DSIZE (65536-64)
typedef struct _llBlock {
	struct _llBlock *next;
  int sizeused;
	char block[LLB_DSIZE];
} llBlock;

typedef struct _startPtr {
  struct _startPtr *next;
  void *startptr;
} startPtr;

typedef struct {
  void *workTable;

  llBlock *blocks;
  void *code;
  int code_stats[4];
} codeHandleType;

#ifndef NSEEL_MAX_TEMPSPACE_ENTRIES
#define NSEEL_MAX_TEMPSPACE_ENTRIES 2048
#endif

static void *__newBlock(llBlock **start,int size);

#define newTmpBlock(x) __newTmpBlock((llBlock **)&ctx->tmpblocks_head,x)
#define newBlock(x,a) __newBlock_align(ctx,x,a)

static void *__newTmpBlock(llBlock **start, int size)
{
  void *p=__newBlock(start,size+4);
  if (p && size>=0) *((int *)p) = size;
  return p;
}

static void *__newBlock_align(compileContext *ctx, int size, int align) // makes sure block is aligned to 32 byte boundary, for code
{
  int a1=align-1;
  char *p=(char*)__newBlock((llBlock **)&ctx->blocks_head,size+a1);
  return p+((align-(((INT_PTR)p)&a1))&a1);
}

static void freeBlocks(llBlock **start);

#define DECL_ASMFUNC(x)         \
  void nseel_asm_##x(void);        \
  void nseel_asm_##x##_end(void);    \

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
  DECL_ASMFUNC(sig)
  DECL_ASMFUNC(sign)
  DECL_ASMFUNC(band)
  DECL_ASMFUNC(bor)
  DECL_ASMFUNC(bnot)
  DECL_ASMFUNC(if)
  DECL_ASMFUNC(repeat)
  DECL_ASMFUNC(repeatwhile)
  DECL_ASMFUNC(equal)
  DECL_ASMFUNC(notequal)
  DECL_ASMFUNC(below)
  DECL_ASMFUNC(above)
  DECL_ASMFUNC(beloweq)
  DECL_ASMFUNC(aboveeq)
  DECL_ASMFUNC(assign)
  DECL_ASMFUNC(add)
  DECL_ASMFUNC(sub)
  DECL_ASMFUNC(add_op)
  DECL_ASMFUNC(sub_op)
  DECL_ASMFUNC(mul)
  DECL_ASMFUNC(div)
  DECL_ASMFUNC(mul_op)
  DECL_ASMFUNC(div_op)
  DECL_ASMFUNC(mod)
  DECL_ASMFUNC(shl)
  DECL_ASMFUNC(shr)
  DECL_ASMFUNC(mod_op)
  DECL_ASMFUNC(or)
  DECL_ASMFUNC(and)
  DECL_ASMFUNC(or_op)
  DECL_ASMFUNC(and_op)
  DECL_ASMFUNC(uplus)
  DECL_ASMFUNC(uminus)
  DECL_ASMFUNC(invsqrt)
  DECL_ASMFUNC(exec2)

static void NSEEL_PProc_GRAM(void *data, int data_size, compileContext *ctx)
{
  if (data_size>0) EEL_GLUE_set_immediate(data, ctx->gram_blocks);
}


static EEL_F g_signs[2]={1.0,-1.0};
static EEL_F negativezeropointfive=-0.5f;
static EEL_F onepointfive=1.5f;
static EEL_F g_closefact = NSEEL_CLOSEFACTOR;
static const EEL_F eel_zero=0.0, eel_one=1.0;

#if defined(_MSC_VER) && _MSC_VER >= 1400
static double __floor(double a) { return floor(a); }
#endif


#ifdef NSEEL_EEL1_COMPAT_MODE
static double eel1band(double a, double b)
{
  return (fabs(a)>g_closefact && fabs(b) > g_closefact) ? 1.0 : 0.0;
}
static double eel1bor(double a, double b)
{
  return (fabs(a)>g_closefact || fabs(b) > g_closefact) ? 1.0 : 0.0;
}

static double eel1sigmoid(double x, double constraint)
{
  double t = (1+exp(-x * (constraint)));
  return fabs(t)>g_closefact ? 1.0/t : 0;
}

#endif

EEL_F NSEEL_CGEN_CALL nseel_int_rand(EEL_F *f);

static functionType fnTable1[] = {
  { "_if",     nseel_asm_if,nseel_asm_if_end,    3,  {&g_closefact} },
  { "_and",   nseel_asm_band,nseel_asm_band_end,  2 } ,
  { "_or",    nseel_asm_bor,nseel_asm_bor_end,   2 } ,
  { "loop", nseel_asm_repeat,nseel_asm_repeat_end, 2 },
  { "while", nseel_asm_repeatwhile,nseel_asm_repeatwhile_end, 1 },

#ifdef __ppc__
  { "_not",   nseel_asm_bnot,nseel_asm_bnot_end,  1, {&g_closefact,&eel_zero,&eel_one} } ,
  { "_equal",  nseel_asm_equal,nseel_asm_equal_end, 2, {&g_closefact,&eel_zero, &eel_one} },
  { "_noteq",  nseel_asm_notequal,nseel_asm_notequal_end, 2, {&g_closefact,&eel_one,&eel_zero} },
  { "_below",  nseel_asm_below,nseel_asm_below_end, 2, {&eel_zero, &eel_one} },
  { "_above",  nseel_asm_above,nseel_asm_above_end, 2, {&eel_zero, &eel_one}  },
  { "_beleq",  nseel_asm_beloweq,nseel_asm_beloweq_end, 2, {&eel_zero, &eel_one}  },
  { "_aboeq",  nseel_asm_aboveeq,nseel_asm_aboveeq_end, 2, {&eel_zero, &eel_one} },
#else
  { "_not",   nseel_asm_bnot,nseel_asm_bnot_end,  1, {&g_closefact} } ,
  { "_equal",  nseel_asm_equal,nseel_asm_equal_end, 2, {&g_closefact} },
  { "_noteq",  nseel_asm_notequal,nseel_asm_notequal_end, 2, {&g_closefact} },
  { "_below",  nseel_asm_below,nseel_asm_below_end, 2 },
  { "_above",  nseel_asm_above,nseel_asm_above_end, 2 },
  { "_beleq",  nseel_asm_beloweq,nseel_asm_beloweq_end, 2 },
  { "_aboeq",  nseel_asm_aboveeq,nseel_asm_aboveeq_end, 2 },
#endif

  { "_set",nseel_asm_assign,nseel_asm_assign_end,2},
  { "_mod",nseel_asm_mod,nseel_asm_mod_end,2},
  { "_shr",nseel_asm_shr,nseel_asm_shr_end,2},
  { "_shl",nseel_asm_shl,nseel_asm_shl_end,2},
  { "_mulop",nseel_asm_mul_op,nseel_asm_mul_op_end,2},
  { "_divop",nseel_asm_div_op,nseel_asm_div_op_end,2},
  { "_orop",nseel_asm_or_op,nseel_asm_or_op_end,2},
  { "_andop",nseel_asm_and_op,nseel_asm_and_op_end,2},
  { "_addop",nseel_asm_add_op,nseel_asm_add_op_end,2},
  { "_subop",nseel_asm_sub_op,nseel_asm_sub_op_end,2},
  { "_modop",nseel_asm_mod_op,nseel_asm_mod_op_end,2},


#ifdef __ppc__
   { "sin",   nseel_asm_1pdd,nseel_asm_1pdd_end,   1, {&sin} },
   { "cos",    nseel_asm_1pdd,nseel_asm_1pdd_end,   1, {&cos} },
   { "tan",    nseel_asm_1pdd,nseel_asm_1pdd_end,   1, {&tan}  },
#else
   { "sin",   nseel_asm_sin,nseel_asm_sin_end,   1 },
   { "cos",    nseel_asm_cos,nseel_asm_cos_end,   1 },
   { "tan",    nseel_asm_tan,nseel_asm_tan_end,   1 },
#endif
   { "asin",   nseel_asm_1pdd,nseel_asm_1pdd_end,  1, {&asin}, },
   { "acos",   nseel_asm_1pdd,nseel_asm_1pdd_end,  1, {&acos}, },
   { "atan",   nseel_asm_1pdd,nseel_asm_1pdd_end,  1, {&atan}, },
   { "atan2",  nseel_asm_2pdd,nseel_asm_2pdd_end, 2, {&atan2}, },
   { "sqr",    nseel_asm_sqr,nseel_asm_sqr_end,   1 },
#ifdef __ppc__
   { "sqrt",   nseel_asm_1pdd,nseel_asm_1pdd_end,  1, {&sqrt}, },
#else
   { "sqrt",   nseel_asm_sqrt,nseel_asm_sqrt_end,  1 },
#endif
   { "pow",    nseel_asm_2pdd,nseel_asm_2pdd_end,   2, {&pow}, },
   { "_powop",    nseel_asm_2pdds,nseel_asm_2pdds_end,   2, {&pow}, },
   { "exp",    nseel_asm_1pdd,nseel_asm_1pdd_end,   1, {&exp}, },
#ifdef __ppc__
   { "log",    nseel_asm_1pdd,nseel_asm_1pdd_end,   1, {&log} },
   { "log10",  nseel_asm_1pdd,nseel_asm_1pdd_end, 1, {&log10} },
#else
   { "log",    nseel_asm_log,nseel_asm_log_end,   1, },
   { "log10",  nseel_asm_log10,nseel_asm_log10_end, 1, },
#endif
   { "abs",    nseel_asm_abs,nseel_asm_abs_end,   1 },
   { "min",    nseel_asm_min,nseel_asm_min_end,   2 },
   { "max",    nseel_asm_max,nseel_asm_max_end,   2 },
#ifdef __ppc__
   { "sign",   nseel_asm_sign,nseel_asm_sign_end,  1, {&eel_zero}} ,
#else
   { "sign",   nseel_asm_sign,nseel_asm_sign_end,  1, {&g_signs}} ,
#endif
	 { "rand",   nseel_asm_1pp,nseel_asm_1pp_end,  1, {&nseel_int_rand}, } ,

#if defined(_MSC_VER) && _MSC_VER >= 1400
   { "floor",  nseel_asm_1pdd,nseel_asm_1pdd_end, 1, {&__floor} },
#else
   { "floor",  nseel_asm_1pdd,nseel_asm_1pdd_end, 1, {&floor} },
#endif
   { "ceil",   nseel_asm_1pdd,nseel_asm_1pdd_end,  1, {&ceil} },
#ifdef __ppc__
   { "invsqrt",   nseel_asm_invsqrt,nseel_asm_invsqrt_end,  1,  },
#else
   { "invsqrt",   nseel_asm_invsqrt,nseel_asm_invsqrt_end,  1, {&negativezeropointfive, &onepointfive} },
#endif


#ifdef NSEEL_EEL1_COMPAT_MODE
  { "sigmoid", nseel_asm_2pdd,nseel_asm_2pdd_end, 2, {&eel1sigmoid}, },

  // these differ from _and/_or, they always evaluate both...
  { "band",  nseel_asm_2pdd,nseel_asm_2pdd_end, 2, {&eel1band}, },
  { "bor",  nseel_asm_2pdd,nseel_asm_2pdd_end, 2, {&eel1bor}, },

  {"exec2",nseel_asm_exec2,nseel_asm_exec2_end,2},
  {"exec3",nseel_asm_exec2,nseel_asm_exec2_end,3},


#endif // end EEL1 compat



  {"_mem",_asm_megabuf,_asm_megabuf_end,1,{&g_closefact,&__NSEEL_RAMAlloc},NSEEL_PProc_RAM},
  {"_gmem",_asm_megabuf,_asm_megabuf_end,1,{&g_closefact,&__NSEEL_RAMAllocGMEM},NSEEL_PProc_GRAM},
  {"freembuf",_asm_generic1parm,_asm_generic1parm_end,1,{&__NSEEL_RAM_MemFree},NSEEL_PProc_RAM},
  {"memcpy",_asm_generic3parm,_asm_generic3parm_end,3,{&__NSEEL_RAM_MemCpy},NSEEL_PProc_RAM},
  {"memset",_asm_generic3parm,_asm_generic3parm_end,3,{&__NSEEL_RAM_MemSet},NSEEL_PProc_RAM},



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
  NSEEL_quit();
  return 0;
}

void NSEEL_addfunctionex2(const char *name, int nparms, char *code_startaddr, int code_len, void *pproc, void *fptr, void *fptr2)
{
  if (!fnTableUser || !(fnTableUser_size&7))
  {
    fnTableUser=(functionType *)realloc(fnTableUser,(fnTableUser_size+8)*sizeof(functionType));
  }
  if (fnTableUser)
  {
    memset(&fnTableUser[fnTableUser_size],0,sizeof(functionType));
    fnTableUser[fnTableUser_size].nParams = nparms;
    fnTableUser[fnTableUser_size].name = name;
    fnTableUser[fnTableUser_size].afunc = code_startaddr;
    fnTableUser[fnTableUser_size].func_e = code_startaddr + code_len;
    fnTableUser[fnTableUser_size].pProc = (NSEEL_PPPROC) pproc;
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
static void *__newBlock(llBlock **start, int size)
{
#ifdef _WIN32
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
  llb->sizeused=(size+7)&~7;
  llb->next = *start;  
  *start = llb;
  return llb->block;
}


//---------------------------------------------------------------------------------------------------------------
INT_PTR nseel_createCompiledValue(compileContext *ctx, EEL_F value, EEL_F *addrValue)
{
  unsigned char *block;

  block=(unsigned char *)newTmpBlock(GLUE_MOV_EAX_DIRECTVALUE_SIZE);

  if (addrValue == NULL)
  {
    *(addrValue = (EEL_F *)newBlock(sizeof(EEL_F),sizeof(EEL_F))) = value;
    ctx->l_stats[3]+=sizeof(EEL_F);
  }

  GLUE_MOV_EAX_DIRECTVALUE_GEN(block+4,(INT_PTR)addrValue);

  return ((INT_PTR)(block));

}

//---------------------------------------------------------------------------------------------------------------
static void *nseel_getFunctionAddress(int fntype, int fn, int *size, NSEEL_PPPROC *pProc, void ***replList)
{
  *replList=0;
  switch (fntype)
	{
  	case MATH_SIMPLE:
	  	switch (fn)
			{
			  case FN_ASSIGN:
				  return GLUE_realAddress(nseel_asm_assign,nseel_asm_assign_end,size);
			  case FN_ADD:
				  return GLUE_realAddress(nseel_asm_add,nseel_asm_add_end,size);
			  case FN_SUB:
				  return GLUE_realAddress(nseel_asm_sub,nseel_asm_sub_end,size);
			  case FN_MULTIPLY:
				  return GLUE_realAddress(nseel_asm_mul,nseel_asm_mul_end,size);
			  case FN_DIVIDE:
				  return GLUE_realAddress(nseel_asm_div,nseel_asm_div_end,size);
			  case FN_MODULO:
				  return GLUE_realAddress(nseel_asm_exec2,nseel_asm_exec2_end,size);
			  case FN_AND:
				  return GLUE_realAddress(nseel_asm_and,nseel_asm_and_end,size);
			  case FN_OR:
				  return GLUE_realAddress(nseel_asm_or,nseel_asm_or_end,size);
			  case FN_UPLUS:
				  return GLUE_realAddress(nseel_asm_uplus,nseel_asm_uplus_end,size);
			  case FN_UMINUS:
				  return GLUE_realAddress(nseel_asm_uminus,nseel_asm_uminus_end,size);
			}
	  case MATH_FN:
      {
        functionType *p=nseel_getFunctionFromTable(fn);
		    if (p) 
        {
          *replList=p->replptrs;
          *pProc=p->pProc;
          return GLUE_realAddress(p->afunc,p->func_e,size);
        }
      }
	}

  *size=0;
  return 0;
}


//---------------------------------------------------------------------------------------------------------------
INT_PTR nseel_createCompiledFunction3(compileContext *ctx, int fntype, INT_PTR fn, INT_PTR code1, INT_PTR code2, INT_PTR code3)
{
  int sizes1=((int *)code1)[0];
  int sizes2=((int *)code2)[0];
  int sizes3=((int *)code3)[0];

  if (fntype == MATH_FN && fn == 0) // special case: IF
  {
    void *func3;
    int size;
    INT_PTR *ptr;
    char *block;

    unsigned char *newblock2,*newblock3;
    unsigned char *p;

    p=newblock2=newBlock(sizes2+sizeof(GLUE_RET)+GLUE_FUNC_ENTER_SIZE+GLUE_FUNC_LEAVE_SIZE,32);
    memcpy(p,&GLUE_FUNC_ENTER,GLUE_FUNC_ENTER_SIZE); p+=GLUE_FUNC_ENTER_SIZE;
    memcpy(p,(char*)code2+4,sizes2); p+=sizes2;
    memcpy(p,&GLUE_FUNC_LEAVE,GLUE_FUNC_LEAVE_SIZE); p+=GLUE_FUNC_LEAVE_SIZE;
    memcpy(p,&GLUE_RET,sizeof(GLUE_RET)); p+=sizeof(GLUE_RET);

    p=newblock3=newBlock(sizes3+sizeof(GLUE_RET)+GLUE_FUNC_ENTER_SIZE+GLUE_FUNC_LEAVE_SIZE,32);
    memcpy(p,&GLUE_FUNC_ENTER,GLUE_FUNC_ENTER_SIZE); p+=GLUE_FUNC_ENTER_SIZE;
    memcpy(p,(char*)code3+4,sizes3); p+=sizes3;
    memcpy(p,&GLUE_FUNC_LEAVE,GLUE_FUNC_LEAVE_SIZE); p+=GLUE_FUNC_LEAVE_SIZE;
    memcpy(p,&GLUE_RET,sizeof(GLUE_RET)); p+=sizeof(GLUE_RET);

    ctx->l_stats[2]+=sizes2+sizes3+sizeof(GLUE_RET)*2;
    
    func3 = GLUE_realAddress(nseel_asm_if,nseel_asm_if_end,&size);

    block=(char *)newTmpBlock(sizes1+size);

    memcpy(block+4,(char*)code1+4,sizes1);
    ptr=(INT_PTR *)(block+4+sizes1);
    memcpy(ptr,func3,size);

    ptr=EEL_GLUE_set_immediate(ptr,&g_closefact);
    ptr=EEL_GLUE_set_immediate(ptr,newblock2);
    EEL_GLUE_set_immediate(ptr,newblock3);

    ctx->computTableTop++;

    return (INT_PTR)block;

  }
  else
  {
    int size2;
    unsigned char *block;
    unsigned char *outp;

    void *myfunc;
    NSEEL_PPPROC preProc=0;
    void **repl;
  
    myfunc = nseel_getFunctionAddress(fntype, fn, &size2, &preProc,&repl);

    block=(unsigned char *)newTmpBlock(size2+sizes1+sizes2+sizes3+
      sizeof(GLUE_PUSH_EAX) + 
      sizeof(GLUE_PUSH_EAX) +
      sizeof(GLUE_POP_EBX) + 
      sizeof(GLUE_POP_ECX));

    outp=block+4;
    memcpy(outp,(char*)code1+4,sizes1); 
    outp+=sizes1;
    memcpy(outp,&GLUE_PUSH_EAX,sizeof(GLUE_PUSH_EAX)); outp+=sizeof(GLUE_PUSH_EAX);
    memcpy(outp,(char*)code2+4,sizes2); 
    outp+=sizes2;
    memcpy(outp,&GLUE_PUSH_EAX,sizeof(GLUE_PUSH_EAX)); outp+=sizeof(GLUE_PUSH_EAX);
    memcpy(outp,(char*)code3+4,sizes3); 
    outp+=sizes3;
    memcpy(outp,&GLUE_POP_EBX,sizeof(GLUE_POP_EBX)); outp+=sizeof(GLUE_POP_EBX);
    memcpy(outp,&GLUE_POP_ECX,sizeof(GLUE_POP_ECX)); outp+=sizeof(GLUE_POP_ECX);

    memcpy(outp,myfunc,size2);
    if (preProc) preProc(outp,size2,ctx);
    if (repl)
    {
      if (repl[0]) outp=(unsigned char *)EEL_GLUE_set_immediate(outp,repl[0]);
      if (repl[1]) outp=(unsigned char *)EEL_GLUE_set_immediate(outp,repl[1]);
      if (repl[2]) outp=(unsigned char *)EEL_GLUE_set_immediate(outp,repl[2]);
      if (repl[3]) outp=(unsigned char *)EEL_GLUE_set_immediate(outp,repl[3]);
    }

    ctx->computTableTop++;

    return ((INT_PTR)(block));  
  }
}

//---------------------------------------------------------------------------------------------------------------
INT_PTR nseel_createCompiledFunction2(compileContext *ctx, int fntype, INT_PTR fn, INT_PTR code1, INT_PTR code2)
{
  int size2;
  unsigned char *outp;
  void *myfunc;
  int sizes1=((int *)code1)[0];
  int sizes2=((int *)code2)[0];
  if (fntype == MATH_FN && (fn == 1 || fn == 2 || fn == 3)) // special case: LOOP/BOR/BAND
  {
    void *func3;
    int size;
    INT_PTR *ptr;
    char *block;

    unsigned char *newblock2, *p;

    p=newblock2=newBlock(sizes2+sizeof(GLUE_RET)+GLUE_FUNC_ENTER_SIZE+GLUE_FUNC_LEAVE_SIZE,32);
    memcpy(p,&GLUE_FUNC_ENTER,GLUE_FUNC_ENTER_SIZE); p+=GLUE_FUNC_ENTER_SIZE;
    memcpy(p,(char*)code2+4,sizes2); p+=sizes2;
    memcpy(p,&GLUE_FUNC_LEAVE,GLUE_FUNC_LEAVE_SIZE); p+=GLUE_FUNC_LEAVE_SIZE;
    memcpy(p,&GLUE_RET,sizeof(GLUE_RET)); p+=sizeof(GLUE_RET);

    ctx->l_stats[2]+=sizes2+2;
    
    if (fn == 1) func3 = GLUE_realAddress(nseel_asm_band,nseel_asm_band_end,&size);
    else if (fn == 3) func3 = GLUE_realAddress(nseel_asm_repeat,nseel_asm_repeat_end,&size);
    else func3 = GLUE_realAddress(nseel_asm_bor,nseel_asm_bor_end,&size);

    block=(char *)newTmpBlock(sizes1+size);
    memcpy(block+4,(char*)code1+4,sizes1);
    ptr=(INT_PTR *)(block+4+sizes1);
    memcpy(ptr,func3,size);

    if (fn!=3) ptr=EEL_GLUE_set_immediate(ptr,&g_closefact); // for or/and
    ptr=EEL_GLUE_set_immediate(ptr,newblock2);
    if (fn!=3) ptr=EEL_GLUE_set_immediate(ptr,&g_closefact); // for or/and
#ifdef __ppc__
    if (fn!=3) // for or/and on ppc we need a one
    {
      ptr=EEL_GLUE_set_immediate(ptr,&eel_one);
    }
#endif

    ctx->computTableTop++;
    return (INT_PTR)block;
  }

  {
    NSEEL_PPPROC preProc=0;
    unsigned char *block;
    void **repl;
    myfunc = nseel_getFunctionAddress(fntype, fn, &size2,&preProc,&repl);

    block=(unsigned char *)newTmpBlock(size2+sizes1+sizes2+sizeof(GLUE_PUSH_EAX)+sizeof(GLUE_POP_EBX));

    outp=block+4;
    memcpy(outp,(char*)code1+4,sizes1); 
    outp+=sizes1;
    memcpy(outp,&GLUE_PUSH_EAX,sizeof(GLUE_PUSH_EAX)); outp+=sizeof(GLUE_PUSH_EAX);
    memcpy(outp,(char*)code2+4,sizes2); 
    outp+=sizes2;
    memcpy(outp,&GLUE_POP_EBX,sizeof(GLUE_POP_EBX)); outp+=sizeof(GLUE_POP_EBX);

    memcpy(outp,myfunc,size2);
    if (preProc) preProc(outp,size2,ctx);
    if (repl)
    {
      if (repl[0]) outp=(unsigned char *)EEL_GLUE_set_immediate(outp,repl[0]);
      if (repl[1]) outp=(unsigned char *)EEL_GLUE_set_immediate(outp,repl[1]);
      if (repl[2]) outp=(unsigned char *)EEL_GLUE_set_immediate(outp,repl[2]);
      if (repl[3]) outp=(unsigned char *)EEL_GLUE_set_immediate(outp,repl[3]);
    }

    ctx->computTableTop++;

    return ((INT_PTR)(block));
  }
}


//---------------------------------------------------------------------------------------------------------------
INT_PTR nseel_createCompiledFunction1(compileContext *ctx, int fntype, INT_PTR fn, INT_PTR code)
{
  NSEEL_PPPROC preProc=0;
  int size,size2;
  char *block;
  void *myfunc;
  void *func1;
  
  size =((int *)code)[0];
  func1 = (void *)(code+4);

 
  if (fntype == MATH_FN && fn == 4) // special case: while
  {
    void *func3;
    INT_PTR *ptr;

    unsigned char *newblock2, *p;

    p=newblock2=newBlock(size+sizeof(GLUE_RET)+GLUE_FUNC_ENTER_SIZE+GLUE_FUNC_LEAVE_SIZE,32);
    memcpy(p,&GLUE_FUNC_ENTER,GLUE_FUNC_ENTER_SIZE); p+=GLUE_FUNC_ENTER_SIZE;
    memcpy(p,func1,size); p+=size;
    memcpy(p,&GLUE_FUNC_LEAVE,GLUE_FUNC_LEAVE_SIZE); p+=GLUE_FUNC_LEAVE_SIZE;
    memcpy(p,&GLUE_RET,sizeof(GLUE_RET)); p+=sizeof(GLUE_RET);

    ctx->l_stats[2]+=size+2;
    
    func3 = GLUE_realAddress(nseel_asm_repeatwhile,nseel_asm_repeatwhile_end,&size);

    block=(char *)newTmpBlock(size);
  	ptr = (INT_PTR *)(block+4);
    memcpy(ptr,func3,size);
    ptr=EEL_GLUE_set_immediate(ptr,newblock2); 
    EEL_GLUE_set_immediate(ptr,&g_closefact); 

    ctx->computTableTop++;
    return (INT_PTR)block;
  }

  {
    void **repl;
	  myfunc = nseel_getFunctionAddress(fntype, fn, &size2,&preProc,&repl);

	  block=(char *)newTmpBlock(size+size2);

	  memcpy(block+4, func1, size);
	  memcpy(block+size+4,myfunc,size2);
	  if (preProc) preProc(block+size+4,size2,ctx);
    if (repl)
    {
      unsigned char *outp=(unsigned char *)block+size+4;
      if (repl[0]) outp=(unsigned char *)EEL_GLUE_set_immediate(outp,repl[0]);
      if (repl[1]) outp=(unsigned char *)EEL_GLUE_set_immediate(outp,repl[1]);
      if (repl[2]) outp=(unsigned char *)EEL_GLUE_set_immediate(outp,repl[2]);
      if (repl[3]) outp=(unsigned char *)EEL_GLUE_set_immediate(outp,repl[3]);
    }

	  ctx->computTableTop++;

	  return ((INT_PTR)(block));
  }
}


static char *preprocessCode(compileContext *ctx, char *expression)
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
        while (expression[0] && expression[0] != '\n') expression++;
	continue;
      }
      else if (expression[1] == '*')
      {
        expression+=2;
        while (expression[0] && (expression[0] != '*' || expression[1] != '/')) 
	{
		if (expression[0] == '\n') onCompileNewLine(ctx,expression+1-expression_start,len);
		expression++;
	}
        if (expression[0]) expression+=2; // at this point we KNOW expression[0]=* and expression[1]=/
	continue;
      }
    }
    
    if (expression[0] == '$')
    {
      if (toupper(expression[1]) == 'X')
      {
        char *p=expression+2;
        unsigned int v=strtoul(expression+2,&p,16);
        char tmp[64];
        expression=p;

        sprintf(tmp,"%u",v);
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

      if (c == '\n') onCompileNewLine(ctx,expression-expression_start,len);
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
				|| nc == '(' || nc == '_' || nc == '!' || nc == '$' 
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
				"", // or any control char that is not parenthed
				":(,;?%", 
				",):?;", // or || or &&
				",);", // jf> removed :? from this, for =
				",);",
        "",  // only scans for a negative ] level

			};


			static struct 
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

				{{'/','='}, 0, 3, "_divop"},
				{{'*','='}, 0, 3, "_mulop"},
				{{'^','='}, 0, 3, "_powop"},

				{{'=','='}, 1, 2, "_equal" },
				{{'<','='}, 1, 2, "_beleq" },
				{{'>','='}, 1, 2, "_aboeq" },
				{{'<','<'}, 0, 0, "_shl" },
				{{'>','>'}, 0, 0, "_shr" },
				{{'<',0  }, 1, 2, "_below" },
				{{'>',0  }, 1, 2, "_above" },
				{{'!','='}, 1, 2, "_noteq" },
				{{'|','|'}, 1, 2, "_or" },
				{{'&','&'}, 1, 2, "_and" },
				{{'=',0  }, 0, 3, "_set" },
				{{'%',0},   0, 0, "_mod" },
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
					// scan forward to an uncommented,  unparenthed semicolon, comma, or )
					int r_semicnt=0;
					int r_qcnt=0;
					char *scan=symbollists[rscan];
					int commentstate=0;
					int hashadch=0;
          int bracketcnt=0;
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
							if (*r_ptr == '(') {hashadch=1; r_semicnt++; }
							else if (*r_ptr == ')') 
							{
								r_semicnt--;
								if (r_semicnt < 0) break;
							}
							else if (!r_semicnt)
							{
								char *sc=scan;
								if (*r_ptr == ';' || *r_ptr == ',') break;
							
								if (!rscan)
								{
									if (*r_ptr == ':') break;
									if (!isspace(*r_ptr) && !isalnum(*r_ptr) && *r_ptr != '_' && *r_ptr != '.' && hashadch) break;
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
                else if (rscan == 5)
                {
                  if (*r_ptr == '[') bracketcnt++;
                  else if (*r_ptr == ']') bracketcnt--;
                  if (bracketcnt < 0) break;
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

						r_ptr=preprocessCode(ctx,expression);
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
						len+=sprintf(buf+len,"_not(%s",
							r_ptr);

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
NSEEL_CODEHANDLE NSEEL_code_compile(NSEEL_VMCTX _ctx, char *_expression, int lineoffs)
{
  compileContext *ctx = (compileContext *)_ctx;
  char *expression,*expression_start;
  int computable_size=0;
  codeHandleType *handle;
  startPtr *scode=NULL;
  startPtr *startpts=NULL;

  if (!ctx) return 0;

  ctx->last_error_string[0]=0;

  if (!_expression || !*_expression) return 0;

  freeBlocks((llBlock **)&ctx->tmpblocks_head);  // free blocks
  freeBlocks((llBlock **)&ctx->blocks_head);  // free blocks
  memset(ctx->l_stats,0,sizeof(ctx->l_stats));
  free(ctx->compileLineRecs); ctx->compileLineRecs=0; ctx->compileLineRecs_size=0; ctx->compileLineRecs_alloc=0;

  handle = (codeHandleType*)newBlock(sizeof(codeHandleType),8);

  if (!handle) 
  {
    return 0;
  }
  
  memset(handle,0,sizeof(codeHandleType));

  expression_start=expression=preprocessCode(ctx,_expression);


  while (*expression)
  {
    void *startptr;
    char *expr;

    ctx->colCount=0;
    ctx->computTableTop=0;

    
    // single out segment
    while (*expression == ';' || isspace(*expression)) expression++;
    if (!*expression) break;
    expr=expression;

    while (*expression && *expression != ';') expression++;
    if (*expression) *expression++ = 0;

    // parse
    
    startptr=nseel_compileExpression(ctx,expr);

    if (ctx->computTableTop > NSEEL_MAX_TEMPSPACE_ENTRIES- /* safety */ 16 - /* alignment */4 ||
        !startptr) 
    { 
      int byteoffs = expr - expression_start;
      int destoffs,linenumber;
      char buf[21], *p;
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
      for (x = 0;x < 20; x ++)
      {
	      if (!*p || *p == '\r' || *p == '\n') break;
	      buf[x]=*p++;
      }
      buf[x]=0;

      sprintf(ctx->last_error_string,"Around line %d '%s'",linenumber+lineoffs,buf);

      ctx->last_error_string[sizeof(ctx->last_error_string)-1]=0;
      scode=NULL; 
      break; 
    }
    if (computable_size < ctx->computTableTop)
    {
      computable_size=ctx->computTableTop;
    }

    {
      startPtr *tmp=(startPtr*) __newBlock((llBlock **)&ctx->tmpblocks_head,sizeof(startPtr));
      if (!tmp) break;

      tmp->startptr = startptr;
      tmp->next=NULL;
      if (!scode) scode=startpts=tmp;
      else
      {
        scode->next=tmp;
        scode=tmp;
      }
    }
  }
  free(ctx->compileLineRecs); ctx->compileLineRecs=0; ctx->compileLineRecs_size=0; ctx->compileLineRecs_alloc=0;

  // check to see if failed on the first startingCode
  if (!scode)
  {
    handle=NULL;              // return NULL (after resetting blocks_head)
  }
  else 
  {
    char *tabptr = (char *)(handle->workTable=calloc(computable_size+64,  sizeof(EEL_F)));
    unsigned char *writeptr;
    startPtr *p=startpts;
    int size=sizeof(GLUE_RET)+GLUE_FUNC_ENTER_SIZE+GLUE_FUNC_LEAVE_SIZE; // for ret at end :)

    if (((INT_PTR)tabptr)&31)
      tabptr += 32-(((INT_PTR)tabptr)&31);

    // now we build one big code segment out of our list of them, inserting a mov esi, computable before each item
    while (p)
    {
      size += GLUE_RESET_ESI(NULL,0);
      size+=*(int *)p->startptr;
      p=p->next;
    }
    handle->code = newBlock(size,32);
    if (handle->code)
    {
      writeptr=(unsigned char *)handle->code;
      memcpy(writeptr,&GLUE_FUNC_ENTER,GLUE_FUNC_ENTER_SIZE); writeptr += GLUE_FUNC_ENTER_SIZE;
      p=startpts;
      while (p)
      {
        int thissize=*(int *)p->startptr;
        writeptr+=GLUE_RESET_ESI(writeptr,tabptr);
        //memcpy(writeptr,&GLUE_MOV_ESI_EDI,sizeof(GLUE_MOV_ESI_EDI));
        //writeptr+=sizeof(GLUE_MOV_ESI_EDI);
        memcpy(writeptr,(char*)p->startptr + 4,thissize);
        writeptr += thissize;
      
        p=p->next;
      }
      memcpy(writeptr,&GLUE_FUNC_LEAVE,GLUE_FUNC_LEAVE_SIZE); writeptr += GLUE_FUNC_LEAVE_SIZE;
      memcpy(writeptr,&GLUE_RET,sizeof(GLUE_RET)); writeptr += sizeof(GLUE_RET);
      ctx->l_stats[1]=size;
    }
    handle->blocks = ctx->blocks_head;
    ctx->blocks_head=0;

  }
  freeBlocks((llBlock **)&ctx->tmpblocks_head);  // free blocks
  freeBlocks((llBlock **)&ctx->blocks_head);  // free blocks

  if (handle)
  {
    memcpy(handle->code_stats,ctx->l_stats,sizeof(ctx->l_stats));
    nseel_evallib_stats[0]+=ctx->l_stats[0];
    nseel_evallib_stats[1]+=ctx->l_stats[1];
    nseel_evallib_stats[2]+=ctx->l_stats[2];
    nseel_evallib_stats[3]+=ctx->l_stats[3];
    nseel_evallib_stats[4]++;
  }
  memset(ctx->l_stats,0,sizeof(ctx->l_stats));

  free(expression_start);

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
  if (tabptr&31)
    tabptr += 32-((tabptr)&31);
  //printf("calling code!\n");
  GLUE_CALL_CODE(tabptr,codeptr);

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
    free(h->workTable);
    nseel_evallib_stats[0]-=h->code_stats[0];
    nseel_evallib_stats[1]-=h->code_stats[1];
    nseel_evallib_stats[2]-=h->code_stats[2];
    nseel_evallib_stats[3]-=h->code_stats[3];
    nseel_evallib_stats[4]--;
    freeBlocks(&h->blocks);


  }

}


//------------------------------------------------------------------------------
void NSEEL_VM_resetvars(NSEEL_VMCTX _ctx)
{
  if (_ctx)
  {
    compileContext *ctx=(compileContext *)_ctx;
    int x;
    if (ctx->varTable_Names || ctx->varTable_Values) for (x = 0; x < ctx->varTable_numBlocks; x ++)
    {
      if (ctx->varTable_Names) free(ctx->varTable_Names[x]);
      if (ctx->varTable_Values) free(ctx->varTable_Values[x]);
    }

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
  return ctx;
}

void NSEEL_VM_free(NSEEL_VMCTX _ctx) // free when done with a VM and ALL of its code have been freed, as well
{

  if (_ctx)
  {
    compileContext *ctx=(compileContext *)_ctx;
    NSEEL_VM_resetvars(_ctx);
    NSEEL_VM_freeRAM(_ctx);

    freeBlocks((llBlock **)&ctx->tmpblocks_head);  // free blocks
    freeBlocks((llBlock **)&ctx->blocks_head);  // free blocks
    free(ctx->compileLineRecs);
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





void NSEEL_PProc_RAM(void *data, int data_size, compileContext *ctx)
{
  if (data_size>0) EEL_GLUE_set_immediate(data, &ctx->ram_blocks); 
}
void NSEEL_PProc_THIS(void *data, int data_size, compileContext *ctx)
{
  if (data_size>0) EEL_GLUE_set_immediate(data, ctx->caller_this);
}
