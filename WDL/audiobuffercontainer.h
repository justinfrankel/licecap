#ifndef _AUDIOBUFFERCONTAINER_
#define _AUDIOBUFFERCONTAINER_

#include "wdltypes.h"
#include <string.h>
#include <stdlib.h>
#include "ptrlist.h"
#include "queue.h"


#define CHANNELPINMAPPER_MAXPINS 64


// eventually ChannelPinMapper etc should use this instead of WDL_UINT64
struct PinMapPin
{
  enum { PINMAP_PIN_MAX_CHANNELS = CHANNELPINMAPPER_MAXPINS };
  // currently must be WDL_UINT64 since we cast to WDL_UINT64 for EffectPinConnectDialog
  // probably should change to unsigned int when we migrate to higher channel counts
  // (more efficient for 32-bit platforms)
  enum { STATE_ENT_BITS=64 };
  WDL_UINT64 state[(PINMAP_PIN_MAX_CHANNELS + STATE_ENT_BITS - 1) / STATE_ENT_BITS];
  static WDL_UINT64 make_mask(unsigned int idx) { return WDL_UINT64_CONST(1) << (idx & (STATE_ENT_BITS-1)); }
  static WDL_UINT64 full_mask() { return ~WDL_UINT64_CONST(0); }


  void clear() { memset(state,0,sizeof(state)); }
  void clear_chan(unsigned int ch) { if (WDL_NORMALLY(ch < PINMAP_PIN_MAX_CHANNELS)) state[ch/STATE_ENT_BITS] &= ~make_mask(ch); }
  void set_chan(unsigned int ch) { if (WDL_NORMALLY(ch < PINMAP_PIN_MAX_CHANNELS)) state[ch/STATE_ENT_BITS] |= make_mask(ch); }
  void tog_chan(unsigned int ch) { if (WDL_NORMALLY(ch < PINMAP_PIN_MAX_CHANNELS)) state[ch/STATE_ENT_BITS] ^= make_mask(ch); }
  void set_chan_lt(unsigned int cnt)
  {
    if (WDL_NOT_NORMALLY(cnt > PINMAP_PIN_MAX_CHANNELS)) cnt = PINMAP_PIN_MAX_CHANNELS;
    for (int x = 0; cnt && x < (int) (sizeof(state)/sizeof(state[0])); x ++)
    {
      if (cnt < STATE_ENT_BITS) { state[x] |= make_mask(cnt)-1; cnt=0; }
      else { state[x] = full_mask(); cnt -= STATE_ENT_BITS; }
    }
  }
  void set_excl(unsigned int ch) { clear(); set_chan(ch); }

  bool has_chan(unsigned int ch) const { return WDL_NORMALLY(ch < PINMAP_PIN_MAX_CHANNELS) && (state[ch/STATE_ENT_BITS] & make_mask(ch)); }
  bool has_chan_lt(unsigned int cnt) const
  {
    if (WDL_NOT_NORMALLY(cnt > PINMAP_PIN_MAX_CHANNELS)) cnt = PINMAP_PIN_MAX_CHANNELS;
    for (int x = 0; cnt && x < (int) (sizeof(state)/sizeof(state[0])); x ++)
    {
      if (cnt < STATE_ENT_BITS) return (state[x] & (make_mask(cnt)-1));
      if (state[x]) return true;
      cnt -= STATE_ENT_BITS;
    }
    return false;
  }

  // call with 0, then increment after each call (returns false when done)
  bool enum_chans(unsigned int *ch, unsigned int maxch=PINMAP_PIN_MAX_CHANNELS) const
  {
    if (WDL_NOT_NORMALLY(maxch > PINMAP_PIN_MAX_CHANNELS))
      maxch = PINMAP_PIN_MAX_CHANNELS;

    unsigned int x = *ch;
    if (x >= maxch) return false;

    WDL_UINT64 s = state[x / STATE_ENT_BITS] >> (x & (STATE_ENT_BITS-1));
    do
    {
      if (s)
      {
        do
        {
          if (s&1) { *ch = x; return true; }
          s>>=1;
          x++;
          WDL_ASSERT(x & (STATE_ENT_BITS-1)); // we should never run out of bits!
        }
        while (x < maxch);
        break;
      }
      x = (x & ~(STATE_ENT_BITS-1)) + STATE_ENT_BITS;
      s = state[x / STATE_ENT_BITS];
    }
    while (x < maxch);

    *ch = x;
    return false;
  }

  PinMapPin & operator |= (const PinMapPin &v)
  {
    for (int x = 0; x < (int) (sizeof(state)/sizeof(state[0])); x ++) state[x]|=v.state[x];
    return *this;
  }

  bool equal_to(const PinMapPin &v, unsigned int nch_top = PINMAP_PIN_MAX_CHANNELS) const
  {
    if (WDL_NOT_NORMALLY(nch_top > PINMAP_PIN_MAX_CHANNELS)) nch_top = PINMAP_PIN_MAX_CHANNELS;
    for (unsigned int x = 0; x < nch_top; x += STATE_ENT_BITS)
    {
      if ((v.state[x/STATE_ENT_BITS]^state[x/STATE_ENT_BITS]) &
          (((nch_top-x) < STATE_ENT_BITS) ? (make_mask(nch_top-x)-1) : full_mask()))
        return false;
    }
    return true;
  }
};
typedef char assert_pinmappin_is_sizeofuint64[sizeof(PinMapPin) == sizeof(WDL_UINT64) ? 1 : -1];


class ChannelPinMapper
{
public: 

  ChannelPinMapper() : m_nCh(0), m_nPins(0) {}
  ~ChannelPinMapper() {}

  void SetNPins(int nPins);
  void SetNChannels(int nCh, bool auto_passthru=true);
  // or ...
  void Init(const WDL_UINT64* pMapping, int nPins);

  int GetNPins() const { return m_nPins; }
  int GetNChannels() const { return m_nCh; }

  void ClearPin(int pinIdx);
  void SetPin(int pinIdx, int chIdx, bool on);
  bool TogglePin(int pinIdx, int chIdx);

  // true if this pin is mapped to this channel
  bool GetPin(int pinIdx, int chIdx) const;
  // true if this pin is to any higher channel
  bool PinHasMoreMappings(int pinIdx, int chIdx) const;
  // true if this mapper is a straight 1:1 passthrough
  bool IsStraightPassthrough() const;

  const char *SaveStateNew(int* pLen); // owned
  bool LoadState(const char* buf, int len);

  WDL_UINT64 m_mapping[CHANNELPINMAPPER_MAXPINS];

private:

  WDL_Queue m_cfgret;
  int m_nCh, m_nPins;
};

// converts interleaved buffer to interleaved buffer, using min(len_in,len_out) and zeroing any extra samples
// isInput means it reads from track channels and writes to plugin pins
// wantZeroExcessOutput=false means that untouched channels will be preserved in buf_out
void PinMapperConvertBuffers(const double *buf, int len_in, int nch_in, 
                             double *buf_out, int len_out, int nch_out,
                             const ChannelPinMapper *pinmap, bool isInput, bool wantZeroExcessOutput);

// use for float and double only ... ints will break it
class AudioBufferContainer
{
public:

  AudioBufferContainer();
  ~AudioBufferContainer() {}

  enum 
  {
    FMT_32FP=4,
    FMT_64FP=8
  };

  static bool BufConvert(void* dest, const void* src, int destFmt, int srcFmt, int nFrames, int destStride, int srcStride);

  int GetNChannels() const { return m_nCh; }
  int GetNFrames() const { return m_nFrames; }
  int GetFormat() const { return m_fmt; }
    
  void Resize(int nCh, int nFrames, bool preserveData);  
  // call Reformat(GetFormat(), false) to discard current data (for efficient repopulating)
  void Reformat(int fmt, bool preserveData); 
    
  // src=NULL to memset(0)
  void* SetAllChannels(int fmt, const void* src, int nCh, int nFrames);
  
  // src=NULL to memset(0)
  void* SetChannel(int fmt, const void* src, int chIdx, int nFrames);
  
  void* MixChannel(int fmt, const void* src, int chIdx, int nFrames, bool addToDest, double wt_start, double wt_end);
  
  void* GetAllChannels(int fmt, bool preserveData);
  void* GetChannel(int fmt, int chIdx, bool preserveData);
  
  void CopyFrom(const AudioBufferContainer* rhs);
  
private:

  void ReLeave(bool interleave, bool preserveData);

  WDL_HeapBuf m_data;
  int m_nCh;
  int m_nFrames;

  int m_fmt;
  bool m_interleaved;
  bool m_hasData;
} WDL_FIXALIGN;


void SetPinsFromChannels(AudioBufferContainer* dest, AudioBufferContainer* src, const ChannelPinMapper* mapper, int forceMinChanCnt=0);
void SetChannelsFromPins(AudioBufferContainer* dest, AudioBufferContainer* src, const ChannelPinMapper* mapper, double wt_start=1.0, double wt_end=1.0);


#endif
