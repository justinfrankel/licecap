#ifndef _WDL_DELAY_LINE_H_
#define _WDL_DELAY_LINE_H_

#include "circbuf.h"

template<class SampleType> class WDL_DelayLine {
public:
  WDL_DelayLine() : m_nch(1) { }
  ~WDL_DelayLine() { }

  // call before accessing anything.
  // total_size is maximum delay line length required (preserves contents on resize)
  // if valid_size >=0, ensures delay line has exactly valid_size pairs set (must be <= total_size!)
  void set_nch_length(int nch, int total_size, int valid_size=-1)
  {
    if (WDL_NOT_NORMALLY(valid_size > total_size)) valid_size=total_size;

    int avail = get_avail_pairs();
    const int minsz = valid_size >= 0 ? valid_size : total_size;
    if (avail > minsz)
    {
      skip_pairs(avail-minsz);
      avail = minsz;
    }

    if (m_nch != nch && WDL_NORMALLY(nch>0))
    {
      if (nch > m_nch) m_q.SetSizePreserveContents(total_size*nch);

      SampleType work[2048];
      const int chunk = 2048 / wdl_max(nch,m_nch);
      int nleft = avail;
      while (nleft > 0)
      {
        const int a = wdl_min(chunk,nleft);
        nleft-=a;

        m_q.Get(work,a*m_nch);
        VALIDATE_BUFFER(work,a*m_nch);
        reinterleave_buffer(work,m_nch,nch,a);
        m_q.Add(work,a*nch);
      }

      if (nch < m_nch) m_q.SetSizePreserveContents(total_size*nch);

      m_nch=nch;
    }
    else
    {
      const int newsz = total_size*nch, cursz = m_q.GetTotalSize();
      if (newsz > cursz || newsz < cursz/2) m_q.SetSizePreserveContents(newsz);
    }

    if (valid_size > 0)
    {
      const int need_extra = valid_size - get_avail_pairs();
      if (need_extra > 0)
      {
        // insert zero data at read pointer in delay line
        m_q.Skip(-need_extra*nch);
        m_q.WriteAtReadPointer(NULL,need_extra*nch);
      }
    }
  }

  void add_pairs(const SampleType *buf, int pairs) // pushes data off old end of queue
  {
    WDL_ASSERT(pairs>=0);
    if (pairs <= 0) return;

    VALIDATE_BUFFER(buf,pairs*m_nch);

    int add_sz = pairs*m_nch;
    if (add_sz > m_q.NbFree())
    {
      if (add_sz > m_q.GetTotalSize())
      {
        if (buf) buf += add_sz - m_q.GetTotalSize();
        add_sz = m_q.GetTotalSize();
      }
      m_q.Skip(add_sz - m_q.NbFree());
    }

    const int added = m_q.Add(buf,add_sz);
    if (added != add_sz) { WDL_ASSERT(added == add_sz); }
  }

  void unadd_pairs(int pairs)
  {
    WDL_ASSERT(pairs>=0);
    if (pairs>0) m_q.UnAdd(pairs*m_nch);
  }

  int peek_pairs(SampleType *buf, int pairs, int offs=0) // allow to request more than available, returns amt returned
  {
    WDL_ASSERT(offs >= 0);
    const int avail = get_avail_pairs();
    const int rdsize = wdl_min(avail - offs, pairs);
    if (rdsize <= 0) return 0;

    int ret = m_q.Peek(buf,offs*m_nch,rdsize*m_nch);
    WDL_ASSERT(ret == rdsize*m_nch);
    VALIDATE_BUFFER(buf,rdsize);
    return ret/m_nch;
  }

  void get_pairs(SampleType *buf, int pairs) // asserts if pairs > available
  {
    int amt;
    WDL_ASSERT(pairs <= get_avail_pairs());
    if (buf)
    {
      amt = peek_pairs(buf,pairs,0);
      WDL_ASSERT(amt == pairs);
      VALIDATE_BUFFER(buf,amt);
    }
    else
    {
      amt = pairs;
    }
    skip_pairs(amt);
  }

  void skip_pairs(int samt) { if (samt > 0) m_q.Skip(samt*m_nch); }
  int get_avail_pairs() const { return m_q.NbInBuf()/m_nch; }

  void free_memory() { m_q.SetSize(0); } // frees memory
  void clear() { m_q.Reset(); } // keeps max buffer size intact but clears contents

  static void reinterleave_buffer(SampleType *rdptr, int in_nch, int out_nch, int len)
  {
    if (len < 1 || !rdptr) return;

    int x=len-1;
    SampleType *wrptr = rdptr;
    if (out_nch < in_nch)
    {
      const int sz1=out_nch*sizeof(SampleType);
      while (x--)
      {
        rdptr += in_nch;
        wrptr += out_nch;
        memmove(wrptr,rdptr,sz1);
      }
    }
    else if (out_nch > in_nch)
    {
      const int sz1=in_nch*sizeof(SampleType);
      const int sz2=(out_nch-in_nch)*sizeof(SampleType);
      rdptr += in_nch*x;
      wrptr += out_nch*x;
      while(x--)
      {
        memmove(wrptr,rdptr,sz1);
        memset(wrptr+in_nch,0,sz2);

        rdptr-=in_nch;
        wrptr-=out_nch;
      }
      memset(wrptr+in_nch,0,sz2); // last iteration doesnt need memcpy (but does need clear)
    }
  }

private:
  WDL_TypedCircBuf<SampleType> m_q;
  int m_nch;

  static void VALIDATE_BUFFER(const SampleType *buf,  int cnt)
  {
#ifdef _DEBUG
    if (buf) for (int x = 0; x < cnt; x ++)
    {
      double v =  buf[x];
      WDL_ASSERT(v >= -40000.0 && v < 40000.0);
    }
#endif
  }
};

#endif
