#ifndef _IPLUG_INCLUDE_SRC_
#define _IPLUG_INCLUDE_SRC_

// Include this file in the main source for your plugin, 
// after #including the main header for your plugin.

#if defined _WIN32
  HINSTANCE gHInstance = 0;
	BOOL WINAPI DllMain(HINSTANCE hDllInst, DWORD fdwReason, LPVOID res)
	{
    gHInstance = hDllInst;
    return TRUE;
  }
  IGraphics* MakeGraphics(IPlug* pPlug, int w, int h, int FPS = 0)
  {
    IGraphicsWin* pGraphics = new IGraphicsWin(pPlug, w, h, FPS);
    pGraphics->SetHInstance(gHInstance);
    return pGraphics;
  }
#elif defined __APPLE__
  IGraphics* MakeGraphics(IPlug* pPlug, int w, int h, int FPS = 0)
  {
    IGraphicsMac* pGraphics = new IGraphicsMac(pPlug, w, h, FPS);
    pGraphics->SetBundleID(BUNDLE_ID);
    return pGraphics;
  }   
#else 
  #error "No OS defined!"
#endif 

#if defined VST_API
  extern "C"
  {
    EXPORT void* VSTPluginMain(audioMasterCallback hostCallback)
    {    
      static WDL_Mutex sMutex;
      WDL_MutexLock lock(&sMutex);
      IPlugInstanceInfo instanceInfo;
      instanceInfo.mVSTHostCallback = hostCallback;
      IPlugVST* pPlug = new PLUG_CLASS_NAME(instanceInfo);
      if (pPlug) { 
        pPlug->EnsureDefaultPreset();
        pPlug->mAEffect.numPrograms = MAX(pPlug->mAEffect.numPrograms, 1);
        return &(pPlug->mAEffect);
      }
	    return 0;
    }
    EXPORT int main(audioMasterCallback hostCallback)
    {
      return (int) VSTPluginMain(hostCallback);
    }
  };
#elif defined AU_API
  IPlug* MakePlug()
  {
    static WDL_Mutex sMutex;
    WDL_MutexLock lock(&sMutex);
    IPlugInstanceInfo instanceInfo;
    instanceInfo.mOSXBundleID.Set(BUNDLE_ID);
    instanceInfo.mCocoaViewFactoryClassName.Set(VIEW_CLASS_STR);
    return new PLUG_CLASS_NAME(instanceInfo);
  }
  extern "C"
  {
    ComponentResult PLUG_ENTRY(ComponentParameters* params, void* pPlug)
    {
      return IPlugAU::IPlugAUEntry(params, pPlug);
    }
    ComponentResult PLUG_VIEW_ENTRY(ComponentParameters* params, void* pView)
    {
      return IPlugAU::IPlugAUCarbonViewEntry(params, pView);
    }
  };
#else
  #error "No API defined!"
#endif

#if defined _DEBUG
  #define PUBLIC_NAME APPEND_TIMESTAMP(PLUG_NAME " DEBUG")
#elif defined TRACER_BUILD
  #define PUBLIC_NAME APPEND_TIMESTAMP(PLUG_NAME " TRACER")
#elif defined TIMESTAMP_PLUG_NAME
  #pragma REMINDER("plug name is timestamped")
  #define PUBLIC_NAME APPEND_TIMESTAMP(PLUG_NAME)
#else
  #define PUBLIC_NAME PLUG_NAME
#endif

#define IPLUG_CTOR(nParams, nPresets, instanceInfo) \
  IPlug(instanceInfo, nParams, PLUG_CHANNEL_IO, nPresets, \
    PUBLIC_NAME, "", PLUG_MFR, PLUG_VER, PLUG_UNIQUE_ID, PLUG_MFR_ID, \
    PLUG_LATENCY, PLUG_DOES_MIDI, PLUG_DOES_STATE_CHUNKS, PLUG_IS_INST)

#endif