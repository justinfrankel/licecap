/*
    WDL - wdlcstring.h
    Copyright (C) 2005 and later, Cockos Incorporated
  
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

/*
C string manipulation utilities -- [v]snprintf for Win32, also snprintf_append, lstrcatn, etc
  */
#ifndef _WDL_CSTRING_H_
#define _WDL_CSTRING_H_

#include <stdarg.h>
#include <stdio.h>

#include "wdltypes.h"

#ifdef _WDL_CSTRING_IMPL_ONLY_
  #ifdef _WDL_CSTRING_IF_ONLY_
    #undef _WDL_CSTRING_IF_ONLY_
  #endif
  #define _WDL_CSTRING_PREFIX 
#else
  #define _WDL_CSTRING_PREFIX static
#endif



#if defined(_WIN32) && defined(_MSC_VER)
  // provide snprintf()/vsnprintf() for win32 -- note that these have no way of knowing
  // what the amount written was, code should(must) be written to not depend on this.
  #ifdef snprintf
  #undef snprintf
  #endif
  #define snprintf WDL_snprintf

  #ifdef vsnprintf
  #undef vsnprintf
  #endif
  #define vsnprintf WDL_vsnprintf

#endif // win32 snprintf/vsnprintf


#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WDL_CSTRING_IF_ONLY_

  void lstrcpyn_safe(char *o, const char *in, int count);
  void lstrcatn(char *o, const char *in, int count);
  void WDL_VARARG_WARN(printf,3,4) snprintf_append(char *o, int count, const char *format, ...);
  void vsnprintf_append(char *o, int count, const char *format, va_list va);

  #if defined(_WIN32) && defined(_MSC_VER)
    void WDL_vsnprintf(char *o, size_t count, const char *format, va_list args);
    void WDL_VARARG_WARN(printf,3,4) WDL_snprintf(char *o, size_t count, const char *format, ...);
  #endif

#else


  #if defined(_WIN32) && defined(_MSC_VER)

    _WDL_CSTRING_PREFIX void WDL_vsnprintf(char *o, size_t count, const char *format, va_list args)
    {
      if (count>0)
      {
        int rv;
        o[0]=0;
        rv=_vsnprintf(o,count,format,args); // returns -1  if over, and does not null terminate, ugh
        if (rv < 0 || rv>=(int)count-1) o[count-1]=0;
      }
    }
    _WDL_CSTRING_PREFIX void WDL_VARARG_WARN(printf,3,4) WDL_snprintf(char *o, size_t count, const char *format, ...)
    {
      if (count>0)
      {
        int rv;
        va_list va;
        va_start(va,format);
        o[0]=0;
        rv=_vsnprintf(o,count,format,va); // returns -1  if over, and does not null terminate, ugh
        va_end(va);

        if (rv < 0 || rv>=(int)count-1) o[count-1]=0; 
      }
    }
  #endif

  _WDL_CSTRING_PREFIX void lstrcpyn_safe(char *o, const char *in, int count)
  {
    if (count>0)
    {
      while (--count>0 && *in) *o++ = *in++;
      *o=0;
    }
  }

  _WDL_CSTRING_PREFIX void lstrcatn(char *o, const char *in, int count)
  {
    if (count>0)
    {
      while (*o) { if (--count < 1) return; o++; }
      while (--count>0 && *in) *o++ = *in++;
      *o=0;
    }
  }
  _WDL_CSTRING_PREFIX void WDL_VARARG_WARN(printf,3,4) snprintf_append(char *o, int count, const char *format, ...)
  {
    if (count>0)
    {
      va_list va;
      while (*o) { if (--count < 1) return; o++; }
      va_start(va,format);
      vsnprintf(o,count,format,va);
      va_end(va);
    }
  } 

  _WDL_CSTRING_PREFIX void vsnprintf_append(char *o, int count, const char *format, va_list va)
  {
    if (count>0)
    {
      while (*o) { if (--count < 1) return; o++; }
      vsnprintf(o,count,format,va);
    }
  }

#endif


#ifdef __cplusplus
};
#endif

#undef _WDL_CSTRING_PREFIX

#endif
