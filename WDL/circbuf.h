/*
    WDL - circbuf.h
    Copyright (C) 2005 Cockos Incorporated

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

  This file provides a simple class for a circular FIFO queue of bytes. It 
  has a strong advantage over WDL_Queue with large buffers, as it does far 
  fewer memcpy()'s.

*/

#ifndef _WDL_CIRCBUF_H_
#define _WDL_CIRCBUF_H_

#include "heapbuf.h"

class WDL_CircBuf
{
public:
  WDL_CircBuf() : m_hb(4096 WDL_HEAPBUF_TRACEPARM("WDL_CircBuf"))
  {
    m_size = 0;
    m_inbuf = 0;
    m_head = m_tail = 0;
  }
  ~WDL_CircBuf()
  {
  }
  void SetSize(int size, bool keepcontents=false)
  {
    WDL_HeapBuf tmp(4096 WDL_HEAPBUF_TRACEPARM("WDL_CircBuf/TEMP"));
    if (keepcontents) 
    {
      int ms=NbInBuf(); 
      if (ms>size) ms=size;
      if (ms>0) Get(tmp.Resize(ms),ms);
    }
    m_size = size;
    m_hb.Resize(size);
    Reset();
    if (tmp.GetSize()) Add(tmp.Get(),tmp.GetSize()); // add old data back in
  }
  void Reset()
  {
    m_head = (char *)m_hb.Get();
    m_tail = (char *)m_hb.Get();
    m_endbuf = (char *)m_hb.Get() + m_size;
    m_inbuf = 0;
  }
  int Add(const void *buf, int l)
  {
    char *p = (char *)buf;
    if (l > m_size) l = m_size;
    int put = l;
    int l2;
    if (!m_size) return 0;
    l2 = (int) (m_endbuf - m_head);
    if (l2 <= l) 
    {
      memcpy(m_head, p, l2);
      m_head = (char *)m_hb.Get();
      p += l2;
      l -= l2;
    }
    if (l) 
    {
      memcpy(m_head, p, l);
      m_head += l;
      p += l;
    }
    m_inbuf += put;
    if (m_inbuf > m_size) m_inbuf = m_size;
    return put;
  }
  int Get(void *buf, int l)
  {
    char *p = (char *)buf;
    int got = 0;
    if (!m_size) return 0;
    if (m_inbuf <= 0) return 0;
    if (l > m_inbuf) l = m_inbuf;
    m_inbuf -= l;
    got = l;
    if (m_tail+l >= m_endbuf) 
    {
      int l1 = (int) (m_endbuf - m_tail);
      l -= l1;
      memcpy(p, m_tail, l1);
      m_tail = (char *)m_hb.Get();
      p += l1;
      memcpy(p, m_tail, l);
      m_tail += l;
    } 
    else 
    {
      memcpy(p, m_tail, l);
      m_tail += l;
    }
    return got;
  }
  int Available() { return m_size - m_inbuf; }
  int NbInBuf() { return m_inbuf; }

private:
  WDL_HeapBuf m_hb;
  char *m_head, *m_tail, *m_endbuf;
  int m_size, m_inbuf;
} WDL_FIXALIGN;


template <class T>
class WDL_TypedCircBuf
{
public:

    WDL_TypedCircBuf() {}
    ~WDL_TypedCircBuf() {}

    void SetSize(int size, bool keepcontents = false)
    {
        mBuf.SetSize(size * sizeof(T), keepcontents);
    }

    void Reset()
    {
        mBuf.Reset();
    }

    int Add(const T* buf, int l)
    {
        return mBuf.Add(buf, l * sizeof(T)) / sizeof(T);
    }

    int Get(T* buf, int l)
    {
        return mBuf.Get(buf, l * sizeof(T)) / sizeof(T);
    }

    int Available() 
    {
        return mBuf.Available() / sizeof(T);
    }
     int NbInBuf() 
     { 
         return mBuf.NbInBuf() / sizeof(T);
     }

private:
    WDL_CircBuf mBuf;
} WDL_FIXALIGN;

#endif
