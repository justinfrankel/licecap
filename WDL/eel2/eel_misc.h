#ifndef _EEL_MISC_H_
#define _EEL_MISC_H_


#ifndef _WIN32
#include <sys/time.h>
#else
#include <time.h>
#endif
// some generic EEL functions for things like time

static EEL_F NSEEL_CGEN_CALL _eel_sleep(void *opaque, EEL_F *amt)
{
  if (*amt >= 0.0) 
  {
  #ifdef _WIN32
    if (*amt > 30000000.0) Sleep(30000000);
    else Sleep((DWORD)(*amt+0.5));
  #else
    if (*amt > 30000000.0) usleep(((useconds_t)30000000)*1000);
    else usleep((useconds_t)(*amt*1000.0+0.5));
  #endif
  }
  return 0.0;
}

static EEL_F * NSEEL_CGEN_CALL _eel_time(void *opaque, EEL_F *v)
{
  *v = (EEL_F) time(NULL);
  return v;
}

static EEL_F * NSEEL_CGEN_CALL _eel_time_precise(void *opaque, EEL_F *v)
{
#ifdef _WIN32
  LARGE_INTEGER freq,now;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&now);
  *v = (double)now.QuadPart / (double)freq.QuadPart;
  // *v = (EEL_F)timeGetTime() * 0.001;
#else
  struct timeval tm={0,};
  gettimeofday(&tm,NULL);
  *v = tm.tv_sec + tm.tv_usec*0.000001;
#endif
  return v;
}


void EEL_misc_register()
{
  NSEEL_addfunctionex("sleep",1,(char *)_asm_generic1parm_retd,(char *)_asm_generic1parm_retd_end-(char *)_asm_generic1parm_retd,NSEEL_PProc_THIS,(void *)&_eel_sleep);
  NSEEL_addfunctionex("time",1,(char *)_asm_generic1parm,(char *)_asm_generic1parm_end-(char *)_asm_generic1parm,NSEEL_PProc_THIS,(void *)&_eel_time);
  NSEEL_addfunctionex("time_precise",1,(char *)_asm_generic1parm,(char *)_asm_generic1parm_end-(char *)_asm_generic1parm,NSEEL_PProc_THIS,(void *)&_eel_time_precise);

}




#endif
