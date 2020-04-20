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

  void SetSizePreserveContents(int newsz)
  {
    if (newsz < NbInBuf()) newsz = NbInBuf(); // do not allow destructive resize down
    const int oldsz = m_alloc, dsize = newsz - oldsz;
    if (!dsize) return;
    if (!m_inbuf||!m_buf) { SetSize(newsz); return; }

    const int div1 = m_inbuf - m_wrptr; // div1>0 is size of end block, div1<0 is offset of start block
    char *buf=NULL;
    if (dsize > 0)
    {
      buf = (char *)realloc(m_buf, newsz);
      if (WDL_NORMALLY(buf) && div1 > 0) // block crossing loop, need to shuffle some data
      {
        if (div1 > m_wrptr) // m_wrptr is size of start block, div1 is size of end block
        {
          // end block is larger than start, move some of start block to end of end block and shuffle forward start
          if (dsize >= m_wrptr)
          {
            if (m_wrptr>0) memmove(buf+oldsz,buf,m_wrptr);
            m_wrptr += oldsz;
          }
          else
          {
            memmove(buf + oldsz, buf, dsize);
            m_wrptr -= dsize;
            memmove(buf, buf+dsize, m_wrptr);
          }
        }
        else // end block is smaller, move it to the new end of buffer
        {
          memmove(buf + newsz - div1, buf + oldsz - div1, div1);
        }
      }
    }
    else if (div1 < 0) // shrinking, and not a wrapped buffer
    {
      if (m_wrptr > newsz)
      {
        memmove(m_buf,m_buf-div1, m_inbuf);
        m_wrptr = m_inbuf;
      }
      buf = (char *)realloc(m_buf, newsz);
    }

    if (!buf) // failed realloc(), or sizing down with block crossing loop boundary
    {
      buf = (char *)malloc(newsz);
      if (WDL_NOT_NORMALLY(!buf)) return;
      const int peeked = Peek(buf,0,m_inbuf);
      if (peeked != m_inbuf) { WDL_ASSERT(peeked == m_inbuf); }
      free(m_buf);
      m_wrptr = m_inbuf = peeked;
    }
    if (m_wrptr > newsz) { WDL_ASSERT(m_wrptr <= newsz); }
    if (m_wrptr >= newsz) m_wrptr=0;
    m_alloc = newsz;
    m_buf = buf;
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
  void UnAdd(int amt)
  {
    if (amt > 0)
    {
      if (amt > m_inbuf) amt=m_inbuf;
      m_wrptr -= amt;
      if (m_wrptr < 0) m_wrptr += m_alloc;
      m_inbuf -= amt;
    }
  }

  void Skip(int l) // can be used to rewind read pointer
  {
    m_inbuf -= l;
    if (m_inbuf<0) m_inbuf=0;
    else if (m_inbuf>m_alloc) m_inbuf=m_alloc;
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

  void WriteAtReadPointer(const void *buf, int len, int offs=0)
  {
    if (WDL_NOT_NORMALLY(offs<0) || WDL_NOT_NORMALLY(offs>=m_inbuf)) return;
    if (!m_buf || len<1) return;
    if (offs+len > m_inbuf) len = m_inbuf-offs;

    int write_offs = m_wrptr - m_inbuf + offs;
    if (write_offs < 0) write_offs += m_alloc;
    __write_bytes(write_offs, len, buf);
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
    void SetSizePreserveContents(int size)
    {
      mBuf.SetSizePreserveContents(size*sizeof(T));
    }

    void Reset()
    {
      mBuf.Reset();
    }

    void UnAdd(int l) { mBuf.UnAdd(l*sizeof(T)); }

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
    void Skip(int l) { mBuf.Skip(l*sizeof(T)); }

    void WriteAtReadPointer(const void *buf, int len, int offs=0) { mBuf.WriteAtReadPointer(buf,len*sizeof(T),offs*sizeof(T)); }

    int NbFree() const { return mBuf.NbFree() / sizeof(T); } // formerly Available()
    int ItemsInQueue() const { return mBuf.NbInBuf() / sizeof(T); }
    int NbInBuf() const { return mBuf.NbInBuf() / sizeof(T); }
    int GetTotalSize() const { return mBuf.GetTotalSize() / sizeof(T); }

private:
    WDL_CircBuf mBuf;
} WDL_FIXALIGN;

#endif
