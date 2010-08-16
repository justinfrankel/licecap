#ifndef _AUDIOBUFFERCONTAINER_
#define _AUDIOBUFFERCONTAINER_

#include "wdltypes.h"
#include <string.h>
#include <stdlib.h>
#include "ptrlist.h"
#include "bufcontainer.h"

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
  bool GetPin(int pinIdx, int chIdx);
  bool TogglePin(int pinIdx, int chIdx);

  bool PinIsUsed(int pinIdx);
  // true if at most one channel is mapped to this pin.  
  bool PinIsPassthrough(int pinIdx);
  
  WDL_TypedBuf<WDL_UINT64> m_mapping;

private:

  int m_nCh, m_nPins;
};

// ----------------------------------------

// use for float and double only ... ints will break it
class AudioBufferContainer
{
public:

  AudioBufferContainer() : m_nCh(0), m_nFrames(0) {}
  ~AudioBufferContainer() { m_chans.Empty(true); }

  void Resize(int nCh, int nFrames);
  int GetNChannels() { return m_nCh; }
  int GetNFrames() { return m_nFrames; }

  // pSrc=NULL to clear all channels.
  void AudioBufferContainer::SetAllChannels(double* pSrc, int nCh, int nFrames);
  double* AudioBufferContainer::GetAllChannels();

  template <class T> T* SetChannel(T* pSrc, int chIdx, int nFrames)
  {
    if (chIdx >= 0 && chIdx < m_nCh && nFrames == m_nFrames) {
      T* pDest = (T*) GetChanBufPtr(chIdx, sizeof(T), false);
      memcpy(pDest, pSrc, nFrames*sizeof(T));
      return pDest;
    }
    return 0;
  }

  template <class T> T* GetChannel(int chIdx)
  {
    if (chIdx >= 0 && chIdx < m_nCh) {
      return (T*) GetChanBufPtr(chIdx, sizeof(T), true);
    }
    return 0;
  }

private:

  enum EChannelValid {
    eInterleaved64 = 0,
    eDeleaved32 = sizeof(float),
    eDeleaved64 = sizeof(double)
  };

  struct ChannelBuf {
    EChannelValid m_valid;
    WDL_TypedBuf<float> m_buf32;
    WDL_TypedBuf<double> m_buf64;
  };

  void* GetChanBufPtr(int chIdx, int destTypeSize, bool convert);

  int m_nCh, m_nFrames;
  WDL_TypedBuf<double> m_interleaved;
  WDL_PtrList<ChannelBuf> m_chans;
};

// ----------------------------------------

template <class T> void SetChannelsFromPins(AudioBufferContainer* pDest, WDL_BufContainer<T>* pSrc, ChannelPinMapper* pMapper)
{
  int c, p, nCh = pMapper->GetNChannels(), nPins = pMapper->GetNPins(), nFrames = pSrc->GetBufSize();
  pDest->Resize(nCh, nFrames);
  for (c = 0; c < nCh; ++c) {
    T* pChanDest = 0;
    for (p = 0; p < nPins; ++p) {
      if (pMapper->GetPin(p, c)) {
        if (!pChanDest) {
          pChanDest = pDest->SetChannel(pSrc->GetBuf(p), c, nFrames);
        }
        else {
          Accumulate(pChanDest, pSrc->GetBuf(p), nFrames);
        }
      }
    }
    // Don't clear unused channels.
  }
}

template <class T> void SetPinsFromChannels(WDL_BufContainer<T>* pDest, AudioBufferContainer* pSrc, ChannelPinMapper* pMapper)
{
  int c, p, nCh = pMapper->GetNChannels(), nPins = pMapper->GetNPins(), nFrames = pSrc->GetNFrames();
  pDest->Resize(nPins, nFrames);
  for (p = 0; p < nPins; ++p) {
    if (pMapper->PinIsUsed(p)) {
      if (pMapper->PinIsPassthrough(p)) {
        for (c = 0; c < nCh; ++c) {
          if (pMapper->GetPin(p, c)) {
            T* pChanSrc = pSrc->GetChannel<T>(c);
            pDest->AssignBuf(pChanSrc, p, nFrames);
            break;
          }
        }
      }
      else {
        T* pPinDest = 0;
        for (c = 0; c < nCh; ++c) {
          if (pMapper->GetPin(p, c)) { 
            T* pChanSrc = pSrc->GetChannel<T>(c);
            if (!pPinDest) {
              pPinDest = pDest->CopyBuf(pChanSrc, p, nFrames);
            }
            else {
              Accumulate(pPinDest, pChanSrc, nFrames);
            }
          }
        }
      }
    }
    else {
      pDest->CopyBuf(0, p, nFrames);  // Clear unused pins. 
    }
  }
}

#endif
