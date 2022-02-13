; these must be synced with any changes in ns-eel-int.h
%define NSEEL_RAM_BLOCKS_DEFAULTMAX 128
%define NSEEL_RAM_BLOCKS_LOG2 9
%define NSEEL_RAM_ITEMSPERBLOCK_LOG2 16
%define NSEEL_RAM_BLOCKS (1 << NSEEL_RAM_BLOCKS_LOG2)
%define NSEEL_RAM_ITEMSPERBLOCK (1<<NSEEL_RAM_ITEMSPERBLOCK_LOG2)
%define EEL_F_SIZE 8

; todo: also determine FTZ?
; also: do FP flags rounding mode affect SSE ops? other things? tbd
; %define EEL_X64_NO_CHANGE_FPFLAGS

SECTION .text

%ifdef AMD64ABI

  ; non-win64 needs to preserve xmm4-xmm7 when calling other functions
  %macro pre_call 0
     sub rsp, 32
     movsd [rsp], xmm4
     movsd [rsp+8], xmm5
     movsd [rsp+16], xmm6
     movsd [rsp+24], xmm7
  %endmacro
  %macro post_call 0
     movsd xmm4, [rsp]
     movsd xmm5, [rsp+8]
     movsd xmm6, [rsp+16]
     movsd xmm7, [rsp+24]
     add rsp, 32
  %endmacro

%else
  ; win64 doesn't need to preserve any spill registers when calling functions, but must when called
  %macro pre_call 0
     sub rsp, 32
  %endmacro
  %macro post_call 0
     add rsp, 32
  %endmacro

  %macro save_spill_full 0
     sub rsp, 64
     movdqu [rsp], xmm6
     movdqu [rsp+16], xmm7
     movdqu [rsp+32], xmm8
     movdqu [rsp+48], xmm9
  %endmacro
  %macro restore_spill_full 0
     movdqu xmm6, [rsp]
     movdqu xmm7, [rsp+16]
     movdqu xmm8, [rsp+32]
     movdqu xmm9, [rsp+48]
     add rsp, 64
  %endmacro


%endif

global nseel_asm_1pdd
nseel_asm_1pdd:


   mov rdi, qword 0xFEFEFEFEFEFEFEFE
   pre_call
%ifdef AMD64ABI
   mov r15, rsi
   call rdi
   mov rsi, r15
%else
   call rdi
%endif
  post_call

db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00



global nseel_asm_2pdd
nseel_asm_2pdd:


  movsd xmm2, xmm0
  movsd xmm0, xmm1
  movsd xmm1, xmm2
  mov rdi, qword 0xFEFEFEFEFEFEFEFE

  pre_call

%ifdef AMD64ABI
  mov r15, rsi
  call rdi
  mov rsi, r15
%else
  call rdi
%endif

  post_call

db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_2pdds
nseel_asm_2pdds:


  mov rax, qword 0xFEFEFEFEFEFEFEFE
  movsd xmm1, xmm0
  movsd xmm0, [rdi]
  pre_call

%ifdef AMD64ABI
  mov r15, rsi
  mov r14, rdi
  call rax
  mov rsi, r15
  movq [r14], xmm0
  mov rax, r14  ;  set return value
%else
  call rax
  movq [rdi], xmm0
  mov rax, rdi  ;  set return value
%endif

  post_call

db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00


global nseel_asm_invsqrt
nseel_asm_invsqrt:

; optimize me
    movsd [rsi], xmm0
    fld qword [rsi]

    mov rdx, 0x5f3759df
    fst dword [rsi]
    mov rax, qword 0xFEFEFEFEFEFEFEFE
    fmul qword [rax]
    movsx rcx, dword [rsi]
    sar rcx, 1
    sub rdx, rcx
    mov dword [rsi], edx
    fmul dword [rsi]
    fmul dword [rsi]
    mov rax, qword 0xFEFEFEFEFEFEFEFE
    fadd qword [rax]
    fmul dword [rsi]

; optimize me
    fstp qword [rsi]
    movsd xmm0, [rsi]

db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_dbg_getstackptr
nseel_asm_dbg_getstackptr:

    mov qword [rsi], rsp
    cvtsi2sd xmm0, qword [rsi]
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_sqr
nseel_asm_sqr:

    mulsd xmm0, xmm0
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_abs
nseel_asm_abs:

  andps xmm0, [r12-32]
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_assign
nseel_asm_assign:

    mov rdx, qword [rax]
    sub eax, eax
    mov rcx, rdx

    shr rdx, 32
    add edx, 0x00100000
    and edx, 0x7ff00000
    cmp edx, 0x00200000
    cmovle rcx, rax
    mov rax, rdi
    mov qword [rdi], rcx
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_assign_fromfp
nseel_asm_assign_fromfp:


    movq rdx, xmm0
    sub eax, eax
    mov rcx, rdx

    shr rdx, 32
    add edx, 0x00100000
    and edx, 0x7ff00000
    cmp edx, 0x00200000
    cmovle rcx, rax
    mov rax, rdi
    mov qword [rdi], rcx

db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_assign_fast_fromfp
nseel_asm_assign_fast_fromfp:

    mov rax, rdi
    movsd [rdi], xmm0
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_assign_fast
nseel_asm_assign_fast:

    mov rdx, qword [rax]
    mov qword [rdi], rdx
    mov rax, rdi
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_add
nseel_asm_add:

          addsd xmm0, xmm1
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_add_op
nseel_asm_add_op:

      addsd xmm0, [rdi]
      movsd [rdi], xmm0
      mov rax, rdi
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_add_op_fast
nseel_asm_add_op_fast:

      addsd xmm0, [rdi]
      movsd [rdi], xmm0
      mov rax, rdi
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_sub
nseel_asm_sub:

      subsd xmm1, xmm0
      movsd xmm0, xmm1
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_sub_op
nseel_asm_sub_op:

      movsd xmm1, [rdi]
      subsd xmm1, xmm0
      movsd [rdi], xmm1
      mov rax, rdi
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_sub_op_fast
nseel_asm_sub_op_fast:

      movsd xmm1, [rdi]
      subsd xmm1, xmm0
      movsd [rdi], xmm1
      mov rax, rdi
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_mul
nseel_asm_mul:

      mulsd xmm0, xmm1
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_mul_op
nseel_asm_mul_op:

      mulsd xmm0, [rdi]
      movsd [rdi], xmm0
      mov rax, rdi
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_mul_op_fast
nseel_asm_mul_op_fast:

      mulsd xmm0, [rdi]
      movsd [rdi], xmm0
      mov rax, rdi
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_div
nseel_asm_div:

      divsd xmm1, xmm0
      movapd xmm0, xmm1
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_div_op
nseel_asm_div_op:

      movsd xmm1, [rdi]
      divsd xmm1, xmm0
      movsd [rdi], xmm1
      mov rax, rdi
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_div_op_fast
nseel_asm_div_op_fast:

      movsd xmm1, [rdi]
      divsd xmm1, xmm0
      movsd [rdi], xmm1
      mov rax, rdi
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_mod
nseel_asm_mod:

  andps xmm0, [r12-32]
  andps xmm1, [r12-32]
    cvttsd2si rcx, xmm0
    cvttsd2si rax, xmm1
    xor rdx, rdx
    cmp rcx, 0
    je label_0
    div rcx
label_0:

    cvtsi2sd xmm0, rdx
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_shl
nseel_asm_shl:

    cvttsd2si rcx, xmm0
    cvttsd2si rax, xmm1
    and eax, 0xffffffff ; match x87 impl
    shl rax, cl
    cvtsi2sd xmm0, eax
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_shr
nseel_asm_shr:

    cvttsd2si rcx, xmm0
    cvttsd2si rax, xmm1
    and eax, 0xffffffff ; match x87 impl
    sar rax, cl
    cvtsi2sd xmm0, eax
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_mod_op
nseel_asm_mod_op:

    movsd xmm1, [rdi]
    andps xmm0, [r12-32]
    andps xmm1, [r12-32]
    cvttsd2si rcx, xmm0
    cvttsd2si rax, xmm1
    xor rdx, rdx
    cmp rcx, 0
    je label_1  ;  skip devide, set return to 0
    div rcx
label_1:

    cvtsi2sd xmm0, rdx
    movsd [rdi], xmm0
    mov rax, rdi

db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_or
nseel_asm_or:

    cvttsd2si rcx, xmm0
    cvttsd2si rax, xmm1
    or rax, rcx
    cvtsi2sd xmm0, rax

db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_or0
nseel_asm_or0:

    cvttsd2si rax, xmm0
    cvtsi2sd xmm0, rax
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_or_op
nseel_asm_or_op:

    movsd xmm1, [rdi]
    cvttsd2si rcx, xmm0
    cvttsd2si rax, xmm1
    or rax, rcx
    cvtsi2sd xmm0, rax
    movsd [rdi], xmm0
    mov rax, rdi

db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_xor
nseel_asm_xor:

    cvttsd2si rcx, xmm0
    cvttsd2si rax, xmm1
    xor rax, rcx
    cvtsi2sd xmm0, rax
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_xor_op
nseel_asm_xor_op:

    movsd xmm1, [rdi]
    cvttsd2si rcx, xmm0
    cvttsd2si rax, xmm1
    xor rax, rcx
    cvtsi2sd xmm0, rax
    movsd [rdi], xmm0
    mov rax, rdi
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_and
nseel_asm_and:

    cvttsd2si rcx, xmm0
    cvttsd2si rax, xmm1
    and rax, rcx
    cvtsi2sd xmm0, rax
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_and_op
nseel_asm_and_op:

    movsd xmm1, [rdi]
    cvttsd2si rcx, xmm0
    cvttsd2si rax, xmm1
    and rax, rcx
    cvtsi2sd xmm0, rax
    movsd [rdi], xmm0
    mov rax, rdi
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00



global nseel_asm_uminus
nseel_asm_uminus:

    xorps xmm0, [r12-48]
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_sign
nseel_asm_sign:


    movsd [rsi], xmm0
    mov rdx, qword [rsi]
    sub rcx, rcx
    dec rcx
    shr rcx, 1 ; 7ffff...
    test rdx, rcx
    jz label_2  ;  zero zero, return the value passed directly
       ;  calculate sign
      inc rcx  ;  rcx becomes 0x80000...
      sub rax, rax
      test rdx, rcx
      jnz label_3
      inc rax
      jmp label_4
label_3:

      dec rax
label_4:

      cvtsi2sd xmm0, rax
label_2:


db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_bnot
nseel_asm_bnot:

    test rax, rax
    setz al
    and eax, 0xff
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_fcall
nseel_asm_fcall:

     mov rdx, qword 0xFEFEFEFEFEFEFEFE
     sub rsp, 8
     call rdx
     add rsp, 8
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_band
nseel_asm_band:

    test rax, rax
    jz label_5

     mov rcx, qword 0xFEFEFEFEFEFEFEFE
        sub rsp, 8
        call rcx
        add rsp, 8
label_5:

db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_bor
nseel_asm_bor:

    test rax, rax
    jnz label_6

    mov rcx, qword 0xFEFEFEFEFEFEFEFE
    sub rsp, 8
    call rcx
    add rsp, 8
label_6:

db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_equal
nseel_asm_equal:

    subsd xmm0, xmm1
    andps xmm0, [r12-32]

    xor eax,eax
    ucomisd xmm0, [r12-8]
    setb al

db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_equal_exact
nseel_asm_equal_exact:

    xor eax,eax
    ucomisd xmm0,xmm1
    sete al
label_7:

db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_notequal_exact
nseel_asm_notequal_exact:

    xor eax,eax
    ucomisd xmm0,xmm1
    setne al

db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_notequal
nseel_asm_notequal:


    subsd xmm0, xmm1
    andps xmm0, [r12-32]

    xor eax,eax
    ucomisd xmm0, [r12-8]
    setnb al

db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_above
nseel_asm_above:

    xor eax,eax
    ucomisd xmm1,xmm0
    seta al
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_beloweq
nseel_asm_beloweq:

    xor eax,eax
    ucomisd xmm1,xmm0
    setbe al
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00



global nseel_asm_aboveeq
nseel_asm_aboveeq:

    xor eax,eax
    ucomisd xmm1,xmm0
    setae al
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_below
nseel_asm_below:

    xor eax,eax
    ucomisd xmm1,xmm0
    setb al
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00






global nseel_asm_booltofp
nseel_asm_booltofp:

    mov rdi, rax
    sub eax, eax
    test rdi, rdi
    setnz al
    cvtsi2sd xmm0, rax
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_fptobool
nseel_asm_fptobool:

    andps xmm0, [r12-32]

    xor eax,eax
    ucomisd xmm0, [r12-8]
    setnb al
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_fptobool_rev
nseel_asm_fptobool_rev:

    andps xmm0, [r12-32]

    xor eax,eax
    ucomisd xmm0, [r12-8]
    setb al
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_min
nseel_asm_min:

    movsd xmm1, [rdi]
    ucomisd xmm1, [rax]
    cmovna rax, rdi

db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_max
nseel_asm_max:

    movsd xmm0, [rax]
    movsd xmm1, [rdi]
    ucomisd xmm1, [rax]
    cmova rax, rdi

db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_min_fp
nseel_asm_min_fp:

  minsd xmm0, xmm1
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_max_fp
nseel_asm_max_fp:

  maxsd xmm0, xmm1
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global _asm_generic3parm
_asm_generic3parm:


   pre_call
%ifdef AMD64ABI

    mov r15, rsi
    mov rdx, rdi  ;  third parameter = parm
    mov rdi, qword 0xFEFEFEFEFEFEFEFE  ;  first parameter= context

    mov rsi, rcx  ;  second parameter = parm
    mov rcx, rax  ;  fourth parameter = parm
    mov rax, qword 0xFEFEFEFEFEFEFEFE  ;  call function
    call rax

    mov rsi, r15
%else
    mov rdx, rcx  ;  second parameter = parm
    mov rcx, qword 0xFEFEFEFEFEFEFEFE  ;  first parameter= context
    mov r8, rdi  ;  third parameter = parm
    mov r9, rax  ;  fourth parameter = parm
    mov rdi, qword 0xFEFEFEFEFEFEFEFE  ;  call function
    call rdi
%endif
   post_call

db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global _asm_generic3parm_retd
_asm_generic3parm_retd:

  pre_call
%ifdef AMD64ABI
    mov r15, rsi
    mov rdx, rdi  ;  third parameter = parm
    mov rdi, qword 0xFEFEFEFEFEFEFEFE  ;  first parameter= context
    mov rsi, rcx  ;  second parameter = parm
    mov rcx, rax  ;  fourth parameter = parm
    mov rax, qword 0xFEFEFEFEFEFEFEFE  ;  call function
    call rax
    mov rsi, r15
%else
    mov rdx, rcx  ;  second parameter = parm
    mov rcx, qword 0xFEFEFEFEFEFEFEFE  ;  first parameter= context
    mov r8, rdi  ;  third parameter = parm
    mov r9, rax  ;  fourth parameter = parm
    mov rdi, qword 0xFEFEFEFEFEFEFEFE  ;  call function
    call rdi
%endif
  post_call
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global _asm_generic2parm
_asm_generic2parm:


  pre_call
%ifdef AMD64ABI
    mov r15, rsi
    mov rsi, rdi  ;  second parameter = parm
    mov rdi, qword 0xFEFEFEFEFEFEFEFE  ;  first parameter= context
    mov rdx, rax  ;  third parameter = parm
    mov rcx, qword 0xFEFEFEFEFEFEFEFE  ;  call function
    call rcx
    mov rsi, r15
%else
    mov rcx, qword 0xFEFEFEFEFEFEFEFE  ;  first parameter= context
    mov rdx, rdi  ;  second parameter = parm
    mov r8, rax  ;  third parameter = parm
    mov rdi, qword 0xFEFEFEFEFEFEFEFE  ;  call function
    call rdi
%endif
  post_call
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global _asm_generic2parm_retd
_asm_generic2parm_retd:

  pre_call
%ifdef AMD64ABI
    mov r15, rsi
    mov rsi, rdi  ;  second parameter = parm
    mov rdi, qword 0xFEFEFEFEFEFEFEFE  ;  first parameter= context
    mov rcx, qword 0xFEFEFEFEFEFEFEFE  ;  call function
    mov rdx, rax  ;  third parameter = parm
    call rcx
    mov rsi, r15
%else
    mov rdx, rdi  ;  second parameter = parm
    mov rcx, qword 0xFEFEFEFEFEFEFEFE  ;  first parameter= context
    mov rdi, qword 0xFEFEFEFEFEFEFEFE  ;  call function
    mov r8, rax  ;  third parameter = parm
    call rdi
%endif
  post_call
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00



global _asm_generic2xparm_retd
_asm_generic2xparm_retd:

  pre_call
%ifdef AMD64ABI
    mov r15, rsi
    mov rdx, rdi  ;  third parameter = parm
    mov rcx, rax  ;  fourth parameter = parm
    mov rdi, qword 0xFEFEFEFEFEFEFEFE  ;  first parameter= context
    mov rsi, qword 0xFEFEFEFEFEFEFEFE  ;  second parameter= context
    mov rax, qword 0xFEFEFEFEFEFEFEFE  ;  call function
    call rax
    mov rsi, r15
%else
    mov r8, rdi  ;  third parameter = parm
    mov rcx, qword 0xFEFEFEFEFEFEFEFE  ;  first parameter= context
    mov rdx, qword 0xFEFEFEFEFEFEFEFE  ;  second parameter= context
    mov rdi, qword 0xFEFEFEFEFEFEFEFE  ;  call function
    mov r9, rax  ;  fourth parameter = parm
    call rdi
%endif
  post_call
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global _asm_generic1parm
_asm_generic1parm:

  pre_call
%ifdef AMD64ABI
    mov rdi, qword 0xFEFEFEFEFEFEFEFE  ;  first parameter= context
    mov r15, rsi
    mov rsi, rax  ;  second parameter = parm
    mov rcx, qword 0xFEFEFEFEFEFEFEFE  ;  call function
    call rcx
    mov rsi, r15
%else
    mov rcx, qword 0xFEFEFEFEFEFEFEFE  ;  first parameter= context
    mov rdx, rax  ;  second parameter = parm
    mov rdi, qword 0xFEFEFEFEFEFEFEFE  ;  call function
    call rdi
%endif
  post_call
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global _asm_generic1parm_retd
_asm_generic1parm_retd:

  pre_call
%ifdef AMD64ABI
    mov rdi, qword 0xFEFEFEFEFEFEFEFE  ;  first parameter = context pointer
    mov rcx, qword 0xFEFEFEFEFEFEFEFE  ;  function address
    mov r15, rsi  ;  save rsi
    mov rsi, rax  ;  second parameter = parameter

    call rcx

    mov rsi, r15
%else
    mov rcx, qword 0xFEFEFEFEFEFEFEFE  ;  first parameter= context
    mov rdi, qword 0xFEFEFEFEFEFEFEFE  ;  call function

    mov rdx, rax  ;  second parameter = parm

    call rdi
%endif
  post_call
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global _asm_megabuf
_asm_megabuf:



    addsd xmm0, qword [r12-8]
%ifdef AMD64ABI
    cvttsd2si rdx, xmm0
     ;  check if edx is in range, and buffer available, otherwise call function
    cmp rdx, ((NSEEL_RAM_BLOCKS*NSEEL_RAM_ITEMSPERBLOCK))       ; REPLACE=((NSEEL_RAM_BLOCKS*NSEEL_RAM_ITEMSPERBLOCK))
    jae label_15
      mov rax, rdx
      shr rax, (NSEEL_RAM_ITEMSPERBLOCK_LOG2 - 3   ) ; log2(sizeof(void *))      ; REPLACE=(NSEEL_RAM_ITEMSPERBLOCK_LOG2 - 3   ) ; log2(sizeof(void *))
      and rax, ((NSEEL_RAM_BLOCKS-1)*8                    ) ; sizeof(void*)      ; REPLACE=((NSEEL_RAM_BLOCKS-1)*8                    ) ; sizeof(void*)
      mov rax, qword [r12+rax]
      test rax, rax
      jnz label_16
label_15:

      mov rax, qword 0xFEFEFEFEFEFEFEFE
      mov rdi, r12  ;  set first parm to ctx
      mov r15, rsi  ;  save rsi
      mov rsi, rdx  ;  esi becomes second parameter (edi is first, context pointer)
      pre_call
      call rax
      post_call
      mov rsi, r15  ;  restore rsi
      jmp label_17
label_16:

      and rdx, (NSEEL_RAM_ITEMSPERBLOCK-1)       ; REPLACE=(NSEEL_RAM_ITEMSPERBLOCK-1)
      shl rdx, 3       ;  3 is log2(sizeof(EEL_F))
      add rax, rdx
label_17:


%else
     ;  check if (%rsi) is in range...
    cvttsd2si rdi, xmm0
    cmp rdi, ((NSEEL_RAM_BLOCKS*NSEEL_RAM_ITEMSPERBLOCK))        ; REPLACE=((NSEEL_RAM_BLOCKS*NSEEL_RAM_ITEMSPERBLOCK))
    jae label_18
      mov rax, rdi
      shr rax, (NSEEL_RAM_ITEMSPERBLOCK_LOG2 - 3   ) ; log2(sizeof(void *))        ; REPLACE=(NSEEL_RAM_ITEMSPERBLOCK_LOG2 - 3   ) ; log2(sizeof(void *))
      and rax, ((NSEEL_RAM_BLOCKS-1)*8                    ) ; sizeof(void*)        ; REPLACE=((NSEEL_RAM_BLOCKS-1)*8                    ) ; sizeof(void*)
      mov rax, qword [r12+rax]
      test rax, rax
      jnz label_19
label_18:

      mov rax, qword 0xFEFEFEFEFEFEFEFE  ;  function ptr
      mov rcx, r12  ;  set first parm to ctx
      mov rdx, rdi  ;  rdx is second parameter (rcx is first)
      pre_call
      call rax
      post_call
      jmp label_20
label_19:

      and rdi, (NSEEL_RAM_ITEMSPERBLOCK-1)        ; REPLACE=(NSEEL_RAM_ITEMSPERBLOCK-1)
      shl rdi, 3        ;  3 is log2(sizeof(EEL_F))
      add rax, rdi
label_20:

%endif


db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global _asm_gmegabuf
_asm_gmegabuf:



    addsd xmm0, qword [r12-8]

    pre_call
%ifdef AMD64ABI

    mov r15, rsi
    mov rdi, qword 0xFEFEFEFEFEFEFEFE  ;  first parameter = context pointer
    mov rdx, qword 0xFEFEFEFEFEFEFEFE
    cvttsd2si rsi, xmm0
    call rdx
    mov rsi, r15

%else
    mov rcx, qword 0xFEFEFEFEFEFEFEFE  ;  first parameter = context pointer
    mov rdi, qword 0xFEFEFEFEFEFEFEFE
    cvttsd2si rdx, xmm0
    call rdi
%endif
  post_call

db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_stack_push
nseel_asm_stack_push:

    mov rdi, qword 0xFEFEFEFEFEFEFEFE
    mov rcx, qword [rax]
    mov rax, qword [rdi]
    add rax, 8
    mov rdx, qword 0xFEFEFEFEFEFEFEFE
    and rax, rdx
    mov rdx, qword 0xFEFEFEFEFEFEFEFE
    or rax, rdx
    mov qword [rax], rcx
    mov qword [rdi], rax
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00



global nseel_asm_stack_pop
nseel_asm_stack_pop:

      mov rdi, qword 0xFEFEFEFEFEFEFEFE
      mov rcx, qword [rdi]
      movq xmm0, [rcx]
      sub rcx, 8
      mov rdx, qword 0xFEFEFEFEFEFEFEFE
      and rcx, rdx
      mov rdx, qword 0xFEFEFEFEFEFEFEFE
      or rcx, rdx
      mov qword [rdi], rcx
      movq [rax], xmm0
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00



global nseel_asm_stack_pop_fast
nseel_asm_stack_pop_fast:

      mov rdi, qword 0xFEFEFEFEFEFEFEFE
      mov rcx, qword [rdi]
      mov rax, rcx
      sub rcx, 8
      mov rdx, qword 0xFEFEFEFEFEFEFEFE
      and rcx, rdx
      mov rdx, qword 0xFEFEFEFEFEFEFEFE
      or rcx, rdx
      mov qword [rdi], rcx
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00



global nseel_asm_stack_peek_int
nseel_asm_stack_peek_int:

    mov rdi, qword 0xFEFEFEFEFEFEFEFE
    mov rax, qword [rdi]
    mov rdx, qword 0xFEFEFEFEFEFEFEFE
    sub rax, rdx
    mov rdx, qword 0xFEFEFEFEFEFEFEFE
    and rax, rdx
    mov rdx, qword 0xFEFEFEFEFEFEFEFE
    or rax, rdx
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00



global nseel_asm_stack_peek
nseel_asm_stack_peek:

    mov rdi, qword 0xFEFEFEFEFEFEFEFE
    cvttsd2si rdx, xmm0
    mov rax, qword [rdi]
    shl rdx, 3  ;  log2(sizeof(EEL_F))
    sub rax, rdx
    mov rdx, qword 0xFEFEFEFEFEFEFEFE
    and rax, rdx
    mov rdx, qword 0xFEFEFEFEFEFEFEFE
    or rax, rdx
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00



global nseel_asm_stack_peek_top
nseel_asm_stack_peek_top:

    mov rdi, qword 0xFEFEFEFEFEFEFEFE
    mov rax, qword [rdi]
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00




global nseel_asm_stack_exch
nseel_asm_stack_exch:

    mov rdi, qword 0xFEFEFEFEFEFEFEFE
    mov rcx, qword [rdi]
    movq xmm0, [rcx]
    movq xmm1, [rax]
    movq [rax], xmm0
    movq [rcx], xmm1
db 0x89,0x90,0x90,0x90,0x90,0x90,0x90,0x00


global eel_callcode64
eel_callcode64:

		sub rsp, 16
%ifndef EEL_X64_NO_CHANGE_FPFLAGS
		fnstcw [rsp]
		mov ax, [rsp]
		or ax, 0x23F  ;  53 or 64 bit precision, masking all exceptions
		mov [rsp+4], ax
		fldcw [rsp+4]
%endif
		stmxcsr [rsp+8]
		mov eax, [rsp+8]
		or ah, 136 ; 128|8, bits 15 and 11
		mov [rsp+12], eax
		ldmxcsr [rsp+12]

		push rbx
		push rbp
		push r12
		push r13
		push r14
		push r15

%ifdef AMD64ABI
    		mov r12, rsi  ;  second parameter is ram-blocks pointer
		call rdi
%else
		push rdi
		push rsi

    		mov r12, rdx  ;  second parameter is ram-blocks pointer

                save_spill_full

		call rcx

                restore_spill_full

		pop rsi
		pop rdi
%endif

		fclex

		pop r15
		pop r14
		pop r13
		pop r12
		pop rbp
		pop rbx

		ldmxcsr [rsp+8]

%ifndef EEL_X64_NO_CHANGE_FPFLAGS
		fldcw [rsp]
%endif
		add rsp, 16

		ret

global eel_callcode64_fast
eel_callcode64_fast:

		push rbx
		push rbp
		push r12
		push r13
		push r14
		push r15

%ifdef AMD64ABI
    		mov r12, rsi  ;  second parameter is ram-blocks pointer
		call rdi
%else
		push rdi
		push rsi

    		mov r12, rdx  ;  second parameter is ram-blocks pointer

                save_spill_full

		call rcx

                restore_spill_full

		pop rsi
		pop rdi
%endif

		pop r15
		pop r14
		pop r13
		pop r12
		pop rbp
		pop rbx

		ret


global eel_enterfp
eel_enterfp:

%ifdef AMD64ABI
		fnstcw [rdi]
		mov ax, [rdi]
		or ax, 0x23F  ;  53 or 64 bit precision, masking all exceptions
		mov [rdi+4], ax
		fldcw [rdi+4]

		sub rsp, 16
		stmxcsr [rdi+4]
		mov eax, [rdi+4]
		or ah, 136 ; 128|8, bits 15 and 11
		mov [rsp], eax
		ldmxcsr [rsp]
		add rsp, 16
%else
		fnstcw [rcx]
		mov ax, [rcx]
		or ax, 0x23F  ;  53 or 64 bit precision, masking all exceptions
		mov [rcx+4], ax
		fldcw [rcx+4]

		sub rsp, 16
		stmxcsr [rcx+4]
		mov eax, [rcx+4]
		or ah, 136 ; 128|8, bits 15 and 11
		mov [rsp], eax
		ldmxcsr [rsp]
		add rsp, 16
%endif
            ret


global eel_leavefp
eel_leavefp:

%ifdef AMD64ABI
		fldcw [rdi]
		ldmxcsr [rdi+4]
%else
		fldcw [rcx]
		ldmxcsr [rcx+4]
%endif
                ret;
