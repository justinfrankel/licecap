#include "IPlugBase.h"
#include "IGraphics.h"
#include "IControl.h"
#include <math.h>
#include <stdio.h>
#include <time.h>

const double DEFAULT_SAMPLE_RATE = 44100.0;

template <class SRC, class DEST> 
void CastCopy(DEST* pDest, SRC* pSrc, int n)
{
  for (int i = 0; i < n; ++i, ++pDest, ++pSrc) {
    *pDest = (DEST) *pSrc;
  }
}

void GetVersionParts(int version, int* pVer, int* pMaj, int* pMin)
{
  *pVer = (version & 0xFFFF0000) >> 16;
  *pMaj = (version & 0x0000FF00) >> 8;
  *pMin = version & 0x000000FF;
}

int GetDecimalVersion(int version)
{
 int ver, rmaj, rmin;
 GetVersionParts(version, &ver, &rmaj, &rmin);
 return 10000 * ver + 100 * rmaj + rmin;
}

void GetVersionStr(int version, char* str)
{
  int ver, rmaj, rmin;
  GetVersionParts(version, &ver, &rmaj, &rmin);
  //if (rmin) {
  //  sprintf(str, "v%d.%d.%d", ver, rmaj, rmin);
  //}
  //else
  //if (rmaj) {
    sprintf(str, "v%d.%02d", ver, rmaj);
  //}
  //else {
  //  sprintf(str, "v%d", ver);
  //}
}

IPlugBase::IPlugBase(int nParams, const char* channelIOStr, int nPresets,
	const char* effectName, const char* productName, const char* mfrName,
	int vendorVersion, int uniqueID, int mfrID, int latency, 
  bool plugDoesMidi, bool plugDoesChunks, bool plugIsInst)
: mUniqueID(uniqueID), mMfrID(mfrID), mVersion(vendorVersion),
  mSampleRate(DEFAULT_SAMPLE_RATE), mBlockSize(0), mLatency(latency), mHost(kHostUninit), mHostVersion(0),
  mStateChunks(plugDoesChunks), mGraphics(0), mCurrentPresetIdx(0), mIsInst(plugIsInst)
{
  Trace(TRACELOC, "%s:%s", effectName, CurrentTime());
  
  for (int i = 0; i < nParams; ++i) {
    mParams.Add(new IParam);
  }

  for (int i = 0; i < nPresets; ++i) {
    mPresets.Add(new IPreset(i));
  }

  strcpy(mEffectName, effectName);
  strcpy(mProductName, productName);
  strcpy(mMfrName, mfrName);

  int nInputs = 0, nOutputs = 0;
  while (channelIOStr) {
    int nIn = 0, nOut = 0;
    assert(sscanf(channelIOStr, "%d-%d", &nIn, &nOut) == 2);
    nInputs = MAX(nInputs, nIn);
    nOutputs = MAX(nOutputs, nOut);
    mChannelIO.Add(new ChannelIO(nIn, nOut));
    channelIOStr = strstr(channelIOStr, " ");
    if (channelIOStr) {
      ++channelIOStr;
    }
  }

  mInData.Resize(nInputs);
  mOutData.Resize(nOutputs);

  double** ppInData = mInData.Get();
  for (int i = 0; i < nInputs; ++i, ++ppInData) {
    InChannel* pInChannel = new InChannel;
    pInChannel->mConnected = false;
    pInChannel->mSrc = ppInData;
    mInChannels.Add(pInChannel);
  }
  double** ppOutData = mOutData.Get();
  for (int i = 0; i < nOutputs; ++i, ++ppOutData) {
    OutChannel* pOutChannel = new OutChannel;
    pOutChannel->mConnected = false;
    pOutChannel->mDest = ppOutData;
    pOutChannel->mFDest = 0;
    mOutChannels.Add(pOutChannel);
  }
}

IPlugBase::~IPlugBase()
{ 
  TRACE;
	DELETE_NULL(mGraphics);
  mParams.Empty(true);
  mPresets.Empty(true);
  mInChannels.Empty(true);
  mOutChannels.Empty(true);
  mChannelIO.Empty(true);
}

int IPlugBase::GetHostVersion(bool decimal)
{
  GetHost();
  if (decimal) {
    return GetDecimalVersion(mHostVersion);
  }    
  return mHostVersion;
} 

void IPlugBase::GetHostVersionStr(char* str)
{
  GetVersionStr(mHostVersion, str);
}

bool IPlugBase::LegalIO(int nIn, int nOut)
{
  bool legal = false;
  int i, n = mChannelIO.GetSize();
  for (i = 0; i < n && !legal; ++i) {
    ChannelIO* pIO = mChannelIO.Get(i);
    legal = ((nIn < 0 || nIn == pIO->mIn) && (nOut < 0 || nOut == pIO->mOut));
  }
  Trace(TRACELOC, "%d:%d:%s", nIn, nOut, (legal ? "legal" : "illegal"));
  return legal;  
}

void IPlugBase::LimitToStereoIO()
{
  int nIn = NInChannels(), nOut = NOutChannels();
  if (nIn > 2) {
    SetInputChannelConnections(2, nIn - 2, false);
  }
  if (nOut > 2) {
    SetOutputChannelConnections(2, nOut - 2, true);  
  }
}

void IPlugBase::SetHost(const char* host, int version)
{
  mHost = LookUpHost(host);
  mHostVersion = version;
  
  char vStr[32];
  GetVersionStr(version, vStr);
  Trace(TRACELOC, "host_%sknown:%s:%s", (mHost == kHostUnknown ? "un" : ""), host, vStr);
}

void IPlugBase::AttachGraphics(IGraphics* pGraphics)
{
	if (pGraphics) {
    WDL_MutexLock lock(&mMutex);
    int i, n = mParams.GetSize();
		for (i = 0; i < n; ++i) {
			pGraphics->SetParameterFromPlug(i, GetParam(i)->GetNormalized(), true);
		}
    pGraphics->PrepDraw();
		mGraphics = pGraphics;
	}
}

// Decimal = VVVVRRMM, otherwise 0xVVVVRRMM.
int IPlugBase::GetEffectVersion(bool decimal)   
{
  if (decimal) {
    return GetDecimalVersion(mVersion);
  }
  return mVersion;
}

void IPlugBase::GetEffectVersionStr(char* str)
{
  GetVersionStr(mVersion, str);
  #if defined _DEBUG
    strcat(str, "D");
  #elif defined TRACER_BUILD
    strcat(str, "T");
  #endif
}

double IPlugBase::GetSamplesPerBeat()
{
	double tempo = GetTempo();
	if (tempo > 0.0) {
		return GetSampleRate() * 60.0 / tempo;	
	}
	return 0.0;
}

void IPlugBase::SetSampleRate(double sampleRate)
{
  mSampleRate = sampleRate;
}

void IPlugBase::SetBlockSize(int blockSize)
{
  if (blockSize != mBlockSize) {
    int i, nIn = NInChannels(), nOut = NOutChannels();
    for (i = 0; i < nIn; ++i) {
      InChannel* pInChannel = mInChannels.Get(i);
      pInChannel->mScratchBuf.Resize(blockSize);
      memset(pInChannel->mScratchBuf.Get(), 0, blockSize * sizeof(double));
    }
    for (i = 0; i < nOut; ++i) {
      OutChannel* pOutChannel = mOutChannels.Get(i);
      pOutChannel->mScratchBuf.Resize(blockSize);
      memset(pOutChannel->mScratchBuf.Get(), 0, blockSize * sizeof(double));
    }
    mBlockSize = blockSize;
  }
}

void IPlugBase::SetInputChannelConnections(int idx, int n, bool connected)
{
  int iEnd = MIN(idx + n, mInChannels.GetSize());
  for (int i = idx; i < iEnd; ++i) {
    InChannel* pInChannel = mInChannels.Get(i);
    pInChannel->mConnected = connected;
    if (!connected) {
      *(pInChannel->mSrc) = pInChannel->mScratchBuf.Get();
    }
  }
}

void IPlugBase::SetOutputChannelConnections(int idx, int n, bool connected)
{
  int iEnd = MIN(idx + n, mOutChannels.GetSize());
  for (int i = idx; i < iEnd; ++i) {
    OutChannel* pOutChannel = mOutChannels.Get(i);
    pOutChannel->mConnected = connected;
    if (!connected) {
      *(pOutChannel->mDest) = pOutChannel->mScratchBuf.Get();
    } 
  }
}

bool IPlugBase::IsInChannelConnected(int chIdx) 
{ 
  return (chIdx < mInChannels.GetSize() && mInChannels.Get(chIdx)->mConnected);
}

bool IPlugBase:: IsOutChannelConnected(int chIdx) 
{
  return (chIdx < mOutChannels.GetSize() && mOutChannels.Get(chIdx)->mConnected); 
}

void IPlugBase::AttachInputBuffers(int idx, int n, double** ppData, int nFrames)
{
  int iEnd = MIN(idx + n, mInChannels.GetSize());
  for (int i = idx; i < iEnd; ++i) {
    InChannel* pInChannel = mInChannels.Get(i);
    if (pInChannel->mConnected) {
      *(pInChannel->mSrc) = *(ppData++);
    }
  }
}

void IPlugBase::AttachInputBuffers(int idx, int n, float** ppData, int nFrames)
{
  int iEnd = MIN(idx + n, mInChannels.GetSize());
  for (int i = idx; i < iEnd; ++i) {
    InChannel* pInChannel = mInChannels.Get(i);
    if (pInChannel->mConnected) {
      double* pScratch = pInChannel->mScratchBuf.Get();
      CastCopy(pScratch, *(ppData++), nFrames);
      *(pInChannel->mSrc) = pScratch;
    }
  }
}
 
void IPlugBase::AttachOutputBuffers(int idx, int n, double** ppData)
{
  int iEnd = MIN(idx + n, mOutChannels.GetSize());
  for (int i = idx; i < iEnd; ++i) {
    OutChannel* pOutChannel = mOutChannels.Get(i);
    if (pOutChannel->mConnected) {
      *(pOutChannel->mDest) = *(ppData++);
    }
  }
}

void IPlugBase::AttachOutputBuffers(int idx, int n, float** ppData)
{
  int iEnd = MIN(idx + n, mOutChannels.GetSize());
  for (int i = idx; i < iEnd; ++i) {
    OutChannel* pOutChannel = mOutChannels.Get(i);
    if (pOutChannel->mConnected) {
      *(pOutChannel->mDest) = pOutChannel->mScratchBuf.Get();
      pOutChannel->mFDest = *(ppData++);
    }
  }
}

#pragma REMINDER("lock mutex before calling into any IPlugBase processing functions")

void IPlugBase::ProcessBuffers(double sampleType, int nFrames) 
{
  ProcessDoubleReplacing(mInData.Get(), mOutData.Get(), nFrames);
}

void IPlugBase::ProcessBuffers(float sampleType, int nFrames)
{
  ProcessDoubleReplacing(mInData.Get(), mOutData.Get(), nFrames);
  int i, n = NOutChannels();
  OutChannel** ppOutChannel = mOutChannels.GetList();
  for (i = 0; i < n; ++i, ++ppOutChannel) {
    OutChannel* pOutChannel = *ppOutChannel;
    if (pOutChannel->mConnected) {
      CastCopy(pOutChannel->mFDest, *(pOutChannel->mDest), nFrames);
    }
  }   
}

void IPlugBase::ProcessBuffersAccumulating(float sampleType, int nFrames)
{
  ProcessDoubleReplacing(mInData.Get(), mOutData.Get(), nFrames);
  int i, n = NOutChannels();
  OutChannel** ppOutChannel = mOutChannels.GetList();
  for (i = 0; i < n; ++i, ++ppOutChannel) {
    OutChannel* pOutChannel = *ppOutChannel;  
    if (pOutChannel->mConnected) {
      float* pDest = pOutChannel->mFDest;
      double* pSrc = *(pOutChannel->mDest);
      for (int j = 0; j < nFrames; ++j, ++pDest, ++pSrc) { 
        *pDest += (float) *pSrc;
      }
    }
  }
}

// If latency changes after initialization (often not supported by the host).
void IPlugBase::SetLatency(int samples)
{
  mLatency = samples;
}

void IPlugBase::SetParameterFromGUI(int idx, double normalizedValue)
{
  Trace(TRACELOC, "%d:%f", idx, normalizedValue);
  WDL_MutexLock lock(&mMutex);
  GetParam(idx)->SetNormalized(normalizedValue);
  InformHostOfParamChange(idx, normalizedValue);
	OnParamChange(idx);
}

void IPlugBase::OnParamReset()
{
	for (int i = 0; i < mParams.GetSize(); ++i) {
		OnParamChange(i);
	}
	//Reset();
}

// Default passthrough.
void IPlugBase::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked.
  int i, nIn = mInChannels.GetSize(), nOut = mOutChannels.GetSize();
  for (i = 0; i < nIn; ++i) {
    memcpy(outputs[i], inputs[i], nFrames * sizeof(double));
  }
  for (/* same i */; i < nOut; ++i) {
    memset(outputs[i], 0, nFrames * sizeof(double));
  }
}

// Default passthrough.
void IPlugBase::ProcessMidiMsg(IMidiMsg* pMsg)
{
	SendMidiMsg(pMsg);
}

IPreset* GetNextUninitializedPreset(WDL_PtrList<IPreset>* pPresets)
{
  int n = pPresets->GetSize();
  for (int i = 0; i < n; ++i) {
    IPreset* pPreset = pPresets->Get(i);
    if (!(pPreset->mInitialized)) {
      return pPreset;
    }
  }
  return 0;
}

void IPlugBase::MakeDefaultPreset(char* name, int nPresets)
{
  for (int i = 0; i < nPresets; ++i) {
    IPreset* pPreset = GetNextUninitializedPreset(&mPresets);
    if (pPreset) {
      pPreset->mInitialized = true;
      strcpy(pPreset->mName, (name ? name : "Default"));
      SerializeParams(&(pPreset->mChunk)); 
    }
  }
}

#define GET_PARAM_FROM_VARARG(paramType, vp, v) \
{ \
  v = 0.0; \
  switch (paramType) { \
    case IParam::kTypeBool: \
    case IParam::kTypeInt: \
    case IParam::kTypeEnum: { \
      v = (double) va_arg(vp, int); \
      break; \
    } \
    case IParam::kTypeDouble: \
    default: { \
      v = (double) va_arg(vp, double); \
      break; \
    } \
  } \
}

void IPlugBase::MakePreset(char* name, ...)
{
  IPreset* pPreset = GetNextUninitializedPreset(&mPresets);
  if (pPreset) {
    pPreset->mInitialized = true;
    strcpy(pPreset->mName, name);
    int i, n = mParams.GetSize();
      
    double v = 0.0;
    va_list vp;
    va_start(vp, name);
    for (i = 0; i < n; ++i) {
      GET_PARAM_FROM_VARARG(GetParam(i)->Type(), vp, v);
      pPreset->mChunk.Put(&v);
    }
  }
}

#define PARAM_UNINIT 99.99e-9

void IPlugBase::MakePresetFromNamedParams(char* name, int nParamsNamed, ...)
{
  IPreset* pPreset = GetNextUninitializedPreset(&mPresets);
  if (pPreset) {
    pPreset->mInitialized = true;
    strcpy(pPreset->mName, name);

    int i = 0, n = mParams.GetSize();

    WDL_TypedBuf<double> vals;
    vals.Resize(n);
    double* pV = vals.Get();
    for (i = 0; i < n; ++i, ++pV) {
      *pV = PARAM_UNINIT;
    }

    va_list vp;
    va_start(vp, nParamsNamed);
    for (int i = 0; i < nParamsNamed; ++i) {
      int paramIdx = (int) va_arg(vp, int);
      // This assert will fire if any of the passed-in param values do not match
      // the type that the param was initialized with (int for bool, int, enum; double for double).
      assert(paramIdx >= 0 && paramIdx < n);
      GET_PARAM_FROM_VARARG(GetParam(paramIdx)->Type(), vp, *(vals.Get() + paramIdx));
    }
    va_end(vp);

    pV = vals.Get();
    for (int i = 0; i < n; ++i, ++pV) {
      if (*pV == PARAM_UNINIT) {      // Any that weren't explicitly set, use the defaults.
        *pV = GetParam(i)->Value();
      }
      pPreset->mChunk.Put(pV);
    }
  }
}

#define DEFAULT_USER_PRESET_NAME "user preset"

void MakeDefaultUserPresetName(WDL_PtrList<IPreset>* pPresets, char* str)
{
  int nDefaultNames = 0;
  int n = pPresets->GetSize();
  for (int i = 0; i < n; ++i) {
    IPreset* pPreset = pPresets->Get(i);
    if (strstr(pPreset->mName, DEFAULT_USER_PRESET_NAME)) {
      ++nDefaultNames;
    }
  }
  sprintf(str, "%s %d", DEFAULT_USER_PRESET_NAME, nDefaultNames + 1);
}

void IPlugBase::EnsureDefaultPreset()
{
  if (!(mPresets.GetSize())) {
    mPresets.Add(new IPreset(0));
    MakeDefaultPreset();
  }
}

void IPlugBase::PruneUninitializedPresets()
{
  int i = 0;
  while (i < mPresets.GetSize()) {
    IPreset* pPreset = mPresets.Get(i);
    if (pPreset->mInitialized) {
      ++i;
    }
    else {
      mPresets.Delete(i, true);
    }
  }
}

bool IPlugBase::RestorePreset(int idx)
{
  bool restoredOK = false;
  if (idx >= 0 && idx < mPresets.GetSize()) {
    IPreset* pPreset = mPresets.Get(idx);

    if (!(pPreset->mInitialized)) {
      pPreset->mInitialized = true;
      MakeDefaultUserPresetName(&mPresets, pPreset->mName);
      restoredOK = SerializeParams(&(pPreset->mChunk)); 
    }
    else {
      restoredOK = (UnserializeParams(&(pPreset->mChunk), 0) > 0);
    }

    if (restoredOK) {
      mCurrentPresetIdx = idx;
      RedrawParamControls();
    }
  }
  return restoredOK;
}

bool IPlugBase::RestorePreset(const char* name)
{
  if (CSTR_NOT_EMPTY(name)) {
    int n = mPresets.GetSize();
    for (int i = 0; i < n; ++i) {
      IPreset* pPreset = mPresets.Get(i);
      if (!strcmp(pPreset->mName, name)) {
        return RestorePreset(i);
      }
    }
  }
  return false;
}

const char* IPlugBase::GetPresetName(int idx)
{
  if (idx >= 0 && idx < mPresets.GetSize()) {
    return mPresets.Get(idx)->mName;
  }
  return "";
}

void IPlugBase::ModifyCurrentPreset(const char* name)
{
  if (mCurrentPresetIdx >= 0 && mCurrentPresetIdx < mPresets.GetSize()) {
    IPreset* pPreset = mPresets.Get(mCurrentPresetIdx);
    pPreset->mChunk.Clear();


    SerializeParams(&(pPreset->mChunk));

    if (CSTR_NOT_EMPTY(name)) 
    {
      strcpy(pPreset->mName, name);
    }
  }
}

bool IPlugBase::SerializePresets(ByteChunk* pChunk)
{
  bool savedOK = true;
  int n = mPresets.GetSize();
  for (int i = 0; i < n && savedOK; ++i) {
    IPreset* pPreset = mPresets.Get(i);
    pChunk->PutStr(pPreset->mName);
    pChunk->PutBool(pPreset->mInitialized);
    if (pPreset->mInitialized) {
      savedOK &= (pChunk->PutChunk(&(pPreset->mChunk)) > 0);
    }
  }
  return savedOK;
}

int IPlugBase::UnserializePresets(ByteChunk* pChunk, int startPos)
{
  WDL_String name;
  int n = mPresets.GetSize(), pos = startPos;
  for (int i = 0; i < n && pos >= 0; ++i) {
    IPreset* pPreset = mPresets.Get(i);
    pos = pChunk->GetStr(&name, pos);
    strcpy(pPreset->mName, name.Get());
    pos = pChunk->GetBool(&(pPreset->mInitialized), pos);
    if (pPreset->mInitialized) {
      pos = UnserializeParams(pChunk, pos);
      if (pos > 0) {
        pPreset->mChunk.Clear();
        SerializeParams(&(pPreset->mChunk));
      }
    }
  }
  RestorePreset(mCurrentPresetIdx);
  return pos;
}

bool IPlugBase::SerializeParams(ByteChunk* pChunk)
{
  TRACE;
  
  WDL_MutexLock lock(&mMutex);
  bool savedOK = true;
  int i, n = mParams.GetSize();
  for (i = 0; i < n && savedOK; ++i) {
    IParam* pParam = mParams.Get(i);
    double v = pParam->Value();
    savedOK &= (pChunk->Put(&v) > 0);
  }
  return savedOK;
}

int IPlugBase::UnserializeParams(ByteChunk* pChunk, int startPos)
{
  TRACE;
  
  WDL_MutexLock lock(&mMutex);
  int i, n = mParams.GetSize(), pos = startPos;
  for (i = 0; i < n && pos >= 0; ++i) {
    IParam* pParam = mParams.Get(i);
    double v = 0.0;
    Trace(TRACELOC, "%d %s", i, pParam->GetNameForHost());
    pos = pChunk->Get(&v, pos);
    pParam->Set(v);
  }
  OnParamReset();
  return pos;
}

void IPlugBase::RedrawParamControls()
{
  if (mGraphics) {
    int i, n = mParams.GetSize();
    for (i = 0; i < n; ++i) {
      double v = mParams.Get(i)->Value();
      mGraphics->SetParameterFromPlug(i, v, false);
    }
  }
}

void IPlugBase::DumpPresetSrcCode(const char* filename, const char* paramEnumNames[])
{
  static bool sDumped = false;
  if (!sDumped) {
    sDumped = true;
    int i, n = NParams();
    FILE* fp = fopen(filename, "w");
    fprintf(fp, "  MakePresetFromNamedParams(\"name\", %d", n - 1);
    for (i = 0; i < n - 1; ++i) {
      IParam* pParam = GetParam(i);
      char paramVal[32];
      switch (pParam->Type()) {
        case IParam::kTypeBool:
          sprintf(paramVal, "%s", (pParam->Bool() ? "true" : "false"));
          break;
        case IParam::kTypeInt: 
          sprintf(paramVal, "%d", pParam->Int());
          break;
        case IParam::kTypeEnum:
          sprintf(paramVal, "%d", pParam->Int());
          break;
        case IParam::kTypeDouble:
        default:
          sprintf(paramVal, "%.2f", pParam->Value());
          break;
      }
      fprintf(fp, ",\n    %s, %s", paramEnumNames[i], paramVal);
    }
    fprintf(fp, ");\n");
    fclose(fp);
  } 
}