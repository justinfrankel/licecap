#ifndef _IPLUG_INCLUDE_HDR_
#define _IPLUG_INCLUDE_HDR_

// Include this file in the main header for your plugin, 
// after #defining either VST_API or AU_API.

#include "resource.h" // This is your plugin's resource.h.

#if defined VST_API
  #include "IPlugVST.h"
  typedef IPlugVST IPlug;
  #define API_EXT "vst"
#elif defined AU_API
  #include "IPlugAU.h"
  typedef IPlugAU IPlug;
  #define API_EXT "audiounit"
#else
  #error "No API defined!"
#endif

#if defined _WIN32
  #include "IGraphicsWin.h"
  #define EXPORT __declspec(dllexport)
#elif defined __APPLE__
  #include "IGraphicsMac.h"
  #define EXPORT
  #define BUNDLE_ID "com." BUNDLE_MFR "." API_EXT "." BUNDLE_NAME
#else
  #error "No OS defined!"
#endif

#endif