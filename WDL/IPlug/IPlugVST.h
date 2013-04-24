#ifndef _IPLUGAPI_
#define _IPLUGAPI_
// Only load one API class!

#include "IPlugBase.h"
#include "../../VST_SDK/aeffectx.h"

struct IPlugInstanceInfo
{
  audioMasterCallback mVSTHostCallback;
};

class IPlugVST : public IPlugBase
{
public:

  // Use IPLUG_CTOR instead of calling directly (defined in IPlug_include_in_plug_src.h).
	IPlugVST(IPlugInstanceInfo instanceInfo, int nParams, const char* channelIOStr, int nPresets,
		const char* effectName, const char* productName, const char* mfrName,
		int vendorVersion, int uniqueID, int mfrID, int latency = 0, 
    bool plugDoesMidi = false, bool plugDoesChunks = false, 
    bool plugIsInst = false);

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
  void ResizeGraphics(int w, int h);

protected:

  void HostSpecificInit();
	void AttachGraphics(IGraphics* pGraphics);  
  void SetLatency(int samples);
	bool SendMidiMsg(IMidiMsg* pMsg);
  bool SendMidiMsgs(WDL_TypedBuf<IMidiMsg>* pMsgs);
  audioMasterCallback GetHostCallback();

private:

  template <class SAMPLETYPE>
  void VSTPrepProcess(SAMPLETYPE** inputs, SAMPLETYPE** outputs, VstInt32 nFrames);

  ERect mEditRect;
  audioMasterCallback mHostCallback;

  bool SendVSTEvent(VstEvent* pEvent);
  bool SendVSTEvents(WDL_TypedBuf<VstEvent>* pEvents);

  VstSpeakerArrangement mInputSpkrArr, mOutputSpkrArr;

  bool mHostSpecificInitDone;
  bool mDoesMidi;
  
  enum { VSTEXT_NONE=0, VSTEXT_COCKOS, VSTEXT_COCOA }; // list of VST extensions supported by host
  int mHasVSTExtensions;
  
  ByteChunk mState;     // Persistent storage if the host asks for plugin state.
  ByteChunk mBankState; // Persistent storage if the host asks for bank state.

public:

	static VstIntPtr VSTCALLBACK VSTDispatcher(AEffect *pEffect, VstInt32 opCode, VstInt32 idx, VstIntPtr value, void *ptr, float opt);
	static void VSTCALLBACK VSTProcess(AEffect *pEffect, float **inputs, float **outputs, VstInt32 nFrames);	// Deprecated.
	static void VSTCALLBACK VSTProcessReplacing(AEffect *pEffect, float **inputs, float **outputs, VstInt32 nFrames);
	static void VSTCALLBACK VSTProcessDoubleReplacing(AEffect *pEffect, double **inputs, double **outputs, VstInt32 nFrames);
  static float VSTCALLBACK VSTGetParameter(AEffect *pEffect, VstInt32 idx);
	static void VSTCALLBACK VSTSetParameter(AEffect *pEffect, VstInt32 idx, float value);
	AEffect mAEffect;
};

IPlugVST* MakePlug();

#endif