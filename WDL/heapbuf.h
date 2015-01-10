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
    // interface 
#ifdef WDL_HEAPBUF_INTF_ONLY
    void *Resize(int newsize, bool resizedown=true);
    void CopyFrom(const WDL_HeapBuf *hb, bool exactCopyOfConfig=false);
#endif
    void *Get() const { return m_size?m_buf:NULL; }
    int GetSize() const { return m_size; }
    void *GetAligned(int align) const {  return (void *)(((UINT_PTR)Get() + (align-1)) & ~(UINT_PTR)(align-1)); }

    void SetGranul(int granul) { m_granul = granul; }
    int GetGranul() const { return m_granul; }

    void *ResizeOK(int newsize, bool resizedown = true) { void *p=Resize(newsize, resizedown); return GetSize() == newsize ? p : NULL; }
    
    WDL_HeapBuf(const WDL_HeapBuf &cp)
    {
      m_buf=0;
      CopyFrom(&cp,true);
    }
    WDL_HeapBuf &operator=(const WDL_HeapBuf &cp)
    {
      CopyFrom(&cp,false);
      return *this;
    }



  #ifndef WDL_HEAPBUF_TRACE
    explicit WDL_HeapBuf(int granul=4096) : m_buf(NULL), m_alloc(0), m_size(0), m_granul(granul)
    {
    }
    ~WDL_HeapBuf()
    {
      free(m_buf);
    }
  #else
    explicit WDL_HeapBuf(int granul=4096, const char *tracetype="WDL_HeapBuf"
      ) : m_buf(NULL), m_alloc(0), m_size(0), m_granul(granul)
    {
      m_tracetype = tracetype;
      char tmp[512];
      wsprintf(tmp,"WDL_HeapBuf: created type: %s granul=%d\n",tracetype,granul);
      OutputDebugString(tmp);
    }
    ~WDL_HeapBuf()
    {
      char tmp[512];
      wsprintf(tmp,"WDL_HeapBuf: destroying type: %s (alloc=%d, size=%d)\n",m_tracetype,m_alloc,m_size);
      OutputDebugString(tmp);
      free(m_buf);
    }
  #endif

#endif // !WDL_HEAPBUF_IMPL_ONLY

    // implementation bits
#ifndef WDL_HEAPBUF_INTF_ONLY
    #ifdef WDL_HEAPBUF_IMPL_ONLY
      void *WDL_HeapBuf::Resize(int newsize, bool resizedown)
    #else
      void *Resize(int newsize, bool resizedown=true)
    #endif
      {
        if (newsize<0) newsize=0;
        #ifdef DEBUG_TIGHT_ALLOC // horribly slow, do not use for release builds
          if (newsize == m_size) return m_buf;

          int a = newsize; 
          if (a > m_size) a=m_size;
          void *newbuf = newsize ? malloc(newsize) : 0;
          if (!newbuf && newsize) 
          {
            #ifdef WDL_HEAPBUF_ONMALLOCFAIL
                WDL_HEAPBUF_ONMALLOCFAIL(newsize)
            #endif
            return m_buf;
          }
          if (newbuf&&m_buf) memcpy(newbuf,m_buf,a);
          m_size=m_alloc=newsize;
          free(m_buf);
          return m_buf=newbuf;
        #endif

        //#define WDL_HEAPBUF_DYNAMIC
        #ifdef WDL_HEAPBUF_DYNAMIC
          // ignoring m_granul

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
            #ifdef WDL_HEAPBUF_ONMALLOCFAIL
              if (!newbuf && n) { WDL_HEAPBUF_ONMALLOCFAIL(n) } ;
            #endif
            if (newbuf || !newsize)
            {
              m_alloc = n;
              m_buf = newbuf;
              m_size = newsize;
            }      
          }
      
          return (m_size ? m_buf : 0);
        #else // WDL_HEAPBUF_DYNAMIC
          if (newsize!=m_size)
          {
            int resizedown_under = 0;
            if (resizedown && newsize < m_size)
            {
              // shrinking buffer: only shrink if allocation decreases to min(alloc/2, alloc-granul*4) or 0
              resizedown_under = m_alloc - (m_granul<<2);
              if (resizedown_under > m_alloc/2) resizedown_under = m_alloc/2;
              if (resizedown_under < 1) resizedown_under=1;
            }
  
            if (newsize > m_alloc || newsize < resizedown_under)
            {
              int granul=newsize/2;
              int newalloc;
              if (granul < m_granul) granul=m_granul;
  
              if (newsize<1) newalloc=0;
              else if (m_granul<4096) newalloc=newsize+granul;
              else
              {
                granul &= ~4095;
                if (granul< 4096) granul=4096;
                else if (granul>4*1024*1024) granul=4*1024*1024;
                newalloc = ((newsize + granul + 96)&~4095)-96;
              }
         
              if (newalloc != m_alloc)
              {

                #ifdef WDL_HEAPBUF_TRACE
                  char tmp[512];
                  wsprintf(tmp,"WDL_HeapBuf: type %s realloc(%d) from %d\n",m_tracetype,newalloc,m_alloc);
                  OutputDebugString(tmp);
                #endif
                if (newalloc <= 0)
                {
                  free(m_buf);
                  m_buf=0;
                  m_alloc=0;
                  m_size=0;
                  return 0;
                }
                void *nbuf=realloc(m_buf,newalloc);
                if (!nbuf)
                {
                  if (!(nbuf=malloc(newalloc))) 
                  {
                    #ifdef WDL_HEAPBUF_ONMALLOCFAIL
                      WDL_HEAPBUF_ONMALLOCFAIL(newalloc);
                    #endif
                    return m_size?m_buf:0; // failed, do not resize
                  }

                  if (m_buf) 
                  {
                    int sz=newsize<m_size?newsize:m_size;
                    if (sz>0) memcpy(nbuf,m_buf,sz);
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
        #endif // WDL_HEAPBUF_DYNAMIC
      }

    #ifdef WDL_HEAPBUF_IMPL_ONLY
      void WDL_HeapBuf::CopyFrom(const WDL_HeapBuf *hb, bool exactCopyOfConfig)
    #else
      void CopyFrom(const WDL_HeapBuf *hb, bool exactCopyOfConfig=false)
    #endif
      {
        if (exactCopyOfConfig) // copy all settings
        {
          free(m_buf);

          #ifdef WDL_HEAPBUF_TRACE
            m_tracetype = hb->m_tracetype;
          #endif
          m_granul = hb->m_granul;

          m_size=m_alloc=0;
          m_buf=hb->m_buf && hb->m_alloc>0 ? malloc(m_alloc = hb->m_alloc) : NULL;
          #ifdef WDL_HEAPBUF_ONMALLOCFAIL
            if (!m_buf && m_alloc) { WDL_HEAPBUF_ONMALLOCFAIL(m_alloc) } ;
          #endif
          if (m_buf) memcpy(m_buf,hb->m_buf,m_size = hb->m_size);
          else m_alloc=0;
        }
        else // copy just the data + size
        {
          const int newsz=hb->GetSize();
          Resize(newsz,true);
          if (GetSize()!=newsz) Resize(0);
          else memcpy(Get(),hb->Get(),newsz);
        }
      }

#endif // ! WDL_HEAPBUF_INTF_ONLY

#ifndef WDL_HEAPBUF_IMPL_ONLY

  private:
    void *m_buf;
    int m_alloc;
    int m_size;
    int m_granul;

  #if defined(_WIN64) || defined(__LP64__)
    int ___pad; // keep size 8 byte aligned
  #endif

  #ifdef WDL_HEAPBUF_TRACE
    const char *m_tracetype;
  #endif

};

template<class PTRTYPE> class WDL_TypedBuf 
{
  public:
    PTRTYPE *Get() const { return (PTRTYPE *) m_hb.Get(); }
    int GetSize() const { return m_hb.GetSize()/(unsigned int)sizeof(PTRTYPE); }

    PTRTYPE *Resize(int newsize, bool resizedown = true) { return (PTRTYPE *)m_hb.Resize(newsize*sizeof(PTRTYPE),resizedown); }
    PTRTYPE *ResizeOK(int newsize, bool resizedown = true) { return (PTRTYPE *)m_hb.ResizeOK(newsize*sizeof(PTRTYPE), resizedown);  }

    PTRTYPE *GetAligned(int align) const  { return (PTRTYPE *) m_hb.GetAligned(align); }

    PTRTYPE *Add(PTRTYPE val) 
    {
      return Insert(val, GetSize());
    }

    PTRTYPE* Insert(PTRTYPE val, int idx)
    {
      int sz=GetSize();
      if (idx >= 0 && idx <= sz)
      {
        PTRTYPE* p=Resize(sz+1);
        if (p && GetSize() == sz+1)
        {
          memmove(p+idx+1, p+idx, (sz-idx)*(unsigned int)sizeof(PTRTYPE));
          p[idx]=val;
          return p+idx;
        }
      }
      return 0;
    }

    void Delete(int idx)
    {
      PTRTYPE* p=Get();
      int sz=GetSize();
      if (idx >= 0 && idx < sz)
      {
        memmove(p+idx, p+idx+1, (sz-idx-1)*sizeof(PTRTYPE));
        Resize(sz-1,false);
      }
    }

    void SetGranul(int gran) { m_hb.SetGranul(gran); }

    int Find(PTRTYPE val) const
    {
      PTRTYPE* p=Get();
      int i;
      for (i=0; i < GetSize(); ++i) if (p[i] == val) return i;
      return -1;
    }

#ifndef WDL_HEAPBUF_TRACE
    explicit WDL_TypedBuf(int granul=4096) : m_hb(granul) { }
#else
    explicit WDL_TypedBuf(int granul=4096, const char *tracetype="WDL_TypedBuf") : m_hb(granul WDL_HEAPBUF_TRACEPARM(tracetype)) { }
#endif
    ~WDL_TypedBuf()
    {
    }

  private:
    WDL_HeapBuf m_hb;
};

#endif // ! WDL_HEAPBUF_IMPL_ONLY

#endif // _WDL_HEAPBUF_H_
