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


// #define WDL_HEAPBUF_SMALLONSTACKSIZE 216 // if you'd like to allocate some data for small buffers as a member variable uncomment this
//#define WDL_HEAPBUF_TRACE

#ifdef WDL_HEAPBUF_TRACE
#include <windows.h>
#define WDL_HEAPBUF_TRACEPARM(x) ,(x)
#else
#define WDL_HEAPBUF_TRACEPARM(x)
#endif

#include "wdltypes.h"

class WDL_HeapBuf
{
  public:
    WDL_HeapBuf(int granul=4096
#ifdef WDL_HEAPBUF_TRACE
      , const char *tracetype="WDL_HeapBuf"
#endif
      ) : m_alloc(0), m_size(0), m_mas(0)
    {
      SetGranul(granul);
#ifdef WDL_HEAPBUF_SMALLONSTACKSIZE
      m_buf=m_smallbuf;
#else
      m_buf=0;
#endif

#ifdef WDL_HEAPBUF_TRACE
      m_tracetype = tracetype;
      char tmp[512];
      wsprintf(tmp,"WDL_HeapBuf: created type: %s granul=%d\n",tracetype,granul);
      OutputDebugString(tmp);
#endif
    }
    ~WDL_HeapBuf()
    {
#ifdef WDL_HEAPBUF_TRACE
      char tmp[512];
      wsprintf(tmp,"WDL_HeapBuf: destroying type: %s (alloc=%d, size=%d)\n",m_tracetype,m_alloc,m_size);
      OutputDebugString(tmp);
#endif
#ifdef WDL_HEAPBUF_SMALLONSTACKSIZE
      if (m_buf!=m_smallbuf) 
#endif
        free(m_buf);
    }


    WDL_HeapBuf(const WDL_HeapBuf &cp)
    {
      m_buf=0;
      CopyFrom(&cp,true);
    }
    WDL_HeapBuf &operator=(const WDL_HeapBuf &cp)
    {
      m_buf=0;
      CopyFrom(&cp,true);
      return *this;
    }

    void CopyFrom(const WDL_HeapBuf *hb, bool exactCopyOfConfig=false)
    {
      if (exactCopyOfConfig) // copy all settings
      {
        free(m_buf);
        memcpy(this,hb,sizeof(WDL_HeapBuf));
        if (hb->m_buf) 
        {
          m_buf=malloc(m_alloc);
          if (m_buf) memcpy(m_buf,hb->m_buf,m_size);
          else m_size=m_alloc=0;
        }
      }
      else // copy just the data + size
      {
        int newsz=hb->GetSize();
        Resize(newsz);
        if (GetSize()!=newsz) Resize(0);
        else memcpy(Get(),hb->Get(),newsz);
      }

    }

    void *Get() const { return m_size?m_buf:NULL; }
    int GetSize() const { return m_size; }

    void SetGranul(int granul) 
    {
      m_granul = granul;      
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

//#define WDL_HEAPBUF_DYNAMIC
#ifdef WDL_HEAPBUF_DYNAMIC
      // ignoring m_granul and m_mas

      if (newsize!=m_size)
      {
        if ((newsize > m_size && newsize <= m_alloc) || (newsize < m_size && !resizedown))
        {
          m_size = newsize;
          return m_buf;
        }

        // next highest power of 2
        int n = newsize;
        if (n)
        {
          if (n < 64)
          {
            n = 64;
          }
          else
          {
            --n;
            n = (n>>1)|n;
            n = (n>>2)|n;
            n = (n>>4)|n;
            n = (n>>8)|n;
            n = (n>>16)|n;
            ++n;
          }
        }

        if (n == m_alloc)
        {
          m_size = newsize;
          return m_buf;
        }

        void* newbuf = realloc(m_buf, n);  // realloc==free when size==0
        if (newbuf || !newsize)
        {
          m_alloc = n;
          m_buf = newbuf;
          m_size = newsize;
        }      
      }
      
      return (m_size ? m_buf : 0);
    }
#else // WDL_HEAPBUF_DYNAMIC
      if (newsize!=m_size)
      {
        // if we are not using m_smallbuf or we need to not use it
        // and if if growing or resizing down
        if (
#ifdef WDL_HEAPBUF_SMALLONSTACKSIZE
          (m_buf != m_smallbuf || newsize > sizeof(m_smallbuf)) &&
#endif
            (newsize > m_alloc || 
             (resizedown && newsize < m_size && 
                            newsize < m_alloc/2 && 
                            newsize < m_alloc - (m_granul<<2)))) 
        {
          int granul=newsize/2;
          int newalloc;
          if (granul < m_granul) granul=m_granul;

          if (m_granul<4096) newalloc=newsize+granul;
          else
          {
            granul &= ~4095;
            if (granul< 4096) granul=4096;
            else if (granul>4*1024*1024) granul=4*1024*1024;
            newalloc = ((newsize + granul + 96)&~4095)-96;
          }
         
          if (newalloc < m_mas) newalloc=m_mas;

          if (newalloc != m_alloc)
          {
  #ifdef WDL_HEAPBUF_TRACE
            char tmp[512];
            wsprintf(tmp,"WDL_HeapBuf: type %s realloc(%d) from %d\n",m_tracetype,newalloc,m_alloc);
            OutputDebugString(tmp);
  #endif
            void *nbuf=
  #ifdef WDL_HEAPBUF_SMALLONSTACKSIZE
              (m_buf == m_smallbuf) ? NULL : 
  #endif
            realloc(m_buf,newalloc);
            if (!nbuf && newalloc) 
            {
              if (!(nbuf=malloc(newalloc))) return m_size?m_buf:0; // failed, do not resize

              if (m_buf) 
              {
                int sz=newsize<m_size?newsize:m_size;
                if (sz>0) memcpy(nbuf,m_buf,sz);
#ifdef WDL_HEAPBUF_SMALLONSTACKSIZE
                if (m_buf != m_smallbuf) 
#endif
                  free(m_buf);
              }
            }

            m_buf=nbuf;
            m_alloc=newalloc;
          } // alloc size change
        } // need size up or down
        m_size=newsize;
      } // size change
      return m_size?m_buf:0;
    }
#endif // WDL_HEAPBUF_DYNAMIC

#endif // !WDL_HEAPBUF_IMPL_ONLY, I think

#ifndef WDL_HEAPBUF_IMPL_ONLY
  private:
    void *m_buf;
    int m_granul;
    int m_alloc;
    int m_size;
    int m_mas;

#ifdef WDL_HEAPBUF_SMALLONSTACKSIZE
    double m_smallbuf[WDL_HEAPBUF_SMALLONSTACKSIZE/sizeof(double)]; // for small uses
#endif
#ifdef WDL_HEAPBUF_TRACE
    const char *m_tracetype;
#endif
};

template<class PTRTYPE> class WDL_TypedBuf 
{
  public:
    WDL_TypedBuf(int granul=4096
#ifdef WDL_HEAPBUF_TRACE
      , const char *tracetype="WDL_TypedBuf"
#endif      
      ) : m_hb(granul WDL_HEAPBUF_TRACEPARM(tracetype))
    {
    }
    ~WDL_TypedBuf()
    {
    }
    PTRTYPE *Get() { return (PTRTYPE *) m_hb.Get(); }
    int GetSize() { return m_hb.GetSize()/sizeof(PTRTYPE); }

    PTRTYPE *Resize(int newsize, bool resizedown=true) { return (PTRTYPE *)m_hb.Resize(newsize*sizeof(PTRTYPE),resizedown); }

  private:
    WDL_HeapBuf m_hb;
};

#endif
#endif
