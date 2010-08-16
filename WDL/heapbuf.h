/*
    WDL - heapbuf.h
    Copyright (C) 2005 and later Cockos Incorporated

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

  This file provides the interface and implementation for WDL_HeapBuf, a simple 
  malloc() wrapper for resizeable blocks.

  Also in this file is WDL_TypedBuf which is a templated version WDL_HeapBuf 
  that manages type and type-size.
 
*/

#ifndef _WDL_HEAPBUF_H_
#define _WDL_HEAPBUF_H_

#ifndef WDL_HEAPBUF_IMPL_ONLY

class WDL_HeapBuf
{
  public:
    WDL_HeapBuf(int granul=4096) : m_granul(granul), m_buf(NULL), m_alloc(0), m_size(0), m_mas(0)
    {
    }
    ~WDL_HeapBuf()
    {
      Resize(0);
    }

    void *Get() { return m_buf; }
    int GetSize() { return m_size; }

    void SetGranul(int granul) 
    {
      m_granul=granul;
    }

    void SetMinAllocSize(int mas)
    {
      m_mas=mas;
    }
#define WDL_HEAPBUF_PREFIX 
#else
#define WDL_HEAPBUF_PREFIX WDL_HeapBuf::
#endif

    void * WDL_HEAPBUF_PREFIX Resize(int newsize, bool resizedown
#ifdef WDL_HEAPBUF_INTF_ONLY
      =true); 
#else
#ifdef WDL_HEAPBUF_IMPL_ONLY
    )
#else
    =true)
#endif
    {
      if (newsize < m_mas && newsize < m_alloc)
      {
        m_size=newsize;
        return Get();
      }
      if (!newsize && !m_mas) // special case, free all when resized to 0
      {
        if (resizedown)
        {
          free(m_buf);
          m_buf=NULL;
          m_alloc=0;
        }
      }
      else if (newsize > m_alloc || (resizedown && newsize < m_alloc - (m_granul<<2))) // if we grew over our allocation, or shrunk too far down we should resize down
      {
        int newalloc = (newsize > m_alloc) ? (newsize + m_granul) : newsize;
        if (newalloc < m_mas) newalloc=m_mas;

        if (newalloc != m_alloc || !m_buf)
        {
          void *nbuf=realloc(m_buf,newalloc);
          if (!nbuf) 
          {
            if (!newalloc) return m_buf;
            nbuf=malloc(newalloc);
            if (!nbuf) 
            {
              // todo: throw some error here, because we couldnt allocate our block!!
              nbuf=m_buf;
            }
            else
            {
              if (m_buf) memcpy(nbuf,m_buf,newsize<m_size?newsize:m_size);
              free(m_buf);
              m_buf=0;
            }
          }

          m_buf=nbuf;
          m_alloc=newalloc;
        }
      }

      m_size=newsize;

      return m_buf;
    }
#endif


#ifndef WDL_HEAPBUF_IMPL_ONLY
  private:
    int m_granul;
    void *m_buf;
    int m_alloc;
    int m_size;
    int m_mas;
};

template<class PTRTYPE> class WDL_TypedBuf 
{
  public:
    WDL_TypedBuf(int granul=4096) : m_hb(granul)
    {
    }
    ~WDL_TypedBuf()
    {
    }
    PTRTYPE *Get() { return (PTRTYPE *) m_hb.Get(); }
    int GetSize() { return m_hb.GetSize()/sizeof(PTRTYPE); }

    PTRTYPE *Resize(int newsize, bool resizedown=true) { return (PTRTYPE *)m_hb.Resize(newsize*sizeof(PTRTYPE),resizedown); }

    // leave public so people can set its parameters directly
    WDL_HeapBuf m_hb;
};

#endif
#endif
