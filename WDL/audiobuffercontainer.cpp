#include "audiobuffercontainer.h"
#include <assert.h>

void ChannelPinMapper::SetNPins(int nPins)
{
  m_mapping.Resize(nPins);
  int i;
  for (i = m_nPins; i < nPins; ++i) {
    ClearPin(i);
    if (i < m_nCh) {
      SetPin(i, i, true);
    }
  }
  m_nPins = nPins;
}

void ChannelPinMapper::SetNChannels(int nCh)
{
  int i;
  for (i = m_nCh; i < nCh && i < m_nPins; ++i) {
    SetPin(i, i, true);
  }
  m_nCh = nCh;
}

void ChannelPinMapper::Init(WDL_UINT64* pMapping, int nPins)
{
  m_mapping.Resize(nPins);
  memcpy(m_mapping.Get(), pMapping, nPins*sizeof(WDL_UINT64));
  m_nPins = m_nCh = nPins;
}

#define BITMASK64(bitIdx) (((WDL_UINT64)1)<<bitIdx)
 
void ChannelPinMapper::ClearPin(int pinIdx)
{
  *(m_mapping.Get()+pinIdx) = 0;
}

void ChannelPinMapper::SetPin(int pinIdx, int chIdx, bool on)
{
  if (on) {
    *(m_mapping.Get()+pinIdx) |= BITMASK64(chIdx);
  }
  else {
   *(m_mapping.Get()+pinIdx) &= ~BITMASK64(chIdx);
  }
}

bool ChannelPinMapper::GetPin(int pinIdx, int chIdx)
{
  WDL_UINT64 map = *(m_mapping.Get()+pinIdx);
  return (map & BITMASK64(chIdx));
}

bool ChannelPinMapper::TogglePin(int pinIdx, int chIdx) 
{
  bool on = GetPin(pinIdx, chIdx);
  on = !on;
  SetPin(pinIdx, chIdx, on); 
  return on;
}

bool ChannelPinMapper::PinIsUsed(int pinIdx)
{
  return *(m_mapping.Get()+pinIdx);
}

// true if at most one channel is mapped to this pin.  
bool ChannelPinMapper::PinIsPassthrough(int pinIdx)
{
  WDL_UINT64 map = *(m_mapping.Get()+pinIdx);
  return !(map&(map-1));
}

// ----------------------------------------

void AudioBufferContainer::Resize(int nCh, int nFrames)
{
  if (nCh != m_nCh || nFrames != m_nFrames) {
    m_interleaved.Resize(nCh*nFrames);
    m_chans.Empty(true);
    int i;
    for (i = 0; i < nCh; ++i) {
      ChannelBuf* pChan = new ChannelBuf;
      pChan->m_valid = eInterleaved64;
      pChan->m_buf32.Resize(nFrames);
      pChan->m_buf64.Resize(nFrames);
      m_chans.Add(pChan);
    }
    m_nCh = nCh;
    m_nFrames = nFrames;
  }
}

void AudioBufferContainer::SetAllChannels(double* pSrc, int nCh, int nFrames)
{
  Resize(nCh, nFrames);
  if (!pSrc) {
    memset(m_interleaved.Get(), 0, nCh*nFrames*sizeof(double));
  }
  else {
    memcpy(m_interleaved.Get(), pSrc, nCh*nFrames*sizeof(double));
  }
  int i;
  for (i = 0; i < m_nCh; ++i) {
    m_chans.Get(i)->m_valid = eInterleaved64;
  }
}

double* AudioBufferContainer::GetAllChannels()
{
  double* pDest = m_interleaved.Get();
  int i;
  for (i = 0; i < m_nCh; ++i) {
    ChannelBuf* pChan = m_chans.Get(i);
    if (pChan->m_valid == eDeleaved32) {
      Interleave(pDest+i, pChan->m_buf32.Get(), m_nCh, m_nFrames);
    }
    else 
    if (pChan->m_valid == eDeleaved64) {
     Interleave(pDest+i, pChan->m_buf64.Get(), m_nCh, m_nFrames);
    }
    pChan->m_valid = eInterleaved64;
  }
  return pDest;
}

void* AudioBufferContainer::GetChanBufPtr(int chIdx, int destTypeSize, bool convert) 
{
  if (chIdx >= 0 && chIdx < m_nCh) {
    ChannelBuf* pChan = m_chans.Get(chIdx);
    if (destTypeSize == sizeof(float)) {
     float* pDest = pChan->m_buf32.Get();
     if (convert) {
       if (pChan->m_valid == eInterleaved64) {
         Deleave(pDest, m_interleaved.Get()+chIdx, m_nCh, m_nFrames);
       }
       else
       if (pChan->m_valid == eDeleaved64) {
         TypeConvert(pDest, pChan->m_buf64.Get(), m_nFrames);
       }
     }
     pChan->m_valid = eDeleaved32;     
     return pDest;
    }
    if (destTypeSize == sizeof(double)) {
     double* pDest = pChan->m_buf64.Get();
     if (convert) {
       if (pChan->m_valid == eInterleaved64) {
         Deleave(pDest, m_interleaved.Get()+chIdx, m_nCh, m_nFrames);
       }
       else
       if (pChan->m_valid == eDeleaved32) {
         TypeConvert(pDest, pChan->m_buf32.Get(), m_nFrames);
       }
     }
     pChan->m_valid = eDeleaved64;
     return pDest;
    }
  }
  return 0;
}
