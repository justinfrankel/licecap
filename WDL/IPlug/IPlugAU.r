#include "resource.h"   // This is your plugin's resource.h.
#include <AudioUnit/AudioUnit.r>

#define UseExtendedThingResource 1

#include <CoreServices/CoreServices.r>

// this is a define used to indicate that a component has no static data that would mean 
// that no more than one instance could be open at a time - never been true for AUs
#ifndef cmpThreadSafeOnMac
#define cmpThreadSafeOnMac	0x10000000
#endif

#undef  TARGET_REZ_MAC_PPC
#ifdef ppc_YES
	#define TARGET_REZ_MAC_PPC        1
#else
	#define TARGET_REZ_MAC_PPC        0
#endif

#undef  TARGET_REZ_MAC_X86
#ifdef i386_YES
	#define TARGET_REZ_MAC_X86        1
#else
	#define TARGET_REZ_MAC_X86        0
#endif

#if TARGET_OS_MAC
	#if TARGET_REZ_MAC_PPC && TARGET_REZ_MAC_X86
		#define TARGET_REZ_FAT_COMPONENTS	1
		#define Target_PlatformType			platformPowerPCNativeEntryPoint
		#define Target_SecondPlatformType	platformIA32NativeEntryPoint
	#elif TARGET_REZ_MAC_X86
		#define Target_PlatformType			platformIA32NativeEntryPoint
	#else
		#define Target_PlatformType			platformPowerPCNativeEntryPoint
	#endif
	#define Target_CodeResType		'dlle'
	#define TARGET_REZ_USE_DLLE		1
#else
	#error get a real platform type
#endif // not TARGET_OS_MAC

#ifndef TARGET_REZ_FAT_COMPONENTS
  #define TARGET_REZ_FAT_COMPONENTS			0
#endif

// ----------------

//#ifdef _DEBUG
//  #define PLUG_PUBLIC_NAME PLUG_NAME "_DEBUG"
//#else
#define PLUG_PUBLIC_NAME PLUG_NAME
//#endif

#define RES_ID 1000
#define RES_NAME PLUG_MFR ": " PLUG_PUBLIC_NAME

resource 'STR ' (RES_ID, purgeable) {
	RES_NAME
};

resource 'STR ' (RES_ID + 1, purgeable) {
	PLUG_PUBLIC_NAME " AU"
};

resource 'dlle' (RES_ID) {
	PLUG_ENTRY_STR
};

resource 'thng' (RES_ID, RES_NAME) {
#if PLUG_IS_INST
  kAudioUnitType_MusicDevice,
#else
  kAudioUnitType_Effect,
#endif
	PLUG_UNIQUE_ID,
	PLUG_MFR_ID,
	0, 0, 0, 0,								//	no 68K
	'STR ',	RES_ID,
	'STR ',	RES_ID + 1,
	0,	0,			// icon 
	PLUG_VER,
	componentHasMultiplePlatforms | componentDoAutoVersion,
	0,
	{
		cmpThreadSafeOnMac, 
		Target_CodeResType, RES_ID,
		Target_PlatformType,
#if TARGET_REZ_FAT_COMPONENTS
		cmpThreadSafeOnMac, 
		Target_CodeResType, RES_ID,
		Target_SecondPlatformType,
#endif
	}
};

#undef RES_ID
#define RES_ID 2000
#undef RES_NAME
#define RES_NAME PLUG_MFR ": " PLUG_PUBLIC_NAME " Carbon View"

resource 'STR ' (RES_ID, purgeable) {
	RES_NAME
};

resource 'STR ' (RES_ID + 1, purgeable) {
	PLUG_PUBLIC_NAME " AU Carbon View"
};

resource 'dlle' (RES_ID) {
	PLUG_VIEW_ENTRY_STR
};

resource 'thng' (RES_ID, RES_NAME) {
	kAudioUnitCarbonViewComponentType,
	PLUG_UNIQUE_ID,
	PLUG_MFR_ID,
	0, 0, 0, 0,								//	no 68K
	'STR ',	RES_ID,
	'STR ',	RES_ID + 1,
	0,	0,			// icon 
	PLUG_VER,
	componentHasMultiplePlatforms | componentDoAutoVersion,
	0,
	{
		cmpThreadSafeOnMac, 
		Target_CodeResType, RES_ID,
		Target_PlatformType,
#if TARGET_REZ_FAT_COMPONENTS
		cmpThreadSafeOnMac, 
		Target_CodeResType, RES_ID,
		Target_SecondPlatformType,
#endif
	}
};

#undef RES_ID


