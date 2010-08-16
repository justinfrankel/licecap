#ifndef _AUDIOBUFFERCONTAINER_
#define _AUDIOBUFFERCONTAINER_

#include "wdltypes.h"
#include <string.h>
#include <stdlib.h>
#include "ptrlist.h"

class ChannelPinMapper
{
public: 

  ChannelPinMapper() : m_nCh(0), m_nPins(0) {}
  ~ChannelPinMapper() {}

  void SetNPins(int nPins);
  void SetNChannels(int nCh);
  // or ...
  void Init(WDL_UINT64* pMapping, int nPins);

  int GetNPins() { return m_nPins; }
  int GetNChannels() { return m_nCh; }

  void ClearPin(int pinIdx);
  void SetPin(int pinIdx, int chIdx, bool on);
  bool TogglePin(int pinIdx, int chIdx);

  // true if this pin is mapped to this channel
  bool GetPin(int pinIdx, int chIdx);
  // true if this pin is to any higher channel
  bool PinHasMoreMappings(int pinIdx, int chIdx);
  // true if this mapper is a straight 1:1 passthrough
  bool IsStraightPassthrough();

  // return is on the heap
  char* SaveState(int* pLen);
  bool LoadState(char* buf, int len);

  WDL_TypedBuf<WDL_UINT64> m_mapping;

private:

  int m_nCh, m_nPins;
};

// stupid ... fucking ... VS6 ... templates

// use for float and double only ... ints will break it
class AudioBufferContainer
{
public:

  static void BufConvert(void* pDest, void* pSrc, int destTypeSize, int srcTypeSize, int n, int destStride, int srcStride);

  AudioBufferContainer() : m_nCh(0), m_nFrames(0) {}
  ~AudioBufferContainer() {}

  void Resize(int nCh, int nFrames);
  int GetNChannels() { return m_nCh; }
  int GetNFrames() { return m_nFrames; }

  // Set and get pointers to interleaved data. pSrc=NULL to clear all channels.
  template <class T> T* SetAllChannels(T* pSrc, int nCh, int nFrames)
  {
    Resize(nCh, nFrames);
    T* pDest = (T*) GetInternalBackingStore(0, false, sizeof(T));
    if (pSrc) memcpy(pDest, pSrc, nCh*nFrames*sizeof(T));
    else memset(pDest, 0, nCh*nFrames*sizeof(T));
    int i;
    for (i = 0; i < m_nCh; ++i) {
      SetChannelDesc(i, false, sizeof(T));
    }
    return pDest;
  }

  // forWriteOnly=true saves the work of converting the existing data, if it's in another format.
  template <class T> T* GetAllChannels(bool forWriteOnly, T* dummy = 0)
  {
    T* pDest = (T*) GetInternalBackingStore(0, false, sizeof(T));
    int i;
    for (i = 0; i < m_nCh; ++i) {
      bool srcDeleaved;
      int srcTypeSize;
      void* pSrc = GetValidChannelData(i, &srcDeleaved, &srcTypeSize);
      if (srcDeleaved || srcTypeSize != sizeof(T)) {
        if (!forWriteOnly) BufConvert(pDest+i, pSrc, sizeof(T), srcTypeSize,  m_nFrames, m_nCh, (srcDeleaved ? 1 : m_nCh));
        SetChannelDesc(i, false, sizeof(T));        
      }
    }
    return pDest;
  }

  // Copy a channel from an external buffer.
  template <class T> T* SetChannel(T* pSrc, int chIdx)
  {
    T* pDest = (T*) GetInternalBackingStore(chIdx, true, sizeof(T));
    if (pDest) {
      if (pSrc) memcpy(pDest, pSrc, m_nFrames*sizeof(T));
      else memset(pDest, 0, m_nFrames*sizeof(T));
      SetChannelDesc(chIdx, true, sizeof(T));
    }
    return pDest;
  }

  template <class T> T* AccumulateChannel(T* pSrc, int chIdx)
  {
    T* pDest = GetChannel(chIdx, false, pSrc);
    if (pDest) {
      T* ptDest = pDest;
      int i;
      for (i = 0; i < m_nFrames; ++i, ++ptDest, ++pSrc) {
        *ptDest += *pSrc;
      }
    }
    return pDest;
  }

  // Attach an external buffer.
  template <class T> T* AttachChannel(T* pSrc, int chIdx)
  {
    SetChannelDesc(chIdx, true, sizeof(T), pSrc);
    return pSrc;
  }

  // forWriteOnly=true saves the work of converting the existing data, if it's in another format.
  template <class T> T* GetChannel(int chIdx, bool forWriteOnly, T* dummy = 0)
  {
    bool srcDeleaved;
    int srcTypeSize;
    void* pSrc = GetValidChannelData(chIdx, &srcDeleaved, &srcTypeSize);
    if (srcDeleaved && srcTypeSize == sizeof(T)) return (T*) pSrc;
    void* pDest = GetInternalBackingStore(chIdx, true, sizeof(T));
    if (!forWriteOnly && pDest) 
    {
      if (pSrc)
        BufConvert(pDest, pSrc, sizeof(T), srcTypeSize,  m_nFrames, 1, (srcDeleaved ? 1 : m_nCh));
      else memset(pDest,0,sizeof(T)*m_nFrames);
    }
    SetChannelDesc(chIdx, true, sizeof(T), pDest);
    return (T*) pDest;
  }

  // forWriteOnly=true saves the work of converting the existing data, if it's in another format.
  template <class T> T** GetAllChannelPtrs(bool forWriteOnly = false, T* dummy = 0)
  {
    int i;
    for (i = 0; i < m_nCh; ++i) {
      GetChannel(i, forWriteOnly); // converts if necessary
    }
    return (T**) m_backingStorePtrs.Get();
  }

private:

  // backingStore=0 means use internal backing store
  void SetChannelDesc(int chIdx, bool deleaved, int typeSize, void* backingStore = 0);  
  void* GetInternalBackingStore(int chIdx, bool deleaved, int typeSize);
  void* GetValidChannelData(int chIdx, bool* pDeleaved, int* pTypeSize);

  struct ChannelDesc {
    bool m_isDeleaved;
    int m_typeSize;
  };
  WDL_TypedBuf<ChannelDesc> m_chanDescs;
  WDL_HeapBuf m_buf;  // backing store for everything.

  // Backing store for deleaved buffers can be internal or external.
  WDL_TypedBuf<void*> m_backingStorePtrs;

  int m_nCh, m_nFrames;
};

template <class T> void SetPinsFromChannels(AudioBufferContainer* pDest, AudioBufferContainer* pSrc, ChannelPinMapper* pMapper, T* dummy = 0)
{
  int c, p, nCh = pMapper->GetNChannels(), nPins = pMapper->GetNPins(), nFrames = pSrc->GetNFrames();
  pDest->Resize(nPins, nFrames);
  for (p = 0; p < nPins; ++p) {
    bool pinUsed = false, more = true;
    for (c = 0; c < nCh && more; ++c) {
      more = pMapper->PinHasMoreMappings(p, c);
      if (pMapper->GetPin(p, c)) {
        T* pSrcBuf = pSrc->GetChannel(c, false, dummy);
        if (!pinUsed) {
          if (!more) pDest->AttachChannel(pSrcBuf, p);
          else pDest->SetChannel(pSrcBuf, p);
          pinUsed = true;
        }
        else {
          pDest->AccumulateChannel(pSrcBuf, p);
        }
      }
    }
    if (!pinUsed) pDest->SetChannel((T*) 0, p);
  }
}

template <class T> void SetChannelsFromPins(AudioBufferContainer* pDest, AudioBufferContainer* pSrc, ChannelPinMapper* pMapper, T* dummy = 0)
{
  int c, p, nCh = pMapper->GetNChannels(), nPins = pMapper->GetNPins(), nFrames = pSrc->GetNFrames();
  pDest->Resize(nCh, nFrames);
  for (c = 0; c < nCh; ++c) {
    bool chanUsed = false;
    for (p = 0; p < nPins; ++p) {
      if (pMapper->GetPin(p, c)) {
        T* pSrcBuf = pSrc->GetChannel(p, false, dummy);
        if (!chanUsed) {
          pDest->SetChannel(pSrcBuf, c);
          chanUsed = true;
        }
        else {
          pDest->AccumulateChannel(pSrcBuf, c);
        }
      }
    }
    // don't clear unused channels
  }
}

#endif
