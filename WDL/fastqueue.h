/*
  WDL - fastqueue.h
  Copyright (C) 2006 and later Cockos Incorporated

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
  

  This file defines and implements a class which can queue arbitrary amounts of data. 
  It is optimized for lots of reads and writes with a significant queue (i.e. it doesnt 
  have to shuffle much memory around).

  The downside is that you can't just ask for a pointer to specific bytes, it may have to peice 
  it together into a buffer of your choosing (or you can step through the buffers using GetPtr()).


*/

#ifndef _WDL_FASTQUEUE_H_
#define _WDL_FASTQUEUE_H_


#include "ptrlist.h"

class WDL_FastQueue
{
public:
  WDL_FastQueue()
  {
    m_avail=0;
    m_bsize=65536-64;
    m_offs=0;
  }
  ~WDL_FastQueue()
  {
    m_queue.Empty(true);
    m_empties.Empty(true);
  }
  
  void Add(const void *buf, int len) // buf can be NULL to add zeroes
  {
    char *inptr=(char *)buf;
    while (len>0)
    {
      WDL_HeapBuf *qb=m_queue.Get(m_queue.GetSize()-1);
      int os;
      if (!qb || (os=qb->GetSize()) >= m_bsize)
      {
        int esz=m_empties.GetSize()-1;

        qb=m_empties.Get(esz);
        if (qb) m_empties.Delete(esz);
        else qb=new WDL_HeapBuf(4096 WDL_HEAPBUF_TRACEPARM("WDL_FastQueue"));

        if (qb) m_queue.Add(qb);
        os=0;
      }

      if (!qb) break;

      int addl=m_bsize-os;
      if (addl>len) addl=len;
      char *b=(char *)qb->Resize(os+addl,false)+os;
      if (inptr)
      {
        memcpy(b,inptr,addl);
        inptr+=addl;
      }
      else memset(b,0,addl);
      len -= addl;
      m_avail+=addl;
    }
  }

  void Clear()
  {
    int x=m_queue.GetSize();
    while (x > 0)
    {
      m_empties.Add(m_queue.Get(--x));
      m_queue.Delete(x);      
    }
    m_offs=0;
    m_avail=0;
  }

  void Advance(int cnt)
  {
    m_offs += cnt;
    m_avail -= cnt;
    if (m_avail<0)m_avail=0;

    WDL_HeapBuf *mq;
    while ((mq=m_queue.Get(0)))
    {
      int sz=mq->GetSize();
      if (m_offs < sz) break;
      m_offs -= sz;
      m_empties.Add(mq);
      m_queue.Delete(0);
    }
    if (!mq||m_offs<0) m_offs=0;
  }

  int Available() // bytes available
  {
    return m_avail;
  }


  int GetPtr(int offset, void **buf) // returns bytes available in this block
  {
    offset += m_offs;

    int x=0;
    WDL_HeapBuf *mq;
    while ((mq=m_queue.Get(x)))
    {
      int sz=mq->GetSize();
      if (offset < sz)
      {
        *buf = (char *)mq->Get() + offset;
        return sz-offset;
      }
      x++;
      offset -= sz;
    }
    *buf=NULL;
    return 0;
  }

  int SetFromBuf(int offs, void *buf, int len) // returns length set
  {
    int pos=0;
    while (len > 0)
    {
      void *p=NULL;
      int l=GetPtr(offs+pos,&p);
      if (!l || !p) break;
      if (l > len) l=len;
      memcpy(p,(char *)buf + pos,l);
      pos += l;
      len -= l;
    }
    return pos;
  }

  int GetToBuf(int offs, void *buf, int len)
  {
    int pos=0;
    while (len > 0)
    {
      void *p=NULL;
      int l=GetPtr(offs+pos,&p);
      if (!l || !p) break;
      if (l > len) l=len;
      memcpy((char *)buf + pos,p,l);
      pos += l;
      len -= l;
    }
    return pos;
  }

private:

  WDL_PtrList<WDL_HeapBuf> m_queue, m_empties;
  int m_offs;
  int m_avail;
  int m_bsize;
  int __pad;
} WDL_FIXALIGN;


#endif //_WDL_FASTQUEUE_H_