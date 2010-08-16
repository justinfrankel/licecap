#include "audiobuffercontainer.h"
#include "queue.h"
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

#define BITMASK64(bitIdx) (((WDL_UINT64)1)<<(bitIdx))
 
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

bool ChannelPinMapper::TogglePin(int pinIdx, int chIdx) 
{
  bool on = GetPin(pinIdx, chIdx);
  on = !on;
  SetPin(pinIdx, chIdx, on); 
  return on;
}

bool ChannelPinMapper::GetPin(int pinIdx, int chIdx)
{
  WDL_UINT64 map = *(m_mapping.Get()+pinIdx);
  return !!(map & BITMASK64(chIdx));
}

bool ChannelPinMapper::PinHasMoreMappings(int pinIdx, int chIdx)
{
  WDL_UINT64 map = *(m_mapping.Get()+pinIdx);
  return (chIdx < 64 && map >= BITMASK64(chIdx+1));
}

bool ChannelPinMapper::IsStraightPassthrough()
{
  if (m_nCh != m_nPins) return false;
  WDL_UINT64* pMap = m_mapping.Get();
  int i;
  for (i = 0; i < m_nPins; ++i, ++pMap) {
    if (*pMap != BITMASK64(i)) return false;
  }
  return true;
}

#define PINMAPPER_MAGIC 1000

// return is on the heap
char* ChannelPinMapper::SaveState(int* pLen)
{
  WDL_Queue chunk;
  int magic = PINMAPPER_MAGIC;
  WDL_Queue__AddToLE(&chunk, &magic);
  WDL_Queue__AddToLE(&chunk, &m_nCh);
  WDL_Queue__AddToLE(&chunk, &m_nPins);
  WDL_Queue__AddDataToLE(&chunk, m_mapping.Get(), m_mapping.GetSize()*sizeof(WDL_UINT64), sizeof(WDL_UINT64));
  *pLen = chunk.GetSize();
  char* buf = (char*) malloc(*pLen);
  memcpy(buf, chunk.Get(), *pLen);
  return buf;
}

bool ChannelPinMapper::LoadState(char* buf, int len)
{
  WDL_Queue chunk;
  chunk.Add(buf, len);
  int* pMagic = WDL_Queue__GetTFromLE(&chunk, (int*)0);
  if (!pMagic || *pMagic != PINMAPPER_MAGIC) return false;
  int* pNCh = WDL_Queue__GetTFromLE(&chunk, (int*) 0);
  int* pNPins = WDL_Queue__GetTFromLE(&chunk, (int*) 0);
  if (!pNCh || !pNPins || !(*pNCh) || !(*pNPins)) return false;
  SetNPins(*pNCh);
  SetNChannels(*pNCh);
  int maplen = *pNPins*sizeof(WDL_UINT64);
  if (chunk.Available() < maplen) return false;
  void* pMap = WDL_Queue__GetDataFromLE(&chunk, maplen, sizeof(WDL_UINT64));
  memcpy(m_mapping.Get(), pMap, maplen);
  return true;
}

template <class TDEST, class TSRC> void BufConvertT(TDEST* pDest, TSRC* pSrc, int n, int destStride, int srcStride)
{
  int i;
  for (i = 0; i < n; ++i, pDest += destStride, pSrc += srcStride) {
    *pDest = (TDEST) *pSrc;
  }
}

// static 
void AudioBufferContainer::BufConvert(void* pDest, void* pSrc, int destTypeSize, int srcTypeSize, int n, int destStride, int srcStride)
{
  if (destTypeSize == sizeof(float)) {
    if (srcTypeSize == sizeof(float)) {
      BufConvertT((float*) pDest, (float*) pSrc, n, destStride, srcStride);
    }
    else {
      BufConvertT((float*) pDest, (double*) pSrc, n, destStride, srcStride);
    }
  }
  else {
    if (srcTypeSize == sizeof(float)) {
      BufConvertT((double*) pDest, (float*) pSrc, n, destStride, srcStride);
    }
    else {
      BufConvertT((double*) pDest, (double*) pSrc, n, destStride, srcStride);
    }
  }
}

// m_buf organization is: interleaved32, ch1deleaved32, ch2deleaved32, ..., interleaved64, ch1deleaved64, ch2deleaved64, ...

void AudioBufferContainer::Resize(int nCh, int nFrames)
{
  if (nCh != m_nCh || nFrames != m_nFrames) {
    int len = 2 * nCh * nFrames * (sizeof(float) + sizeof(double));
    m_buf.Resize(len);
    memset(m_buf.Get(), 0, len);
    
    m_backingStorePtrs.Resize(nCh);
    m_chanDescs.Resize(nCh);
    int i;
    for (i = 0; i < nCh; ++i) {
      SetChannelDesc(i, false, sizeof(double), 0);
    }

    m_nCh = nCh;
    m_nFrames = nFrames;
  }
}

void AudioBufferContainer::SetChannelDesc(int chIdx, bool deleaved, int typeSize, void* backingStore)
{
  if (chIdx >= 0 && chIdx < m_nCh) {
    ChannelDesc* pDesc = m_chanDescs.Get() + chIdx;
    pDesc->m_isDeleaved = deleaved;
    pDesc->m_typeSize = typeSize;
    if (!backingStore) backingStore = GetInternalBackingStore(chIdx, deleaved, typeSize);
    *(m_backingStorePtrs.Get() + chIdx) = backingStore;
  }
}

void* AudioBufferContainer::GetInternalBackingStore(int chIdx, bool deleaved, int typeSize)
{
  if (chIdx >= 0 && chIdx < m_nCh) {
    char* pData = (char*) m_buf.Get();
    if (typeSize == sizeof(double)) pData += 2 * m_nCh * m_nFrames * sizeof(float);
    if (deleaved) pData += (m_nCh + chIdx) * m_nFrames * typeSize;
    else pData += chIdx * typeSize;
    return pData;
  }
  return 0;
}

void* AudioBufferContainer::GetValidChannelData(int chIdx, bool* pDeleaved, int* pTypeSize)
{
  if (chIdx >= 0 && chIdx < m_nCh) {
    ChannelDesc* pDesc = m_chanDescs.Get() + chIdx;    
    *pDeleaved = pDesc->m_isDeleaved;
    *pTypeSize = pDesc->m_typeSize;
    return *(m_backingStorePtrs.Get() + chIdx);
  }
  return 0;
}