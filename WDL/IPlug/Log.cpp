#include "Log.h"
#include "stdio.h"
#include "string.h"
#include "time.h"
#include <fstream>

#ifdef _WIN32
  #define LOGFILE "C:\\IPlugLog.txt"
#else
  #define LOGFILE "/IPlugLog.txt"
#endif
  
const int TXTLEN = 1024;

// _vsnsprintf

#define VARARGS_TO_STR(str) { \
	try { \
		va_list argList;  \
		va_start(argList, format);  \
		int i = vsnprintf(str, TXTLEN-2, format, argList); \
		if (i < 0 || i > TXTLEN-2) {  \
			str[TXTLEN-1] = '\0';  \
		} \
		va_end(argList);  \
	} \
	catch(...) {  \
		strcpy(str, "parse error"); \
	} \
  strcat(str, "\r\n"); \
}

struct LogFile
{
	FILE* mFP;

	LogFile() 
  {
    mFP = fopen(LOGFILE, "w");
    assert(mFP);
  }
  
	~LogFile()
  {
    fclose(mFP); 
    mFP = 0; 
  }
};

Timer::Timer()
{
	mT = clock();
}

bool Timer::Every(double sec)
{
	if (clock() - mT > sec * CLOCKS_PER_SEC) {
		mT = clock();
		return true;
	}
	return false;
};

// Needs rewriting for WDL.
//StrVector ReadFileIntoStr(WDL_String* pFileName)
//{
//	WDL_PtrList<WDL_String> lines;
//	if (!strlen(pFileName->Get())) {
//		WDL_String line;
//		std::ifstream ifs(pFileName->Get());
//		if (ifs) {
//			while (std::getline(ifs, line, '\n')) {
//				if (!line.empty()) {
//					lines.push_back(line);
//				}
//			}
//			ifs.close();
//		}
//	}
//	return lines;
//}

// Needs rewriting for WDL.
//int WriteBufferToFile(const DBuffer& buf, const std::string& fileName)
//{
//    std::ofstream ofs(fileName.c_str());
//    if (ofs) {
//        for (int i = 0, p = buf.Pos(); i < buf.Size(); ++i, ++p) {
//            ofs << buf[p] << ' ';
//        }
//        ofs << '\n';
//        ofs.close();
//        return buf.Size();
//    }
//    return 0;
//}

bool IsWhitespace(char c)
{
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

// Needs rewriting for WDL.
//StrVector SplitStr(const std::string& line, char separator)
//{
//	std::string field;
//	StrVector fields;
//	for (int i = 0; i < line.size(); ++i) {
//        bool sep = (line[i] == separator);
//        sep |= (IsWhitespace(separator) && IsWhitespace(line[i]));
//        
//        if (!sep) {
//			field.push_back(line[i]);
//		}
//		else
//        if (!field.empty()) {
//			fields.push_back(field);
//			field.clear();
//		}
//	}
//	if (!field.empty()) {
//		fields.push_back(field);
//	}
//	return fields;
//}

void ToLower(char* cDest, const char* cSrc)
{
  int i, n = (int) strlen(cSrc);
	for (i = 0; i < n; ++i) {
		cDest[i] = tolower(cSrc[i]);
	}
  cDest[i] = '\0';
}

const char* CurrentTime()
{
	time_t t = time(0);
	tm* pT = localtime(&t);

	char cStr[64];
	strftime(cStr, 64, "%Y%m%d %H:%M ", pT);

  char cTZ[64], tz[64];
	strftime(cTZ, 64, "%Z", pT);  
	int i, j, nZ = strlen(cTZ);
	for (i = 0, j = 0; i < nZ; ++i) {
		if (isupper(cTZ[i])) {
      tz[j++] = cTZ[i];
    }
	}
  tz[j] = '\0';
  
  static char sTimeStr[256];
  strcpy(sTimeStr, cStr);
  strcat(sTimeStr, tz);
  return sTimeStr;
}

void CompileTimestamp(const char* Mmm_dd_yyyy, const char* hh_mm_ss, WDL_String* pStr)
{
    pStr->Set("[");
    pStr->Append(Mmm_dd_yyyy);
    pStr->SetLen(7);
    pStr->DeleteSub(4, 1);
    pStr->Append(" ");
    pStr->Append(hh_mm_ss);
    pStr->SetLen(12);
    pStr->Append("]");
}

const char* AppendTimestamp(const char* Mmm_dd_yyyy, const char* hh_mm_ss, const char* cStr)
{
    static WDL_String str(cStr);
    WDL_String tStr;
    CompileTimestamp(Mmm_dd_yyyy, hh_mm_ss, &tStr);
    str.Append(" ");
    str.Append(tStr.Get());
    return str.Get();
}

#if defined TRACER_BUILD

  int GetOrdinalThreadID(int sysThreadID)
  {
    static WDL_TypedBuf<int> sThreadIDs;
    int i, n = sThreadIDs.GetSize();
    int* pThreadID = sThreadIDs.Get();
    for (i = 0; i < n; ++i, ++pThreadID) {
      if (sysThreadID == *pThreadID) {
        return i;
      }
    }
    sThreadIDs.Resize(n + 1);
    *(sThreadIDs.Get() + n) = sysThreadID;
    return n;
  }

  #define MAX_LOG_LINES 16384
  void Trace(const char* funcName, int line, const char* format, ...)
  {
    static int sTrace = 0;
    if (sTrace++ < MAX_LOG_LINES) {
      static LogFile sLogFile;
      static WDL_Mutex sLogMutex;      
      char str[TXTLEN];
      VARARGS_TO_STR(str);
      printf("[%d:%s:%d]%s", GetOrdinalThreadID(SYS_THREAD_ID), funcName, line, str);
     
      //WDL_MutexLock lock(&sLogMutex);
      //fprintf(sLogFile.mFP, "[%d:%s:%d]%s", GetOrdinalThreadID(SYS_THREAD_ID), funcName, line, str);
      //fflush(sLogFile.mFP);
    }
  }

  #include "../../VST_SDK/aeffectx.h"
  const char* VSTOpcodeStr(int opCode)
  {
    switch (opCode) {
      case effOpen:    return "effOpen";
      case effClose:    return "effClose";
      case effSetProgram:    return "effSetProgram";
      case effGetProgram:    return "effGetProgram";
      case effSetProgramName:    return "effSetProgramName";
      case effGetProgramName:    return "effGetProgramName";
      case effGetParamLabel:    return "effGetParamLabel";
      case effGetParamDisplay:    return "effGetParamDisplay";
      case effGetParamName:    return "effGetParamName";
      case __effGetVuDeprecated:    return "__effGetVuDeprecated";
      case effSetSampleRate:    return "effSetSampleRate";
      case effSetBlockSize:    return "effSetBlockSize";
      case effMainsChanged:    return "effMainsChanged";
      case effEditGetRect:    return "effEditGetRect";
      case effEditOpen:    return "effEditOpen";
      case effEditClose:    return "effEditClose";
      case __effEditDrawDeprecated:    return "__effEditDrawDeprecated";
      case __effEditMouseDeprecated:    return "__effEditMouseDeprecated";
      case __effEditKeyDeprecated:    return "__effEditKeyDeprecated";
      case effEditIdle:    return "effEditIdle";
      case __effEditTopDeprecated:    return "__effEditTopDeprecated";
      case __effEditSleepDeprecated:    return "__effEditSleepDeprecated";
      case __effIdentifyDeprecated:    return "__effIdentifyDeprecated";
      case effGetChunk:    return "effGetChunk";
      case effSetChunk:    return "effSetChunk";
      case effProcessEvents:    return "effProcessEvents";
      case effCanBeAutomated:    return "effCanBeAutomated";
      case effString2Parameter:    return "effString2Parameter";
      case __effGetNumProgramCategoriesDeprecated:    return "__effGetNumProgramCategoriesDeprecated";
      case effGetProgramNameIndexed:    return "effGetProgramNameIndexed";
      case __effCopyProgramDeprecated:    return "__effCopyProgramDeprecated";
      case __effConnectInputDeprecated:    return "__effConnectInputDeprecated";
      case __effConnectOutputDeprecated:    return "__effConnectOutputDeprecated";
      case effGetInputProperties:    return "effGetInputProperties";
      case effGetOutputProperties:    return "effGetOutputProperties";
      case effGetPlugCategory:    return "effGetPlugCategory";
      case __effGetCurrentPositionDeprecated:    return "__effGetCurrentPositionDeprecated";
      case __effGetDestinationBufferDeprecated:    return "__effGetDestinationBufferDeprecated";
      case effOfflineNotify:    return "effOfflineNotify";
      case effOfflinePrepare:    return "effOfflinePrepare";
      case effOfflineRun:    return "effOfflineRun";
      case effProcessVarIo:    return "effProcessVarIo";
      case effSetSpeakerArrangement:    return "effSetSpeakerArrangement";
      case __effSetBlockSizeAndSampleRateDeprecated:    return "__effSetBlockSizeAndSampleRateDeprecated";
      case effSetBypass:    return "effSetBypass";
      case effGetEffectName:    return "effGetEffectName";
      case __effGetErrorTextDeprecated:    return "__effGetErrorTextDeprecated";
      case effGetVendorString:    return "effGetVendorString";
      case effGetProductString:    return "effGetProductString";
      case effGetVendorVersion:    return "effGetVendorVersion";
      case effVendorSpecific:    return "effVendorSpecific";
      case effCanDo:    return "effCanDo";
      case effGetTailSize:    return "effGetTailSize";
      case __effIdleDeprecated:    return "__effIdleDeprecated";
      case __effGetIconDeprecated:    return "__effGetIconDeprecated";
      case __effSetViewPositionDeprecated:    return "__effSetViewPositionDeprecated";
      case effGetParameterProperties:    return "effGetParameterProperties";
      case __effKeysRequiredDeprecated:    return "__effKeysRequiredDeprecated";
      case effGetVstVersion:    return "effGetVstVersion";
      case effEditKeyDown:    return "effEditKeyDown";
      case effEditKeyUp:    return "effEditKeyUp";
      case effSetEditKnobMode:    return "effSetEditKnobMode";
      case effGetMidiProgramName:    return "effGetMidiProgramName";
      case effGetCurrentMidiProgram:    return "effGetCurrentMidiProgram";
      case effGetMidiProgramCategory:    return "effGetMidiProgramCategory";
      case effHasMidiProgramsChanged:    return "effHasMidiProgramsChanged";
      case effGetMidiKeyName:    return "effGetMidiKeyName";
      case effBeginSetProgram:    return "effBeginSetProgram";
      case effEndSetProgram:    return "effEndSetProgram";
      case effGetSpeakerArrangement:    return "effGetSpeakerArrangement";
      case effShellGetNextPlugin:    return "effShellGetNextPlugin";
      case effStartProcess:    return "effStartProcess";
      case effStopProcess:    return "effStopProcess";
      case effSetTotalSampleToProcess:    return "effSetTotalSampleToProcess";
      case effSetPanLaw:    return "effSetPanLaw";
      case effBeginLoadBank:    return "effBeginLoadBank";
      case effBeginLoadProgram:    return "effBeginLoadProgram";
      case effSetProcessPrecision:    return "effSetProcessPrecision";
      case effGetNumMidiInputChannels:    return "effGetNumMidiInputChannels";
      case effGetNumMidiOutputChannels:    return "effGetNumMidiOutputChannels";
      default:    return "unknown";
    }
  }
  #if defined __APPLE__
    #include <AudioUnit/AudioUnitProperties.h>
    #include <AudioUnit/AudioUnitCarbonView.h>
    const char* AUSelectStr(int select)
    {
      switch (select) {
        case kComponentOpenSelect:  return "kComponentOpenSelect";
        case kComponentCloseSelect:   return "kComponentCloseSelect";
        case kComponentVersionSelect:  return "kComponentVersionSelect";
        case kAudioUnitInitializeSelect:  return "kAudioUnitInitializeSelect";
        case kAudioUnitUninitializeSelect:  return "kAudioUnitUninitializeSelect";
        case kAudioUnitGetPropertyInfoSelect:  return "kAudioUnitGetPropertyInfoSelect";
        case kAudioUnitGetPropertySelect: return "kAudioUnitGetPropertySelect";
        case kAudioUnitSetPropertySelect:  return "kAudioUnitSetPropertySelect";
        case kAudioUnitAddPropertyListenerSelect:  return "kAudioUnitAddPropertyListenerSelect";
        case kAudioUnitRemovePropertyListenerSelect:  return "kAudioUnitRemovePropertyListenerSelect";
        case kAudioUnitAddRenderNotifySelect:  return "kAudioUnitAddRenderNotifySelect";
        case kAudioUnitRemoveRenderNotifySelect:  return "kAudioUnitRemoveRenderNotifySelect";
        case kAudioUnitGetParameterSelect:  return "kAudioUnitGetParameterSelect";
        case kAudioUnitSetParameterSelect:  return "kAudioUnitSetParameterSelect";
        case kAudioUnitScheduleParametersSelect:  return "kAudioUnitScheduleParametersSelect";
        case kAudioUnitRenderSelect:  return "kAudioUnitRenderSelect";
        case kAudioUnitResetSelect:  return "kAudioUnitResetSelect";
        case kComponentCanDoSelect:  return "kComponentCanDoSelect";
        case kAudioUnitCarbonViewRange:  return "kAudioUnitCarbonViewRange";
        case kAudioUnitCarbonViewCreateSelect:  return "kAudioUnitCarbonViewCreateSelect";
        case kAudioUnitCarbonViewSetEventListenerSelect:  return "kAudioUnitCarbonViewSetEventListenerSelect";   
        default:  return "unknown";
      }
    }
    const char* AUPropertyStr(int propID)
    {
      switch (propID) {
        case kAudioUnitProperty_ClassInfo:  return "kAudioUnitProperty_ClassInfo";
        case kAudioUnitProperty_MakeConnection:  return "kAudioUnitProperty_MakeConnection";
        case kAudioUnitProperty_SampleRate:  return "kAudioUnitProperty_SampleRate";
        case kAudioUnitProperty_ParameterList:  return "kAudioUnitProperty_ParameterList";
        case kAudioUnitProperty_ParameterInfo:  return "kAudioUnitProperty_ParameterInfo";
        case kAudioUnitProperty_FastDispatch:  return "kAudioUnitProperty_FastDispatch";
        case kAudioUnitProperty_CPULoad:  return "kAudioUnitProperty_CPULoad";
        case kAudioUnitProperty_StreamFormat:  return "kAudioUnitProperty_StreamFormat";
        case kAudioUnitProperty_ElementCount:  return "kAudioUnitProperty_ElementCount";   
        case kAudioUnitProperty_Latency:  return "kAudioUnitProperty_Latency";   
        case kAudioUnitProperty_SupportedNumChannels:  return "kAudioUnitProperty_SupportedNumChannels";   
        case kAudioUnitProperty_MaximumFramesPerSlice:  return "kAudioUnitProperty_MaximumFramesPerSlice";
        case kAudioUnitProperty_SetExternalBuffer:  return "kAudioUnitProperty_SetExternalBuffer";
        case kAudioUnitProperty_ParameterValueStrings:  return "kAudioUnitProperty_ParameterValueStrings";
        case kAudioUnitProperty_GetUIComponentList:  return "kAudioUnitProperty_GetUIComponentList";    
        case kAudioUnitProperty_AudioChannelLayout:  return "kAudioUnitProperty_AudioChannelLayout";  
        case kAudioUnitProperty_TailTime:  return "kAudioUnitProperty_TailTime";
        case kAudioUnitProperty_BypassEffect:  return "kAudioUnitProperty_BypassEffect";
        case kAudioUnitProperty_LastRenderError:  return "kAudioUnitProperty_LastRenderError";
        case kAudioUnitProperty_SetRenderCallback:  return "kAudioUnitProperty_SetRenderCallback";
        case kAudioUnitProperty_FactoryPresets:  return "kAudioUnitProperty_FactoryPresets";
        case kAudioUnitProperty_ContextName:  return "kAudioUnitProperty_ContextName";
        case kAudioUnitProperty_RenderQuality:  return "kAudioUnitProperty_RenderQuality";
        case kAudioUnitProperty_HostCallbacks:  return "kAudioUnitProperty_HostCallbacks";
        case kAudioUnitProperty_CurrentPreset: return "kAudioUnitProperty_CurrentPreset";
        case kAudioUnitProperty_InPlaceProcessing:  return "kAudioUnitProperty_InPlaceProcessing";
        case kAudioUnitProperty_ElementName:  return "kAudioUnitProperty_ElementName";    
        case kAudioUnitProperty_CocoaUI:  return "kAudioUnitProperty_CocoaUI";
        case kAudioUnitProperty_SupportedChannelLayoutTags:  return "kAudioUnitProperty_SupportedChannelLayoutTags";
        case kAudioUnitProperty_ParameterIDName:  return "kAudioUnitProperty_ParameterIDName";
        case kAudioUnitProperty_ParameterClumpName:  return "kAudioUnitProperty_ParameterClumpName";
        case kAudioUnitProperty_PresentPreset:  return "kAudioUnitProperty_PresentPreset";
        case kAudioUnitProperty_OfflineRender:  return "kAudioUnitProperty_OfflineRender";
        case kAudioUnitProperty_ParameterStringFromValue:  return "kAudioUnitProperty_ParameterStringFromValue";
        case kAudioUnitProperty_ParameterValueFromString:  return "kAudioUnitProperty_ParameterValueFromString";
        case kAudioUnitProperty_IconLocation:  return "kAudioUnitProperty_IconLocation";
        case kAudioUnitProperty_PresentationLatency:  return "kAudioUnitProperty_PresentationLatency";
        case kAudioUnitProperty_DependentParameters:  return "kAudioUnitProperty_DependentParameters";
        #if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
          case kAudioUnitProperty_AUHostIdentifier:  return "kAudioUnitProperty_AUHostIdentifier";
          case kAudioUnitProperty_MIDIOutputCallbackInfo:  return "kAudioUnitProperty_MIDIOutputCallbackInfo";
          case kAudioUnitProperty_MIDIOutputCallback:  return "kAudioUnitProperty_MIDIOutputCallback";
          case kAudioUnitProperty_InputSamplesInOutput:  return "kAudioUnitProperty_InputSamplesInOutput";
          case kAudioUnitProperty_ClassInfoFromDocument:  return "kAudioUnitProperty_ClassInfoFromDocument";
        #endif
        default:  return "unknown";
      }
    }
    const char* AUScopeStr(int scope)
    {
      switch (scope) {
        case kAudioUnitScope_Global:  return "kAudioUnitScope_Global";
        case kAudioUnitScope_Input: return "kAudioUnitScope_Input";
        case kAudioUnitScope_Output:  return "kAudioUnitScope_Output";
        case kAudioUnitScope_Group: return "kAudioUnitScope_Group";
        case kAudioUnitScope_Part:  return "kAudioUnitScope_Part";
        #if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5      
          case kAudioUnitScope_Note:  return "kAudioUnitScope_Note";
        #endif
        default:  return "unknown";
      }
    }
  #endif // __APPLE__
#else 
  void Trace(const char* funcName, int line, const char* format, ...) {}
  const char* VSTOpcodeStr(int opCode) { return ""; }
  const char* AUSelectStr(int select) { return ""; }
  const char* AUPropertyStr(int propID) { return ""; }
  const char* AUScopeStr(int scope) { return ""; }
#endif // TRACER_BUILD