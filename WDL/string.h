/*
    WDL - string.h
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

  This file provides a simple class for variable-length string manipulation.
  It provides only the simplest features, and does not do anything confusing like
  operator overloading. It uses a WDL_HeapBuf for internal storage.

  You can do Set, Get, Append, Insert, and SetLen.. that's about it
  
*/

#ifndef _WDL_STRING_H_
#define _WDL_STRING_H_

#include "heapbuf.h"

#ifndef WDL_STRING_IMPL_ONLY
class WDL_String
{
public:
  WDL_String(const char *initial=NULL, int initial_len=0)
  {
    if (initial) Set(initial,initial_len);
  }
  WDL_String(WDL_String &s)
  {
    Set(s.Get());
  }
  WDL_String(WDL_String *s)
  {
    if (s && s != this) Set(s->Get());
  }
  ~WDL_String()
  {
  }
#define WDL_STRING_PREFIX 
#else
#define WDL_STRING_PREFIX WDL_String::
#endif

  void WDL_STRING_PREFIX Set(const char *str, int maxlen
#ifdef WDL_STRING_INTF_ONLY
      =0); 
#else
#ifdef WDL_STRING_IMPL_ONLY
    )
#else
    =0)
#endif
  {
    int s=strlen(str);
    if (maxlen && s > maxlen) s=maxlen;   

    char *newbuf=(char*)m_hb.Resize(s+1);
    if (newbuf) 
    {
      memcpy(newbuf,str,s);
      newbuf[s]=0;
    }
  }
#endif

  void WDL_STRING_PREFIX Append(const char *str, int maxlen
#ifdef WDL_STRING_INTF_ONLY
      =0); 
#else
#ifdef WDL_STRING_IMPL_ONLY
    )
#else
    =0)
#endif
  {
    int s=strlen(str);
    if (maxlen && s > maxlen) s=maxlen;

    int olds=strlen(Get());

    char *newbuf=(char*)m_hb.Resize(olds + s + 1);
    if (newbuf)
    {
      memcpy(newbuf + olds, str, s);
      newbuf[olds+s]=0;
    }
  }
#endif

  void WDL_STRING_PREFIX DeleteSub(int position, int len)
#ifdef WDL_STRING_INTF_ONLY
    ;
#else
    {
	  char *p=Get();
	  if (!p || !*p) return;

	  int l=strlen(p);
	  if (position < 0 || position >= l) return;
	  if (position+len > l) len=l-position;
	  memmove(p+position,p+position+len,l-position-len+1); // +1 for null
  }
#endif

  void WDL_STRING_PREFIX Insert(const char *str, int position, int maxlen
#ifdef WDL_STRING_INTF_ONLY
      =0); 
#else
#ifdef WDL_STRING_IMPL_ONLY
    )
#else
    =0)
#endif
  {
	  int sl=strlen(Get());
	  if (position > sl) position=sl;

	  int ilen=strlen(str);
	  if (maxlen > 0 && maxlen < ilen) ilen=maxlen;

	  Append(str);
	  char *cur=Get();

      	  memmove(cur+position+ilen,cur+position,sl-position);
	  memcpy(cur+position,str,ilen);
	  cur[sl+ilen]=0;
  }
#endif

  void WDL_STRING_PREFIX SetLen(int length) // sets number of chars allocated for string, not including null terminator
#ifdef WDL_STRING_INTF_ONLY
    ; 
#else
  {                       // can use to resize down, too, or resize up for a sprintf() etc
    char *b=(char*)m_hb.Resize(length+1);
    if (b) b[length]=0;
  }
#endif

#ifndef WDL_STRING_IMPL_ONLY
  char *Get()
  {
    if (m_hb.Get()) return (char *)m_hb.Get();
    return "";
  }

  private:
    WDL_HeapBuf m_hb;
};
#endif

#endif

