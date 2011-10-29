#if EEL_F_SIZE == 8

void nseel_asm_1pdd(void)
{

  __asm__( 
    "addis r5, 0, 0xdead\n" 
    "ori r5, r5, 0xbeef\n"  
    "lfd f1, 0(r3)\n" 
    "mtctr r5\n" 
    "subi r1, r1, 64\n" 
    "bctrl\n" 
    "addi r1, r1, 64\n" 
    "stfdu f1, 8(r16)\n" 
    "mr r3, r16\n" 
   :: );
}
void nseel_asm_1pdd_end(void){}

void nseel_asm_2pdd(void)
{

  __asm__( 
    "addis r7, 0, 0xdead\n" 
    "ori r7, r7, 0xbeef\n"  
    "lfd f2, 0(r3)\n" 
    "lfd f1, 0(r14)\n" 
    "mtctr r7\n" 
    "subi r1, r1, 64\n" 
    "bctrl\n" 
    "addi r1, r1, 64\n" 
    "stfdu f1, 8(r16)\n" 
    "mr r3, r16\n" 
   :: );
};
void nseel_asm_2pdd_end(void){}

void nseel_asm_2pdds(void)
{
  __asm__( 
    "addis r5, 0, 0xdead\n" 
    "ori r5, r5, 0xbeef\n"  
    "lfd f2, 0(r3)\n" 
    "lfd f1, 0(r14)\n" 
    "mtctr r5\n" 
    "subi r1, r1, 64\n" 
    "bctrl\n" 
    "addi r1, r1, 64\n" 
    "stfd f1, 0(r14)\n" 
    "mr r3, r14\n" 
   :: );
}
void nseel_asm_2pdds_end(void){}

#else // 32 bit floating point calls

#error mac only can do 64 bit floats for now

#endif


void nseel_asm_2pp(void)
{
// r3=firstparm, r4=second parm, returns in f1
  __asm__( 
    "addis r5, 0, 0xdead\n" 
    "ori r5, r5, 0xbeef\n"  
    "mtctr r5\n" 
    "mr r4, r3\n" 
    "mr r3, r14\n" 
    "subi r1, r1, 64\n" 
    "bctrl\n" 
    "addi r1, r1, 64\n" 
    "stfdu f1, 8(r16)\n" 
    "mr r3, r16\n" 
   :: );
};
void nseel_asm_2pp_end(void){}

void nseel_asm_1pp(void)
{
  __asm__( 
    "addis r5, 0, 0xdead\n" 
    "ori r5, r5, 0xbeef\n"  
    "mtctr r5\n" 
    "subi r1, r1, 64\n" 
    "bctrl\n" 
    "addi r1, r1, 64\n" 
    "stfdu f1, 8(r16)\n" 
    "mr r3, r16\n" 
   :: );
};
void nseel_asm_1pp_end(void){}


//---------------------------------------------------------------------------------------------------------------



// do nothing, eh
void nseel_asm_exec2(void)
{
}
void nseel_asm_exec2_end(void) { }



void nseel_asm_invsqrt(void)
{
  __asm__(
   "lfd f1, 0(r3)\n"
   "frsqrte f1, f1\n" // less accurate than our x86 equivilent, but invsqrt() is inherently inaccurate anyway
   "stfdu f1, 8(r16)\n"
   "mr r3, r16\n"
  );
}
void nseel_asm_invsqrt_end(void) {}



//---------------------------------------------------------------------------------------------------------------
void nseel_asm_sqr(void)
{
  __asm__(
   "lfd f1, 0(r3)\n"
   "fmul f1, f1, f1\n"
   "stfdu f1, 8(r16)\n"
   "mr r3, r16\n"
  );
}
void nseel_asm_sqr_end(void) {}


//---------------------------------------------------------------------------------------------------------------
void nseel_asm_abs(void)
{
  __asm__(
   "lfd f1, 0(r3)\n"
   "fabs f1, f1\n"
   "stfdu f1, 8(r16)\n"
   "mr r3, r16\n"
  );
}
void nseel_asm_abs_end(void) {}


//---------------------------------------------------------------------------------------------------------------
void nseel_asm_assign(void)
{
  __asm__(
   "lfd f1, 0(r3)\n"
   "stfd f1, 0(r14)\n"
  );
}
void nseel_asm_assign_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_add(void)
{
  __asm__(
   "lfd f1, 0(r3)\n"
   "lfd f2, 0(r14)\n"
   "fadd f1, f1, f2\n"
   "stfdu f1, 8(r16)\n"
   "mr r3, r16\n"
  );
}
void nseel_asm_add_end(void) {}

void nseel_asm_add_op(void)
{
  __asm__(
   "lfd f1, 0(r3)\n"
   "lfd f2, 0(r14)\n"
   "fadd f1, f1, f2\n"
   "stfd f1, 0(r14)\n"
   "mr r3, r14\n"
  );
}
void nseel_asm_add_op_end(void) {}


//---------------------------------------------------------------------------------------------------------------
void nseel_asm_sub(void)
{
  __asm__(
   "lfd f1, 0(r3)\n"
   "lfd f2, 0(r14)\n"
   "fsub f1, f2, f1\n"
   "stfdu f1, 8(r16)\n"
   "mr r3, r16\n"
  );
}
void nseel_asm_sub_end(void) {}

void nseel_asm_sub_op(void)
{
  __asm__(
   "lfd f1, 0(r3)\n"
   "lfd f2, 0(r14)\n"
   "fsub f1, f2, f1\n"
   "stfd f1, 0(r14)\n"
   "mr r3, r14\n"
  );
}
void nseel_asm_sub_op_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_mul(void)
{
  __asm__(
   "lfd f1, 0(r3)\n"
   "lfd f2, 0(r14)\n"
   "fmul f1, f2, f1\n"
   "stfdu f1, 8(r16)\n"
   "mr r3, r16\n"
  );
}
void nseel_asm_mul_end(void) {}

void nseel_asm_mul_op(void)
{
  __asm__(
   "lfd f1, 0(r3)\n"
   "lfd f2, 0(r14)\n"
   "fmul f1, f2, f1\n"
   "stfd f1, 0(r14)\n"
   "mr r3, r14\n"
  );
}
void nseel_asm_mul_op_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_div(void)
{
  __asm__(
   "lfd f1, 0(r3)\n"
   "lfd f2, 0(r14)\n"
   "fdiv f1, f2, f1\n"
   "stfdu f1, 8(r16)\n"
   "mr r3, r16\n"
  );
}
void nseel_asm_div_end(void) {}

void nseel_asm_div_op(void)
{
  __asm__(
   "lfd f1, 0(r3)\n"
   "lfd f2, 0(r14)\n"
   "fdiv f1, f2, f1\n"
   "stfd f1, 0(r14)\n"
   "mr r3, r14\n"
  );
}
void nseel_asm_div_op_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_mod(void)
{
  __asm__(

   "lfd f1, 0(r3)\n"
   "lfd f2, 0(r14)\n"
   "fabs f1, f1\n"
   "fabs f2, f2\n"
   "fctiwz f1, f1\n"
   "fctiwz f2, f2\n"
   "stfd f1, 8(r16)\n"
   "stfd f2, 16(r16)\n"
   "lwz r10, 12(r16)\n"
   "lwz r11, 20(r16)\n" //r11 and r12 have the integers

   "divw r12, r11, r10\n"
   "mullw r12, r12, r10\n"
   "subf r10, r12, r11\n"

   "addis r11, 0, 0x4330\n"
   "addis r12, 0, 0x8000\n"
   "xoris r10, r10, 0x8000\n"
   "stw r11, 8(r16)\n"   // 0x43300000
   "stw r10, 12(r16)\n"  // our integer sign flipped
   "stw r11, 16(r16)\n"  // 0x43300000
   "stw r12, 20(r16)\n"  // 0x80000000
   "lfd f1, 8(r16)\n"
   "lfd f2, 16(r16)\n"
   "fsub f1, f1, f2\n"
   "stfdu f1, 8(r16)\n"
   "mr r3, r16\n"


  );
}
void nseel_asm_mod_end(void) {}

void nseel_asm_shl(void)
{
  __asm__(
   "lfd f1, 0(r3)\n"
   "lfd f2, 0(r14)\n"
   "fctiwz f1, f1\n"
   "fctiwz f2, f2\n"
   "stfd f1, 8(r16)\n"
   "stfd f2, 16(r16)\n"
   "lwz r10, 12(r16)\n"
   "lwz r11, 20(r16)\n" //r11 and r12 have the integers
   "slw r10, r11, r10\n" // r10 has the result
   "addis r11, 0, 0x4330\n"
   "addis r12, 0, 0x8000\n"
   "xoris r10, r10, 0x8000\n"
   "stw r11, 8(r16)\n"   // 0x43300000
   "stw r10, 12(r16)\n"  // our integer sign flipped
   "stw r11, 16(r16)\n"  // 0x43300000
   "stw r12, 20(r16)\n"  // 0x80000000
   "lfd f1, 8(r16)\n"
   "lfd f2, 16(r16)\n"
   "fsub f1, f1, f2\n"
   "stfdu f1, 8(r16)\n"
   "mr r3, r16\n"
  );
}
void nseel_asm_shl_end(void) {}

void nseel_asm_shr(void)
{
  __asm__(
   "lfd f1, 0(r3)\n"
   "lfd f2, 0(r14)\n"
   "fctiwz f1, f1\n"
   "fctiwz f2, f2\n"
   "stfd f1, 8(r16)\n"
   "stfd f2, 16(r16)\n"
   "lwz r10, 12(r16)\n"
   "lwz r11, 20(r16)\n" //r11 and r12 have the integers
   "sraw r10, r11, r10\n" // r10 has the result
   "addis r11, 0, 0x4330\n"
   "addis r12, 0, 0x8000\n"
   "xoris r10, r10, 0x8000\n"
   "stw r11, 8(r16)\n"   // 0x43300000
   "stw r10, 12(r16)\n"  // our integer sign flipped
   "stw r11, 16(r16)\n"  // 0x43300000
   "stw r12, 20(r16)\n"  // 0x80000000
   "lfd f1, 8(r16)\n"
   "lfd f2, 16(r16)\n"
   "fsub f1, f1, f2\n"
   "stfdu f1, 8(r16)\n"
   "mr r3, r16\n"
  );
}
void nseel_asm_shr_end(void) {}

void nseel_asm_mod_op(void)
{

  __asm__(

   "lfd f1, 0(r3)\n"
   "lfd f2, 0(r14)\n"
   "fabs f1, f1\n"
   "fabs f2, f2\n"
   "fctiwz f1, f1\n"
   "fctiwz f2, f2\n"
   "stfd f1, 8(r16)\n"
   "stfd f2, 16(r16)\n"
   "lwz r10, 12(r16)\n"
   "lwz r11, 20(r16)\n" //r11 and r12 have the integers

   "divw r12, r11, r10\n"
   "mullw r12, r12, r10\n"
   "subf r10, r12, r11\n"

   "addis r11, 0, 0x4330\n"
   "addis r12, 0, 0x8000\n"
   "xoris r10, r10, 0x8000\n"
   "stw r11, 8(r16)\n"   // 0x43300000
   "stw r10, 12(r16)\n"  // our integer sign flipped
   "stw r11, 16(r16)\n"  // 0x43300000
   "stw r12, 20(r16)\n"  // 0x80000000
   "lfd f1, 8(r16)\n"
   "lfd f2, 16(r16)\n"
   "fsub f1, f1, f2\n"
   "stfd f1, 0(r14)\n"
   "mr r3, r14\n"
  );

}
void nseel_asm_mod_op_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_or(void)
{
  __asm__(
   "lfd f1, 0(r3)\n"
   "lfd f2, 0(r14)\n"
   "fctiwz f1, f1\n"
   "fctiwz f2, f2\n"
   "stfd f1, 8(r16)\n"
   "stfd f2, 16(r16)\n"
   "lwz r10, 12(r16)\n"
   "lwz r11, 20(r16)\n" //r11 and r12 have the integers
   "or r10, r10, r11\n" // r10 has the result
   "addis r11, 0, 0x4330\n"
   "addis r12, 0, 0x8000\n"
   "xoris r10, r10, 0x8000\n"
   "stw r11, 8(r16)\n"   // 0x43300000
   "stw r10, 12(r16)\n"  // our integer sign flipped
   "stw r11, 16(r16)\n"  // 0x43300000
   "stw r12, 20(r16)\n"  // 0x80000000
   "lfd f1, 8(r16)\n"
   "lfd f2, 16(r16)\n"
   "fsub f1, f1, f2\n"
   "stfdu f1, 8(r16)\n"
   "mr r3, r16\n"
  );
}
void nseel_asm_or_end(void) {}

void nseel_asm_or_op(void)
{
  __asm__(
   "lfd f1, 0(r3)\n"
   "lfd f2, 0(r14)\n"
   "fctiwz f1, f1\n"
   "fctiwz f2, f2\n"
   "stfd f1, 8(r16)\n"
   "stfd f2, 16(r16)\n"
   "lwz r10, 12(r16)\n"
   "lwz r11, 20(r16)\n" //r11 and r12 have the integers
   "or r10, r10, r11\n" // r10 has the result
   "addis r11, 0, 0x4330\n"
   "addis r12, 0, 0x8000\n"
   "xoris r10, r10, 0x8000\n"
   "stw r11, 8(r16)\n"   // 0x43300000
   "stw r10, 12(r16)\n"  // our integer sign flipped
   "stw r11, 16(r16)\n"  // 0x43300000
   "stw r12, 20(r16)\n"  // 0x80000000
   "lfd f1, 8(r16)\n"
   "lfd f2, 16(r16)\n"
   "fsub f1, f1, f2\n"
   "stfd f1, 0(r14)\n"
   "mr r3, r14\n"
  );
}
void nseel_asm_or_op_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_and(void)
{
  __asm__(
   "lfd f1, 0(r3)\n"
   "lfd f2, 0(r14)\n"
   "fctiwz f1, f1\n"
   "fctiwz f2, f2\n"
   "stfd f1, 8(r16)\n"
   "stfd f2, 16(r16)\n"
   "lwz r10, 12(r16)\n"
   "lwz r11, 20(r16)\n" //r11 and r12 have the integers
   "and r10, r10, r11\n" // r10 has the result
   "addis r11, 0, 0x4330\n"
   "addis r12, 0, 0x8000\n"
   "xoris r10, r10, 0x8000\n"
   "stw r11, 8(r16)\n"   // 0x43300000
   "stw r10, 12(r16)\n"  // our integer sign flipped
   "stw r11, 16(r16)\n"  // 0x43300000
   "stw r12, 20(r16)\n"  // 0x80000000
   "lfd f1, 8(r16)\n"
   "lfd f2, 16(r16)\n"
   "fsub f1, f1, f2\n"
   "stfdu f1, 8(r16)\n"
   "mr r3, r16\n"
  );}
void nseel_asm_and_end(void) {}

void nseel_asm_and_op(void)
{
  __asm__(
   "lfd f1, 0(r3)\n"
   "lfd f2, 0(r14)\n"
   "fctiwz f1, f1\n"
   "fctiwz f2, f2\n"
   "stfd f1, 8(r16)\n"
   "stfd f2, 16(r16)\n"
   "lwz r10, 12(r16)\n"
   "lwz r11, 20(r16)\n" //r11 and r12 have the integers
   "and r10, r10, r11\n" // r10 has the result
   "addis r11, 0, 0x4330\n"
   "addis r12, 0, 0x8000\n"
   "xoris r10, r10, 0x8000\n"
   "stw r11, 8(r16)\n"   // 0x43300000
   "stw r10, 12(r16)\n"  // our integer sign flipped
   "stw r11, 16(r16)\n"  // 0x43300000
   "stw r12, 20(r16)\n"  // 0x80000000
   "lfd f1, 8(r16)\n"
   "lfd f2, 16(r16)\n"
   "fsub f1, f1, f2\n"
   "stfd f1, 0(r14)\n"
   "mr r3, r14\n"
  );
}
void nseel_asm_and_op_end(void) {}


//---------------------------------------------------------------------------------------------------------------
void nseel_asm_uplus(void) // this is the same as doing nothing, it seems
{
}
void nseel_asm_uplus_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_uminus(void)
{
  __asm__(
   "lfd f1, 0(r3)\n"
   "fneg f1, f1\n"
   "stfdu f1, 8(r16)\n"
   "mr r3, r16\n"
  );
}
void nseel_asm_uminus_end(void) {}


//---------------------------------------------------------------------------------------------------------------
void nseel_asm_sign(void)
{
  __asm__(
    "lfd f1, 0(r3)\n"
    "addis r5, 0, 0xdead\n"
    "ori r5, r5, 0xbeef\n"
    "lfd f2, 0(r5)\n"
    "lis r9, 0xbff0\n"
    "fcmpu cr7, f1, f2\n"
    "blt- cr7, 0f\n"
    "ble- cr7, 1f\n"
    "  lis r9, 0x3ff0\n"
    "0:\n"
    "  li r10, 0\n"
    "  stwu r9, 8(r16)\n"
    "  stw  r10, 4(r16)\n"
    "  b 2f\n"
    "1:\n"
    "  stfdu f1, 8(r16)\n"
    "2:\n"
    "  mr r3, r16\n"
    :: 
  );
}
void nseel_asm_sign_end(void) {}



//---------------------------------------------------------------------------------------------------------------
void nseel_asm_bnot(void)
{
  __asm__(
    "addis r5, 0, 0xdead\n"
    "ori r5, r5, 0xbeef\n"
    "lfd f2, 0(r5)\n"
    "lfd f1, 0(r3)\n"
    "fabs f1, f1\n"
    "fcmpu cr7, f1, f2\n"
    "blt cr7, 0f\n"
    "addis r5, 0, 0xdead\n"
    "ori r5, r5, 0xbeef\n"
    "lfd f1, 0(r5)\n"
    "b 1f\n"
    "0:\n"
    "addis r5, 0, 0xdead\n"
    "ori r5, r5, 0xbeef\n"
    "lfd f1, 0(r5)\n"
    "1:\n"
    "  stfdu f1, 8(r16)\n"
    "  mr r3, r16\n"
    :: 
  );
}
void nseel_asm_bnot_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_if(void)
{
  __asm__(
   "addis r5, 0, 0xdead\n"
   "ori r5, r5, 0xbeef\n"
   "lfd f2, 0(r5)\n"
   "lfd f1, 0(r3)\n"
   "addis r6, 0, 0xdead\n"
   "ori r6, r6, 0xbeef\n"
   "addis r7, 0, 0xdead\n"
   "ori r7, r7, 0xbeef\n"
   "fabs f1, f1\n"
   "fcmpu cr7, f1, f2\n"
   "blt cr7, 0f\n"
   "  mtctr r6\n"
   "b 1f\n"
   "0:\n"
   "  mtctr r7\n"
   "1:\n"
   "bctrl\n"
  :: );
}
void nseel_asm_if_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_repeat(void)
{
  __asm__(
   "addis r6, 0, 0xdead\n"
   "ori r6, r6, 0xbeef\n"
   "addis r7, 0, ha16(%0)\n"
   "addi r7, r7, lo16(%0)\n"
   "lfd f1, 0(r3)\n"
   "fctiwz f1, f1\n"
   "stfd f1, 8(r16)\n"
   "lwz r5, 12(r16)\n" // r5 has count now
   "cmpwi cr0, r5, 0\n"
   "ble cr0, 1f\n"
   "cmpw cr0, r7, r5\n"
   "bge cr0, 0f\n"
   "mr r5, r7\n" // set r5 to max if we have to
"0:\n"
   "stw r5, -4(r1)\n"
   "stw r6, -8(r1)\n"
   "stwu r16, -12(r1)\n"

   "mtctr r6\n"
   "bctrl\n"

   "lwz r16, 0(r1)\n"
   "lwz r6, 4(r1)\n"
   "lwz r5, 8(r1)\n"
   "addi r1, r1, 12\n"
   "addi r5, r5, -1\n"

   "cmpwi cr0, r5, 0\n"
   "bgt cr0, 0b\n"
   "1:\n"
    ::"g" (NSEEL_LOOPFUNC_SUPPORT_MAXLEN)
  );
}
void nseel_asm_repeat_end(void) {}

void nseel_asm_repeatwhile(void)
{
  __asm__(
   "addis r6, 0, 0xdead\n"
   "ori r6, r6, 0xbeef\n"
   "addis r5, 0, ha16(%0)\n"
   "addi r5, r5, lo16(%0)\n"
"0:\n"
   "stw r5, -4(r1)\n"
   "stw r6, -8(r1)\n"
   "stwu r16, -12(r1)\n"

   "mtctr r6\n"
   "bctrl\n"

   "lwz r16, 0(r1)\n"
   "lwz r6, 4(r1)\n"
   "lwz r5, 8(r1)\n"
   "addi r1, r1, 12\n"
   "addi r5, r5, -1\n"

   "addis r7, 0, 0xdead\n"
   "ori r7, r7, 0xbeef\n"
   "lfd f2, 0(r7)\n"

   "lfd f1, 0(r3)\n"
   "fabs f1, f1\n"
   "fcmpu cr7, f1, f2\n"
   "blt cr7, 1f\n"

   "cmpwi cr0, r5, 0\n"
   "bgt cr0, 0b\n"
   "1:\n"
    ::"g" (NSEEL_LOOPFUNC_SUPPORT_MAXLEN)
  );
}
void nseel_asm_repeatwhile_end(void) {}


void nseel_asm_band(void)
{
  __asm__(

   "addis r5, 0, 0xdead\n"
   "ori r5, r5, 0xbeef\n"
   "lfd f2, 0(r5)\n"
   "lfd f1, 0(r3)\n"
   "fabs f1, f1\n"
   "fcmpu cr7, f1, f2\n"
   "blt cr7, 0f\n"
   "addis r6, 0, 0xdead\n"
   "ori r6, r6, 0xbeef\n"
   "  mtctr r6\n"
   "  bctrl\n"
   "  addis r5, 0, 0xdead\n"
   "  ori r5, r5, 0xbeef\n"
   "  lfd f2, 0(r5)\n"
   "  lfd f1, 0(r3)\n"
   "  fabs f1, f1\n"
   "  fcmpu cr7, f1, f2\n"
   "  bge cr7, 1f\n"
   "0:\n"
   "  fsub f1, f1, f1\n" // set f1 to 0!
   "  b 2f\n"
   "1:\n"
   "  addis r5, 0, 0xdead\n" // set f1 to 1
   "  ori r5, r5, 0xbeef\n"
   "  lfd f1, 0(r5)\n"
   "2:\n"
   "stfdu f1, 8(r16)\n"
   "mr r3, r16\n"
  :: );
}
void nseel_asm_band_end(void) {}

void nseel_asm_bor(void)
{
  __asm__(
   "addis r5, 0, 0xdead\n"
   "ori r5, r5, 0xbeef\n"
   "lfd f2, 0(r5)\n"
   "lfd f1, 0(r3)\n"
   "fabs f1, f1\n"
   "fcmpu cr7, f1, f2\n"
   "bge cr7, 0f\n"
   "addis r6, 0, 0xdead\n"
   "ori r6, r6, 0xbeef\n"
   "  mtctr r6\n"
   "  bctrl\n"
   "  addis r5, 0, 0xdead\n"
   "  ori r5, r5, 0xbeef\n"
   "  lfd f2, 0(r5)\n"
   "  lfd f1, 0(r3)\n"
   "  fabs f1, f1\n"
   "  fcmpu cr7, f1, f2\n"
   "  blt cr7, 1f\n"
   "0:\n"
   "  addis r5, 0, 0xdead\n" // set f1 to 1
   "  ori r5, r5, 0xbeef\n"
   "  lfd f1, 0(r5)\n"
   "  b 2f\n"
   "1:\n"
   "  fsub f1, f1, f1\n" // set f1 to 0!
   "2:\n"
   "stfdu f1, 8(r16)\n"
   "mr r3, r16\n"
  :: );
}
void nseel_asm_bor_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_equal(void)
{
  __asm__(
    "lfd f1, 0(r3)\n"
    "lfd f2, 0(r14)\n"
    "addis r5, 0, 0xdead\n"
    "ori r5, r5, 0xbeef\n"
    "fsub f1, f1, f2\n"
    "fabs f1, f1\n"
    "lfd f2, 0(r5)\n"
    "fcmpu cr7, f1, f2\n"
    "blt cr7, 0f\n"
    "addis r5, 0, 0xdead\n"
    "ori r5, r5, 0xbeef\n"
    "b 1f\n"
    "0:\n"
    "addis r5, 0, 0xdead\n"
    "ori r5, r5, 0xbeef\n"
    "1:\n"
    "lfd f1, 0(r5)\n"
    "  stfdu f1, 8(r16)\n"
    "  mr r3, r16\n"
    :: 
  );
}
void nseel_asm_equal_end(void) {}
//
//---------------------------------------------------------------------------------------------------------------
void nseel_asm_notequal(void)
{
  __asm__(
    "lfd f1, 0(r3)\n"
    "lfd f2, 0(r14)\n"
    "addis r5, 0, 0xdead\n"
    "ori r5, r5, 0xbeef\n"
    "fsub f1, f1, f2\n"
    "fabs f1, f1\n"
    "lfd f2, 0(r5)\n"
    "fcmpu cr7, f1, f2\n"
    "blt cr7, 0f\n"
    "addis r5, 0, 0xdead\n"
    "ori r5, r5, 0xbeef\n"
    "b 1f\n"
    "0:\n"
    "addis r5, 0, 0xdead\n"
    "ori r5, r5, 0xbeef\n"
    "1:\n"
    "lfd f1, 0(r5)\n"
    "  stfdu f1, 8(r16)\n"
    "  mr r3, r16\n"
    :: 
  );
}
void nseel_asm_notequal_end(void) {}


//---------------------------------------------------------------------------------------------------------------
void nseel_asm_below(void)
{
  __asm__(
    "lfd f1, 0(r3)\n"
    "lfd f2, 0(r14)\n"
    "fcmpu cr7, f2, f1\n"
    "blt cr7, 0f\n"
    "addis r5, 0, 0xdead\n"
    "ori r5, r5, 0xbeef\n"
    "lfd f1, 0(r5)\n"
    "b 1f\n"
    "0:\n"
    "addis r5, 0, 0xdead\n"
    "ori r5, r5, 0xbeef\n"
    "lfd f1, 0(r5)\n"
    "1:\n"
    "  stfdu f1, 8(r16)\n"
    "  mr r3, r16\n"
    ::
  );
}
void nseel_asm_below_end(void) {}

//---------------------------------------------------------------------------------------------------------------
void nseel_asm_beloweq(void)
{
  __asm__(
    "lfd f1, 0(r3)\n"
    "lfd f2, 0(r14)\n"
    "fcmpu cr7, f2, f1\n"
    "ble cr7, 0f\n"
    "addis r5, 0, 0xdead\n"
    "ori r5, r5, 0xbeef\n"
    "lfd f1, 0(r5)\n"
    "b 1f\n"
    "0:\n"
    "addis r5, 0, 0xdead\n"
    "ori r5, r5, 0xbeef\n"
    "lfd f1, 0(r5)\n"
    "1:\n"
    "  stfdu f1, 8(r16)\n"
    "  mr r3, r16\n"
    :: 
  );
}
void nseel_asm_beloweq_end(void) {}


//---------------------------------------------------------------------------------------------------------------
void nseel_asm_above(void)
{
  __asm__(
    "lfd f1, 0(r3)\n"
    "lfd f2, 0(r14)\n"
    "fcmpu cr7, f2, f1\n"
    "bgt cr7, 0f\n"
    "addis r5, 0, 0xdead\n"
    "ori r5, r5, 0xbeef\n"
    "lfd f1, 0(r5)\n"
    "b 1f\n"
    "0:\n"
    "addis r5, 0, 0xdead\n"
    "ori r5, r5, 0xbeef\n"
    "lfd f1, 0(r5)\n"
    "1:\n"
    "  stfdu f1, 8(r16)\n"
    "  mr r3, r16\n"
    ::
  );
}
void nseel_asm_above_end(void) {}

void nseel_asm_aboveeq(void)
{
  __asm__(
    "lfd f1, 0(r3)\n"
    "lfd f2, 0(r14)\n"
    "fcmpu cr7, f2, f1\n"
    "bge cr7, 0f\n"
    "addis r5, 0, 0xdead\n"
    "ori r5, r5, 0xbeef\n"
    "lfd f1, 0(r5)\n"
    "b 1f\n"
    "0:\n"
    "addis r5, 0, 0xdead\n"
    "ori r5, r5, 0xbeef\n"
    "lfd f1, 0(r5)\n"
    "1:\n"
    "  stfdu f1, 8(r16)\n"
    "  mr r3, r16\n"
    :: 
  );
}
void nseel_asm_aboveeq_end(void) {}



void nseel_asm_min(void)
{
  __asm__(
    "lfd f1, 0(r3)\n"
    "lfd f2, 0(r14)\n"
    "fcmpu cr7, f2, f1\n"
    "bgt cr7, 0f\n"
    "fmr f1, f2\n"
    "0:\n"
    "  stfdu f1, 8(r16)\n"
    "  mr r3, r16\n"
  );
}
void nseel_asm_min_end(void) {}

void nseel_asm_max(void)
{
  __asm__(
    "lfd f1, 0(r3)\n"
    "lfd f2, 0(r14)\n"
    "fcmpu cr7, f2, f1\n"
    "blt cr7, 0f\n"
    "fmr f1, f2\n"
    "0:\n"
    "  stfdu f1, 8(r16)\n"
    "  mr r3, r16\n"
  );
}

void nseel_asm_max_end(void) {}









void _asm_generic3parm(void)
{
  __asm__(
   "mr r6, r3\n"
   "addis r3, 0, 0xdead\n"
   "ori r3, r3, 0xbeef\n"
   "addis r7, 0, 0xdead\n"
   "ori r7, r7, 0xbeef\n"
   "mr r4, r15\n"
   "mr r5, r14\n"
   "mtctr r7\n"
   "subi r1, r1, 64\n"
   "bctrl\n"
   "addi r1, r1, 64\n"
  ::
 ); 
}
void _asm_generic3parm_end(void) {}

void _asm_generic3parm_retd(void)
{
  __asm__(
   "mr r6, r3\n"
   "addis r3, 0, 0xdead\n"
   "ori r3, r3, 0xbeef\n"
   "addis r7, 0, 0xdead\n"
   "ori r7, r7, 0xbeef\n"
   "mr r4, r15\n"
   "mr r5, r14\n"
   "mtctr r7\n"
   "subi r1, r1, 64\n"
   "bctrl\n"
   "addi r1, r1, 64\n"
   "stfdu f1, 8(r16)\n"
   "mr r3, r16\n"
  ::
 ); 
}
void _asm_generic3parm_retd_end(void) {}


void _asm_generic2parm(void) // this prob neds to be fixed for ppc
{
  __asm__(
   "mr r5, r3\n"
   "addis r3, 0, 0xdead\n"
   "ori r3, r3, 0xbeef\n"
   "addis r7, 0, 0xdead\n"
   "ori r7, r7, 0xbeef\n"
   "mr r4, r14\n"
   "mtctr r7\n"
   "subi r1, r1, 64\n"
   "bctrl\n"
   "addi r1, r1, 64\n"
  ::
 ); 
}
void _asm_generic2parm_end(void) {}


void _asm_generic2parm_retd(void)
{
  __asm__(
   "mr r5, r3\n"
   "addis r3, 0, 0xdead\n"
   "ori r3, r3, 0xbeef\n"
   "addis r7, 0, 0xdead\n"
   "ori r7, r7, 0xbeef\n"
   "mr r4, r14\n"
   "mtctr r7\n"
   "subi r1, r1, 64\n"
   "bctrl\n"
   "addi r1, r1, 64\n"
   "stfdu f1, 8(r16)\n"
   "mr r3, r16\n"
  ::
 ); 
}
void _asm_generic2parm_retd_end(void) {}

void _asm_generic1parm(void) // this prob neds to be fixed for ppc
{
  __asm__(
   "mr r4, r3\n"
   "addis r3, 0, 0xdead\n"
   "ori r3, r3, 0xbeef\n"
   "addis r7, 0, 0xdead\n"
   "ori r7, r7, 0xbeef\n"
   "mtctr r7\n"
   "subi r1, r1, 64\n"
   "bctrl\n"
   "addi r1, r1, 64\n"
  ::
 ); 
}
void _asm_generic1parm_end(void) {}



void _asm_generic1parm_retd(void)
{
  __asm__(
   "mr r4, r3\n"
   "addis r3, 0, 0xdead\n"
   "ori r3, r3, 0xbeef\n"
   "addis r7, 0, 0xdead\n"
   "ori r7, r7, 0xbeef\n"
   "mtctr r7\n"
   "subi r1, r1, 64\n"
   "bctrl\n"
   "addi r1, r1, 64\n"
   "stfdu f1, 8(r16)\n"
   "mr r3, r16\n"
  ::
 ); 
}
void _asm_generic1parm_retd_end(void) {}




void _asm_megabuf(void)
{
  __asm__(
   "lfd f1, 0(r3)\n"
   "addis r3, 0, 0xdead\n" // set up context pointer
   "ori r3, r3, 0xbeef\n"
   "addis r4, 0, 0xdead\n"
   "ori r4, r4, 0xbeef\n"
   "lfd f2, 0(r4)\n"
   "fadd f1, f2, f1\n"
   "addis r7, 0, 0xdead\n"
   "ori r7, r7, 0xbeef\n"
   "mtctr r7\n"
   "fctiwz f1, f1\n"
   "stfd f1, 8(r16)\n"
   "lwz r4, 12(r16)\n"
   "subi r1, r1, 64\n"
   "bctrl\n"
   "addi r1, r1, 64\n"
   "cmpi cr0, r3, 0\n"
   "bne cr0, 0f\n"
   "sub r5, r5, r5\n"
   "stwu r5, 8(r16)\n"
   "stw r5, 4(r16)\n"
   "mr r3, r16\n"
   "0:\n"
  ::
 ); 
}

void _asm_megabuf_end(void) {}
