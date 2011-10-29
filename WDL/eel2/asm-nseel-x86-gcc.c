#if defined(__APPLE__)
#define SAVE_STACK "pushl %ebp\nmovl %esp, %ebp\nandl $-16, %esp\n"
#define RESTORE_STACK "leave\n"
#else
#define SAVE_STACK
#define RESTORE_STACK
#endif

/* note: only EEL_F_SIZE=8 is now supported (no float EEL_F's) */

void nseel_asm_1pdd(void)
{
  __asm__(
    SAVE_STACK 
#ifdef TARGET_X64
     "movq (%eax), %xmm0\n"
     "subl $128, %rsp\n"
     "movl $0xffffffff, %edi\n" 
#ifdef AMD64ABI
     "movl %rsi, %r15\n"
     "call *%edi\n" 
     "movl %r15, %rsi\n"
     "movq xmm0, (%r15)\n"
#else
     "call *%edi\n" 
     "movq xmm0, (%esi)\n"
#endif
     "addl $128, %rsp\n"
#else
     "subl $8, %esp\n" /* keep stack aligned */ 
     "pushl 4(%eax)\n" /* push parameter */ 
     "pushl (%eax)\n"    /* push the rest of the parameter */ 
     "movl $0xffffffff, %edi\n" 
     "call *%edi\n" 
     "fstpl (%esi)\n" /* store result */ 
     "addl $16, %esp\n" 
#endif

     "movl %esi, %eax\n" /* set return value */ 
     "addl $8, %esi\n" /* advance worktab ptr */ 
     RESTORE_STACK 
  );
}
void nseel_asm_1pdd_end(void){}

void nseel_asm_2pdd(void)
{
  __asm__(
    SAVE_STACK
#ifdef TARGET_X64
    "movq (%eax), xmm1\n"
    "movq (%edi), xmm0\n"
    "subl $128, %rsp\n"
    "movl $0xffffffff, %edi\n"
#ifdef AMD64ABI
    "movl %rsi, %r15\n"
    "call *%edi\n"
    "movl %r15, %rsi\n"
    "movq xmm0, (%r15)\n"
#else
    "call *%edi\n"
    "movq xmm0, (%esi)\n"
#endif
    "addl $128, %rsp\n"
#else
    "pushl 4(%eax)\n" /* push parameter */
    "pushl (%eax)\n"    /* push the rest of the parameter */
    "pushl 4(%edi)\n" /* push parameter */
    "pushl (%edi)\n"    /* push the rest of the parameter */
    "movl $0xffffffff, %edi\n"
    "call *%edi\n"
    "fstpl (%esi)\n" /* store result */
    "addl $16, %esp\n"
#endif

    "movl %esi, %eax\n" /* set return value */
    "addl $8, %esi\n" /* advance worktab ptr */
    RESTORE_STACK
  );
}
void nseel_asm_2pdd_end(void){}

void nseel_asm_2pdds(void)
{
  __asm__(
    SAVE_STACK
#ifdef TARGET_X64
    "movq (%eax), xmm1\n"
    "movq (%edi), xmm0\n"
    "subl $128, %rsp\n"
    "movl $0xffffffff, %eax\n"
#ifdef AMD64ABI
    "movl %rsi, %r15\n"
    "movl %rdi, %r14\n"
    "call *%eax\n"
    "movl %r15, %rsi\n"
    "movq xmm0, (%r14)\n"
    "movl %r14, %rax\n" /* set return value */
#else
    "call *%eax\n"
    "movq xmm0, (%edi)\n"
    "movl %edi, %eax\n" /* set return value */
#endif
    "subl $128, %rsp\n"
#else
    "pushl 4(%eax)\n" /* push parameter */
    "pushl (%eax)\n"    /* push the rest of the parameter */
    "pushl 4(%edi)\n" /* push parameter */
    "pushl (%edi)\n"    /* push the rest of the parameter */
    "movl $0xffffffff, %eax\n"
    "call *%eax\n"
    "fstpl (%edi)\n" /* store result */
    "addl $16, %esp\n"
    "movl %edi, %eax\n" /* set return value */
#endif
    RESTORE_STACK
  );
}
void nseel_asm_2pdds_end(void){}

void nseel_asm_2pp(void)
{
__asm__(
    SAVE_STACK
#ifdef TARGET_X64

#ifdef AMD64ABI
    "movl %rsi, %r15\n"
    /* rdi is first parameter */
    "movl %rax, %rsi\n"
    "subl $128, %rsp\n"
    "movl $0xffffffff, %eax\n"
    "call *%eax\n"
    "movl %r15, %rsi\n"
    "movq xmm0, (%r15)\n"
#else
    "movl %edi, %ecx\n"
    "movl %eax, %edx\n"
    "subl $128, %rsp\n"
    "movl $0xffffffff, %edi\n"
    "call *%edi\n"
    "movq xmm0, (%esi)\n"
#endif
    "addl $128, %rsp\n"
#else
    "subl $8, %esp\n" /* keep stack aligned */
    "pushl %eax\n" /* push parameter */
    "pushl %edi\n"    /* push second parameter */
    "movl $0xffffffff, %edi\n"
    "call *%edi\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n" /* store result */
    "addl $16, %esp\n"
#endif
    "movl %esi, %eax\n" /* set return value */
    "addl $" EEL_F_SSTR ", %esi\n" /* advance worktab ptr */
    RESTORE_STACK
  );
}
void nseel_asm_2pp_end(void) {}
  

void nseel_asm_1pp(void)
{
__asm__(
    SAVE_STACK
#ifdef TARGET_X64
#ifdef AMD64ABI
    "movl %rsi, %r15\n"
    "movl %eax, %edi\n"
    "subl $128, %rsp\n"
    "movl $0xffffffff, %rax\n"
    "call *%rax\n"
    "movl %r15, %rsi\n"
    "movq xmm0, (%r15)\n"
#else
    "movl %eax, %ecx\n"
    "subl $128, %rsp\n"
    "movl $0xffffffff, %edi\n"
    "call *%edi\n"
    "movq xmm0, (%esi)\n"
#endif
    "addl $128, %rsp\n"
#else
    "subl $12, %esp\n" /* keep stack aligned */
    "pushl %eax\n" /* push parameter */
    "movl $0xffffffff, %edi\n"
    "call *%edi\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n" /* store result */
    "addl $16, %esp\n"
#endif
    "movl %esi, %eax\n" /* set return value */
    "addl $" EEL_F_SSTR ", %esi\n" /* advance worktab ptr */
    RESTORE_STACK
  );
}
void nseel_asm_1pp_end(void){}



//---------------------------------------------------------------------------------------------------------------


// do nothing, eh
void nseel_asm_exec2(void)
{
   __asm__(
      ""
    );
}
void nseel_asm_exec2_end(void) { }



void nseel_asm_invsqrt(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "movl $0x5f3759df, %edx\n"
    "fsts (%esi)\n"
#ifdef TARGET_X64
    "movl 0xffffffff, %rax\n"
    "subl %ecx, %ecx\n"
    "fmul" EEL_F_SUFFIX " (%rax)\n"
#else
    "fmul" EEL_F_SUFFIX " (0xffffffff)\n"
#endif
    "movl (%esi), %ecx\n"
    "sarl $1, %ecx\n"
    "subl %ecx, %edx\n"
    "movl %edx, (%esi)\n"
    "fmuls (%esi)\n"
    "fmuls (%esi)\n"
#ifdef TARGET_X64
    "movl 0xffffffff, %rax\n"
    "fadd" EEL_F_SUFFIX " (%rax)\n"
#else
    "fadd" EEL_F_SUFFIX " (0xffffffff)\n"
#endif
    "fmuls (%esi)\n"
    "movl %esi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_invsqrt_end(void) {}


//---------------------------------------------------------------------------------------------------------------
void nseel_asm_sin(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fsin\n"
    "movl %esi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_sin_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_cos(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fcos\n"
    "movl %esi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_cos_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_tan(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fptan\n"
    "movl %esi, %eax\n"
    "fstp %st(0)\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_tan_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_sqr(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fmul %st(0), %st(0)\n"
    "movl %esi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_sqr_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_sqrt(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fabs\n"
    "fsqrt\n"
    "movl %esi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_sqrt_end(void) {}


//---------------------------------------------------------------------------------------------------------------
void nseel_asm_log(void)
{
  __asm__(
    "fldln2\n"
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "movl %esi, %eax\n"
    "fyl2x\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_log_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_log10(void)
{
  __asm__(
    "fldlg2\n"
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "movl %esi, %eax\n"
    "fyl2x\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
    
  );
}
void nseel_asm_log10_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_abs(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fabs\n"
    "movl %esi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_abs_end(void) {}


//---------------------------------------------------------------------------------------------------------------
void nseel_asm_assign(void)
{
#ifdef TARGET_X64

  __asm__(
    "movll (%rax), %rdx\n"
    "movll %rdx, %rcx\n"
    "shrl $32, %rdx\n"
    "andl $0x7FF00000, %edx\n"
    "jz 1f\n"
    "cmpl $0x7FF00000, %edx\n"
    "je 1f\n"
    "jmp 0f\n"
    "1:\n"
    "subl %rcx, %rcx\n"
    "0:\n"
    "movll %rcx, (%edi)\n"
    );

#else

#if EEL_F_SIZE == 8
  __asm__(
    "movl 4(%eax), %edx\n"
    "movl (%eax), %ecx\n"
    "andl  $0x7ff00000, %edx\n"
    "jz 1f\n"   // if exponent=zero, zero
    "cmpl  $0x7ff00000, %edx\n"
    "je 1f\n" // if exponent=all 1s, zero
    "movl 4(%eax), %edx\n" // reread
    "jmp 0f\n"
    "1:\n"
    "subl %ecx, %ecx\n"
    "subl %edx, %edx\n"
    "0:\n"
    "movl %ecx, (%edi)\n"
    "movl %edx, 4(%edi)\n"
  );
#else
  __asm__(
    "movl (%eax), %ecx\n"
    "movl %ecx, (%edi)\n"
  );
#endif

#endif

}
void nseel_asm_assign_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_add(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fadd" EEL_F_SUFFIX " (%edi)\n"
    "movl %esi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_add_end(void) {}

void nseel_asm_add_op(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fadd" EEL_F_SUFFIX " (%edi)\n"
    "movl %edi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%edi)\n"
  );
}
void nseel_asm_add_op_end(void) {}


//---------------------------------------------------------------------------------------------------------------
void nseel_asm_sub(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fsub" EEL_F_SUFFIX " (%eax)\n"
    "movl %esi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_sub_end(void) {}

void nseel_asm_sub_op(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fsub" EEL_F_SUFFIX " (%eax)\n"
    "movl %edi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%edi)\n"
  );
}
void nseel_asm_sub_op_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_mul(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fmul" EEL_F_SUFFIX " (%eax)\n"
    "movl %esi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_mul_end(void) {}

void nseel_asm_mul_op(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fmul" EEL_F_SUFFIX " (%edi)\n"
    "movl %edi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%edi)\n"
  );
}
void nseel_asm_mul_op_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_div(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fdiv" EEL_F_SUFFIX " (%eax)\n"
    "movl %esi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_div_end(void) {}

void nseel_asm_div_op(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fdiv" EEL_F_SUFFIX " (%eax)\n"
    "movl %edi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%edi)\n"
  );
}
void nseel_asm_div_op_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_mod(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fabs\n"
    "fistpl (%esi)\n"
    "fabs\n"
    "fistpl 4(%esi)\n"
    "xorl %edx, %edx\n"
#ifdef TARGET_X64
    "subl %eax, %eax\n"
#endif
    "cmpl $0, (%esi)\n"
    "je 0f\n" // skip devide, set return to 0
    "movl 4(%esi), %eax\n"
    "divl (%esi)\n"
    "0:\n"
    "movl %edx, (%esi)\n"
    "fildl (%esi)\n"
    "movl %esi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_mod_end(void) {}

void nseel_asm_shl(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fistpl (%esi)\n"
    "fistpl 4(%esi)\n"
    "pushl %ecx\n"
    "movl (%esi), %ecx\n"
    "movl 4(%esi), %eax\n"
    "shll %cl, %eax\n"
    "movl %eax, (%esi)\n"
    "popl %ecx\n"
    "fildl (%esi)\n"
    "movl %esi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_shl_end(void) {}

void nseel_asm_shr(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fistpl (%esi)\n"
    "fistpl 4(%esi)\n"
    "pushl %ecx\n"
    "movl (%esi), %ecx\n"
    "movl 4(%esi), %eax\n"
    "sarl %cl, %eax\n"
    "movl %eax, (%esi)\n"
    "popl %ecx\n"
    "fildl (%esi)\n"
    "movl %esi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_shr_end(void) {}


void nseel_asm_mod_op(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fabs\n"
    "fistpl (%edi)\n"
    "fabs\n"
    "fistpl (%esi)\n"
#ifdef TARGET_X64
    "subl %eax, %eax\n"
#endif
    "xorl %edx, %edx\n"
    "cmpl $0, (%edi)\n"
    "je 0f\n" // skip devide, set return to 0
    "movl (%esi), %eax\n"
    "divl (%edi)\n"
    "0:\n"
    "movl %edx, (%edi)\n"
    "fildl (%edi)\n"
    "movl %edi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%edi)\n"
    );
}
void nseel_asm_mod_op_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_or(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "movl %esi, %eax\n"
    "fistpll (%esi)\n"
    "fistpll 8(%esi)\n"
#ifdef TARGET_X64
    "movll 8(%rsi), %rdi\n"
    "orll %rdi, (%rsi)\n"
#else
    "movl 8(%esi), %edi\n"
    "movl 12(%esi), %ecx\n"
    "orl %edi, (%esi)\n"
    "orl %ecx, 4(%esi)\n"
#endif
    "fildll (%esi)\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_or_end(void) {}

void nseel_asm_or_op(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fistpll (%edi)\n"
    "fistpll (%esi)\n"
#ifdef TARGET_X64
    "movll (%rsi), %rax\n"
    "orll %rax, (%rdi)\n"
#else
    "movl (%esi), %eax\n"
    "movl 4(%esi), %ecx\n"
    "orl %eax, (%edi)\n"
    "orl %ecx, 4(%edi)\n"
#endif
    "fildll (%edi)\n"
    "movl %edi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%edi)\n"
  );
}
void nseel_asm_or_op_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_and(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "movl %esi, %eax\n"
    "fistpll (%esi)\n"
    "fistpll 8(%esi)\n"
#ifdef TARGET_X64
    "movll 8(%rsi), %rdi\n"
    "andll %rdi, (%rsi)\n"
#else
    "movl 8(%esi), %edi\n"
    "movl 12(%esi), %ecx\n"
    "andl %edi, (%esi)\n"
    "andl %ecx, 4(%esi)\n"
#endif
    "fildll (%esi)\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_and_end(void) {}

void nseel_asm_and_op(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fistpll (%edi)\n"
    "fistpll (%esi)\n"
#ifdef TARGET_X64
    "movll (%rsi), %rax\n"
    "andll %rax, (%rdi)\n"
#else
    "movl (%esi), %eax\n"
    "movl 4(%esi), %ecx\n"
    "andl %eax, (%edi)\n"
    "andl %ecx, 4(%edi)\n"
#endif
    "fildll (%edi)\n"
    "movl %edi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%edi)\n"
  );
}
void nseel_asm_and_op_end(void) {}


//---------------------------------------------------------------------------------------------------------------
void nseel_asm_uplus(void) // this is the same as doing nothing, it seems
{
   __asm__(
      ""
    );
}
void nseel_asm_uplus_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_uminus(void)
{
  __asm__(
#if EEL_F_SIZE == 8
    "movl (%eax), %ecx\n"
    "movl 4(%eax), %edi\n"
    "movl %ecx, (%esi)\n"
    "xorl $0x80000000, %edi\n"
    "movl %esi, %eax\n"
    "movl %edi, 4(%esi)\n"
    "addl $8, %esi\n"
#else
    "movl (%eax), %ecx\n"
    "xorl $0x80000000, %ecx\n"
    "movl %esi, %eax\n"
    "movl %ecx, (%esi)\n"
    "addl $4, %esi\n"
#endif
  );
}
void nseel_asm_uminus_end(void) {}



//---------------------------------------------------------------------------------------------------------------
void nseel_asm_sign(void)
{
  __asm__(

#ifdef TARGET_X64


    "movl $0xFFFFFFFF, %rdi\n"
    "movll (%rax), %rcx\n"
    "movll $0x7FFFFFFFFFFFFFFF, %rdx\n"
    "testl %rdx, %rcx\n"
    "jz 1f\n"
    "shr $60, %rcx\n"
    "andl $8, %rcx\n"
    "addll %rdi, %rcx\n"
    "movl %rsi, %rax\n"
    "addl $8, %rsi\n"
    "movll (%rcx), %rdi\n"
    "movll %rdi, (%rax)\n"
	"1:\n"
   

#else

    "movl $0xFFFFFFFF, %edi\n"
#if EEL_F_SIZE == 8
    "movl 4(%eax), %ecx\n"
    "movl (%eax), %edx\n"
    "testl $0xFFFFFFFF, %edx\n"
    "jnz 0f\n"
#else
    "movl (%eax), %ecx\n"
#endif
    // high dword (minus sign bit) is zero 
    "test $0x7FFFFFFF, %ecx\n"
    "jz 1f\n" // zero zero, return the value passed directly
    "0:\n"
#if EEL_F_SIZE == 8
	"shrl $28, %ecx\n"
#else
	"shrl $29, %ecx\n"
#endif

    "andl $" EEL_F_SSTR ", %ecx\n"
    "addl %edi, %ecx\n"

    "movl %esi, %eax\n"
    "addl $" EEL_F_SSTR ", %esi\n"

    "movl (%ecx), %edi\n"
#if EEL_F_SIZE == 8
    "movl 4(%ecx), %edx\n"
#endif
    "movl %edi, (%eax)\n"
#if EEL_F_SIZE == 8
    "movl %edx, 4(%eax)\n"
#endif
	"1:\n"
   
#endif
);
}
void nseel_asm_sign_end(void) {}



//---------------------------------------------------------------------------------------------------------------
void nseel_asm_bnot(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fabs\n"
#ifdef TARGET_X64
    "movl $0xFFFFFFFF, %rax\n"
    "fcomp" EEL_F_SUFFIX " (%rax)\n" //[g_closefact]
#else
    "fcomp" EEL_F_SUFFIX " (0xFFFFFFFF)\n" //[g_closefact]
#endif
    "fstsw %ax\n"
    "test $256, %eax\n"
    "movl %esi, %eax\n"
    "jz 0f\n"
    "fld1\n"
    "jmp 1f\n"
    "0:\n"
    "fldz\n"
    "1:\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_bnot_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_if(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fabs\n"
#ifdef TARGET_X64
    "movl $0xFFFFFFFF, %rax\n"
    "fcomp" EEL_F_SUFFIX " (%rax)\n" //[g_closefact]
    "movll $0xFFFFFFFF, %rax\n"
    "movll %rax, (%esi)\n" // conversion script will extend these out to full len
    "movll $0xFFFFFFFF, %rax\n"
    "movll %rax, 8(%esi)\n"
    "fstsw %ax\n"
    "shrl $5, %rax\n"
    "andl $8, %rax\n"
    "movll (%rax, %rsi), %rax\n"
    "subl $8, %rsp\n"
#else
    "fcomp" EEL_F_SUFFIX " (0xFFFFFFFF)\n" //[g_closefact]
    "movl $0xFFFFFFFF, (%esi)\n"
    "movl $0xFFFFFFFF, 4(%esi)\n"
    "fstsw %ax\n"
    "shrl $6, %eax\n"
    "andl $4, %eax\n"
    "movl (%eax, %esi), %eax\n"
#endif
    "call *%eax\n"
#ifdef TARGET_X64
    "addl $8, %rsp\n"
#endif

  );
}
void nseel_asm_if_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_repeat(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fistpl (%esi)\n"
#ifdef TARGET_X64 // safe not sure if movl ecx will zero the high word
    "xorl %ecx, %ecx\n"
#endif 
    "movl (%esi), %ecx\n"
    "cmpl $1, %ecx\n"
    "jl 1f\n"
    "cmpl $" NSEEL_LOOPFUNC_SUPPORT_MAXLEN_STR ", %ecx\n"
    "jl 0f\n"
    "movl $" NSEEL_LOOPFUNC_SUPPORT_MAXLEN_STR ", %ecx\n"
"0:\n"
      "movl $0xFFFFFFFF, %edx\n"
      "subl $8, %esp\n" /* keep stack aligned -- note this is required on x64 too!*/ 
      "pushl %esi\n" // revert back to last temp workspace
      "pushl %ecx\n"
      "call *%edx\n"
      "popl %ecx\n"
      "popl %esi\n"
      "addl $8, %esp\n" /* keep stack aligned -- also required on x64*/ 
    "decl %ecx\n"
    "jnz 0b\n"
"1:\n"
  );
}
void nseel_asm_repeat_end(void) {}

void nseel_asm_repeatwhile(void)
{
  __asm__(
    "movl $" NSEEL_LOOPFUNC_SUPPORT_MAXLEN_STR ", %ecx\n"
"0:\n"
      "movl $0xFFFFFFFF, %edx\n"
      "subl $8, %esp\n" /* keep stack aligned -- required on x86 and x64*/ 
      "pushl %esi\n" // revert back to last temp workspace
      "pushl %ecx\n"
      "call *%edx\n"
      "popl %ecx\n"
      "popl %esi\n"
      "addl $8, %esp\n" /* keep stack aligned -- required on x86 and x64 */ 
      "fld" EEL_F_SUFFIX " (%eax)\n"
	  "fabs\n"
#ifdef TARGET_X64
    "movl $0xFFFFFFFF, %rax\n"
    "fcomp" EEL_F_SUFFIX " (%rax)\n" //[g_closefact]
#else
    "fcomp" EEL_F_SUFFIX " (0xFFFFFFFF)\n" //[g_closefact]
#endif
      "fstsw %ax\n"
	  "testl $256, %eax\n"
	  "jnz 0f\n"
    "decl %ecx\n"
    "jnz 0b\n"
	"0:\n"
	"movl %esi, %eax\n"
  );
}
void nseel_asm_repeatwhile_end(void) {}


void nseel_asm_band(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fabs\n"
#ifdef TARGET_X64
    "movl $0xFFFFFFFF, %rax\n"
    "fcomp" EEL_F_SUFFIX " (%rax)\n" //[g_closefact]
#else
    "fcomp" EEL_F_SUFFIX " (0xFFFFFFFF)\n" //[g_closefact]
#endif
    "fstsw %ax\n"
    "testl $256, %eax\n"
    "jnz 0f\n" // if Z, then we are nonzero

        "movl $0xFFFFFFFF, %ecx\n"
#ifdef TARGET_X64
    "subl $8, %rsp\n"
#endif
        "call *%ecx\n"
#ifdef TARGET_X64
    "addl $8, %rsp\n"
#endif
    	"fld" EEL_F_SUFFIX " (%eax)\n"
    	"fabs\n"
#ifdef TARGET_X64
    "movl $0xFFFFFFFF, %rax\n"
    "fcomp" EEL_F_SUFFIX " (%rax)\n" //[g_closefact]
#else
    "fcomp" EEL_F_SUFFIX " (0xFFFFFFFF)\n" //[g_closefact]
#endif
    	"fstsw %ax\n"
        "testl $256, %eax\n"
	"jnz 0f\n"
	"fld1\n"
	"jmp 1f\n"

"0:\n"
    "fldz\n"
"1:\n"

    "movl %esi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_band_end(void) {}

void nseel_asm_bor(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fabs\n"
#ifdef TARGET_X64
    "movl $0xFFFFFFFF, %rax\n"
    "fcomp" EEL_F_SUFFIX " (%rax)\n" //[g_closefact]
#else
    "fcomp" EEL_F_SUFFIX " (0xFFFFFFFF)\n" //[g_closefact]
#endif
    "fstsw %ax\n"
    "testl $256, %eax\n"
    "jz 0f\n" // if Z, then we are nonzero

        "movl $0xFFFFFFFF, %ecx\n"
#ifdef TARGET_X64
    "subl $8, %rsp\n"
#endif
        "call *%ecx\n"
#ifdef TARGET_X64
    "addl $8, %rsp\n"
#endif
    	"fld" EEL_F_SUFFIX " (%eax)\n"
    	"fabs\n"
#ifdef TARGET_X64
    "movl $0xFFFFFFFF, %rax\n"
    "fcomp" EEL_F_SUFFIX " (%rax)\n" //[g_closefact]
#else
    "fcomp" EEL_F_SUFFIX " (0xFFFFFFFF)\n" //[g_closefact]
#endif
    	"fstsw %ax\n"
        "testl $256, %eax\n"
	"jz 0f\n"
	"fldz\n"
	"jmp 1f\n"

"0:\n"
    "fld1\n"
"1:\n"

    "movl %esi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_bor_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_equal(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fsub" EEL_F_SUFFIX " (%edi)\n"
    "fabs\n"
#ifdef TARGET_X64
    "movl $0xFFFFFFFF, %rax\n"
    "fcomp" EEL_F_SUFFIX " (%rax)\n" //[g_closefact]
#else
    "fcomp" EEL_F_SUFFIX " (0xFFFFFFFF)\n" //[g_closefact]
#endif
    "fstsw %ax\n"
    "testl $256, %eax\n"
    "movl %esi, %eax\n"
    "jz 0f\n"
    "fld1\n"
    "jmp 1f\n"
    "0:\n"
    "fldz\n"
    "1:\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n" 
  );
}
void nseel_asm_equal_end(void) {}
//
//---------------------------------------------------------------------------------------------------------------
void nseel_asm_notequal(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fsub" EEL_F_SUFFIX " (%edi)\n"
    "fabs\n"
#ifdef TARGET_X64
    "movl $0xFFFFFFFF, %rax\n"
    "fcomp" EEL_F_SUFFIX " (%rax)\n" //[g_closefact]
#else
    "fcomp" EEL_F_SUFFIX " (0xFFFFFFFF)\n" //[g_closefact]
#endif
    "fstsw %ax\n"
    "testl $256, %eax\n"
    "movl %esi, %eax\n"
    "jnz 0f\n"
    "fld1\n"
    "jmp 1f\n"
    "0:\n"
    "fldz\n"
    "1:\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n" 
  );
}
void nseel_asm_notequal_end(void) {}


//---------------------------------------------------------------------------------------------------------------
void nseel_asm_below(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fcomp" EEL_F_SUFFIX " (%eax)\n"
    "fstsw %ax\n"
    "testl $256, %eax\n"
    "movl %esi, %eax\n"
    "jz 0f\n"
    "fld1\n"
    "jmp 1f\n"
    "0:\n"
    "fldz\n"
    "1:\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_below_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_beloweq(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fcomp" EEL_F_SUFFIX " (%edi)\n"
    "fstsw %ax\n"
    "testl $256, %eax\n"
    "movl %esi, %eax\n"
    "jnz 0f\n"
    "fld1\n"
    "jmp 1f\n"
    "0:\n"
    "fldz\n"
    "1:\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_beloweq_end(void) {}


//---------------------------------------------------------------------------------------------------------------
void nseel_asm_above(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fcomp" EEL_F_SUFFIX " (%edi)\n"
    "fstsw %ax\n"
    "testl $256, %eax\n"
    "movl %esi, %eax\n"
    "jz 0f\n"
    "fld1\n"
    "jmp 1f\n"
    "0:\n"
    "fldz\n"
    "1:\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_above_end(void) {}

void nseel_asm_aboveeq(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fcomp" EEL_F_SUFFIX " (%eax)\n"
    "fstsw %ax\n"
    "testl $256, %eax\n"
    "movl %esi, %eax\n"
    "jnz 0f\n"
    "fld1\n"
    "jmp 1f\n"
    "0:\n"
    "fldz\n"
    "1:\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
  );
}
void nseel_asm_aboveeq_end(void) {}



void nseel_asm_min(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fcomp" EEL_F_SUFFIX " (%eax)\n"
    "pushl %eax\n"
    "fstsw %ax\n"
    "testl $256, %eax\n"
    "popl %eax\n"
    "jz 0f\n"
    "movl %edi, %eax\n"
    "0:\n"
    );

}
void nseel_asm_min_end(void) {}

void nseel_asm_max(void)
{
  __asm__(
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fcomp" EEL_F_SUFFIX " (%eax)\n"
    "pushl %eax\n"
    "fstsw %ax\n"
    "testl $256, %eax\n"
    "popl %eax\n"
    "jnz 0f\n"
    "movl %edi, %eax\n"
    "0:\n"
    );
}
void nseel_asm_max_end(void) {}





// just generic functions left, yay




void _asm_generic3parm(void)
{
  __asm__(
#ifdef TARGET_X64

#ifdef AMD64ABI

    "movl %rsi, %r15\n"
    "movl %rdi, %rdx\n" // third parameter = parm
    "movl $0xFFFFFFFF, %rdi\n" // first parameter= context

    "movl %ecx, %rsi\n" // second parameter = parm
    "movl %rax, %rcx\n" // fourth parameter = parm
    "movl $0xffffffff, %rax\n" // call function
    "subl $128, %rsp\n"
    "call *%rax\n"

    "movl %r15, %rsi\n"
    "addl $128, %rsp\n"

#else
    "movl %ecx, %edx\n" // second parameter = parm
    "movl $0xFFFFFFFF, %ecx\n" // first parameter= context
    "movl %rdi, %r8\n" // third parameter = parm
    "movl %rax, %r9\n" // fourth parameter = parm
    "movl $0xffffffff, %edi\n" // call function
    "subl $128, %rsp\n"
    "call *%edi\n"
    "addl $128, %rsp\n"
#endif

#else
    SAVE_STACK
    "movl $0xFFFFFFFF, %edx\n"
    "pushl %eax\n" // push parameter
    "pushl %edi\n" // push parameter
    "pushl %ecx\n" // push parameter
    "pushl %edx\n" // push context pointer
    "movl $0xffffffff, %edi\n"
    "call *%edi\n"
    "addl $16, %esp\n"
    RESTORE_STACK
#endif
 );
}
void _asm_generic3parm_end(void) {}


void _asm_generic3parm_retd(void)
{
  __asm__(
#ifdef TARGET_X64
#ifdef AMD64ABI
    "movl %rsi, %r15\n"
    "movl %rdi, %rdx\n" // third parameter = parm
    "movl $0xFFFFFFFF, %rdi\n" // first parameter= context
    "movl %ecx, %rsi\n" // second parameter = parm
    "movl %rax, %rcx\n" // fourth parameter = parm
    "movl $0xffffffff, %rax\n" // call function
    "subl $128, %rsp\n"
    "call *%rax\n"
    "addl $128, %rsp\n"
    "movl %r15, %rsi\n"
    "movl %r15, %rax\n"
    "movq xmm0, (%r15)\n"
    "addl $8, %rsi\n"
#else
    "movl %ecx, %edx\n" // second parameter = parm
    "movl $0xFFFFFFFF, %ecx\n" // first parameter= context
    "movl %rdi, %r8\n" // third parameter = parm
    "movl %rax, %r9\n" // fourth parameter = parm
    "movl $0xffffffff, %edi\n" // call function
    "subl $128, %rsp\n"
    "call *%edi\n"
    "addl $128, %rsp\n"
    "movq xmm0, (%rsi)\n"
    "movl %rsi, %rax\n"
    "addl $8, %rsi\n"
#endif
#else
    SAVE_STACK
    "movl $0xFFFFFFFF, %edx\n"
    "pushl %eax\n" // push parameter
    "pushl %edi\n" // push parameter
    "pushl %ecx\n" // push parameter
    "pushl %edx\n" // push context pointer
    "movl $0xffffffff, %edi\n"
    "call *%edi\n"
    "movl %esi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
    "addl $16, %esp\n"
    RESTORE_STACK
#endif
 );
}
void _asm_generic3parm_retd_end(void) {}


void _asm_generic2parm(void) // this prob neds to be fixed for ppc
{
  __asm__(
#ifdef TARGET_X64

#ifdef AMD64ABI
    "movl %rsi, %r15\n"
    "movl %edi, %esi\n" // second parameter = parm
    "movl $0xFFFFFFFF, %edi\n" // first parameter= context
    "movl %rax, %rdx\n" // third parameter = parm
    "movl $0xffffffff, %rcx\n" // call function
    "subl $128, %rsp\n"
    "call *%rcx\n"
    "movl %r15, %rsi\n"
    "addl $128, %rsp\n"
#else
    "movl $0xFFFFFFFF, %ecx\n" // first parameter= context
    "movl %edi, %edx\n" // second parameter = parm
    "movl %rax, %r8\n" // third parameter = parm
    "movl $0xffffffff, %edi\n" // call function
    "subl $128, %rsp\n"
    "call *%edi\n"
    "addl $128, %rsp\n"
#endif
#else
    SAVE_STACK
    "movl $0xFFFFFFFF, %edx\n"
    "subl $4, %esp\n" // keep stack aligned
    "pushl %eax\n" // push parameter
    "pushl %edi\n" // push parameter
    "pushl %edx\n" // push context pointer
    "movl $0xffffffff, %edi\n"
    "call *%edi\n"
    "addl $16, %esp\n"
    RESTORE_STACK
#endif
 );
}
void _asm_generic2parm_end(void) {}


void _asm_generic2parm_retd(void)
{
  __asm__(
#ifdef TARGET_X64
#ifdef AMD64ABI
    "movl %rsi, %r15\n"
    "movl %rdi, %rsi\n" // second parameter = parm
    "movl $0xFFFFFFFF, %rdi\n" // first parameter= context
    "movl %rax, %rdx\n" // third parameter = parm
    "movl $0xffffffff, %rcx\n" // call function
    "subl $128, %rsp\n"
    "call *%rcx\n"
    "movl %r15, %rsi\n"
    "addl $128, %rsp\n"
    "movq xmm0, (%r15)\n"
    "movl %r15, %rax\n"
    "addl $8, %rsi\n"
#else
    "movl $0xFFFFFFFF, %ecx\n" // first parameter= context
    "movl %edi, %edx\n" // second parameter = parm
    "movl %rax, %r8\n" // third parameter = parm
    "movl $0xffffffff, %edi\n" // call function
    "subl $128, %rsp\n"
    "call *%edi\n"
    "addl $128, %rsp\n"
    "movq xmm0, (%rsi)\n"
    "movl %rsi, %rax\n"
    "addl $8, %rsi\n"
#endif
#else
    SAVE_STACK
    "movl $0xFFFFFFFF, %edx\n"
    "pushl %eax\n" // push parameter
    "pushl %edi\n" // push parameter
    "pushl %ecx\n" // push parameter
    "pushl %edx\n" // push context pointer
    "movl $0xffffffff, %edi\n"
    "call *%edi\n"
    "movl %esi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
    "addl $16, %esp\n"
    RESTORE_STACK
#endif
 );
}
void _asm_generic2parm_retd_end(void) {}





void _asm_generic1parm(void) // this prob neds to be fixed for ppc
{
  __asm__(
#ifdef TARGET_X64
#ifdef AMD64ABI
    "movl $0xFFFFFFFF, %rdi\n" // first parameter= context
    "movl %rsi, %r15\n"
    "movl %eax, %rsi\n" // second parameter = parm
    "subl $128, %rsp\n"
    "movl $0xffffffff, %rcx\n" // call function
    "call *%rcx\n"
    "movl %r15, %rsi\n"
    "addl $128, %rsp\n"
#else
    "movl $0xFFFFFFFF, %ecx\n" // first parameter= context
    "movl %eax, %edx\n" // second parameter = parm
    "movl $0xffffffff, %edi\n" // call function
    "subl $128, %rsp\n"
    "call *%edi\n"
    "addl $128, %rsp\n"
#endif
#else
    SAVE_STACK
    "movl $0xFFFFFFFF, %edx\n"
    "subl $8, %esp\n" // keep stack aligned
    "pushl %eax\n" // push parameter
    "pushl %edx\n" // push context pointer
    "movl $0xffffffff, %edi\n"
    "call *%edi\n"
    "addl $16, %esp\n"
    RESTORE_STACK
#endif

 );
}
void _asm_generic1parm_end(void) {}


void _asm_generic1parm_retd(void) // 1 parameter returning double
{
  __asm__(
#ifdef TARGET_X64
#ifdef AMD64ABI
    "movl %rsi, %r15\n"
    "movl $0xFFFFFFFF, %rdi\n" // first parameter= context
    "movl %rax, %rsi\n" // second parameter = parm
    "movl $0xffffffff, %rcx\n" // call function
    "subl $128, %rsp\n"
    "call *%rcx\n"
    "movl %r15, %rsi\n"
    "addl $128, %rsp\n"
    "movq xmm0, (%r15)\n"
    "movl %r15, %rax\n"
    "addl $8, %rsi\n"
#else
    "movl $0xFFFFFFFF, %ecx\n" // first parameter= context
    "movl %eax, %edx\n" // second parameter = parm
    "movl $0xffffffff, %edi\n" // call function
    "subl $128, %rsp\n"
    "call *%edi\n"
    "addl $128, %rsp\n"
    "movq xmm0, (%rsi)\n"
    "movl %rsi, %rax\n"
    "addl $8, %rsi\n"
#endif
#else
    SAVE_STACK
    "movl $0xFFFFFFFF, %edx\n"
    "subl $8, %esp\n" // keep stack aligned
    "pushl %eax\n" // push parameter
    "pushl %edx\n" // push context pointer
    "movl $0xffffffff, %edi\n"
    "call *%edi\n"
    "movl %esi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
    "addl $16, %esp\n"
    RESTORE_STACK
#endif
 );
}
void _asm_generic1parm_retd_end(void) {}





// this gets its own stub because it's pretty crucial for performance :/

void _asm_megabuf(void)
{
  __asm__(
SAVE_STACK

#ifdef TARGET_X64


#ifdef AMD64ABI

    "movl %rsi, %r15\n"
    "movl $0xFFFFFFFF, %rdi\n" // first parameter = context pointer
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "movl $0xFFFFFFFF, %rdx\n"
    "fadd" EEL_F_SUFFIX " (%rdx)\n"
    "fistpl (%r15)\n"
    "xorl %rsi, %rsi\n"
    "movl (%r15), %esi\n" // r15 = esi (from above)
    "movl $0xffffffff, %edx\n"
    "subl $128, %rsp\n"
    "call *%edx\n"
    "movl %r15, %rsi\n"
    "addl $128, %rsp\n"
    "and %rax, %rax\n"
    "jnz 0f\n"
    "movl %r15, %rax\n"
    "movll $0, (%esi)\n"
    "addl $" EEL_F_SSTR ", %rsi\n"
    "0:"

#else
    "movl $0xFFFFFFFF, %ecx\n" // first parameter = context pointer
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "movl $0xFFFFFFFF, %edx\n"
    "fadd" EEL_F_SUFFIX " (%rdx)\n"
    "fistpl (%esi)\n"
    "xorl %rdx, %rdx\n"
    "movl (%esi), %edx\n"
    "movl $0xffffffff, %edi\n"
    "subl $128, %rsp\n"
    "call *%edi\n"
    "addl $128, %rsp\n"
    "and %rax, %rax\n"
    "jnz 0f\n"
    "movl %rsi, %rax\n"
    "movll $0, (%esi)\n"
    "addl $" EEL_F_SSTR ", %esi\n"
    "0:"
#endif


#else
    "movl $0xFFFFFFFF, %edx\n"
    "fld" EEL_F_SUFFIX " (%eax)\n"
    "fadd" EEL_F_SUFFIX " (0xFFFFFFFF)\n"
    "fistpl (%esi)\n"
    "subl $8, %esp\n" // keep stack aligned
    "pushl (%esi)\n" // parameter
    "pushl %edx\n" // push context pointer
    "movl $0xffffffff, %edi\n"
    "call *%edi\n"
    "addl $16, %esp\n"
    "and %eax, %eax\n"
    "jnz 0f\n"
    "movl %esi, %eax\n"
    "movl $0, (%esi)\n"
#if EEL_F_SIZE == 8
    "movl $0, 4(%esi)\n"
#endif
    "addl $" EEL_F_SSTR ", %esi\n"
    "0:"


#endif

RESTORE_STACK

 );
}

void _asm_megabuf_end(void) {}


#ifdef TARGET_X64
void win64_callcode() 
{
	__asm__(
#ifdef AMD64ABI
		"movll %edi, %eax\n"
#else
		"movll %ecx, %eax\n"
#endif

		"push %rbx\n"
		"push %rbp\n"
#ifndef AMD64ABI
		"push %rdi\n"
		"push %rsi\n"
		"push %r12\n"
		"push %r13\n"
#endif
		"push %r14\n" // on AMD64ABI, we'll use r14/r15 to save edi/esi
		"push %r15\n"
		"call %eax\n"
		"pop %r15\n"
		"pop %r14\n"
#ifndef AMD64ABI
		"pop %r13\n"
		"pop %r12\n"
		"pop %rsi\n"
		"pop %rdi\n"
		"fclex\n"
#endif
		"pop %rbp\n"
		"pop %rbx\n"
		"ret\n"
	);
}

#endif
