#ifndef _IPLUGAPI_
#define _IPLUGAPI_
// Only load one API class!

#include "IPlugBase.h"
#include <CoreServices/CoreServices.h>
#include <AudioUnit/AUComponent.h>
#include <AudioUnit/AudioUnitProperties.h>
#include <AudioToolbox/AudioUnitUtilities.h>
#include <AudioUnit/AudioUnitCarbonView.h>

// Argh!
#if MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_4
  typedef Float32 AudioSampleType;
  typedef	Float32	AudioUnitParameterValue;
  typedef OSStatus (*AUMIDIOutputCallback)(void*, const AudioTimeStamp*, UInt32, const struct MIDIPacketList*);
  struct AUMIDIOutputCallbackStruct { AUMIDIOutputCallback midiOutputCallback; void* userData; };
  #define kAudioFormatFlagsCanonical (kAudioFormatFlagIsFloat|kAudioFormatFlagsNativeEndian|kAudioFormatFlagIsPacked)
#endif

#define MAX_IO_CHANNELS 128

struct IPlugInstanceInfo
{
  WDL_String mOSXBundleID, mCocoaViewFactoryClassName;
};

class IPlugAU : public IPlugBase
{
public:

  // Use IPLUG_CTOR instead of calling directly (defined in IPlug_include_in_plug_src.h).
	IPlugAU(IPlugInstanceInfo instanceInfo, int nParams, const char* channelIOStr, int nPresets, 
		const char* effectName, const char* productName, const char* mfrName,
		int vendorVersion, int uniqueID, int mfrID, int latency, 
    bool plugDoesMidi, bool plugDoesChunks,  bool plugIsInst);

  virtual ~IPlugAU();
  
  // ----------------------------------------
  // See IPlugBase for the full list of methods that your plugin class can implement.

  void BeginInformHostOfParamChange(int idx);
  void InformHostOfParamChange(int idx, double normalizedValue);
  void EndInformHostOfParamChange(int idx);
  
	int GetSamplePos();   // Samples since start of project.
	double GetTempo();
	void GetTimeSig(int* pNum, int* pDenom);
	EHost GetHost();  // GetHostVersion() is inherited.
  
  // Tell the host that the graphics resized.
  // Should be called only by the graphics object when it resizes itself.
  void ResizeGraphics(int w, int h) {}
  
  enum EAUInputType {
    eNotConnected = 0,
    eDirectFastProc,
    eDirectNoFastProc,
    eRenderCallback
  };      
  
protected:

  void HostSpecificInit();
  void SetBlockSize(int blockSize);
  void SetLatency(int samples);
	bool SendMidiMsg(IMidiMsg* pMsg);
  bool SendMidiMsgs(WDL_TypedBuf<IMidiMsg>* pMsgs);
  
private:

  WDL_String mOSXBundleID, mCocoaViewFactoryClassName;
  ComponentInstance mCI;
  bool mActive, mBypassed;
  double mRenderTimestamp, mTempo;
  HostCallbackInfo mHostCallbacks;

 // InScratchBuf is only needed if the upstream connection is a callback.
 // OutScratchBuf is only needed if the downstream connection fails to give us a buffer.  
  WDL_TypedBuf<AudioSampleType> mInScratchBuf, mOutScratchBuf; 
  WDL_PtrList<AURenderCallbackStruct> mRenderNotify;
  AUMIDIOutputCallbackStruct mMidiCallback;
  
  // Every stereo pair of plugin input or output is a bus.
  // Buses can have zero host channels if the host hasn't connected the bus at all,
  // one host channel if the plugin supports mono and the host has supplied a mono stream,
  // or two host channels if the host has supplied a stereo stream.  
  struct BusChannels {
    bool mConnected;
    int mNHostChannels, mNPlugChannels, mPlugChannelStartIdx;
  };
  WDL_PtrList<BusChannels> mInBuses, mOutBuses;
  BusChannels* GetBus(AudioUnitScope scope, AudioUnitElement busIdx);
  int NHostChannelsConnected(WDL_PtrList<BusChannels>* pBuses, int excludeIdx = -1);
  void ClearConnections();
  
  struct InputBusConnection {
    void* mUpstreamObj;
    AudioUnit mUpstreamUnit;
    int mUpstreamBusIdx;    
    AudioUnitRenderProc mUpstreamRenderProc; 
    AURenderCallbackStruct mUpstreamRenderCallback;
    EAUInputType mInputType;
  };
  WDL_PtrList<InputBusConnection> mInBusConnections;
  
  bool CheckLegalIO(AudioUnitScope scope, int busIdx, int nChannels); 
  bool CheckLegalIO(); 
  void AssessInputConnections();
  
  struct PropertyListener {
    AudioUnitPropertyID mPropID;
    AudioUnitPropertyListenerProc mListenerProc;
    void* mProcArgs;
  };
  WDL_PtrList<PropertyListener> mPropertyListeners;
  
  ComponentResult GetPropertyInfo(AudioUnitPropertyID propID, AudioUnitScope scope, AudioUnitElement element,
    UInt32* pDataSize, Boolean* pWriteable);
  ComponentResult GetProperty(AudioUnitPropertyID propID, AudioUnitScope scope, AudioUnitElement element,
    UInt32* pDataSize, Boolean* pWriteable, void* pData); 
  ComponentResult SetProperty(AudioUnitPropertyID propID, AudioUnitScope scope, AudioUnitElement element,
    UInt32* pDataSize, const void* pData);
  ComponentResult GetProc(AudioUnitElement element, UInt32* pDataSize, void* pData);
  ComponentResult GetState(CFPropertyListRef* ppPropList);
  ComponentResult SetState(CFPropertyListRef pPropList);
  void InformListeners(AudioUnitPropertyID propID, AudioUnitScope scope);
	

public:

  static ComponentResult IPlugAUEntry(ComponentParameters *params, void* pVPlug);
  static ComponentResult IPlugAUCarbonViewEntry(ComponentParameters *params, void* pView);
  static ComponentResult GetParamProc(void* pPlug, AudioUnitParameterID paramID, AudioUnitScope scope, AudioUnitElement element, 
    AudioUnitParameterValue* pValue);
  static ComponentResult SetParamProc(void* pPlug, AudioUnitParameterID paramID, AudioUnitScope scope, AudioUnitElement element, 
    AudioUnitParameterValue value, UInt32 offsetFrames);
  static ComponentResult RenderProc(void* pPlug, AudioUnitRenderActionFlags* pFlags, const AudioTimeStamp* pTimestamp,
    UInt32 outputBusIdx, UInt32 nFrames, AudioBufferList* pBufferList);
};

IPlugAU* MakePlug();

#endif


