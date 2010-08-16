#ifndef _BUFCONTAINER_
#define _BUFCONTAINER_

#include "heapbuf.h"


// Simple class to manage pointers to buffers 
// that may be persistent and external, or stored internally.

template <class T> class WDL_BufContainer
{
public:

  WDL_BufContainer() : m_nBufs(0), m_bufSize(0) {}
  ~WDL_BufContainer() {}

  void Resize(int nBufs, int bufSize)
  {
    if (nBufs != m_nBufs || bufSize != m_bufSize) {
      T** pp = m_bufPtrs.Resize(nBufs);
      memset(pp, 0, nBufs*sizeof(T*));
      T* pData = m_data.Resize(nBufs*bufSize);
      int i;
      for (i = 0; i < nBufs; ++i, ++pp, pData += bufSize) {
        *pp = pData;
      }
      m_nBufs = nBufs;
      m_bufSize = bufSize;      
    }
  }

  int GetNBufs() { return m_nBufs; }
  int GetBufSize() { return m_bufSize; }

  // pSrc points to a persistent external buffer.
  void AssignBuf(T* pSrc, int bufIdx, int bufSize)
  {
    if (bufIdx >= 0 && bufIdx < m_nBufs && bufSize == m_bufSize) {
      *(m_bufPtrs.Get()+bufIdx) = pSrc;
    }
  }

  // pSrc = NULL clears the buffer.
  T* CopyBuf(T* pSrc, int bufIdx, int bufSize) 
  {
    if (bufIdx >= 0 && bufIdx < m_nBufs && bufSize == m_bufSize) {
      T* pDest = m_data.Get()+bufIdx*bufSize;
      if (pSrc) {
        memcpy(pDest, pSrc, bufSize*sizeof(T));
      }
      else {
        memset(pDest, 0, bufSize*sizeof(T));
      }
      *(m_bufPtrs.Get()+bufIdx) = pDest;
      return pDest;
    }
    return 0;
  }

  T* GetBuf(int bufIdx) 
  {
    if (bufIdx >= 0 && bufIdx < m_nBufs) {
      return *(m_bufPtrs.Get()+bufIdx);
    }
    return 0;
  }
  
  T** GetBufPtrs()
  {
    return m_bufPtrs.Get();
  }

private:

  int m_nBufs, m_bufSize;
  WDL_TypedBuf<T*> m_bufPtrs;
  WDL_TypedBuf<T> m_data;
};

// ----------------------------------------

// And some basic utilities for manipulating buffers.

template <class TDEST, class TSRC> void TypeConvert(TDEST* pDest, TSRC* pSrc, int n)
{
  int i;
  for (i = 0; i < n; ++i, ++pDest, ++pSrc) {
    *pDest = (TDEST) *pSrc;
  }
}

template <class T> void Accumulate(T* pDest, T* pSrc, int n)
{
  int i;
  for (i = 0; i < n; ++i, ++pDest, ++pSrc) {
    *pDest += *pSrc;
  }
}

// deleave one channel.
template <class TDEST, class TSRC> void Deleave(TDEST* pDest, TSRC* pSrc, int nCh, int nFrames)
{
  int i;
  for (i = 0; i < nFrames; ++i, ++pDest, pSrc += nCh) {
    *pDest = (TDEST) *pSrc;
  }
}  

// interleave one channel.
template <class TDEST, class TSRC> void Interleave(TDEST* pDest, TSRC* pSrc, int nCh, int nFrames)
{
  int i;
  for (i = 0; i < nFrames; ++i, ++pSrc, pDest += nCh) {
    *pDest = (TDEST) *pSrc;
  }
}

#endif
