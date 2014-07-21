/*
  WDL - lameencdec.cpp
  Copyright (C) 2005 and later Cockos Incorporated

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
  


*/



#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wdlcstring.h"
#include "lameencdec.h"


#if defined(_WIN32) && !defined(_WIN64)

#define SUPPORT_BLADE_MODE

/// the blade mode is only use on 32-bit windows and is completely obsolete (libmp3lame.dll API is preferable and better)
#pragma pack(push)
#pragma pack(1)

#ifdef	__cplusplus
extern "C" {
#endif

/* encoding formats */

#define		BE_CONFIG_MP3			0										
#define		BE_CONFIG_LAME			256		

/* type definitions */

typedef		void *HBE_STREAM;
typedef		HBE_STREAM				*PHBE_STREAM;
typedef		unsigned long			BE_ERR;

/* error codes */

#define		BE_ERR_SUCCESSFUL					0x00000000
#define		BE_ERR_INVALID_FORMAT				0x00000001
#define		BE_ERR_INVALID_FORMAT_PARAMETERS	0x00000002
#define		BE_ERR_NO_MORE_HANDLES				0x00000003
#define		BE_ERR_INVALID_HANDLE				0x00000004
#define		BE_ERR_BUFFER_TOO_SMALL				0x00000005

/* other constants */

#define		BE_MAX_HOMEPAGE			128

/* format specific variables */

#define		BE_MP3_MODE_STEREO		0
#define		BE_MP3_MODE_JSTEREO		1
#define		BE_MP3_MODE_DUALCHANNEL	2
#define		BE_MP3_MODE_MONO		3

#define		MPEG1	1
#define		MPEG2	0

#define CURRENT_STRUCT_VERSION 1
#define CURRENT_STRUCT_SIZE sizeof(BE_CONFIG)	// is currently 331 bytes

/* OBSOLETE, VALUES STILL WORK
typedef enum 
{
	NORMAL_QUALITY=0,
	LOW_QUALITY,
	HIGH_QUALITY,
	VOICE_QUALITY
} LAME_QUALTIY_PRESET;

*/


typedef enum
{
	VBR_METHOD_NONE			= -1,
	VBR_METHOD_DEFAULT		=  0,
	VBR_METHOD_OLD			=  1,
	VBR_METHOD_NEW			=  2,
	VBR_METHOD_MTRH			=  3,
	VBR_METHOD_ABR			=  4
} VBRMETHOD;

typedef enum 
{
	LQP_NOPRESET=-1,

	// QUALITY PRESETS
	LQP_NORMAL_QUALITY=0,
	LQP_LOW_QUALITY,
	LQP_HIGH_QUALITY,
	LQP_VOICE_QUALITY,
	LQP_R3MIX_QUALITY,
	LQP_VERYHIGH_QUALITY,

	// NEW PRESET VALUES
	LQP_PHONE	=1000,
	LQP_SW		=2000,
	LQP_AM		=3000,
	LQP_FM		=4000,
	LQP_VOICE	=5000,
	LQP_RADIO	=6000,
	LQP_TAPE	=7000,
	LQP_HIFI	=8000,
	LQP_CD		=9000,
	LQP_STUDIO	=10000

} LAME_QUALTIY_PRESET;



typedef struct	{
	DWORD	dwConfig;			// BE_CONFIG_XXXXX
								// Currently only BE_CONFIG_MP3 is supported
	union	{

		struct	{

			DWORD	dwSampleRate;	// 48000, 44100 and 32000 allowed.  RG note: also seems to support 16000, 22050, 24000.
			BYTE	byMode;			// BE_MP3_MODE_STEREO, BE_MP3_MODE_DUALCHANNEL, BE_MP3_MODE_MONO
			WORD	wBitrate;		// 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256 and 320 allowed.  RG note: also seems to support 8,16,24.
			BOOL	bPrivate;		
			BOOL	bCRC;
			BOOL	bCopyright;
			BOOL	bOriginal;

			} mp3;					// BE_CONFIG_MP3

			struct
			{
			// STRUCTURE INFORMATION
			DWORD			dwStructVersion;	
			DWORD			dwStructSize;

			// BASIC ENCODER SETTINGS
			DWORD			dwSampleRate;	// SAMPLERATE OF INPUT FILE
			DWORD			dwReSampleRate;	// DOWNSAMPLERATE, 0=ENCODER DECIDES  
			LONG			nMode;			// BE_MP3_MODE_STEREO, BE_MP3_MODE_DUALCHANNEL, BE_MP3_MODE_MONO
			DWORD			dwBitrate;		// CBR bitrate, VBR min bitrate
			DWORD			dwMaxBitrate;	// CBR ignored, VBR Max bitrate
			LONG			nPreset;		// Quality preset, use one of the settings of the LAME_QUALITY_PRESET enum
			DWORD			dwMpegVersion;	// FUTURE USE, MPEG-1 OR MPEG-2
			DWORD			dwPsyModel;		// FUTURE USE, SET TO 0
			DWORD			dwEmphasis;		// FUTURE USE, SET TO 0

			// BIT STREAM SETTINGS
			BOOL			bPrivate;		// Set Private Bit (TRUE/FALSE)
			BOOL			bCRC;			// Insert CRC (TRUE/FALSE)
			BOOL			bCopyright;		// Set Copyright Bit (TRUE/FALSE)
			BOOL			bOriginal;		// Set Original Bit (TRUE/FALSE)
			
			// VBR STUFF
			BOOL			bWriteVBRHeader;	// WRITE XING VBR HEADER (TRUE/FALSE)
			BOOL			bEnableVBR;			// USE VBR ENCODING (TRUE/FALSE)
			INT				nVBRQuality;		// VBR QUALITY 0..9
			DWORD			dwVbrAbr_bps;		// Use ABR in stead of nVBRQuality
			VBRMETHOD		nVbrMethod;
			BOOL			bNoRes;				// Disable Bit resorvoir

			BYTE			btReserved[255-3*sizeof(DWORD)];	// FUTURE USE, SET TO 0

			} LHV1;					// LAME header version 1

		struct	{

			DWORD	dwSampleRate;
			BYTE	byMode;
			WORD	wBitrate;
			BYTE	byEncodingMethod;

		} aac;

	} format;
		
} BE_CONFIG, *PBE_CONFIG;


typedef struct	{

	// BladeEnc DLL Version number

	BYTE	byDLLMajorVersion;
	BYTE	byDLLMinorVersion;

	// BladeEnc Engine Version Number

	BYTE	byMajorVersion;
	BYTE	byMinorVersion;

	// DLL Release date

	BYTE	byDay;
	BYTE	byMonth;
	WORD	wYear;

	// BladeEnc	Homepage URL

	CHAR	zHomepage[BE_MAX_HOMEPAGE + 1];	

	BYTE	byAlphaLevel;
	BYTE	byBetaLevel;
	BYTE	byMMXEnabled;

	BYTE	btReserved[125];


} BE_VERSION, *PBE_VERSION;			


#pragma pack(pop)

#ifdef	__cplusplus
}
#endif

static struct
{
  BE_ERR (*beInitStream)(PBE_CONFIG, PDWORD, PDWORD, void **str);
  BE_ERR (*beCloseStream)(void *str);
  BE_ERR (*beEncodeChunkFloatS16NI)(void *str, DWORD, PFLOAT, PFLOAT, PBYTE, PDWORD);
  BE_ERR (*beDeinitStream)(void *str, PBYTE, PDWORD);
  BE_ERR (*beWriteInfoTag)(HBE_STREAM, const char *);
  VOID (*beVersion)(PBE_VERSION);
} bladeAPI;

#endif // SUPPORT_BLADE_MODE



#ifdef __APPLE__
  #include <Carbon/Carbon.h>
#endif
#ifndef _WIN32
  #include <dlfcn.h>
#endif

typedef enum MPEG_mode_e {
  STEREO = 0,
  JOINT_STEREO,
  DUAL_CHANNEL,   /* LAME doesn't supports this! */
  MONO,
  NOT_SET,
  MAX_INDICATOR   /* Don't use this! It's used for sanity checks. */
} MPEG_mode;

typedef void *lame_t;

static struct {
  int (*close)(lame_t);
  lame_t (*init)();
  int (*set_in_samplerate)(lame_t, int);
  int (*set_num_channels)(lame_t,int);
  int (*set_out_samplerate)(lame_t,int);
  int (*set_quality)(lame_t,int);
  int (*set_mode)(lame_t,MPEG_mode);
  int (*set_brate)(lame_t, int);
  int (*init_params)(lame_t);
  int (*get_framesize)(lame_t);

  int (*encode_buffer_float)(lame_t,
          const float     buffer_l [],       /* PCM data for left channel     */
          const float     buffer_r [],       /* PCM data for right channel    */
          const int           nsamples,      /* number of samples per channel */
          unsigned char*      mp3buf,        /* pointer to encoded MP3 stream */
          const int           mp3buf_size ); 
  int (*encode_flush)(lame_t,unsigned char*       mp3buf,  int                  size);

// these are optional
  int (*set_VBR)(lame_t, int);
  int (*set_VBR_q)(lame_t, int);
  int (*set_VBR_mean_bitrate_kbps)(lame_t, int);
  int (*set_VBR_min_bitrate_kbps)(lame_t, int);
  int (*set_VBR_max_bitrate_kbps)(lame_t, int);
  size_t (*get_lametag_frame)(lame_t, unsigned char *, size_t);
  const char *(*get_lame_version)();
} lame;

#if 1
#define LAME_DEBUG_LOADING(x) 
#else
#define LAME_DEBUG_LOADING(x) OutputDebugString(x)
#endif

static char s_last_dll_file[128];

static bool tryLoadDLL2(const char *name)
{
#ifdef _WIN32
  HINSTANCE dll = LoadLibrary(name);
#else
  void *dll=dlopen(name,RTLD_NOW|RTLD_LOCAL);
#endif

  if (!dll) return false;

  LAME_DEBUG_LOADING("trying to load");
  LAME_DEBUG_LOADING(name);
  int errcnt = 0;
  #ifdef _WIN32
    #define GETITEM(x) if (NULL == (*(void **)&lame.x = GetProcAddress((HINSTANCE)dll,"lame_" #x))) errcnt++;
    #define GETITEM_NP(x) if (NULL == (*(void **)&lame.x = GetProcAddress((HINSTANCE)dll, #x))) errcnt++;
  #else
    #define GETITEM(x) if (NULL == (*(void **)&lame.x = dlsym(dll,"lame_" #x))) errcnt++;
    #define GETITEM_NP(x) if (NULL == (*(void **)&lame.x = dlsym(dll,#x))) errcnt++;
  #endif
  GETITEM(close)
  GETITEM(init)
  GETITEM(set_in_samplerate)
  GETITEM(set_num_channels)
  GETITEM(set_out_samplerate)
  GETITEM(set_quality)
  GETITEM(set_mode)
  GETITEM(set_brate)
  GETITEM(init_params)
  GETITEM(get_framesize)
  GETITEM(encode_buffer_float)
  GETITEM(encode_flush)

  int errcnt2 = errcnt;
  GETITEM(set_VBR)
  GETITEM(set_VBR_q)
  GETITEM(set_VBR_mean_bitrate_kbps)
  GETITEM(set_VBR_min_bitrate_kbps)
  GETITEM(set_VBR_max_bitrate_kbps)
  GETITEM(get_lametag_frame)
  GETITEM_NP(get_lame_version)
  #undef GETITEM   
  #undef GETITEM_NP
  if (errcnt2)
  {
    memset(&lame, 0, sizeof(lame));

    LAME_DEBUG_LOADING("trying blade mode");

    #ifdef SUPPORT_BLADE_MODE
    // try getting blade API
      *((void**)&bladeAPI.beInitStream) = (void *) GetProcAddress(dll, "beInitStream");
      *((void**)&bladeAPI.beCloseStream) = (void *) GetProcAddress(dll, "beCloseStream");
      *((void**)&bladeAPI.beEncodeChunkFloatS16NI) = (void *) GetProcAddress(dll, "beEncodeChunkFloatS16NI");
      *((void**)&bladeAPI.beDeinitStream) = (void *) GetProcAddress(dll, "beDeinitStream");
      *((void**)&bladeAPI.beVersion) = (void *) GetProcAddress(dll, "beVersion");
      *((void**)&bladeAPI.beWriteInfoTag) = (void*) GetProcAddress(dll, "beWriteInfoTag");
      
      
      if (bladeAPI.beInitStream && 
          bladeAPI.beCloseStream &&
          bladeAPI.beEncodeChunkFloatS16NI &&
          bladeAPI.beDeinitStream && 
          bladeAPI.beVersion)
      {
        LAME_DEBUG_LOADING("got blade mode");
        errcnt2 = 0;
      }
      else
      {
        memset(&bladeAPI, 0, sizeof(bladeAPI));
      }
    #endif
    if (errcnt2)
    {
#ifdef _WIN32
      FreeLibrary(dll);
#else
      dlclose(dll);
#endif
      return false;
    }
  }

  LAME_DEBUG_LOADING("loaded normal mode");

  lstrcpyn_safe(s_last_dll_file, name, sizeof(s_last_dll_file));
#ifdef _WIN32
//  if (!strstr(name,"\\"))
  GetModuleFileName(dll,s_last_dll_file, (DWORD)sizeof(s_last_dll_file));
#else
//  if (!strstr(name,"/"))
  {
    Dl_info inf={0,};
    dladdr((void*)lame.init,&inf);
    if (inf.dli_fname)
      lstrcpyn_safe(s_last_dll_file,inf.dli_fname,sizeof(s_last_dll_file));
  }
#endif

  return true;
}


#ifdef _WIN32
static bool tryLoadDLL(const char *extrapath, const char *dllname)
{
  char me[1024];

  if (extrapath)
  {
    lstrcpyn_safe(me,extrapath,sizeof(me)-64);
    lstrcatn(me,"\\",sizeof(me));
    lstrcatn(me,dllname,sizeof(me));
    if (tryLoadDLL2(me)) return true;
  }

  GetModuleFileName(NULL,me,sizeof(me)-64);

  char *p=me;
  while (*p) p++;
  while(p>=me && *p!='\\') p--;

  strcpy(++p,dllname);

  if (tryLoadDLL2(me) || tryLoadDLL2(dllname)) return true;
  
  return false;
}
#elif defined(__APPLE__)
static bool tryLoadDLL(const char *extrapath)
{
  char me[1024];

  if (extrapath)
  {
    lstrcpyn_safe(me,extrapath,sizeof(me)-64);
    lstrcatn(me,"/libmp3lame.dylib",sizeof(me));
    if (tryLoadDLL2(me)) return true;
  }

  if (tryLoadDLL2("/Library/Application Support/libmp3lame.dylib") ||
      tryLoadDLL2("/usr/local/lib/libmp3lame.dylib") ||
      tryLoadDLL2("/usr/lib/libmp3lame.dylib") ||
      tryLoadDLL2("libmp3lame.dylib")) return true;

  bool ok=false;
  CFBundleRef bund=CFBundleGetMainBundle();
  if (bund) 
  {
    CFURLRef url=CFBundleCopyBundleURL(bund);
    if (url)
    {
      char buf[8192];
      if (CFURLGetFileSystemRepresentation(url,true,(UInt8*)buf,sizeof(buf)-128))
      {
        char *p=buf;
        while (*p) p++;
        while (p>=buf && *p != '/') p--;
        if (p>=buf)
        {
          strcat(buf,"/Contents/Plugins/libmp3lame.dylib");
          ok = tryLoadDLL2(buf);

          if (!ok)
          {
            strcpy(p,"/libmp3lame.dylib");
            ok = tryLoadDLL2(buf);
          }
          if (!ok)
          {
            strcpy(p,"/Plugins/libmp3lame.dylib");
            ok = tryLoadDLL2(buf);
          }
        }          
      }
      CFRelease(url);
    }
  }
  return ok;
}
#else // generic posix
static bool tryLoadDLL(const char *extrapath)
{
  if (extrapath)
  {
    char me[1024];
    lstrcpyn_safe(me,extrapath,sizeof(me)-64);
    lstrcatn(me,"/libmp3lame.so",sizeof(me));
    if (tryLoadDLL2(me)) return true;
  }
  if (tryLoadDLL2("/usr/local/lib/libmp3lame.so")||
      tryLoadDLL2("/usr/lib/libmp3lame.so")||
      tryLoadDLL2("libmp3lame.so")||
      tryLoadDLL2("/usr/local/lib/libmp3lame.so.0")||
      tryLoadDLL2("/usr/lib/libmp3lame.so.0")||
      tryLoadDLL2("libmp3lame.so.0")) return true;

  char tmp[64];
  sprintf(tmp,"/proc/%d/exe",getpid());
  char buf[1024];
  buf[0]=0;
  if (readlink(tmp,buf,sizeof(buf)-128)>0 && buf[0])
  {
    char *p = buf;
    while (*p) p++;
    while (p>buf && *p != '/') p--;
    if (p>buf)
    {
      strcpy(p,"/Plugins/libmp3lame.so");
      if (tryLoadDLL2(buf)) return true;

      strcpy(p,"/libmp3lame.so");
      if (tryLoadDLL2(buf)) return true;
    }
  }
  return false;
}
#endif


void LameEncoder::InitDLL(const char *extrapath, bool forceRetry)
{
  static int a;
  if (a<0) return;

  if (forceRetry) a=0;
  else if (a > 30) return; // give up

  a++;
  
  #ifdef _WIN32
    #ifdef _WIN64
      if (tryLoadDLL(extrapath, "libmp3lame64.dll")||
          tryLoadDLL(extrapath, "libmp3lame.dll")||
          tryLoadDLL(extrapath, "lame_enc64.dll")||
          tryLoadDLL(extrapath, "lame_enc.dll")) a = -1;
    #else
      if (tryLoadDLL(extrapath, "libmp3lame.dll") ||
          tryLoadDLL(extrapath, "lame_enc.dll")) a = -1;
    #endif
  #else
    if (tryLoadDLL(extrapath)) a=-1;
  #endif     
}

const char *LameEncoder::GetLibName() { return s_last_dll_file; }

const char *LameEncoder::GetInfo()
{
  static char buf[128];
  int ver = CheckDLL();
  if (!ver) return NULL;
#ifdef SUPPORT_BLADE_MODE
  if (ver == 2)
  {
    BE_VERSION ver={0,};
    if (bladeAPI.beVersion) bladeAPI.beVersion(&ver);
    snprintf(buf, sizeof(buf), "LAME/BladeAPI %d.%d", ver.byMajorVersion, ver.byMinorVersion);
    return buf;
  }
#endif
  const char *p = lame.get_lame_version ? lame.get_lame_version() : NULL;
  if (p && *p)
  {
    snprintf(buf, sizeof(buf), "LAME %s", p);
    return buf;
  }
  return "LAME ?.??";
}

int LameEncoder::CheckDLL() // returns 1 for lame API, 2 for Blade, 0 for none
{
  InitDLL();
  if (!lame.close||
      !lame.init||
      !lame.set_in_samplerate||
      !lame.set_num_channels||
      !lame.set_out_samplerate||
      !lame.set_quality||
      !lame.set_mode||
      !lame.set_brate||
      !lame.init_params||
      !lame.get_framesize||
      !lame.encode_buffer_float||
      !lame.encode_flush) 
  {
    #ifdef SUPPORT_BLADE_MODE
      if (bladeAPI.beInitStream&&
          bladeAPI.beCloseStream&&
          bladeAPI.beEncodeChunkFloatS16NI&&
          bladeAPI.beDeinitStream&&
          bladeAPI.beVersion) return 2;
    #endif
    return 0;
  }

  return 1;

}

LameEncoder::LameEncoder(int srate, int nch, int bitrate, int stereomode, int quality, int vbrmethod, int vbrquality, int vbrmax, int abr)
{
  m_lamestate=0;
  m_encmode = CheckDLL();

  if (!m_encmode)
  {
    errorstat=1;
    return;
  }

  errorstat=0;
  m_nch=nch;
  m_encoder_nch = stereomode == 3 ? 1 : m_nch;

#ifdef SUPPORT_BLADE_MODE
  if (m_encmode==2)
  {
    BE_CONFIG beConfig;
    memset(&beConfig,0,sizeof(beConfig));
	  beConfig.dwConfig = BE_CONFIG_LAME;

	  beConfig.format.LHV1.dwStructVersion	= 1;
	  beConfig.format.LHV1.dwStructSize		= sizeof(beConfig);		
	  beConfig.format.LHV1.dwSampleRate		= srate;
    if (m_encoder_nch == 1) beConfig.format.LHV1.nMode = BE_MP3_MODE_MONO;
    else beConfig.format.LHV1.nMode = stereomode; //bitrate >= 192 ? BE_MP3_MODE_STEREO : BE_MP3_MODE_JSTEREO;

    beConfig.format.LHV1.dwBitrate=bitrate;
    beConfig.format.LHV1.dwMaxBitrate=(vbrmethod!=-1?vbrmax:bitrate);
    beConfig.format.LHV1.dwReSampleRate	= srate;

    // if mpeg 1, and bitrate is less than 32kbps per channel, switch to mpeg 2
    if (beConfig.format.LHV1.dwReSampleRate >= 32000 && beConfig.format.LHV1.dwMaxBitrate/m_encoder_nch <= 32)
    {
      beConfig.format.LHV1.dwReSampleRate/=2;
    }
    beConfig.format.LHV1.dwMpegVersion = (beConfig.format.LHV1.dwReSampleRate < 32000 ? MPEG2 : MPEG1);

    beConfig.format.LHV1.nPreset=quality;

    beConfig.format.LHV1.bNoRes	= 0;//1;

    beConfig.format.LHV1.bWriteVBRHeader= 1;
    beConfig.format.LHV1.bEnableVBR= vbrmethod!=-1;
    beConfig.format.LHV1.nVBRQuality= vbrquality;
    beConfig.format.LHV1.dwVbrAbr_bps = (vbrmethod!=-1?1000*abr:0);
    beConfig.format.LHV1.nVbrMethod=(VBRMETHOD)vbrmethod;


  /* LAME sets unwise bit if:
      if (gfp->short_blocks == short_block_forced || gfp->short_blocks == short_block_dispensed || ((gfp->lowpassfreq == -1) && (gfp->highpassfreq == -1)) || // "-k"
          (gfp->scale_left < gfp->scale_right) ||
          (gfp->scale_left > gfp->scale_right) ||
          (gfp->disable_reservoir && gfp->brate < 320) ||
          gfp->noATH || gfp->ATHonly || (nAthType == 0) || gfp->in_samplerate <= 32000)
  */

    int out_size_bytes=0;

    if (bladeAPI.beInitStream(&beConfig, (DWORD*)&in_size_samples, (DWORD*)&out_size_bytes, (PHBE_STREAM) &m_lamestate) != BE_ERR_SUCCESSFUL)
    {
      errorstat=2;
      return;
    }
    if (out_size_bytes < 4096) out_size_bytes = 4096;
    in_size_samples/=m_encoder_nch;
    outtmp.Resize(out_size_bytes);
    return;
  }
#endif

  m_lamestate=lame.init();
  if (!m_lamestate)
  {
    errorstat=1; 
    return;
  }

  lame.set_in_samplerate(m_lamestate, srate);
  lame.set_num_channels(m_lamestate,m_encoder_nch);
  int outrate=srate;

  int maxbr = (vbrmethod != -1 ? vbrmax : bitrate);
  if (outrate>=32000 && maxbr <= 32*m_encoder_nch) outrate/=2;

  lame.set_out_samplerate(m_lamestate,outrate);
  lame.set_quality(m_lamestate,(quality>9 ||quality<0) ? 0 : quality);
  lame.set_mode(m_lamestate,(MPEG_mode) (m_encoder_nch==1?3 :stereomode ));
  lame.set_brate(m_lamestate,bitrate);
  
  //int vbrmethod (-1 no vbr), int vbrquality (nVBRQuality), int vbrmax, int abr
  if (vbrmethod != -1 && lame.set_VBR)
  {
    int vm=4; // mtrh
    if (vbrmethod == 4) vm = 3; //ABR
    lame.set_VBR(m_lamestate,vm);
    
    if (lame.set_VBR_q) lame.set_VBR_q(m_lamestate,vbrquality);
    
    if (vbrmethod == 4&&lame.set_VBR_mean_bitrate_kbps)
    {
      lame.set_VBR_mean_bitrate_kbps(m_lamestate,abr);
    }
    if (lame.set_VBR_max_bitrate_kbps)
    {
      lame.set_VBR_max_bitrate_kbps(m_lamestate,vbrmax);
    }
    if (lame.set_VBR_min_bitrate_kbps)
    {
      lame.set_VBR_min_bitrate_kbps(m_lamestate,bitrate);
    }
  }
  lame.init_params(m_lamestate);
  in_size_samples=lame.get_framesize(m_lamestate);

  outtmp.Resize(65536);
}

void LameEncoder::Encode(float *in, int in_spls, int spacing)
{
  if (errorstat) return;

  if (in_spls > 0)
  {
    if (m_nch > 1 && m_encoder_nch==1)
    {
      // downmix
      int x;
      int pos=0;
      int adv=2*spacing;
      for (x = 0; x < in_spls; x ++)
      {
        float f=in[pos]+in[pos+1];
        f*=16383.5f;
        spltmp[0].Add(&f,sizeof(float));
        pos+=adv;
      }
    }
    else if (m_encoder_nch > 1) // deinterleave
    {
      int x;
      int pos=0;
      int adv=2*spacing;
      for (x = 0; x < in_spls; x ++)
      {
        float f=in[pos];
        f*=32767.0f;
        spltmp[0].Add(&f,sizeof(float));

        f=in[pos+1];
        f*=32767.0f;
        spltmp[1].Add(&f,sizeof(float));

        pos+=adv;
      }
    }
    else 
    {
      int x;
      int pos=0;
      for (x = 0; x < in_spls; x ++)
      {
        float f=in[pos];
        f*=32767.0f;
        spltmp[0].Add(&f,sizeof(float));

        pos+=spacing;
      }
    }
  }
  for (;;)
  {
    int a = spltmp[0].Available()/sizeof(float);
    if (a >= in_size_samples) a = in_size_samples;
    else if (a<1 || in_spls>0) break; // not enough samples available, and not flushing

    int dwo;
#ifdef SUPPORT_BLADE_MODE
    if (m_encmode==2)
    {
      DWORD outa=0;
      if (bladeAPI.beEncodeChunkFloatS16NI(m_lamestate, a, (float*)spltmp[0].Get(), (float*)spltmp[m_encoder_nch > 1].Get(), 
                          (unsigned char*)outtmp.Get(), &outa) != BE_ERR_SUCCESSFUL)
      {
        errorstat=3;
        return;
      }
      dwo = (int)outa;
    }
    else
#endif
    {
      dwo=lame.encode_buffer_float(m_lamestate,(float *)spltmp[0].Get(),(float*)spltmp[m_encoder_nch>1].Get(), a,(unsigned char *)outtmp.Get(),outtmp.GetSize());
    }
    outqueue.Add(outtmp.Get(),dwo);
    spltmp[0].Advance(a*sizeof(float));
    if (m_encoder_nch > 1) spltmp[1].Advance(a*sizeof(float));
  }

  if (in_spls<1)
  {
#ifdef SUPPORT_BLADE_MODE
    if (m_encmode==2)
    {
      DWORD dwo=0;
      if (bladeAPI.beDeinitStream(m_lamestate, (unsigned char *)outtmp.Get(), &dwo) == BE_ERR_SUCCESSFUL && dwo>0)
        outqueue.Add(outtmp.Get(),dwo);
    }
    else
#endif
    {
      int a=lame.encode_flush(m_lamestate,(unsigned char *)outtmp.Get(),outtmp.GetSize());
      if (a>0) outqueue.Add(outtmp.Get(),a);
    }
  }



  spltmp[0].Compact();
  spltmp[1].Compact();

}

#ifdef _WIN32

static BOOL HasUTF8(const char *_str)
{
  const unsigned char *str = (const unsigned char *)_str;
  if (!str) return FALSE;
  while (*str) 
  {
    unsigned char c = *str;
    if (c >= 0xC2) // discard overlongs
    {
      if (c <= 0xDF && str[1] >=0x80 && str[1] <= 0xBF) return TRUE;
      else if (c <= 0xEF && str[1] >=0x80 && str[1] <= 0xBF && str[2] >=0x80 && str[2] <= 0xBF) return TRUE;
      else if (c <= 0xF4 && str[1] >=0x80 && str[1] <= 0xBF && str[2] >=0x80 && str[2] <= 0xBF) return TRUE;
    }
    str++;
  }
  return FALSE;
}
#endif

LameEncoder::~LameEncoder()
{
#ifdef SUPPORT_BLADE_MODE
  if (m_encmode==2 && m_lamestate)
  {
    if (m_vbrfile.Get()[0] && bladeAPI.beWriteInfoTag)
    {
      // support UTF-8 filenames
      if (HasUTF8(m_vbrfile.Get()) && GetVersion()<0x80000000)
      {
        WCHAR wf[2048];
        if (MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,m_vbrfile.Get(),-1,wf,2048))
        {
          FILE *fp = _wfopen(wf,L"r+b");
          if (fp)
          {
            char tmpspace[1024],tmpfn[2048];
            GetTempPath(sizeof(tmpspace),tmpspace);
            GetTempFileName(tmpspace,"lameenc",0,tmpfn);
            FILE *tmpfp = fopen(tmpfn,"wb");
            if (tmpfp)
            {
              fseek(fp,0,SEEK_SET);
              int x=32; // 32kb
              while(x--)
              {
                int a =fread(tmpspace,1,sizeof(tmpspace),fp);
                if (a<1) break;
                fwrite(tmpspace,1,a,tmpfp);
              }

              fclose(tmpfp);

              bladeAPI.beWriteInfoTag(m_lamestate,tmpfn);
              m_lamestate=0;

              tmpfp = fopen(tmpfn,"r+b");
              if (tmpfp)
              {
                fseek(tmpfp,0,SEEK_SET);
                fseek(fp,0,SEEK_SET);

                x=32; // 32kb
                while(x--)
                {
                  int a = fread(tmpspace,1,sizeof(tmpspace),tmpfp);
                  if (a<1) break;
                  fwrite(tmpspace,1,a,fp);
                }

                fclose(tmpfp);
              }

              DeleteFile(tmpfn);
            }
            fclose(fp);

          }
        }
      }


      if (m_lamestate) bladeAPI.beWriteInfoTag(m_lamestate,m_vbrfile.Get());
    }
    else bladeAPI.beCloseStream(m_lamestate);
    m_lamestate=0;
    return;
  }
#endif

  if (m_lamestate)
  {
    if (m_vbrfile.Get()[0] && lame.get_lametag_frame)
    {
      unsigned char buf[16384];
      size_t a=lame.get_lametag_frame(m_lamestate,buf,sizeof(buf));
      if (a>0 && a<=sizeof(buf))
      {
        FILE *fp=NULL;
#ifdef _WIN32
        if (HasUTF8(m_vbrfile.Get()) && GetVersion()<0x80000000)
        {
          WCHAR wf[2048];
          if (MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,m_vbrfile.Get(),-1,wf,2048))
          {
            fp = _wfopen(wf,L"r+b");
          }
        }
#endif
        if (!fp) fp = fopen(m_vbrfile.Get(),"r+b");
        if (fp)
        {
          fseek(fp,0,SEEK_SET);
          fwrite(buf,1,a,fp);
          fclose(fp);
        }
      }
    }
    lame.close(m_lamestate);
    m_lamestate=0;
  }
}

