#include "audiobuffercontainer.h"
#include "queue.h"
#include <assert.h>

void ChannelPinMapper::SetNPins(int nPins)
{
  if (nPins<0) nPins=0;
  else if (nPins>CHANNELPINMAPPER_MAXPINS) nPins=CHANNELPINMAPPER_MAXPINS;
  int i;
  for (i = m_nPins; i < nPins; ++i) {
    ClearPin(i);
    if (i < m_nCh) {
      SetPin(i, i, true);
    }
  }
  m_nPins = nPins;
}

void ChannelPinMapper::SetNChannels(int nCh, bool auto_passthru)
{
  if (auto_passthru) for (int i = m_nCh; i < nCh && i < m_nPins; ++i) {
    SetPin(i, i, true);
  }
  m_nCh = nCh;
}

void ChannelPinMapper::Init(const WDL_UINT64* pMapping, int nPins)
{
  if (nPins<0) nPins=0;
  else if (nPins>CHANNELPINMAPPER_MAXPINS) nPins=CHANNELPINMAPPER_MAXPINS;
  memcpy(m_mapping, pMapping, nPins*sizeof(WDL_UINT64));
  memset(m_mapping+nPins, 0, (CHANNELPINMAPPER_MAXPINS-nPins)*sizeof(WDL_UINT64));
  m_nPins = m_nCh = nPins;
}

#define BITMASK64(bitIdx) (((WDL_UINT64)1)<<(bitIdx))
 
void ChannelPinMapper::ClearPin(int pinIdx)
{
  if (pinIdx >=0 && pinIdx < CHANNELPINMAPPER_MAXPINS) m_mapping[pinIdx] = 0;
}

void ChannelPinMapper::SetPin(int pinIdx, int chIdx, bool on)
{
  if (pinIdx >=0 && pinIdx < CHANNELPINMAPPER_MAXPINS)
  {
    if (on) 
    {
      m_mapping[pinIdx] |= BITMASK64(chIdx);
    }
    else 
    {
      m_mapping[pinIdx] &= ~BITMASK64(chIdx);
    }
  }
}

bool ChannelPinMapper::TogglePin(int pinIdx, int chIdx) 
{
  bool on = GetPin(pinIdx, chIdx);
  on = !on;
  SetPin(pinIdx, chIdx, on); 
  return on;
}

bool ChannelPinMapper::GetPin(int pinIdx, int chIdx) const
{
  if (pinIdx >= 0 && pinIdx < CHANNELPINMAPPER_MAXPINS)
  {
    WDL_UINT64 map = m_mapping[pinIdx];
    return !!(map & BITMASK64(chIdx));
  }
  return false;
}


bool ChannelPinMapper::IsStraightPassthrough() const
{
  if (m_nCh != m_nPins) return false;
  const WDL_UINT64* pMap = m_mapping;
  int i;
  for (i = 0; i < m_nPins; ++i, ++pMap) {
    if (*pMap != BITMASK64(i)) return false;
  }
  return true;
}

#define PINMAPPER_MAGIC 1000

const char *ChannelPinMapper::SaveStateNew(int* pLen)
{
  m_cfgret.Clear();
  int magic = PINMAPPER_MAGIC;
  WDL_Queue__AddToLE(&m_cfgret, &magic);
  WDL_Queue__AddToLE(&m_cfgret, &m_nCh);
  WDL_Queue__AddToLE(&m_cfgret, &m_nPins);
  WDL_Queue__AddDataToLE(&m_cfgret, m_mapping, m_nPins*sizeof(WDL_UINT64), sizeof(WDL_UINT64));
  *pLen = m_cfgret.GetSize();
  return (const char*)m_cfgret.Get();
}

bool ChannelPinMapper::LoadState(const char* buf, int len)
{
  WDL_Queue chunk;
  chunk.Add(buf, len);
  int* pMagic = WDL_Queue__GetTFromLE(&chunk, (int*)0);
  if (!pMagic || *pMagic != PINMAPPER_MAGIC) return false;
  int* pNCh = WDL_Queue__GetTFromLE(&chunk, (int*) 0);
  int* pNPins = WDL_Queue__GetTFromLE(&chunk, (int*) 0);
  if (!pNCh || !pNPins) return false;
  SetNPins(*pNPins);
  SetNChannels(*pNCh);
  int maplen = *pNPins*sizeof(WDL_UINT64);
  if (chunk.Available() < maplen) return false;
  void* pMap = WDL_Queue__GetDataFromLE(&chunk, maplen, sizeof(WDL_UINT64));
  
  int sz= m_nPins*sizeof(WDL_UINT64);
  if (sz>maplen) sz=maplen;
  memcpy(m_mapping, pMap, sz);

  return true;
}


AudioBufferContainer::AudioBufferContainer()
{
  m_nCh = 0;
  m_nFrames = 0;
  m_fmt = FMT_32FP;
  m_interleaved = true;
  m_hasData = false;
}


// converts interleaved buffer to interleaved buffer, using min(len_in,len_out) and zeroing any extra samples
// isInput means it reads from track channels and writes to plugin pins
// wantZeroExcessOutput=false means that untouched channels will be preserved in buf_out
void PinMapperConvertBuffers(const double *buf, int len_in, int nch_in, 
                             double *buf_out, int len_out, int nch_out,
                             const ChannelPinMapper *pinmap, bool isInput, bool wantZeroExcessOutput) 
{

  if (pinmap->IsStraightPassthrough() || !pinmap->GetNPins())
  {
    int x;
    char *op = (char *)buf_out;
    const char *ip = (const char *)buf;

    const int ip_adv = nch_in * sizeof(double);

    const int clen = wdl_min(nch_in, nch_out) * sizeof(double);
    const int zlen = nch_out > nch_in ? (nch_out - nch_in) * sizeof(double) : 0;

    const int cplen = wdl_min(len_in,len_out);

    for (x=0;x<cplen;x++)
    {
      memcpy(op,ip,clen);
      op += clen;
      if (zlen) 
      {
        if (wantZeroExcessOutput) memset(op,0,zlen);
        op += zlen;
      }
      ip += ip_adv;
    }
    if (x < len_out && wantZeroExcessOutput) memset(op, 0, (len_out-x)*sizeof(double)*nch_out);
  }
  else
  {
    if (wantZeroExcessOutput) memset(buf_out,0,len_out*nch_out*sizeof(double));

    const int npins = wdl_min(pinmap->GetNPins(),isInput ? nch_out : nch_in);
    const int nchan = isInput ? nch_in : nch_out;

    int p;
    WDL_UINT64 clearmask=0;
    for (p = 0; p < npins; p ++)
    {
      WDL_UINT64 map = pinmap->m_mapping[p];
      int x;
      for (x = 0; x < nchan && map; x ++)
      {
        if (map & 1)
        {
          int i=len_in;
          const double *ip = buf + (isInput ? x : p);
          const int out_idx = (isInput ? p : x);

          bool want_zero=false;
          if (!wantZeroExcessOutput)
          {
            WDL_UINT64 m = ((WDL_UINT64)1)<<out_idx;
            if (!(clearmask & m))
            {
              clearmask|=m;
              want_zero=true;
            }
          }

          double *op = buf_out + out_idx;

          if (want_zero)
          {
            while (i-- > 0) 
            {
              *op = *ip;
              op += nch_out;
              ip += nch_in;
            }
          }
          else
          {
            while (i-- > 0) 
            {
              *op += *ip;
              op += nch_out;
              ip += nch_in;
            }
          }
        }
        map >>= 1;
      }
    }
  }
}
