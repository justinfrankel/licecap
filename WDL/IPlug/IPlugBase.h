#ifndef _IPLUGBASE_
#define _IPLUGBASE_

#define IPLUG_VERSION 0x010000

#include "Containers.h"
#include "IPlugStructs.h"
#include "IParam.h"
#include "Hosts.h"
#include "Log.h"

// Uncomment to enable IPlug::OnIdle() and IGraphics::OnGUIIdle().
// #define USE_IDLE_CALLS

#define MAX_EFFECT_NAME_LEN 128
#define DEFAULT_BLOCK_SIZE 1024

// All version ints are stored as 0xVVVVRRMM: V = version, R = revision, M = minor revision.

class IGraphics;

class IPlugBase
{
public:
      
  // Use IPLUG_CTOR instead of calling directly (defined in IPlug_include_in_plug_src.h).
	IPlugBase(int nParams, const char* channelIOStr, int nPresets,
		const char* effectName, const char* productName, const char* mfrName,
		int vendorVersion, int uniqueID, int mfrID, int latency, 
    bool plugDoesMidi, bool plugDoesChunks, bool plugIsInst);

  // ----------------------------------------
  // Your plugin class implements these.
  // There are default impls, mostly just for reference.
  
  virtual ~IPlugBase();

  // Implementations should set a mutex lock like in the no-op!
	virtual void Reset() { TRACE; IMutexLock lock(this); }
	virtual void OnParamChange(int paramIdx) { IMutexLock lock(this); }
	
	// Default passthrough.  Inputs and outputs are [nChannel][nSample].
  // Mutex is already locked.
	virtual void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

	// In case the audio processing thread needs to do anything when the GUI opens
	// (like for example, set some state dependent initial values for controls).
	virtual void OnGUIOpen() { TRACE; }
	virtual void OnGUIClose() { TRACE; }

	// This is an idle call from the audio processing thread, as opposed to 
	// IGraphics::OnGUIIdle which is called from the GUI thread.
  // Only active if USE_IDLE_CALLS is defined.
	virtual void OnIdle() {}

  // Not usually needed ... Reset is called on activate regardless of whether this is implemented.
  // Also different hosts have different interpretations of "activate".
  // Implementations should set a mutex lock like in the no-op!  
  virtual void OnActivate(bool active) { TRACE;  IMutexLock lock(this); }
    
	virtual void ProcessMidiMsg(IMidiMsg* pMsg);
	virtual bool MidiNoteName(int noteNumber, char* rName) { *rName = '\0'; return false; }

  // Implementations should set a mutex lock.
	virtual bool SerializeState(ByteChunk* pChunk) { return SerializeParams(pChunk); }
  // Return the new chunk position (endPos).
	virtual int UnserializeState(ByteChunk* pChunk, int startPos) { return UnserializeParams(pChunk, startPos); }

  // ----------------------------------------
  // Your plugin class, or a control class, can call these functions.

  int NParams() { return mParams.GetSize(); }
	IParam* GetParam(int idx) { return mParams.Get(idx); }
	IGraphics* GetGUI() { return mGraphics; }
  
  const char* GetEffectName() { return mEffectName; }
  int GetEffectVersion(bool decimal);   // Decimal = VVVVRRMM, otherwise 0xVVVVRRMM.
  void GetEffectVersionStr(char* str);
  const char* GetMfrName() { return mMfrName; }
  const char* GetProductName() { return mProductName; }
  
  int GetUniqueID() { return mUniqueID; }
  int GetMfrID() { return mMfrID; }

	void SetParameterFromGUI(int idx, double normalizedValue);
  // If a parameter change comes from the GUI, midi, or external input,
  // the host needs to be informed in case the changes are being automated.
  virtual void BeginInformHostOfParamChange(int idx) = 0;
  virtual void InformHostOfParamChange(int idx, double normalizedValue) = 0;
  virtual void EndInformHostOfParamChange(int idx) = 0;

  // ----------------------------------------
  // Useful stuff for your plugin class or an outsider to call, 
  // most of which is implemented by the API class.

  double GetSampleRate() { return mSampleRate; }
	int GetBlockSize() { return mBlockSize; }
  int GetLatency() { return mLatency; }
  
  // In ProcessDoubleReplacing you are always guaranteed to get valid pointers 
  // to all the channels the plugin requested.  If the host hasn't connected all the pins,
  // the unconnected channels will be full of zeros.
  int NInChannels() { return mInChannels.GetSize(); }
  int NOutChannels() { return mOutChannels.GetSize(); }
  bool IsInChannelConnected(int chIdx);
  bool IsOutChannelConnected(int chIdx);
      
	virtual int GetSamplePos() = 0;   // Samples since start of project.
	virtual double GetTempo() = 0;
	double GetSamplesPerBeat();
	virtual void GetTimeSig(int* pNum, int* pDenom) = 0;
  
	virtual EHost GetHost() { return mHost; }
	int GetHostVersion(bool decimal); // Decimal = VVVVRRMM, otherwise 0xVVVVRRMM.
  void GetHostVersionStr(char* str);
  
  // Tell the host that the graphics resized.
  // Should be called only by the graphics object when it resizes itself.
  virtual void ResizeGraphics(int w, int h) = 0;
  
  // Not fully supported.  A call back from the host saying the user has resized the window.
  // If the plugin supports different sizes, it may wish to resize.
  virtual void UserResizedWindow(IRECT* pR) {}
    
  void EnsureDefaultPreset();
  
protected:

  // ----------------------------------------
  // Useful stuff for your plugin class to call, implemented here or in the API class, or partly in both.

  struct ChannelIO 
  { 
    int mIn, mOut; 
    ChannelIO(int nIn, int nOut) : mIn(nIn), mOut(nOut) {}
  };
  WDL_PtrList<ChannelIO> mChannelIO;
  bool LegalIO(int nIn, int nOut);    // -1 for either means check the other value only.
  void LimitToStereoIO();
  
  void SetHost(const char* host, int version);   // Version = 0xVVVVRRMM.
  virtual void HostSpecificInit() = 0;
  
	virtual void AttachGraphics(IGraphics* pGraphics);
  
  void SetSampleRate(double sampleRate);
  virtual void SetBlockSize(int blockSize); 
  // If latency changes after initialization (often not supported by the host).
  virtual void SetLatency(int samples);
	virtual bool SendMidiMsg(IMidiMsg* pMsg) = 0;
  virtual bool SendMidiMsgs(WDL_TypedBuf<IMidiMsg>* pMsgs) = 0;
  bool IsInst() { return mIsInst; }
    
  void MakeDefaultPreset(char* name = 0, int nPresets = 1);
  // MakePreset(name, param1, param2, ..., paramN)
  void MakePreset(char* name, ...);
  // MakePresetFromNamedParams(name, nParamsNamed, paramEnum1, paramVal1, paramEnum2, paramVal2, ..., paramEnumN, paramVal2)
  // nParamsNamed may be less than the total number of params.
  void MakePresetFromNamedParams(char* name, int nParamsNamed, ...);

  bool DoesStateChunks() { return mStateChunks; }
  // Will append if the chunk is already started.
  bool SerializeParams(ByteChunk* pChunk);
  // Returns the new chunk position (endPos).
  int UnserializeParams(ByteChunk* pChunk, int startPos);
  void RedrawParamControls();  // Called after restoring state.

  // ----------------------------------------
  // Internal IPlug stuff (but API classes need to get at it).
  
  void OnParamReset();	// Calls OnParamChange(each param) + Reset().

  int NPresets() { return mPresets.GetSize(); }
  int GetCurrentPresetIdx() { return mCurrentPresetIdx; }
  void PruneUninitializedPresets();
  bool RestorePreset(int idx);
  bool RestorePreset(const char* name);
  const char* GetPresetName(int idx);
  void ModifyCurrentPreset(const char* name = 0);     // Sets the currently active preset to whatever current params are.
  bool SerializePresets(ByteChunk* pChunk);
  // Returns the new chunk position (endPos).
  int UnserializePresets(ByteChunk* pChunk, int startPos);

  // Dump the current state as source code for a call to MakePresetFromNamedParams.
  void DumpPresetSrcCode(const char* filename, const char* paramEnumNames[]);

  // Set connection state for n channels.
  // If a channel is connected, we expect a call to attach the buffers before each process call.
  // If a channel is not connected, we attach scratch buffers now and don't need to do anything else.
  void SetInputChannelConnections(int idx, int n, bool connected);
  void SetOutputChannelConnections(int idx, int n, bool connected);

  void AttachInputBuffers(int idx, int n, double** ppData, int nFrames);
  void AttachInputBuffers(int idx, int n, float** ppData, int nFrames);
  void AttachOutputBuffers(int idx, int n, double** ppData);
  void AttachOutputBuffers(int idx, int n, float** ppData);
  void ProcessBuffers(float sampleType, int nFrames);
  void ProcessBuffers(double sampleType, int nFrames);
  void ProcessBuffersAccumulating(float sampleType, int nFrames); 

public:
  
  WDL_Mutex mMutex;

  struct IMutexLock 
  {
    WDL_Mutex* mpMutex;
    IMutexLock(IPlugBase* pPlug) : mpMutex(&(pPlug->mMutex)) { mpMutex->Enter(); }
    ~IMutexLock() { if (mpMutex) { mpMutex->Leave(); } }
    void Destroy() { mpMutex->Leave(); mpMutex = 0; }
  };
 
private:

  char mEffectName[MAX_EFFECT_NAME_LEN], mProductName[MAX_EFFECT_NAME_LEN], mMfrName[MAX_EFFECT_NAME_LEN];
  int mUniqueID, mMfrID, mVersion;   //  Version stored as 0xVVVVRRMM: V = version, R = revision, M = minor revision.
  
  EHost mHost;
  int mHostVersion;   //  Version stored as 0xVVVVRRMM: V = version, R = revision, M = minor revision.

  bool mStateChunks, mIsInst;
  double mSampleRate;
  int mBlockSize, mLatency;

 	WDL_PtrList<IParam> mParams;
	IGraphics* mGraphics;

  WDL_PtrList<IPreset> mPresets;
  int mCurrentPresetIdx;

  WDL_TypedBuf<double*> mInData, mOutData;
  struct InChannel {
    bool mConnected;
    double** mSrc;   // Points into mInData.
    WDL_TypedBuf<double> mScratchBuf;
  };
  struct OutChannel {
    bool mConnected;
    double** mDest;  // Points into mOutData.
    float* mFDest;
    WDL_TypedBuf<double> mScratchBuf;
  };
  WDL_PtrList<InChannel> mInChannels;
  WDL_PtrList<OutChannel> mOutChannels;
};

#endif
