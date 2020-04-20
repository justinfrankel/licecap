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

  This file provides a simple class for a circular FIFO queue of bytes. 

*/

#ifndef _WDL_CIRCBUF_H_
#define _WDL_CIRCBUF_H_

#include "heapbuf.h"

class WDL_CircBuf
{
public:
  WDL_CircBuf()
  {
    m_inbuf = m_wrptr = 0;
    m_buf = NULL;
    m_alloc = 0;
  }
  ~WDL_CircBuf()
  {
    free(m_buf);
  }
  void SetSize(int size)
  {
    if (size<0) size=0;
    m_inbuf = m_wrptr = 0;
    if (size != m_alloc || !m_buf)
    {
      m_alloc = size;
      free(m_buf);
      m_buf = size ? (char*)malloc(size) : NULL;
    }
  }
  }
  void Reset() { m_inbuf = m_wrptr = 0; }
  int Add(const void *buf, int l)
  {
    if (!m_buf) return 0;
    const int bf = m_alloc - m_inbuf;
    if (l>bf) l = bf;
    if (l > 0)
    {
      m_wrptr = __write_bytes(m_wrptr,l,buf);
      m_inbuf += l;
    }
    return l;
  }
  int Peek(void *buf, int offs, int len) const
  {
    if (offs<0||!m_buf) return 0;
    const int ibo = m_inbuf-offs;
    if (len > ibo) len = ibo;
    if (len > 0)
    {
      int rp = m_wrptr - ibo;
      if (rp < 0) rp += m_alloc;
      const int wr1 = m_alloc - rp;
      char * const rd = m_buf;
      if (wr1 < len)
      {
        memcpy(buf,rd+rp,wr1);
        memcpy((char*)buf+wr1,rd,len-wr1);
      }
      else
      {
        memcpy(buf,rd+rp,len);
      }
    }
    return len;
  }

  int Get(void *buf, int l)
  {
    const int amt = Peek(buf,0,l);
    m_inbuf -= amt;
    return amt;
  }
  int NbFree() const { return m_alloc - m_inbuf; } // formerly Available()
  int NbInBuf() const { return m_inbuf; }
  int GetTotalSize() const { return m_alloc; }

private:
  int __write_bytes(int wrptr, int l, const void *buf) // no bounds checking, return end offset
  {
    const int wr1 = m_alloc-wrptr;
    char * const p = m_buf, * const pw = p + wrptr;
    if (wr1 < l)
    {
      if (buf)
      {
        memcpy(pw, buf, wr1);
        memcpy(p, (char*)buf + wr1, l-wr1);
      }
      else
      {
        memset(pw, 0, wr1);
        memset(p, 0, l-wr1);
      }
      return l-wr1;
    }

    if (buf) memcpy(pw, buf, l);
    else memset(pw, 0, l);
    return wr1 == l ? 0 : wrptr+l;
  }

  char *m_buf;
  int m_inbuf, m_wrptr,m_alloc;
} WDL_FIXALIGN;


template <class T>
class WDL_TypedCircBuf
{
public:

    WDL_TypedCircBuf() {}
    ~WDL_TypedCircBuf() {}

    void SetSize(int size)
    {
      mBuf.SetSize(size * sizeof(T));
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

    int Peek(T* buf, int offs, int l)
    {
      return mBuf.Peek(buf, offs*sizeof(T), l * sizeof(T)) / sizeof(T);
    }
    int NbFree() const { return mBuf.NbFree() / sizeof(T); } // formerly Available()
    int ItemsInQueue() const { return mBuf.NbInBuf() / sizeof(T); }
    int NbInBuf() const { return mBuf.NbInBuf() / sizeof(T); }
    int GetTotalSize() const { return mBuf.GetTotalSize() / sizeof(T); }

private:
    WDL_CircBuf mBuf;
} WDL_FIXALIGN;

#endif
