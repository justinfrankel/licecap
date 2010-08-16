/*
    WDL - queue.h
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

  This file provides a simple class for a FIFO queue of bytes. It uses a simple buffer,
  so should not generally be used for large quantities of data (it can advance the queue 
  pointer, but Compact() needs to be called regularly to keep memory usage down, and when
  it is called, there's a memcpy() penalty for the remaining data. oh well, is what it is).

  You may also wish to look at fastqueue.h or circbuf.h if these limitations aren't acceptable.

*/

#ifndef _WDL_QUEUE_H_
#define _WDL_QUEUE_H_

#include "heapbuf.h"


class WDL_Queue 
{
public:
  WDL_Queue() : m_pos(0) { }
  ~WDL_Queue() { }

  void *Add(const void *buf, int len)
  {
    int olen=m_hb.GetSize();
    void *obuf=m_hb.Resize(olen+len,false);
    if (!obuf) return 0;
    if (buf) memcpy((char*)obuf+olen,buf,len);
    return (char*)obuf+olen;
  }

  int GetSize()
  {
    return m_hb.GetSize()-m_pos;
  }

  void *Get()
  {
    void *buf=m_hb.Get();
    if (buf && m_pos >= 0 && m_pos < m_hb.GetSize()) return (char *)buf+m_pos;
    return NULL;
  }

  int Available() { return m_hb.GetSize() - m_pos; }

  void Clear()
  {
    m_pos=0;
    m_hb.Resize(0,false);
  }

  void Advance(int bytecnt) { m_pos+=bytecnt; }

  void Compact(bool allocdown=false)
  {
    if (m_pos > 0)
    {
      int olen=m_hb.GetSize();
      if (m_pos < olen)
      {
        void *a=m_hb.Get();
        if (a) memmove(a,(char*)a+m_pos,olen-m_pos);
        m_hb.Resize(olen-m_pos,allocdown);
      }
      else m_hb.Resize(0,allocdown);
      m_pos=0;
    }
  }

  void SetGranul(int granul) { m_hb.SetGranul(granul); }

private:
  WDL_HeapBuf m_hb;
  int m_pos;
};





template <class T> class WDL_TypedQueue
{
public:
  WDL_TypedQueue() : m_pos(0) { }
  ~WDL_TypedQueue() { }

  T *Add(const T *buf, int len)
  {
    len *= sizeof(T);
    int olen=m_hb.GetSize();
    void *obuf=m_hb.Resize(olen+len,false);
    if (!obuf) return 0;
    if (buf) memcpy((char*)obuf+olen,buf,len);
    return (T*) ((char*)obuf+olen);
  }

  int GetSize()
  {
    return (m_hb.GetSize()-m_pos)/sizeof(T);
  }

  T *Get()
  {
    void *buf=m_hb.Get();
    if (buf && m_pos >= 0 && m_pos < m_hb.GetSize()) return (T*)((char *)buf+m_pos);
    return NULL;
  }

  int Available() { return (m_hb.GetSize() - m_pos)/sizeof(T); }

  void Clear()
  {
    m_pos=0;
    m_hb.Resize(0,false);
  }

  void Advance(int cnt) { m_pos+=cnt*sizeof(T); }

  void Compact(bool allocdown=false)
  {
    if (m_pos > 0)
    {
      int olen=m_hb.GetSize();
      if (m_pos < olen)
      {
        void *a=m_hb.Get();
        if (a) memmove(a,(char*)a+m_pos,olen-m_pos);
        m_hb.Resize(olen-m_pos,allocdown);
      }
      else m_hb.Resize(0,allocdown);
      m_pos=0;
    }
  }

  void SetGranul(int granul) { m_hb.SetGranul(granul); }

private:
  WDL_HeapBuf m_hb;
  int m_pos;
};



#endif


