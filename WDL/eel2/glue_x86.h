#ifndef _NSEEL_GLUE_X86_H_
#define _NSEEL_GLUE_X86_H_

#define GLUE_JMP_TYPE int
#define GLUE_JMP_OFFSET 0 // offset from end of instruction that is the "source" of the jump

static const unsigned char GLUE_JMP_NC[] = { 0xE9, 0,0,0,0, }; // jmp<offset>

#define GLUE_FUNC_ENTER_SIZE 0
#define GLUE_FUNC_LEAVE_SIZE 0
const static unsigned int GLUE_FUNC_ENTER[1];
const static unsigned int GLUE_FUNC_LEAVE[1];

  // x86
  // stack is 16 byte aligned
  // when pushing values to stack, alignment pushed first, then value (value is at the lower address)
  // when pushing pointers to stack, alignment pushed first, then pointer (pointer is at the lower address)

  static const unsigned char GLUE_PUSH_P1PTR_AS_VALUE[] = 
  { 
    0x83, 0xEC, 8, /* sub esp, 8 */   
    0xff, 0x70, 0x4, /* push dword [eax+4] */ 
    0xff, 0x30, /* push dword [eax] */
  };

  static int GLUE_POP_VALUE_TO_ADDR(unsigned char *buf, void *destptr)
  {    
    if (buf)
    {
      *buf++ = 0xB8; *(void **) buf = destptr; buf+=4; // mov eax, directvalue
           
      *buf++ = 0x8f; *buf++ = 0x00; // pop dword [eax]
      *buf++ = 0x8f; *buf++ = 0x40; *buf++ = 4; // pop dword [eax+4]
      
      *buf++ = 0x59; // pop ecx (alignment)
      *buf++ = 0x59; // pop ecx (alignment)
    }
    
    return 12;
  }

  static int GLUE_COPY_VALUE_AT_P1_TO_PTR(unsigned char *buf, void *destptr)
  {    
    if (buf)
    {
      *buf++ = 0x8B; *buf++ = 0x38; // mov edi, [eax]
      *buf++ = 0x8B; *buf++ = 0x48; *buf++ = 0x04; // mov ecx, [eax+4]
      
      
      *buf++ = 0xB8; *(void **) buf = destptr; buf+=4; // mov eax, directvalue
      *buf++ = 0x89; *buf++ = 0x38; // mov [eax], edi
      *buf++ = 0x89; *buf++ = 0x48; *buf++ = 0x04; // mov [eax+4], ecx
    }
    
    return 2 + 3 + 5 + 2 + 3;
  }

  static int GLUE_POP_FPSTACK_TO_PTR(unsigned char *buf, void *destptr)
  {
    if (buf)
    {
      *buf++ = 0xB8; *(void **) buf = destptr; buf+=4; // mov eax, directvalue
      *buf++ = 0xDD; *buf++ = 0x18;  // fstp qword [eax]
    }
    return 1+4+2;
  }


  #define GLUE_MOV_PX_DIRECTVALUE_SIZE 5
  static void GLUE_MOV_PX_DIRECTVALUE_GEN(void *b, int v, int wv) 
  {   
    const static unsigned char tab[3] = {
      0xB8 /* mov eax, dv*/, 
      0xBF /* mov edi, dv */ , 
      0xB9 /* mov ecx, dv */ 
    };
    *((unsigned char *)b) = tab[wv];  // mov eax, dv
    b= ((unsigned char *)b)+1;
    *(int *)b = v; 
  }
  const static unsigned char  GLUE_PUSH_P1[4]={0x83, 0xEC, 12,   0x50}; // sub esp, 12, push eax
  
  #define GLUE_POP_PX_SIZE 4
  static void GLUE_POP_PX(void *b, int wv)
  {
    static const unsigned char tab[3][GLUE_POP_PX_SIZE]=
    {
      {0x58,/*pop eax*/  0x83, 0xC4, 12 /* add esp, 12*/},
      {0x5F,/*pop edi*/  0x83, 0xC4, 12}, 
      {0x59,/*pop ecx*/  0x83, 0xC4, 12}, 
    };    
    memcpy(b,tab[wv],GLUE_POP_PX_SIZE);
  }

  #define GLUE_SET_PX_FROM_P1_SIZE 2
  static void GLUE_SET_PX_FROM_P1(void *b, int wv)
  {
    static const unsigned char tab[3][GLUE_SET_PX_FROM_P1_SIZE]={
      {0x90,0x90}, // should never be used! (nopnop)
      {0x89,0xC7}, // mov edi, eax
      {0x89,0xC1}, // mov ecx, eax
    };
    memcpy(b,tab[wv],GLUE_SET_PX_FROM_P1_SIZE);
  }

  #define GLUE_POP_FPSTACK_SIZE 2
  static const unsigned char GLUE_POP_FPSTACK[2] = { 0xDD, 0xD8 }; // fstp st0

  static const unsigned char GLUE_POP_FPSTACK_TOSTACK[] = {
    0x83, 0xEC, 16, // sub esp, 16
    0xDD, 0x1C, 0x24 // fstp qword (%esp)  
  };

  static const unsigned char GLUE_POP_STACK_TO_FPSTACK[] = {
    0xDD, 0x04, 0x24, // fld qword (%esp)
    0x83, 0xC4, 16 //  add esp, 16
  };
 
  static const unsigned char GLUE_POP_FPSTACK_TO_WTP_ANDPUSHADDR[] = { 
      0xDD, 0x1E, // fstp qword [esi]
      0x83, 0xEC, 12, // sub esp, 12 
      0x56, //  push esi
      0x83, 0xC6, 8, // add esi, 8
  };

  static const unsigned char GLUE_POP_FPSTACK_TO_WTP[] = { 
      0xDD, 0x1E, /* fstp qword [esi] */
      0x83, 0xC6, 8, /* add esi, 8 */ 
  };

  #define GLUE_SET_PX_FROM_WTP_SIZE 2
  static void GLUE_SET_PX_FROM_WTP(void *b, int wv)
  {
    static const unsigned char tab[3][GLUE_SET_PX_FROM_WTP_SIZE]={
      {0x89,0xF0}, // mov eax, esi
      {0x89,0xF7}, // mov edi, esi
      {0x89,0xF1}, // mov ecx, esi
    };
    memcpy(b,tab[wv],GLUE_SET_PX_FROM_WTP_SIZE);
  }

  #define GLUE_PUSH_VAL_AT_PX_TO_FPSTACK_SIZE 2
  static void GLUE_PUSH_VAL_AT_PX_TO_FPSTACK(void *b, int wv)
  {
    static const unsigned char tab[3][GLUE_PUSH_VAL_AT_PX_TO_FPSTACK_SIZE]={
      {0xDD,0x00}, // fld qword [eax]
      {0xDD,0x07}, // fld qword [edi]
      {0xDD,0x01}, // fld qword [ecx]
    };
    memcpy(b,tab[wv],GLUE_SET_PX_FROM_WTP_SIZE);
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
    *out++ = 0xBE; // mov esi, constant
    memcpy(out,&ptr,sizeof(void *));
    out+=sizeof(void *);
  }
  return 1+sizeof(void *);
}



// for gcc on x86 (msvc should already have _controlfp defined)
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


static void GLUE_CALL_CODE(INT_PTR bp, INT_PTR cp) 
{
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
    __asm__(
          "pushl %%ebp\n"
          "movl %%esp, %%ebp\n"
          "andl $-16, %%esp\n" // align stack to 16 bytes
          "subl $12, %%esp\n" // call will push 4 bytes on stack, align for that
          "call *%%eax\n"
          "leave\n"
          ::
          "a" (cp): "%ecx","%edx","%esi","%edi");
    #ifndef EEL_NO_CHANGE_FPFLAGS
      _controlfp(old_v,_MCW_RC);
    #endif
  #endif //gcc x86
}

static unsigned char *EEL_GLUE_set_immediate(void *_p, const void *newv)
{
  char *p=(char*)_p;
  INT_PTR scan = 0xFEFEFEFE;
  while (*(INT_PTR *)p != scan) p++;
  *(INT_PTR *)p = (INT_PTR)newv;
  return (unsigned char *) (((INT_PTR*)p)+1);
}



#endif
