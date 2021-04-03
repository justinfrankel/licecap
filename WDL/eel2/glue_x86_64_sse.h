#ifndef _NSEEL_GLUE_X86_64_SSE_H_
#define _NSEEL_GLUE_X86_64_SSE_H_

// SSE version (needs the appropriate .o linked!)

#define GLUE_PREFER_NONFP_DV_ASSIGNS
#define GLUE_HAS_FPREG2 1

static const unsigned int GLUE_COPY_FPSTACK_TO_FPREG2[] = { 0xc8100ff2 }; //  movsd  %xmm0,%xmm1
static unsigned char GLUE_POP_STACK_TO_FPREG2[] = {
  0xf2, 0x0f, 0x10, 0x0c, 0x24,  // movsd (%rsp), %xmm1
  0x48, 0x81, 0xC4, 16, 0,0,0, //  add rsp, 16
};

// spill registers
#define GLUE_MAX_SPILL_REGS 4
#ifdef _WIN32
  // win64: xmm6-xmm15 are nonvolatile, so we use xmm6-xmm9 as spill registers (xmm8/xmm9 are 5 byte encodings)
  #define GLUE_SAVE_TO_SPILL_SIZE(x) (4 + ((x)>1))
  #define GLUE_RESTORE_SPILL_TO_FPREG2_SIZE(x) (4 + ((x)>1))

  static void GLUE_RESTORE_SPILL_TO_FPREG2(void *b, int ws)
  {
    if (ws < 2)
    {
      *(unsigned int *)b = 0xce100ff2 + (ws<<24); // movsd xmm1, xmm6+ws
    }
    else
    {
      // movsd xmm1, xmm8 + (ws-2)
      *(unsigned int *)b = 0x100f41f2;
      ((unsigned char *)b)[4] = 0xc8 + (ws-2);
    }
  }
  static void GLUE_SAVE_TO_SPILL(void *b, int ws)
  {
    if (ws < 2)
    {
      *(unsigned int *)b = 0xf0100ff2 + (ws<<27); // movsd xmm6+ws, xmm0
    }
    else
    {
      // movsd xmm8+(ws-2), xmm0
      *(unsigned int *)b = 0x100f44f2;
      ((unsigned char *)b)[4] = 0xc0 + ((ws-2)<<3);
    }
  }
#else
  // non-win32: our function stubs preserve xmm4-xmm7

#ifdef _DEBUG
#define GLUE_VALIDATE_SPILLS
#endif

#ifdef GLUE_VALIDATE_SPILLS

static unsigned char save_validate[]={
0x48,0x83,0xec,0x10, //  subq $16, %rsp
0xf2,0x0f,0x11,0x04,0x24, //  movsd %xmm0, (%rsp)
0x66,0x48,0x0f,0x6e,0xe4, //  movq %rsp, %xmm4  (+ws<<3)
};

static unsigned char restore_validate[] = {
  0xf2, 0x0f, 0x10, 0xcc, // movsd %xmm7, %xmm1 (+ws)
  0x66, 0x48, 0x0f, 0x6e, 0xdc, //  movq %rsp, %xmm3
  0x66, 0x0f, 0x2e, 0xd9, //  ucomisd %xmm1, %xmm3
  0x74, 0x02, //  je 2 <skip>
  0xcd, 0x03, //  int $3
  0xf2, 0x0f, 0x10, 0x0c, 0x24, //  movsd (%rsp), %xmm1
  0x48, 0x83, 0xc4, 0x10, //  addq $16, %rsp
};
  #define GLUE_SAVE_TO_SPILL_SIZE(x) (sizeof(save_validate))
  #define GLUE_RESTORE_SPILL_TO_FPREG2_SIZE(x) (sizeof(restore_validate))

#else

  #define GLUE_SAVE_TO_SPILL_SIZE(x) (4)
  #define GLUE_RESTORE_SPILL_TO_FPREG2_SIZE(x) (4)

#endif

  static void GLUE_RESTORE_SPILL_TO_FPREG2(void *b, int ws)
  {
#ifdef GLUE_VALIDATE_SPILLS
    char *p = (char*) b;
    memcpy(p,restore_validate,sizeof(restore_validate));
    p[3] += ws;
#else
    *(unsigned int *)b = 0xcc100ff2 + (ws<<24); // movsd xmm1, xmm4+ws
#endif
  }
  static void GLUE_SAVE_TO_SPILL(void *b, int ws)
  {
#ifdef GLUE_VALIDATE_SPILLS
    char *p = (char*) b;
    memcpy(p,save_validate,sizeof(save_validate));
    p[sizeof(save_validate)-1] += ws<<3;
#else
    *(unsigned int *)b = 0xe0100ff2 + (ws<<27); // movsd xmm4+ws, xmm0
#endif
  }
#endif

#define GLUE_MAX_FPSTACK_SIZE 0 
#define GLUE_JMP_SET_OFFSET(endOfInstruction,offset) (((int *)(endOfInstruction))[-1] = (int) (offset))

static const unsigned char GLUE_JMP_NC[] = { 0xE9, 0,0,0,0, }; // jmp<offset>
static const unsigned char GLUE_JMP_IF_P1_Z[] = {0x85, 0xC0, 0x0F, 0x84, 0,0,0,0 }; // test eax, eax, jz
static const unsigned char GLUE_JMP_IF_P1_NZ[] = {0x85, 0xC0, 0x0F, 0x85, 0,0,0,0 }; // test eax, eax, jnz


#define GLUE_FUNC_ENTER_SIZE 0
#define GLUE_FUNC_LEAVE_SIZE 0
const static unsigned int GLUE_FUNC_ENTER[1];
const static unsigned int GLUE_FUNC_LEAVE[1];

  // on x86-64:
  //  stack is always 16 byte aligned
  //  pushing values to the stack (for eel functions) has alignment pushed first, then value (value is at the lower address)
  //  pushing pointers to the stack has the pointer pushed first, then the alignment (pointer is at the higher address)
  #define GLUE_MOV_PX_DIRECTVALUE_SIZE 10
  #define GLUE_MOV_PX_DIRECTVALUE_TOSTACK_SIZE 14 // wr=-1, sets xmm0
  #define GLUE_MOV_PX_DIRECTVALUE_TOFPREG2_SIZE 14 // wr=-2, sets xmm1 
  static void GLUE_MOV_PX_DIRECTVALUE_GEN(void *b, INT_PTR v, int wr) {   
    const static unsigned short tab[3] = 
    {
      0xB848 /* mov rax, dv*/, 
      0xBF48 /* mov rdi, dv */ , 
      0xB948 /* mov rcx, dv */ 
    };
    unsigned short *bb = (unsigned short *)b;
    *bb++ = tab[wdl_max(wr,0)];  // mov rax, directvalue
    *(INT_PTR *)bb = v; 
    if (wr == -2) *(unsigned int *)(bb + 4) = 0x08100ff2; // movsd (%rax), %xmm1
    else if (wr == -1) *(unsigned int *)(bb + 4) = 0x00100ff2; // movsd (%rax), %xmm0
  }

  const static unsigned char  GLUE_PUSH_P1[2]={	   0x50,0x50}; // push rax (pointer); push rax (alignment)

  #define GLUE_STORE_P1_TO_STACK_AT_OFFS_SIZE(x) 8
  static void GLUE_STORE_P1_TO_STACK_AT_OFFS(void *b, int offs)
  {
    ((unsigned char *)b)[0] = 0x48; // mov [rsp+offs], rax
    ((unsigned char *)b)[1] = 0x89; 
    ((unsigned char *)b)[2] = 0x84;
    ((unsigned char *)b)[3] = 0x24;
    *(int *)((unsigned char *)b+4) = offs;
  }

  #define GLUE_MOVE_PX_STACKPTR_SIZE 3
  static void GLUE_MOVE_PX_STACKPTR_GEN(void *b, int wv)
  {
    static const unsigned char tab[3][GLUE_MOVE_PX_STACKPTR_SIZE]=
    {
      { 0x48, 0x89, 0xe0 }, // mov rax, rsp
      { 0x48, 0x89, 0xe7 }, // mov rdi, rsp
      { 0x48, 0x89, 0xe1 }, // mov rcx, rsp
    };    
    memcpy(b,tab[wv],GLUE_MOVE_PX_STACKPTR_SIZE);
  }

  #define GLUE_MOVE_STACK_SIZE 7
  static void GLUE_MOVE_STACK(void *b, int amt)
  {
    ((unsigned char *)b)[0] = 0x48;
    ((unsigned char *)b)[1] = 0x81;
    if (amt < 0)
    {
      ((unsigned char *)b)[2] = 0xEC;
      *(int *)((char*)b+3) = -amt; // sub rsp, -amt32
    }
    else
    {
      ((unsigned char *)b)[2] = 0xc4;
      *(int *)((char*)b+3) = amt; // add rsp, amt32
    }
  }

  #define GLUE_POP_PX_SIZE 2
  static void GLUE_POP_PX(void *b, int wv)
  {
    static const unsigned char tab[3][GLUE_POP_PX_SIZE]=
    {
      {0x58,/*pop rax*/  0x58}, // pop alignment, then pop pointer
      {0x5F,/*pop rdi*/  0x5F}, 
      {0x59,/*pop rcx*/  0x59}, 
    };    
    memcpy(b,tab[wv],GLUE_POP_PX_SIZE);
  }

  static const unsigned char GLUE_PUSH_P1PTR_AS_VALUE[] = 
  {  
    0x50, /*push rax - for alignment */  
    0xff, 0x30, /* push qword [rax] */
  };

  static int GLUE_POP_VALUE_TO_ADDR(unsigned char *buf, void *destptr) // trashes P2 (rdi) and P3 (rcx)
  {    
    if (buf)
    {
      *buf++ = 0x48; *buf++ = 0xB9; *(void **) buf = destptr; buf+=8; // mov rcx, directvalue
      *buf++ = 0x8f; *buf++ = 0x01; // pop qword [rcx]      
      *buf++ = 0x5F ; // pop rdi (alignment, safe to trash rdi though)
    }
    return 1+10+2;
  }

  static int GLUE_COPY_VALUE_AT_P1_TO_PTR(unsigned char *buf, void *destptr) // trashes P2/P3
  {    
    if (buf)
    {
      *buf++ = 0x48; *buf++ = 0xB9; *(void **) buf = destptr; buf+=8; // mov rcx, directvalue
      *buf++ = 0x48; *buf++ = 0x8B; *buf++ = 0x38; // mov rdi, [rax]
      *buf++ = 0x48; *buf++ = 0x89; *buf++ = 0x39; // mov [rcx], rdi
    }

    return 3 + 10 + 3;
  }

  static int GLUE_POP_FPSTACK_TO_PTR(unsigned char *buf, void *destptr)
  {
    if (buf)
    {
      *buf++ = 0x48;
      *buf++ = 0xB8; 
      *(void **) buf = destptr; buf+=8; // mov rax, directvalue

      *buf++ = 0xf2; // movsd %xmm0, (%rax)
      *buf++ = 0x0f;
      *buf++ = 0x11;
      *buf++ = 0x00;
    }
    return 2+8+4;
  }


  #define GLUE_SET_PX_FROM_P1_SIZE 3
  static void GLUE_SET_PX_FROM_P1(void *b, int wv)
  {
    static const unsigned char tab[3][GLUE_SET_PX_FROM_P1_SIZE]={
      {0x90,0x90,0x90}, // should never be used! (nopnop)
      {0x48,0x89,0xC7}, // mov rdi, rax
      {0x48,0x89,0xC1}, // mov rcx, rax
    };
    memcpy(b,tab[wv],GLUE_SET_PX_FROM_P1_SIZE);
  }


  #define GLUE_POP_FPSTACK_SIZE 0
  static const unsigned char GLUE_POP_FPSTACK[1] = { 0 };

  static const unsigned char GLUE_POP_FPSTACK_TOSTACK[] = {
    0x48, 0x81, 0xEC, 16, 0,0,0, // sub rsp, 16 
    0xf2, 0x0f, 0x11, 0x04, 0x24, // movsd xmm0, (%rsp)
  };

  static const unsigned char GLUE_POP_FPSTACK_TO_WTP[] = { 
      0xf2, 0x0f, 0x11, 0x06, // movsd xmm0, (%rsi)
      0x48, 0x81, 0xC6, 8, 0,0,0,/* add rsi, 8 */ 
  };

  #define GLUE_SET_PX_FROM_WTP_SIZE 3
  static void GLUE_SET_PX_FROM_WTP(void *b, int wv)
  {
    static const unsigned char tab[3][GLUE_SET_PX_FROM_WTP_SIZE]={
      {0x48, 0x89,0xF0}, // mov rax, rsi
      {0x48, 0x89,0xF7}, // mov rdi, rsi
      {0x48, 0x89,0xF1}, // mov rcx, rsi
    };
    memcpy(b,tab[wv],GLUE_SET_PX_FROM_WTP_SIZE);
  }

  #define GLUE_PUSH_VAL_AT_PX_TO_FPSTACK_SIZE 4
  static void GLUE_PUSH_VAL_AT_PX_TO_FPSTACK(void *b, int wv)
  {
    static const unsigned char tab[3][GLUE_PUSH_VAL_AT_PX_TO_FPSTACK_SIZE]={
      {0xf2, 0x0f, 0x10, 0x00}, // movsd (%rax), %xmm0
      {0xf2, 0x0f, 0x10, 0x07}, // movsd (%rdi), %xmm0
      {0xf2, 0x0f, 0x10, 0x01}, // movsd (%rcx), %xmm0
    };
    memcpy(b,tab[wv],GLUE_PUSH_VAL_AT_PX_TO_FPSTACK_SIZE);
  }

#define GLUE_POP_FPSTACK_TO_WTP_TO_PX_SIZE (GLUE_SET_PX_FROM_WTP_SIZE + sizeof(GLUE_POP_FPSTACK_TO_WTP))
static void GLUE_POP_FPSTACK_TO_WTP_TO_PX(unsigned char *buf, int wv)
{
  GLUE_SET_PX_FROM_WTP(buf,wv);
  memcpy(buf + GLUE_SET_PX_FROM_WTP_SIZE,GLUE_POP_FPSTACK_TO_WTP,sizeof(GLUE_POP_FPSTACK_TO_WTP));
};


const static unsigned char  GLUE_RET=0xC3;

static int GLUE_RESET_WTP(unsigned char *out, void *ptr)
{
  if (out)
  {
	  *out++ = 0x48;
    *out++ = 0xBE; // mov rsi, constant64
  	*(void **)out = ptr;
    out+=sizeof(void *);
  }
  return 2+sizeof(void *);
}

extern void eel_callcode64(INT_PTR code, INT_PTR ram_tab);
extern void eel_callcode64_fast(INT_PTR code, INT_PTR ram_tab);
#define GLUE_CALL_CODE(bp, cp, rt) do { \
  if (h->compile_flags&NSEEL_CODE_COMPILE_FLAG_NOFPSTATE) eel_callcode64_fast(cp, rt); \
  else eel_callcode64(cp, rt);\
  } while(0)
#define GLUE_TABPTR_IGNORED

static unsigned char *EEL_GLUE_set_immediate(void *_p, INT_PTR newv)
{
  char *p=(char*)_p;
  INT_PTR scan = 0xFEFEFEFEFEFEFEFE;
  while (*(INT_PTR *)p != scan) p++;
  *(INT_PTR *)p = newv;
  return (unsigned char *) (((INT_PTR*)p)+1);
}

#define INT_TO_LECHARS(x) ((x)&0xff),(((x)>>8)&0xff), (((x)>>16)&0xff), (((x)>>24)&0xff)

#define GLUE_INLINE_LOOPS

static const unsigned char GLUE_LOOP_LOADCNT[]={
  0xf2, 0x48, 0x0f, 0x2c, 0xc8, // cvttsd2si %xmm0, %rcx
  0x48, 0x81, 0xf9, 1,0,0,0,  // cmp rcx, 1
        0x0F, 0x8C, 0,0,0,0,  // JL <skipptr>
};

#if NSEEL_LOOPFUNC_SUPPORT_MAXLEN > 0
#define GLUE_LOOP_CLAMPCNT_SIZE sizeof(GLUE_LOOP_CLAMPCNT)
static const unsigned char GLUE_LOOP_CLAMPCNT[]={
  0x48, 0x81, 0xf9, INT_TO_LECHARS(NSEEL_LOOPFUNC_SUPPORT_MAXLEN), // cmp rcx, NSEEL_LOOPFUNC_SUPPORT_MAXLEN
        0x0F, 0x8C, 10,0,0,0,  // JL over-the-mov
  0x48, 0xB9, INT_TO_LECHARS(NSEEL_LOOPFUNC_SUPPORT_MAXLEN), 0,0,0,0, // mov rcx, NSEEL_LOOPFUNC_SUPPORT_MAXLEN
};
#else
#define GLUE_LOOP_CLAMPCNT_SIZE 0
#define GLUE_LOOP_CLAMPCNT ""
#endif

#define GLUE_LOOP_BEGIN_SIZE sizeof(GLUE_LOOP_BEGIN)
static const unsigned char GLUE_LOOP_BEGIN[]={ 
  0x56, //push rsi
  0x51, // push rcx
};
static const unsigned char GLUE_LOOP_END[]={ 
  0x59, //pop rcx
  0x5E, // pop rsi
  0xff, 0xc9, // dec rcx
  0x0f, 0x85, 0,0,0,0, // jnz ...
};



#if NSEEL_LOOPFUNC_SUPPORT_MAXLEN > 0
static const unsigned char GLUE_WHILE_SETUP[]={
  0x48, 0xB9, INT_TO_LECHARS(NSEEL_LOOPFUNC_SUPPORT_MAXLEN), 0,0,0,0, // mov rcx, NSEEL_LOOPFUNC_SUPPORT_MAXLEN
};
#define GLUE_WHILE_SETUP_SIZE sizeof(GLUE_WHILE_SETUP)

static const unsigned char GLUE_WHILE_BEGIN[]={ 
  0x56, //push rsi
  0x51, // push rcx
};
static const unsigned char GLUE_WHILE_END[]={ 
  0x59, //pop rcx
  0x5E, // pop rsi

  0xff, 0xc9, // dec rcx
  0x0f, 0x84,  0,0,0,0, // jz endpt
};


#else
#define GLUE_WHILE_SETUP ""
#define GLUE_WHILE_SETUP_SIZE 0
#define GLUE_WHILE_END_NOJUMP

static const unsigned char GLUE_WHILE_BEGIN[]={ 
  0x56, //push rsi
  0x51, // push rcx
};
static const unsigned char GLUE_WHILE_END[]={ 
  0x59, //pop rcx
  0x5E, // pop rsi
};

#endif


static const unsigned char GLUE_WHILE_CHECK_RV[] = {
  0x85, 0xC0, // test eax, eax
  0x0F, 0x85, 0,0,0,0 // jnz  looppt
};

static const unsigned char GLUE_SET_P1_Z[] = { 0x48, 0x29, 0xC0 }; // sub rax, rax
static const unsigned char GLUE_SET_P1_NZ[] = { 0xb0, 0x01 }; // mov al, 1


#define GLUE_HAS_FLDZ
static const unsigned char GLUE_FLDZ[] = {
  0x0f, 0x57, 0xc0  //xorps %xmm0, %xmm0
};


static EEL_F negativezeropointfive=-0.5f;
static EEL_F onepointfive=1.5f;
#define GLUE_INVSQRT_NEEDREPL &negativezeropointfive, &onepointfive,


static void *GLUE_realAddress(void *fn, void *fn_e, int *size)
{
  static const unsigned char sig[12] = { 0x89, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
  unsigned char *p = (unsigned char *)fn;

  while (memcmp(p,sig,sizeof(sig))) p++;
  p+=sizeof(sig);
  fn = p;

  while (memcmp(p,sig,sizeof(sig))) p++;
  *size = (int) (p - (unsigned char *)fn);
  return fn;
}

#define GLUE_HAS_FUSE 1
static int GLUE_FUSE(compileContext *ctx, unsigned char *code, int left_size, int right_size, int fuse_flags, int spill_reg)
{
  const UINT_PTR base = (UINT_PTR) ctx->ram_state->blocks;
  const int is_sse_op = right_size == 4 && // add/mul/sub/min/max
                        code[0] == 0xf2 &&
                        code[1] == 0x0f &&
                        code[3] == 0xc1 && // low nibble is xmm1
                        (code[2] == 0x58 || code[2] == 0x59 || code[2] == 0x5c || code[2]==0x5d || code[2] == 0x5f);

  if (spill_reg >= 0)
  {
#ifndef GLUE_VALIDATE_SPILLS
    if (is_sse_op)
    {
      char tmp[32];
      const int sz = GLUE_RESTORE_SPILL_TO_FPREG2_SIZE(spill_reg);
      GLUE_RESTORE_SPILL_TO_FPREG2(tmp,spill_reg);
      if (left_size>=sz && !memcmp(code-sz,tmp,sz))
      {
        code[-2] = code[2]; // modify the movsd into an addsd
        code[-1] -= 8; // movsd uses 0xc8+(xmmX&7), addsd etc use 0xc0
        return -4;
      }
    }
#endif
  }
  else
  {
    if (left_size==28)
    {
      // if const64_1 is within a 32-bit offset of ctx->ram_blocks->blocks, we can use [r12+offs]
      if (code[-28] == 0x48 && code[-27] == 0xb8 && // mov rax, const64_1
          *(int *)(code - 18) == 0x08100ff2 &&      // movsd xmm1, [rax]
          code[-14] == 0x48 && code[-13] == 0xb8 && // mov rax, const64_2
          *(int *)(code - 4) == 0x00100ff2          // movsd xmm0, [rax]
          )
      {
        UINT_PTR c1, c2;
        INT_PTR c2offs,c1offs;
        unsigned char opc[3];
        int wrpos = -28;
        if (is_sse_op) memcpy(opc,code,3);
        memcpy(&c1,code-26,8);
        memcpy(&c2,code-12,8);

#define PTR_32_OK(x) ((x) == (INT_PTR)(int)(x))
        c2offs = c2-base;
        if (!PTR_32_OK(c2offs))
        {
          code[wrpos++] = 0x48;
          code[wrpos++] = 0xb8;
          memcpy(code+wrpos,&c2,8); // mov rax, const64_2
          wrpos += 8;
        }

        c1offs = c1-base;
        if (!PTR_32_OK(c1offs))
        {
          code[wrpos++] = 0x48;
          code[wrpos++] = 0xbf;
          memcpy(code+wrpos,&c1,8); // mov rdi, const64_1
          wrpos += 8;
        }

        if (!PTR_32_OK(c2offs))
        {
          *(int *)(code+wrpos) = 0x00100ff2; // movsd xmm0, [rax]
          wrpos += 4;
        }
        else
        {
          // movsd xmm0, [r12+offs]
          code[wrpos++] = 0xf2;
          code[wrpos++] = 0x41;
          code[wrpos++] = 0x0f;
          code[wrpos++] = 0x10;
          code[wrpos++] = 0x84;
          code[wrpos++] = 0x24;
          *(int *)(code+wrpos) = (int)c2offs;
          wrpos += 4;
        }

        if (!is_sse_op)
        {
          // load xmm1 from rdi/c1offs
          if (!PTR_32_OK(c1offs))
          {
            *(int *)(code+wrpos) = 0x0f100ff2; // movsd xmm1, [rdi]
            wrpos += 4;
          }
          else
          {
            // movsd xmm1, [r12+offs]
            code[wrpos++] = 0xf2;
            code[wrpos++] = 0x41;
            code[wrpos++] = 0x0f;
            code[wrpos++] = 0x10;
            code[wrpos++] = 0x8c;
            code[wrpos++] = 0x24;
            *(int *)(code+wrpos) = (int)c1offs;
            wrpos += 4;
          }
          if (wrpos<0) memmove(code+wrpos,code,right_size);
          return wrpos;
        }

        // fuse to sse op
        if (!PTR_32_OK(c1offs))
        {
          memcpy(code+wrpos,opc,3);
          code[wrpos+3] = 0x07; // [rdi]
          wrpos += 4;
        }
        else
        {
          // mul/add/sub/min/max/sd xmm0, [r12+offs]
          code[wrpos++] = opc[0]; // 0xf2
          code[wrpos++] = 0x41;
          code[wrpos++] = opc[1]; // 0x0f
          code[wrpos++] = opc[2]; // 0x58 etc
          code[wrpos++] = 0x84;
          code[wrpos++] = 0x24;
          *(int *)(code+wrpos) = (int)c1offs;
          wrpos += 4;
        }
        return wrpos - right_size;
      }
    }
    if ((fuse_flags&1) && left_size >= 14)
    {
      if (code[-14] == 0x48 && code[-13] == 0xb8 && // mov rax, const64_2
          *(int *)(code - 4) == 0x00100ff2)          // movsd xmm0, [rax]
      {
        INT_PTR c1;
        memcpy(&c1,code-12,8);
        c1 -= base;
        if (PTR_32_OK(c1))
        {
          // movsd xmm0, [r12+offs]
          int wrpos = -14;
          code[wrpos++] = 0xf2;
          code[wrpos++] = 0x41;
          code[wrpos++] = 0x0f;
          code[wrpos++] = 0x10;
          code[wrpos++] = 0x84;
          code[wrpos++] = 0x24;
          *(int *)(code+wrpos) = (int)c1;
          wrpos += 4;
          if (wrpos<0) memmove(code+wrpos,code,right_size);
          return wrpos;
        }
      }
    }

    if (left_size == 20 && right_size == 9 &&
        code[-20]==0x48 && code[-19] == 0xbf && // mov rdi, const64_1
        code[-10]==0x48 && code[-9] == 0xb8  // mov rax, const64_2
        )
    {
      static unsigned char assign_copy[9] = { 0x48, 0x8b, 0x10,  // mov rdx, [rax]
                                              0x48, 0x89, 0x17,  // mov [rdi], rdx
                                              0x48, 0x89, 0xf8, // mov rax, rdi
      };
      if (!memcmp(code,assign_copy,9))
      {
        int wrpos = -20;
        INT_PTR c1,c2; // c1 is dest, c2 is src
        memcpy(&c1,code-18,8);
        memcpy(&c2,code-8,8);

        if (!PTR_32_OK(c2-base))
        {
          code[wrpos++] = 0x48; // mov rdi, src
          code[wrpos++] = 0xbf;
          memcpy(code+wrpos,&c2,8);
          wrpos +=8;
        }

        code[wrpos++] = 0x48; // mov rax, dest
        code[wrpos++] = 0xb8;
        memcpy(code+wrpos,&c1,8);
        wrpos +=8;

        if (PTR_32_OK(c2-base))
        {
          // mov rdx, [r12+offs]
          code[wrpos++] = 0x49;
          code[wrpos++] = 0x8b;
          code[wrpos++] = 0x94;
          code[wrpos++] = 0x24;
          *(int *)(code+wrpos) = (int)(c2-base);
          wrpos += 4;
        }
        else
        {
          code[wrpos++] = 0x48; // mov rdx, [rdi]
          code[wrpos++] = 0x8b;
          code[wrpos++] = 0x17;
        }

        code[wrpos++] = 0x48; // mov [rax], rdx
        code[wrpos++] = 0x89;
        code[wrpos++] = 0x10;

        return wrpos - right_size;
      }
    }


  }
  return 0;
}
#endif
