
void nseel_asm_1pdd(void)
{
  __asm__(

    FUNCTION_MARKER
     
    "movl $0xfefefefe, %edi\n" 
    "subl $16, %esp\n"
    "fstpl (%esp)\n"
    "call *%edi\n" 
    "addl $16, %esp\n" 

    FUNCTION_MARKER
     
  );
}

void nseel_asm_2pdd(void)
{
  __asm__(
    FUNCTION_MARKER
    
    "movl $0xfefefefe, %edi\n"
    "subl $16, %esp\n"
    "fstpl 8(%esp)\n"
    "fstpl (%esp)\n"
    "call *%edi\n"
    "addl $16, %esp\n"
    
    FUNCTION_MARKER
  );
}

void nseel_asm_2pdds(void)
{
  __asm__(
    FUNCTION_MARKER
    
    "movl $0xfefefefe, %eax\n"
    "subl $8, %esp\n"
    "fstpl (%esp)\n"
    "pushl 4(%edi)\n" /* push parameter */
    "pushl (%edi)\n"    /* push the rest of the parameter */
    "call *%eax\n"
    "addl $16, %esp\n"
    "fstpl (%edi)\n" /* store result */
    "movl %edi, %eax\n" /* set return value */

    // denormal-fix result (this is only currently used for pow_op, so we want this!)
    "movl 4(%edi), %edx\n"
    "addl $0x00100000, %edx\n"
    "andl $0x7FF00000, %edx\n"
    "cmpl $0x00200000, %edx\n"
    "jg 0f\n"
      "subl %edx, %edx\n"
      "movl %edx, (%edi)\n"
      "movl %edx, 4(%edi)\n"
    "0:\n"

    FUNCTION_MARKER
    
  );
}



//---------------------------------------------------------------------------------------------------------------


void nseel_asm_invsqrt(void)
{
  __asm__(
      FUNCTION_MARKER
    "movl $0x5f3759df, %edx\n"
    "fsts (%esi)\n"
    "fmul" EEL_F_SUFFIX " (0xfefefefe)\n"
    "movl (%esi), %ecx\n"
    "sarl $1, %ecx\n"
    "subl %ecx, %edx\n"
    "movl %edx, (%esi)\n"
    "fmuls (%esi)\n"
    "fmuls (%esi)\n"
    "fadd" EEL_F_SUFFIX " (0xfefefefe)\n"
    "fmuls (%esi)\n"

     FUNCTION_MARKER
  );
}


void nseel_asm_dbg_getstackptr(void)
{
  __asm__(
      FUNCTION_MARKER
#ifdef __clang__
    "ffree %st(0)\n"
#else
    "fstpl %st(0)\n"
#endif
    "movl %esp, (%esi)\n"
    "fildl (%esi)\n"

     FUNCTION_MARKER
  );
}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_sin(void)
{
  __asm__(
      FUNCTION_MARKER
    "fsin\n"
     FUNCTION_MARKER
  );
}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_cos(void)
{
  __asm__(
      FUNCTION_MARKER
    "fcos\n"
     FUNCTION_MARKER
  );
}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_tan(void)
{
  __asm__(
      FUNCTION_MARKER
    "fptan\n"
    "fstp %st(0)\n"
     FUNCTION_MARKER
  );
}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_sqr(void)
{
  __asm__(
      FUNCTION_MARKER
    "fmul %st(0), %st(0)\n"
     FUNCTION_MARKER
  );
}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_sqrt(void)
{
  __asm__(
      FUNCTION_MARKER
    "fabs\n"
    "fsqrt\n"
     FUNCTION_MARKER
  );
}


//---------------------------------------------------------------------------------------------------------------
void nseel_asm_log(void)
{
  __asm__(
      FUNCTION_MARKER
    "fldln2\n"
    "fxch\n"
    "fyl2x\n"
     FUNCTION_MARKER
  );
}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_log10(void)
{
  __asm__(
      FUNCTION_MARKER
    "fldlg2\n"
    "fxch\n"
    "fyl2x\n"
    
     FUNCTION_MARKER
  );
}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_abs(void)
{
  __asm__(
      FUNCTION_MARKER
    "fabs\n"
     FUNCTION_MARKER
  );
}


//---------------------------------------------------------------------------------------------------------------
void nseel_asm_assign(void)
{

  __asm__(
      FUNCTION_MARKER
    "movl (%eax), %ecx\n"
    "movl 4(%eax), %edx\n"
    "movl %edx, %eax\n"
    "addl $0x00100000, %eax\n" // if exponent is zero, make exponent 0x7ff, if 7ff, make 7fe
    "andl $0x7ff00000, %eax\n" 
    "cmpl $0x00200000, %eax\n"
    "jg 0f\n"
      "subl %ecx, %ecx\n"
      "subl %edx, %edx\n"
    "0:\n"
    "movl %edi, %eax\n"
    "movl %ecx, (%edi)\n"
    "movl %edx, 4(%edi)\n"

     FUNCTION_MARKER
  );

}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_assign_fromfp(void)
{
  __asm__(
      FUNCTION_MARKER
    "fstpl (%edi)\n"
    "movl 4(%edi), %edx\n"
    "addl $0x00100000, %edx\n"
    "andl $0x7FF00000, %edx\n"
    "cmpl $0x00200000, %edx\n"
    "movl %edi, %eax\n"
    "jg 0f\n"
      "subl %edx, %edx\n"
      "movl %edx, (%edi)\n"
      "movl %edx, 4(%edi)\n"
    "0:\n"

     FUNCTION_MARKER
    );
}


//---------------------------------------------------------------------------------------------------------------
void nseel_asm_assign_fast_fromfp(void)
{
  __asm__(
      FUNCTION_MARKER
    "movl %edi, %eax\n"
    "fstpl (%edi)\n"
     FUNCTION_MARKER
   );
}



//---------------------------------------------------------------------------------------------------------------
void nseel_asm_assign_fast(void)
{
  __asm__(
      FUNCTION_MARKER
    "movl (%eax), %ecx\n"
    "movl %ecx, (%edi)\n"
    "movl 4(%eax), %ecx\n"

    "movl %edi, %eax\n"
    "movl %ecx, 4(%edi)\n"
     FUNCTION_MARKER
  );
}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_add(void)
{
  __asm__(
          FUNCTION_MARKER
#ifdef __clang__
          "faddp %st(1)\n"
#else
          "fadd\n"
#endif
          FUNCTION_MARKER
          );
}

void nseel_asm_add_op(void)
{
  __asm__(
      FUNCTION_MARKER
    "fadd" EEL_F_SUFFIX " (%edi)\n"
    "movl %edi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%edi)\n"

    "movl 4(%edi), %edx\n"
    "addl $0x00100000, %edx\n"
    "andl $0x7FF00000, %edx\n"
    "cmpl $0x00200000, %edx\n"
    "jg 0f\n"
      "subl %edx, %edx\n"
      "movl %edx, (%edi)\n"
      "movl %edx, 4(%edi)\n"
    "0:\n"
     FUNCTION_MARKER
  );
}

void nseel_asm_add_op_fast(void)
{
  __asm__(
      FUNCTION_MARKER
    "fadd" EEL_F_SUFFIX " (%edi)\n"
    "movl %edi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%edi)\n"
     FUNCTION_MARKER
  );
}


//---------------------------------------------------------------------------------------------------------------
void nseel_asm_sub(void)
{
  __asm__(
      FUNCTION_MARKER
#ifdef __clang__
    "fsubrp %st(0), %st(1)\n"
#else
  #ifdef __GNUC__
    #ifdef __INTEL_COMPILER
      "fsub\n"
    #else
      "fsubr\n" // gnuc has fsub/fsubr backwards, ack
    #endif
  #else
    "fsub\n"
  #endif
#endif
     FUNCTION_MARKER
  );
}

void nseel_asm_sub_op(void)
{
  __asm__(
      FUNCTION_MARKER
    "fsubr" EEL_F_SUFFIX " (%edi)\n"
    "movl %edi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%edi)\n"

    "movl 4(%edi), %edx\n"
    "addl $0x00100000, %edx\n"
    "andl $0x7FF00000, %edx\n"
    "cmpl $0x00200000, %edx\n"
    "jg 0f\n"
      "subl %edx, %edx\n"
      "movl %edx, (%edi)\n"
      "movl %edx, 4(%edi)\n"
    "0:\n"
     FUNCTION_MARKER
  );
}

void nseel_asm_sub_op_fast(void)
{
  __asm__(
      FUNCTION_MARKER
    "fsubr" EEL_F_SUFFIX " (%edi)\n"
    "movl %edi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%edi)\n"
     FUNCTION_MARKER
  );
}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_mul(void)
{
  __asm__(
      FUNCTION_MARKER
#ifdef __clang__
          "fmulp %st(0), %st(1)\n"
#else
          "fmul\n"
#endif
     FUNCTION_MARKER
  );
}

void nseel_asm_mul_op(void)
{
  __asm__(
      FUNCTION_MARKER
    "fmul" EEL_F_SUFFIX " (%edi)\n"
    "movl %edi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%edi)\n"

    "movl 4(%edi), %edx\n"
    "addl $0x00100000, %edx\n"
    "andl $0x7FF00000, %edx\n"
    "cmpl $0x00200000, %edx\n"
    "jg 0f\n"
      "subl %edx, %edx\n"
      "movl %edx, (%edi)\n"
      "movl %edx, 4(%edi)\n"
    "0:\n"
    FUNCTION_MARKER
  );
}

void nseel_asm_mul_op_fast(void)
{
  __asm__(
      FUNCTION_MARKER
    "fmul" EEL_F_SUFFIX " (%edi)\n"
    "movl %edi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%edi)\n"
    FUNCTION_MARKER
  );
}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_div(void)
{
  __asm__(
      FUNCTION_MARKER
#ifdef __clang__
    "fdivrp %st(1)\n"
#else
  #ifdef __GNUC__
    #ifdef __INTEL_COMPILER
      "fdiv\n" 
    #else
      "fdivr\n" // gcc inline asm seems to have fdiv/fdivr backwards
    #endif
  #else
    "fdiv\n"
  #endif
#endif
    FUNCTION_MARKER
  );
}

void nseel_asm_div_op(void)
{
  __asm__(
      FUNCTION_MARKER
    "fld" EEL_F_SUFFIX " (%edi)\n"
#ifdef __clang__
    "fdivp %st(1)\n"
#else
  #ifndef __GNUC__
    "fdivr\n"
  #else
    #ifdef __INTEL_COMPILER
      "fdivp %st(1)\n"
    #else
      "fdiv\n"
    #endif
  #endif
#endif
    "movl %edi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%edi)\n"

    "movl 4(%edi), %edx\n"
    "addl $0x00100000, %edx\n"
    "andl $0x7FF00000, %edx\n"
    "cmpl $0x00200000, %edx\n"
    "jg 0f\n"
      "subl %edx, %edx\n"
      "movl %edx, (%edi)\n"
      "movl %edx, 4(%edi)\n"
    "0:\n"

    FUNCTION_MARKER
  );
}

void nseel_asm_div_op_fast(void)
{
  __asm__(
      FUNCTION_MARKER
    "fld" EEL_F_SUFFIX " (%edi)\n"
#ifdef __clang__
    "fdivp %st(1)\n"
#else
  #ifndef __GNUC__
    "fdivr\n"
  #else 
    #ifdef __INTEL_COMPILER
      "fdivp %st(1)\n"
    #else
      "fdiv\n"
    #endif
  #endif
#endif
    "movl %edi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%edi)\n"

    FUNCTION_MARKER
  );
}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_mod(void)
{
  __asm__(
      FUNCTION_MARKER
    "fabs\n"
    "fistpl (%esi)\n"
    "fabs\n"
    "fistpl 4(%esi)\n"
    "xorl %edx, %edx\n"
    "cmpl $0, (%esi)\n"
    "je 0f\n" // skip devide, set return to 0
    "movl 4(%esi), %eax\n"
    "divl (%esi)\n"
    "0:\n"
    "movl %edx, (%esi)\n"
    "fildl (%esi)\n"

    FUNCTION_MARKER
  );
}

void nseel_asm_shl(void)
{
  __asm__(
      FUNCTION_MARKER
    "fistpl (%esi)\n"
    "fistpl 4(%esi)\n"
    "movl (%esi), %ecx\n"
    "movl 4(%esi), %eax\n"
    "shll %cl, %eax\n"
    "movl %eax, (%esi)\n"
    "fildl (%esi)\n"
    FUNCTION_MARKER
  );
}

void nseel_asm_shr(void)
{
  __asm__(
      FUNCTION_MARKER
    "fistpl (%esi)\n"
    "fistpl 4(%esi)\n"
    "movl (%esi), %ecx\n"
    "movl 4(%esi), %eax\n"
    "sarl %cl, %eax\n"
    "movl %eax, (%esi)\n"
    "fildl (%esi)\n"
    FUNCTION_MARKER
  );
}


void nseel_asm_mod_op(void)
{
  __asm__(
      FUNCTION_MARKER
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fxch\n"
    "fabs\n"
    "fistpl (%edi)\n"
    "fabs\n"
    "fistpl (%esi)\n"
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

    FUNCTION_MARKER
    );
}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_or(void)
{
  __asm__(
      FUNCTION_MARKER
    "fistpll (%esi)\n"
    "fistpll 8(%esi)\n"
    "movl 8(%esi), %edi\n"
    "movl 12(%esi), %ecx\n"
    "orl %edi, (%esi)\n"
    "orl %ecx, 4(%esi)\n"
    "fildll (%esi)\n"

    FUNCTION_MARKER
  );
}

void nseel_asm_or0(void)
{
  __asm__(
      FUNCTION_MARKER
    "fistpll (%esi)\n"
    "fildll (%esi)\n"
    FUNCTION_MARKER
  );
}

void nseel_asm_or_op(void)
{
  __asm__(
      FUNCTION_MARKER
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fxch\n"
    "fistpll (%edi)\n"
    "fistpll (%esi)\n"
    "movl (%esi), %eax\n"
    "movl 4(%esi), %ecx\n"
    "orl %eax, (%edi)\n"
    "orl %ecx, 4(%edi)\n"
    "fildll (%edi)\n"
    "movl %edi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%edi)\n"

    FUNCTION_MARKER
  );
}


void nseel_asm_xor(void)
{
  __asm__(
      FUNCTION_MARKER
    "fistpll (%esi)\n"
    "fistpll 8(%esi)\n"
    "movl 8(%esi), %edi\n"
    "movl 12(%esi), %ecx\n"
    "xorl %edi, (%esi)\n"
    "xorl %ecx, 4(%esi)\n"
    "fildll (%esi)\n"

    FUNCTION_MARKER
  );
}

void nseel_asm_xor_op(void)
{
  __asm__(
      FUNCTION_MARKER
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fxch\n"
    "fistpll (%edi)\n"
    "fistpll (%esi)\n"
    "movl (%esi), %eax\n"
    "movl 4(%esi), %ecx\n"
    "xorl %eax, (%edi)\n"
    "xorl %ecx, 4(%edi)\n"
    "fildll (%edi)\n"
    "movl %edi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%edi)\n"

    FUNCTION_MARKER
  );
}


//---------------------------------------------------------------------------------------------------------------
void nseel_asm_and(void)
{
  __asm__(
      FUNCTION_MARKER
    "fistpll (%esi)\n"
    "fistpll 8(%esi)\n"
    "movl 8(%esi), %edi\n"
    "movl 12(%esi), %ecx\n"
    "andl %edi, (%esi)\n"
    "andl %ecx, 4(%esi)\n"
    "fildll (%esi)\n"

    FUNCTION_MARKER
  );
}

void nseel_asm_and_op(void)
{
  __asm__(
      FUNCTION_MARKER
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fxch\n"
    "fistpll (%edi)\n"
    "fistpll (%esi)\n"
    "movl (%esi), %eax\n"
    "movl 4(%esi), %ecx\n"
    "andl %eax, (%edi)\n"
    "andl %ecx, 4(%edi)\n"
    "fildll (%edi)\n"
    "movl %edi, %eax\n"
    "fstp" EEL_F_SUFFIX " (%edi)\n"

    FUNCTION_MARKER
  );
}


//---------------------------------------------------------------------------------------------------------------
void nseel_asm_uminus(void)
{
  __asm__(
      FUNCTION_MARKER
    "fchs\n"
    FUNCTION_MARKER
  );
}



//---------------------------------------------------------------------------------------------------------------
void nseel_asm_sign(void)
{
  __asm__(
      FUNCTION_MARKER

    "fsts (%esi)\n"
    "movl (%esi), %ecx\n"
    "movl $0x7FFFFFFF, %edx\n"
    "testl %edx, %ecx\n"
    "jz 0f\n" // zero zero, return the value passed directly
      // calculate sign
      "incl %edx\n" // edx becomes 0x8000...
      "fstp %st(0)\n"
      "fld1\n"
      "testl %edx, %ecx\n"
      "jz 0f\n"
      "fchs\n"      
  	"0:\n"
   
    FUNCTION_MARKER
);
}



//---------------------------------------------------------------------------------------------------------------
void nseel_asm_bnot(void)
{
  __asm__(
      FUNCTION_MARKER
    "testl %eax, %eax\n"
    "setz %al\n"   
    "andl $0xff, %eax\n"
    FUNCTION_MARKER
  );
}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_fcall(void)
{
  __asm__(
      FUNCTION_MARKER
     "movl $0xfefefefe, %edx\n"
     "subl $12, %esp\n" /* keep stack 16 byte aligned, 4 bytes for return address */
     "call *%edx\n"
     "addl $12, %esp\n"
      FUNCTION_MARKER
  );
}

void nseel_asm_band(void)
{
  __asm__(
      FUNCTION_MARKER
    "testl %eax, %eax\n"
    "jz 0f\n"

     "movl $0xfefefefe, %ecx\n"
        "subl $12, %esp\n"
        "call *%ecx\n"
        "addl $12, %esp\n"
    "0:\n"
    FUNCTION_MARKER
  );
}

void nseel_asm_bor(void)
{
  __asm__(
      FUNCTION_MARKER
    "testl %eax, %eax\n"
    "jnz 0f\n"

    "movl $0xfefefefe, %ecx\n"
    "subl $12, %esp\n"
    "call *%ecx\n"
    "addl $12, %esp\n"
    "0:\n"
    FUNCTION_MARKER
  );
}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_equal(void)
{
  __asm__(
      FUNCTION_MARKER
#ifdef __clang__
    "fsubp %st(1)\n"
#else
    "fsub\n"
#endif

    "fabs\n"
    "fcomp" EEL_F_SUFFIX " -8(%ebx)\n" //[g_closefact]
    "fstsw %ax\n"
    "andl $256, %eax\n" // old behavior: if 256 set, true (NaN means true)

    FUNCTION_MARKER
  );
}
//
//---------------------------------------------------------------------------------------------------------------
void nseel_asm_equal_exact(void)
{
  __asm__(
      FUNCTION_MARKER
    "fcompp\n"
    "fstsw %ax\n" // for equal 256 and 1024 should be clear, 16384 should be set
    "andl $17664, %eax\n"  // mask C4/C3/C1, bits 8/10/14, 16384|256|1024 -- if equals 16384, then equality
    "cmp $16384, %eax\n" 
    "je 0f\n"
    "subl %eax, %eax\n"
    "0:\n"
    FUNCTION_MARKER
  );
}

void nseel_asm_notequal_exact(void)
{
  __asm__(
      FUNCTION_MARKER
    "fcompp\n"
    "fstsw %ax\n" // for equal 256 and 1024 should be clear, 16384 should be set
    "andl $17664, %eax\n"  // mask C4/C3/C1, bits 8/10/14, 16384|256|1024 -- if equals 16384, then equality
    "cmp $16384, %eax\n" 
    "je 0f\n"
    "subl %eax, %eax\n"
    "0:\n"
    "xorl $16384, %eax\n" // flip the result
    FUNCTION_MARKER
  );
}
//
//---------------------------------------------------------------------------------------------------------------
void nseel_asm_notequal(void)
{
  __asm__(
      FUNCTION_MARKER
#ifdef __clang__
    "fsubp %st(1)\n"
#else
    "fsub\n"
#endif

    "fabs\n"
    "fcomp" EEL_F_SUFFIX " -8(%ebx)\n" //[g_closefact]
    "fstsw %ax\n"
    "andl $256, %eax\n"
    "xorl $256, %eax\n" // old behavior: if 256 set, FALSE (NaN makes for false)
    FUNCTION_MARKER
  );
}


//---------------------------------------------------------------------------------------------------------------
void nseel_asm_above(void)
{
  __asm__(
      FUNCTION_MARKER
    "fcompp\n"
    "fstsw %ax\n"
    "andl $1280, %eax\n" //  (1024+256) old behavior: NaN would mean 1, preserve that
    FUNCTION_MARKER
  );
}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_beloweq(void)
{
  __asm__(
      FUNCTION_MARKER
    "fcompp\n"
    "fstsw %ax\n"
    "andl $256, %eax\n" // old behavior: NaN would be 0 (ugh)
    "xorl $256, %eax\n"
    FUNCTION_MARKER
  );
}


void nseel_asm_booltofp(void)
{
  __asm__(
      FUNCTION_MARKER
    "testl %eax, %eax\n"
    "jz 0f\n"
    "fld1\n"
    "jmp 1f\n"
    "0:\n"
    "fldz\n"
    "1:\n"
    FUNCTION_MARKER
  );
}

void nseel_asm_fptobool(void)
{
  __asm__(
      FUNCTION_MARKER
    "fabs\n"
    "fcomp" EEL_F_SUFFIX " -8(%ebx)\n" //[g_closefact]
    "fstsw %ax\n"
    "andl $256, %eax\n"
    "xorl $256, %eax\n"
    FUNCTION_MARKER
  );
}

void nseel_asm_fptobool_rev(void)
{
  __asm__(
      FUNCTION_MARKER
    "fabs\n"
    "fcomp" EEL_F_SUFFIX " -8(%ebx)\n" //[g_closefact]
    "fstsw %ax\n"
    "andl $256, %eax\n"
    FUNCTION_MARKER
  );
}

void nseel_asm_min(void)
{
  __asm__(
      FUNCTION_MARKER
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fcomp" EEL_F_SUFFIX " (%eax)\n"
    "movl %eax, %ecx\n"
    "fstsw %ax\n"
    "testl $256, %eax\n"
    "movl %ecx, %eax\n"
    "jz 0f\n"
    "movl %edi, %eax\n"
    "0:\n"
    FUNCTION_MARKER
    );

}

void nseel_asm_max(void)
{
  __asm__(
      FUNCTION_MARKER
    "fld" EEL_F_SUFFIX " (%edi)\n"
    "fcomp" EEL_F_SUFFIX " (%eax)\n"
    "movl %eax, %ecx\n"
    "fstsw %ax\n"
    "testl $256, %eax\n"
    "movl %ecx, %eax\n"
    "jnz 0f\n"
    "movl %edi, %eax\n"
    "0:\n"
    FUNCTION_MARKER
    );
}



void nseel_asm_min_fp(void)
{
  __asm__(
      FUNCTION_MARKER
    "fcom\n"
    "fstsw %ax\n"
    "testl $256, %eax\n"
    "jz 0f\n"
    "fxch\n"
    "0:\n"
    "fstp %st(0)\n"
    FUNCTION_MARKER
    );

}

void nseel_asm_max_fp(void)
{
  __asm__(
      FUNCTION_MARKER
    "fcom\n"
    "fstsw %ax\n"
    "testl $256, %eax\n"
    "jnz 0f\n"
    "fxch\n"
    "0:\n"
    "fstp %st(0)\n"
    FUNCTION_MARKER
    );
}



// just generic functions left, yay




void _asm_generic3parm(void)
{
  __asm__(
      FUNCTION_MARKER
    
    "movl $0xfefefefe, %edx\n"
    "pushl %eax\n" // push parameter
    "pushl %edi\n" // push parameter
    "movl $0xfefefefe, %edi\n"
    "pushl %ecx\n" // push parameter
    "pushl %edx\n" // push context pointer
    "call *%edi\n"
    "addl $16, %esp\n"
    
    FUNCTION_MARKER
 );
}


void _asm_generic3parm_retd(void)
{
  __asm__(
      FUNCTION_MARKER
    
    "subl $16, %esp\n"
    "movl $0xfefefefe, %edx\n"
    "movl %edi, 8(%esp)\n"
    "movl $0xfefefefe, %edi\n"
    "movl %eax, 12(%esp)\n"
    "movl %ecx, 4(%esp)\n"
    "movl %edx, (%esp)\n"
    "call *%edi\n"
    "addl $16, %esp\n"
    
    FUNCTION_MARKER
 );
}


void _asm_generic2parm(void) // this prob neds to be fixed for ppc
{
  __asm__(
      FUNCTION_MARKER
    
    "movl $0xfefefefe, %edx\n"
    "movl $0xfefefefe, %ecx\n"
    "subl $4, %esp\n" // keep stack aligned
    "pushl %eax\n" // push parameter
    "pushl %edi\n" // push parameter
    "pushl %edx\n" // push context pointer
    "call *%ecx\n"
    "addl $16, %esp\n"
    
    FUNCTION_MARKER
 );
}


void _asm_generic2parm_retd(void)
{
  __asm__(
      FUNCTION_MARKER
    
    "subl $16, %esp\n"
    "movl $0xfefefefe, %edx\n"
    "movl $0xfefefefe, %ecx\n"
    "movl %edx, (%esp)\n"
    "movl %edi, 4(%esp)\n"
    "movl %eax, 8(%esp)\n"
    "call *%ecx\n"
    "addl $16, %esp\n"
    
    FUNCTION_MARKER
 );
}


void _asm_generic2xparm_retd(void)
{
  __asm__(
      FUNCTION_MARKER
    "subl $16, %esp\n"
    "movl $0xfefefefe, %edx\n" // first parameter
    "movl %edx, (%esp)\n"
    "movl %edi, 8(%esp)\n"
    "movl $0xfefefefe, %edx\n"
    "movl $0xfefefefe, %ecx\n" // function
    "movl %edx, 4(%esp)\n"
    "movl %eax, 12(%esp)\n"
    "call *%ecx\n"
    "addl $16, %esp\n"
    FUNCTION_MARKER
 );
}



void _asm_generic1parm(void) 
{
  __asm__(
      FUNCTION_MARKER
    
    "movl $0xfefefefe, %edx\n"
    "subl $8, %esp\n" // keep stack aligned
    "movl $0xfefefefe, %ecx\n"
    "pushl %eax\n" // push parameter
    "pushl %edx\n" // push context pointer
    "call *%ecx\n"
    "addl $16, %esp\n"
    
    FUNCTION_MARKER
 );
}


void _asm_generic1parm_retd(void) // 1 parameter returning double
{
  __asm__(
      FUNCTION_MARKER
    
    "movl $0xfefefefe, %edx\n" // context pointer
    "movl $0xfefefefe, %ecx\n" // func-addr
    "subl $16, %esp\n"
    "movl %eax, 4(%esp)\n" // push parameter
    "movl %edx, (%esp)\n" // push context pointer
    "call *%ecx\n"
    "addl $16, %esp\n"
    
    FUNCTION_MARKER
 );
}





// this gets its own stub because it's pretty crucial for performance :/

void _asm_megabuf(void)
{
  __asm__(

      FUNCTION_MARKER

    "fadd" EEL_F_SUFFIX " -8(%%ebx)\n"
    "fistpl (%%esi)\n"

    // check if (%esi) is in range, and buffer available, otherwise call function
    "movl (%%esi), %%edi\n"
    "cmpl %0, %%edi\n"     //REPLACE=((NSEEL_RAM_BLOCKS*NSEEL_RAM_ITEMSPERBLOCK))
    "jae 0f\n"

      "movl %%edi, %%eax\n"
      "shrl %1, %%eax\n"      //REPLACE=(NSEEL_RAM_ITEMSPERBLOCK_LOG2 - 2/*log2(sizeof(void *))*/   )
      "andl %2, %%eax\n"      //REPLACE=((NSEEL_RAM_BLOCKS-1)*4 /*sizeof(void*)*/                   )
      "movl (%%ebx, %%eax), %%eax\n"
      "testl %%eax, %%eax\n"
      "jnz 1f\n"
    "0:\n"
      "subl $8, %%esp\n" // keep stack aligned
      "movl $0xfefefefe, %%ecx\n"
      "pushl %%edi\n" // parameter
      "pushl %%ebx\n" // push context pointer
      "call *%%ecx\n"
      "addl $16, %%esp\n"
      "jmp 2f\n"
    "1:\n"
      "andl %3, %%edi\n"      //REPLACE=(NSEEL_RAM_ITEMSPERBLOCK-1)
      "shll $3, %%edi\n"      // 3 is log2(sizeof(EEL_F))
      "addl %%edi, %%eax\n"
    "2:"
    FUNCTION_MARKER

    #ifndef _MSC_VER
        :: "i" (((NSEEL_RAM_BLOCKS*NSEEL_RAM_ITEMSPERBLOCK))),
           "i" ((NSEEL_RAM_ITEMSPERBLOCK_LOG2 - 2/*log2(sizeof(void *))*/   )),
           "i" (((NSEEL_RAM_BLOCKS-1)*4 /*sizeof(void*)*/                   )),
           "i" ((NSEEL_RAM_ITEMSPERBLOCK-1                                  ))
    #endif

  );
}



void _asm_gmegabuf(void)
{
  __asm__(

      FUNCTION_MARKER

    "subl $16, %esp\n" // keep stack aligned
    "movl $0xfefefefe, (%esp)\n"
    "fadd" EEL_F_SUFFIX " -8(%ebx)\n"
    "movl $0xfefefefe, %edi\n"
    "fistpl 4(%esp)\n"
    "call *%edi\n"
    "addl $16, %esp\n"

    FUNCTION_MARKER
 );
}


void nseel_asm_stack_push(void)
{

  __asm__(
      FUNCTION_MARKER
    "movl $0xfefefefe, %edi\n"
    
    "movl (%eax), %ecx\n"
    "movl 4(%eax), %edx\n"

    "movl (%edi), %eax\n"

    "addl $8, %eax\n"
    "andl $0xfefefefe, %eax\n"
    "orl $0xfefefefe, %eax\n"
    
    "movl %ecx, (%eax)\n"
    "movl %edx, 4(%eax)\n"

    "movl %eax, (%edi)\n"
    FUNCTION_MARKER
  );

}



void nseel_asm_stack_pop(void)
{
  __asm__(
      FUNCTION_MARKER
    "movl $0xfefefefe, %edi\n"
    "movl (%edi), %ecx\n"
    "fld" EEL_F_SUFFIX  " (%ecx)\n"
    "subl $8, %ecx\n"
    "andl $0xfefefefe, %ecx\n"
    "orl $0xfefefefe, %ecx\n"
    "movl %ecx, (%edi)\n"
    "fstp" EEL_F_SUFFIX " (%eax)\n"
    FUNCTION_MARKER
  );
}


void nseel_asm_stack_pop_fast(void)
{
  __asm__(
      FUNCTION_MARKER
    "movl $0xfefefefe, %edi\n"
    "movl (%edi), %ecx\n"
    "movl %ecx, %eax\n"
    "subl $8, %ecx\n"
    "andl $0xfefefefe, %ecx\n"
    "orl $0xfefefefe, %ecx\n"
    "movl %ecx, (%edi)\n"        
    FUNCTION_MARKER
  );
}

void nseel_asm_stack_peek_int(void)
{
  __asm__(
      FUNCTION_MARKER
    "movl $0xfefefefe, %edi\n"
    "movl (%edi), %eax\n"   
    "movl $0xfefefefe, %edx\n"
    "subl %edx, %eax\n"
    "andl $0xfefefefe, %eax\n"
    "orl $0xfefefefe, %eax\n"
    FUNCTION_MARKER
  );
}



void nseel_asm_stack_peek(void)
{
  __asm__(
      FUNCTION_MARKER
    "movl $0xfefefefe, %edi\n"
    "fistpl (%esi)\n"
    "movl (%edi), %eax\n"   
    "movl (%esi), %edx\n"
    "shll $3, %edx\n" // log2(sizeof(EEL_F))
    "subl %edx, %eax\n"
    "andl $0xfefefefe, %eax\n"
    "orl $0xfefefefe, %eax\n"
    FUNCTION_MARKER
  );
}


void nseel_asm_stack_peek_top(void)
{
  __asm__(
      FUNCTION_MARKER
    "movl $0xfefefefe, %edi\n"
    "movl (%edi), %eax\n"   
    FUNCTION_MARKER
  );

}

void nseel_asm_stack_exch(void)
{
  __asm__(
      FUNCTION_MARKER
    "movl $0xfefefefe, %edi\n"
    "movl (%edi), %ecx\n"   
    "fld" EEL_F_SUFFIX  " (%ecx)\n"
    "fld" EEL_F_SUFFIX  " (%eax)\n"
    "fstp" EEL_F_SUFFIX  " (%ecx)\n"
    "fstp" EEL_F_SUFFIX " (%eax)\n"
    FUNCTION_MARKER
  );

}

