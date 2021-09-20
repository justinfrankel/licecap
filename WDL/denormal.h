#ifndef _WDL_DENORMAL_H_
#define _WDL_DENORMAL_H_

#include <string.h>
#include "wdltypes.h"
// note: the _aggressive versions filter out anything less than around 1.0e-16 or so (approximately) to 0.0, including -0.0 (becomes 0.0)
// note: new! the _aggressive versions also filter inf and NaN to 0.0

#ifdef __cplusplus
#define WDL_DENORMAL_INLINE inline
#elif defined(_MSC_VER)
#define WDL_DENORMAL_INLINE __inline
#else
  #ifdef WDL_STATICFUNC_UNUSED
    #define WDL_DENORMAL_INLINE WDL_STATICFUNC_UNUSED
  #else
    #define WDL_DENORMAL_INLINE
  #endif
#endif

static WDL_DENORMAL_INLINE unsigned int WDL_DENORMAL_FLOAT_W(const float *a) { unsigned int v; memcpy(&v,a,sizeof(v)); return v; }
static WDL_DENORMAL_INLINE unsigned int WDL_DENORMAL_DOUBLE_HW(const double *a) { WDL_UINT64 v; memcpy(&v,(char*)a,sizeof(v)); return (unsigned int) (v>>32); }

#define WDL_DENORMAL_DOUBLE_AGGRESSIVE_CUTOFF 0x3cA00000 // 0x3B8000000 maybe instead? that's 10^-5 smaller or so
#define WDL_DENORMAL_FLOAT_AGGRESSIVE_CUTOFF 0x25000000


// define WDL_DENORMAL_WANTS_SCOPED_FTZ, and then use a WDL_denormal_ftz_scope in addition to denormal_*(), then 
// if FTZ is available it will be used instead...
//
#ifdef WDL_DENORMAL_WANTS_SCOPED_FTZ

#if defined(__SSE2__) || _M_IX86_FP >= 2 || defined(_WIN64)
  #define WDL_DENORMAL_FTZMODE
  #define WDL_DENORMAL_FTZSTATE_TYPE unsigned int
  #ifdef _MSC_VER
    #include <intrin.h>
  #else
    #include <xmmintrin.h>
  #endif
  #define wdl_denorm_mm_getcsr() _mm_getcsr() 
  #define wdl_denorm_mm_setcsr(x) _mm_setcsr(x) 
  #if defined(__SSE3__)
    #define wdl_denorm_mm_csr_mask ((1<<15)|(1<<11) | (1<<8) | (1<<6)) // FTZ, underflow, denormal mask, DAZ  
  #else
    #define wdl_denorm_mm_csr_mask ((1<<15)|(1<<11)) // FTZ and underflow only (target SSE2)
  #endif
#elif defined(__arm__) || defined(__aarch64__)
  #define WDL_DENORMAL_FTZMODE
  #define WDL_DENORMAL_FTZSTATE_TYPE unsigned long 
  static unsigned long __attribute__((unused)) wdl_denorm_mm_getcsr()
  {
    unsigned long rv;
#ifdef __aarch64__
    asm volatile ( "mrs %0, fpcr" : "=r" (rv));
#else
    asm volatile ( "fmrx %0, fpscr" : "=r" (rv));
#endif
    return rv;
  }
  static void  __attribute__((unused)) wdl_denorm_mm_setcsr(unsigned long v)
  {
#ifdef __aarch64__
    asm volatile ( "msr fpcr, %0" :: "r"(v));
#else
    asm volatile ( "fmxr fpscr, %0" :: "r"(v));
#endif
  }
  #define wdl_denorm_mm_csr_mask (1<<24)
#endif

class WDL_denormal_ftz_scope 
{
  public:
    WDL_denormal_ftz_scope()
    {
#ifdef WDL_DENORMAL_FTZMODE
      const WDL_DENORMAL_FTZSTATE_TYPE b = wdl_denorm_mm_csr_mask;
      old_state = wdl_denorm_mm_getcsr();
      if ((need_restore = (old_state & b) != b))
          wdl_denorm_mm_setcsr(old_state|b);
#endif
    }
    ~WDL_denormal_ftz_scope()
    {
#ifdef WDL_DENORMAL_FTZMODE
      if (need_restore) wdl_denorm_mm_setcsr(old_state);
#endif
    }

#ifdef WDL_DENORMAL_FTZMODE
    WDL_DENORMAL_FTZSTATE_TYPE old_state;
    bool need_restore;
#endif

};


#endif


#if !defined(WDL_DENORMAL_FTZMODE) && !defined(WDL_DENORMAL_DO_NOT_FILTER)

static double WDL_DENORMAL_INLINE denormal_filter_double(double a)
{
  return (WDL_DENORMAL_DOUBLE_HW(&a)&0x7ff00000) ? a : 0.0;
}

static double WDL_DENORMAL_INLINE denormal_filter_double2(double a)
{
  return ((WDL_DENORMAL_DOUBLE_HW(&a)+0x100000)&0x7ff00000) > 0x100000 ? a : 0.0;
}

static double WDL_DENORMAL_INLINE denormal_filter_double_aggressive(double a)
{
  return ((WDL_DENORMAL_DOUBLE_HW(&a)+0x100000)&0x7ff00000) >= WDL_DENORMAL_DOUBLE_AGGRESSIVE_CUTOFF ? a : 0.0;
}

static float WDL_DENORMAL_INLINE denormal_filter_float(float a)
{
  return (WDL_DENORMAL_FLOAT_W(&a)&0x7f800000) ? a : 0.0f;
}

static float WDL_DENORMAL_INLINE denormal_filter_float2(float a)
{
  return ((WDL_DENORMAL_FLOAT_W(&a)+0x800000)&0x7f800000) > 0x800000 ? a : 0.0f; 
}


static float WDL_DENORMAL_INLINE denormal_filter_float_aggressive(float a)
{
  return ((WDL_DENORMAL_FLOAT_W(&a)+0x800000)&0x7f800000) >= WDL_DENORMAL_FLOAT_AGGRESSIVE_CUTOFF ? a : 0.0f; 
}
static void WDL_DENORMAL_INLINE denormal_fix_double(double *a)
{
  if (!(WDL_DENORMAL_DOUBLE_HW(a)&0x7ff00000)) *a=0.0;
}

static void WDL_DENORMAL_INLINE denormal_fix_double_aggressive(double *a)
{
  if (((WDL_DENORMAL_DOUBLE_HW(a)+0x100000)&0x7ff00000) < WDL_DENORMAL_DOUBLE_AGGRESSIVE_CUTOFF) *a=0.0;
}

static void WDL_DENORMAL_INLINE denormal_fix_float(float *a)
{
  if (!(WDL_DENORMAL_FLOAT_W(a)&0x7f800000)) *a=0.0f;
}
static void WDL_DENORMAL_INLINE denormal_fix_float_aggressive(float *a)
{
  if (((WDL_DENORMAL_FLOAT_W(a)+0x800000)&0x7f800000) < WDL_DENORMAL_FLOAT_AGGRESSIVE_CUTOFF) *a=0.0f;
}



#ifdef __cplusplus // automatic typed versions (though one should probably use the explicit versions...


static double WDL_DENORMAL_INLINE denormal_filter(double a)
{
  return (WDL_DENORMAL_DOUBLE_HW(&a)&0x7ff00000) ? a : 0.0;
}
static double WDL_DENORMAL_INLINE denormal_filter_aggressive(double a)
{
  return ((WDL_DENORMAL_DOUBLE_HW(&a)+0x100000)&0x7ff00000) >= WDL_DENORMAL_DOUBLE_AGGRESSIVE_CUTOFF ? a : 0.0;
}

static float WDL_DENORMAL_INLINE denormal_filter(float a)
{
  return (WDL_DENORMAL_FLOAT_W(&a)&0x7f800000) ? a : 0.0f;
}

static float WDL_DENORMAL_INLINE denormal_filter_aggressive(float a)
{
  return ((WDL_DENORMAL_FLOAT_W(&a)+0x800000)&0x7f800000) >= WDL_DENORMAL_FLOAT_AGGRESSIVE_CUTOFF ? a : 0.0f;
}

static void WDL_DENORMAL_INLINE denormal_fix(double *a)
{
  if (!(WDL_DENORMAL_DOUBLE_HW(a)&0x7ff00000)) *a=0.0;
}
static void WDL_DENORMAL_INLINE denormal_fix_aggressive(double *a)
{
  if (((WDL_DENORMAL_DOUBLE_HW(a)+0x100000)&0x7ff00000) < WDL_DENORMAL_DOUBLE_AGGRESSIVE_CUTOFF) *a=0.0;
}
static void WDL_DENORMAL_INLINE denormal_fix(float *a)
{
  if (!(WDL_DENORMAL_FLOAT_W(a)&0x7f800000)) *a=0.0f;
}
static void WDL_DENORMAL_INLINE denormal_fix_aggressive(float *a)
{
  if (((WDL_DENORMAL_FLOAT_W(a)+0x800000)&0x7f800000) < WDL_DENORMAL_FLOAT_AGGRESSIVE_CUTOFF) *a=0.0f;
}



#endif // cplusplus versions

#else // end of !WDL_DENORMAL_DO_NOT_FILTER (and other platform-specific checks)

#define denormal_filter(x) (x)
#define denormal_filter2(x) (x)
#define denormal_filter_double(x) (x)
#define denormal_filter_double2(x) (x)
#define denormal_filter_double_aggressive(x) (x)
#define denormal_filter_float(x) (x)
#define denormal_filter_float2(x) (x)
#define denormal_filter_float_aggressive(x) (x)
#define denormal_filter_aggressive(x) (x)
#define denormal_fix(x) do { } while(0)
#define denormal_fix_aggressive(x) do { } while(0)
#define denormal_fix_double(x) do { } while(0)
#define denormal_fix_double_aggressive(x) do { } while(0)
#define denormal_fix_float(x) do { } while(0)
#define denormal_fix_float_aggressive(x) do { } while(0)

#endif 


////////////////////
// this isnt a denormal function but it is similar, so we'll put it here as a bonus

static void WDL_DENORMAL_INLINE GetDoubleMaxAbsValue(double *out, const double *in) // note: the value pointed to by "out" must be >=0.0, __NOT__ <= -0.0
{
  WDL_UINT64 i, o;
  memcpy(&i,in,sizeof(i));
  memcpy(&o,out,sizeof(o));
  i &= WDL_UINT64_CONST(0x7fffffffffffffff);
  if (i > o) memcpy(out,&i,sizeof(i));
}

static void WDL_DENORMAL_INLINE GetFloatMaxAbsValue(float *out, const float *in) // note: the value pointed to by "out" must be >=0.0, __NOT__ <= -0.0
{
  unsigned int i, o;
  memcpy(&i, in, sizeof(i));
  memcpy(&o, out, sizeof(o));
  i &= 0x7fffffff;
  if (i > o) memcpy(out, &i, sizeof(i));
}


#ifdef __cplusplus
static void WDL_DENORMAL_INLINE GetFloatMaxAbsValue(double *out, const double *in) // note: the value pointed to by "out" must be >=0.0, __NOT__ <= -0.0
{
  GetDoubleMaxAbsValue(out,in);
}
#endif

#endif
