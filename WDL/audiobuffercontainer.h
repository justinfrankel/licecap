#ifndef _AUDIOBUFFERCONTAINER_
#define _AUDIOBUFFERCONTAINER_

#include "wdltypes.h"
#include <string.h>
#include <stdlib.h>
#include "ptrlist.h"
#include "queue.h"


#define CHANNELPINMAPPER_MAXPINS 64

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

  char* SaveStateNew(int* pLen); // owned
  bool LoadState(char* buf, int len);

  WDL_UINT64 m_mapping[CHANNELPINMAPPER_MAXPINS];

private:

  WDL_Queue m_cfgret;
  int m_nCh, m_nPins;
};


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

  static bool BufConvert(void* dest, void* src, int destFmt, int srcFmt, int nFrames, int destStride, int srcStride);

  int GetNChannels() { return m_nCh; }
  int GetNFrames() { return m_nFrames; }
  int GetFormat() { return m_fmt; }
    
  void Resize(int nCh, int nFrames, bool preserveData);  
  // call Reformat(GetFormat(), false) to discard current data (for efficient repopulating)
  void Reformat(int fmt, bool preserveData); 
    
  // src=NULL to memset(0)
  void* SetAllChannels(int fmt, void* src, int nCh, int nFrames);
  
  // src=NULL to memset(0)
  void* SetChannel(int fmt, void* src, int chIdx, int nFrames);
  
  void* MixChannel(int fmt, void* src, int chIdx, int nFrames, bool addToDest, double wt_start, double wt_end);
  
  void* GetAllChannels(int fmt, bool preserveData);
  void* GetChannel(int fmt, int chIdx, bool preserveData);
  
  void CopyFrom(AudioBufferContainer* rhs);
  
private:

  void ReLeave(bool interleave, bool preserveData);

  WDL_HeapBuf m_data;
  int m_nCh;
  int m_nFrames;

  int m_fmt;
  bool m_interleaved;
  bool m_hasData;
} WDL_FIXALIGN;


void SetPinsFromChannels(AudioBufferContainer* dest, AudioBufferContainer* src, ChannelPinMapper* mapper, int forceMinChanCnt=0);
void SetChannelsFromPins(AudioBufferContainer* dest, AudioBufferContainer* src, ChannelPinMapper* mapper, double wt_start=1.0, double wt_end=1.0);


#endif
