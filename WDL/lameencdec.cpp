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


#ifdef USE_LAME_BLADE_API

#pragma pack(push)
#pragma pack(1)

#ifdef	__cplusplus
extern "C" {
#endif

/* encoding formats */

#define		BE_CONFIG_MP3			0										
#define		BE_CONFIG_LAME			256		

/* type definitions */

typedef		UINT_PTR			HBE_STREAM;
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

#ifdef _BLADEDLL
#undef FLOAT
	#include <Windows.h>
#endif

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

typedef BE_ERR	(*BEINITSTREAM)			(PBE_CONFIG, PDWORD, PDWORD, PHBE_STREAM);
typedef BE_ERR	(*BEENCODECHUNK)		(HBE_STREAM, DWORD, PSHORT, PBYTE, PDWORD);
// added for floating point audio  -- DSPguru, jd
typedef BE_ERR	(*BEENCODECHUNKFLOATS16NI)	(HBE_STREAM, DWORD, PFLOAT, PFLOAT, PBYTE, PDWORD);
typedef BE_ERR	(*BEDEINITSTREAM)		(HBE_STREAM, PBYTE, PDWORD);
typedef BE_ERR	(*BECLOSESTREAM)		(HBE_STREAM);
typedef VOID	(*BEVERSION)			(PBE_VERSION);



#define MP3_ERR -1
#define MP3_OK  0
#define MP3_NEED_MORE 1

// void *InitMP3_Create();
// void ExitMP3_Delete(void *);

// int decodeMP3_unclipped(void *,unsigned char *inmemory,int inmemsize,char *outmemory,int outmemsize,int *done);
//    always uses all of inmemory, returns MP3_NEED_MORE if it needs more, and done is the output size.
//    it appears outmemory is interleaved. outmemory is doubles, too. ick.
//    is 'done' bytes or samples? inmemsize should be 1152*2*sizeof(double)

// void get_decode_info(void *,int *nch, int *srate);//JF> added for querying the decode stats
// void remove_buf(void *);



#pragma pack(pop)

#ifdef	__cplusplus
}
#endif

static HINSTANCE hlamedll;
static BEINITSTREAM     beInitStream;
static BECLOSESTREAM    beCloseStream;
static BEENCODECHUNKFLOATS16NI    beEncodeChunkFloatS16NI;
static BEDEINITSTREAM   beDeinitStream;
static BE_ERR (*beWriteInfoTag)(HBE_STREAM, const char *);
static BEVERSION        beVersion;
static void *(*InitMP3_Create)();
static void (*ExitMP3_Delete)(void *);
static int (*decodeMP3_unclipped)(void *,unsigned char *inmemory,int inmemsize,char *outmemory,int outmemsize,int *done);
static void (*get_decode_info)(void *,int *nch, int *srate);//JF> added for querying the decode stats
static void (*remove_buf)(void *);



#endif // USE_LAME_BLADE_API


#ifdef DYNAMIC_LAME
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


static int (*lame_close)(lame_t);
static lame_t (*lame_init)();
static int (*lame_set_in_samplerate)(lame_t, int);
static int (*lame_set_num_channels)(lame_t,int);
static int (*lame_set_out_samplerate)(lame_t,int);
static int (*lame_set_quality)(lame_t,int);
static int (*lame_set_mode)(lame_t,MPEG_mode);
static int (*lame_set_brate)(lame_t, int);
static int (*lame_init_params)(lame_t);
static int (*lame_get_framesize)(lame_t);
static int (*lame_set_VBR)(lame_t, int);
static int (*lame_set_VBR_q)(lame_t, int);
static int (*lame_set_VBR_mean_bitrate_kbps)(lame_t, int);
static int (*lame_set_VBR_min_bitrate_kbps)(lame_t, int);
static int (*lame_set_VBR_max_bitrate_kbps)(lame_t, int);

static size_t (*lame_get_lametag_frame)(lame_t, unsigned char *, size_t);
static int (*lame_encode_buffer_float)(lame_t,
        const float     buffer_l [],       /* PCM data for left channel     */
        const float     buffer_r [],       /* PCM data for right channel    */
        const int           nsamples,      /* number of samples per channel */
        unsigned char*      mp3buf,        /* pointer to encoded MP3 stream */
        const int           mp3buf_size ); 
static int (*lame_encode_flush)(lame_t,unsigned char*       mp3buf,  int                  size);

#endif


void LameEncoder::InitDLL(const char *extrapath)
{
#ifdef USE_LAME_BLADE_API
  if (!hlamedll) 
  {
#ifdef _WIN64
    const char *dllName = "lame_enc64.dll";
#else
    const char *dllName = "lame_enc.dll";
#endif
    char me[1024];
    GetModuleFileName(NULL,me,sizeof(me)-64);

    char *p=me;
    while (*p) p++;
    while(p>=me && *p!='\\') p--;

    strcpy(++p,dllName);

    hlamedll=LoadLibrary(me);
    if (extrapath && !hlamedll)
    {
      lstrcpyn_safe(me,extrapath,sizeof(me)-64);
      lstrcatn(me,"\\",sizeof(me));
      lstrcatn(me,dllName,sizeof(me));
      hlamedll=LoadLibrary(me);
    }

    if (!hlamedll) hlamedll=LoadLibrary(dllName);

    if (hlamedll)
    {
		  *((void**)&beInitStream) = (void *) GetProcAddress(hlamedll, "beInitStream");
		  *((void**)&beCloseStream) = (void *) GetProcAddress(hlamedll, "beCloseStream");
		  *((void**)&beEncodeChunkFloatS16NI) = (void *) GetProcAddress(hlamedll, "beEncodeChunkFloatS16NI");
		  *((void**)&beDeinitStream)	= (void *) GetProcAddress(hlamedll, "beDeinitStream");
		  *((void**)&beVersion) = (void *) GetProcAddress(hlamedll, "beVersion");
      *((void**)&beWriteInfoTag) = (void*) GetProcAddress(hlamedll, "beWriteInfoTag");
      *((void**)&InitMP3_Create) = (void *) GetProcAddress(hlamedll,"InitMP3_Create");
      *((void**)&ExitMP3_Delete) = (void *) GetProcAddress(hlamedll,"ExitMP3_Delete");
      *((void**)&decodeMP3_unclipped) = (void *) GetProcAddress(hlamedll,"decodeMP3_unclipped");
      *((void**)&get_decode_info) = (void *) GetProcAddress(hlamedll,"get_decode_info");
      *((void**)&remove_buf) = (void *) GetProcAddress(hlamedll,"remove_buf");
    }

  }
#elif defined(DYNAMIC_LAME)
  static int a;
  static void *dll;
  if (!dll&& (a<100 || extrapath)) // try a bunch of times before giving up
  {
    a++;
#ifdef _WIN32
    const char *dllname = "libmp3lame.dll";
  #ifdef _WIN64
    const char *dllName2 = "lame_enc64.dll";
  #else
    const char *dllName2 = "lame_enc.dll";
  #endif
    char me[1024];

    if (extrapath)
    {
      lstrcpyn_safe(me,extrapath,sizeof(me)-64);
      lstrcatn(me,"\\",sizeof(me));
      lstrcatn(me,dllname,sizeof(me));
      dll=LoadLibrary(me);
      if (!dll)
      {
        lstrcpyn_safe(me,extrapath,sizeof(me)-64);
        lstrcatn(me,"\\",sizeof(me));
        lstrcatn(me,dllName2,sizeof(me));
        dll=LoadLibrary(me);
      }
    }

    if (!dll)
    {
      GetModuleFileName(NULL,me,sizeof(me)-64);

      char *p=me;
      while (*p) p++;
      while(p>=me && *p!='\\') p--;

      strcpy(++p,dllname);

      dll=(void*)LoadLibrary(me);
      if (!dll)
      {
        strcpy(p,dllName2);
        dll=(void*)LoadLibrary(me);
      }
    }
    if (!dll) dll=LoadLibrary(dllname);
    if (!dll) dll=LoadLibrary(dllName2);

#elif defined(__APPLE__)
    if (!dll && extrapath)
    {
      char me[1024];
      lstrcpyn_safe(me,extrapath,sizeof(me)-64);
      lstrcatn(me,"/libmp3lame.dylib",sizeof(me));
      dll = dlopen(me, RTLD_NOW|RTLD_LOCAL);    
    }
    if (!dll) dll = dlopen("/Library/Application Support/libmp3lame.dylib", RTLD_NOW|RTLD_LOCAL);    
    if (!dll) dll=dlopen("/usr/local/lib/libmp3lame.dylib",RTLD_NOW|RTLD_LOCAL);
    if (!dll) dll=dlopen("/usr/lib/libmp3lame.dylib",RTLD_NOW|RTLD_LOCAL);

    if (!dll) dll=dlopen("libmp3lame.dylib",RTLD_NOW|RTLD_LOCAL);

    if (!dll)
    {
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
              if (!dll) dll=dlopen(buf,RTLD_NOW|RTLD_LOCAL);

              if (!dll)
              {
                strcpy(p,"/libmp3lame.dylib");
                dll=dlopen(buf,RTLD_NOW|RTLD_LOCAL);
              }
              if (!dll)
              {
                strcpy(p,"/Plugins/libmp3lame.dylib");
                if (!dll) dll=dlopen(buf,RTLD_NOW|RTLD_LOCAL);
              }
            }          
          }
          CFRelease(url);
        }
      }
    }
#else // linux default to .so
    if (!dll && extrapath)
    {
      char me[1024];
      lstrcpyn_safe(me,extrapath,sizeof(me)-64);
      lstrcatn(me,"/libmp3lame.so",sizeof(me));
      dll=LoadLibrary(me);
      dll = dlopen(me, RTLD_NOW|RTLD_LOCAL);    
    }
    if (!dll) dll=dlopen("/usr/local/lib/libmp3lame.so",RTLD_NOW|RTLD_LOCAL);
    if (!dll) dll=dlopen("/usr/lib/libmp3lame.so",RTLD_NOW|RTLD_LOCAL);
    if (!dll) dll=dlopen("libmp3lame.so",RTLD_NOW|RTLD_LOCAL);

    if (!dll) dll=dlopen("/usr/local/lib/libmp3lame.so.0",RTLD_NOW|RTLD_LOCAL);
    if (!dll) dll=dlopen("/usr/lib/libmp3lame.so.0",RTLD_NOW|RTLD_LOCAL);
    if (!dll) dll=dlopen("libmp3lame.so.0",RTLD_NOW|RTLD_LOCAL);
    if (!dll) 
    {
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
           dll=dlopen(buf,RTLD_NOW|RTLD_LOCAL);
           if (!dll)  
           {
             strcpy(p,"/libmp3lame.so");
             dll=dlopen(buf,RTLD_NOW|RTLD_LOCAL);
           }
         }
       }
    }
#endif
     
    if (dll)
    {
  #ifdef _WIN32
    #define TURD(x) *(void **)&x = GetProcAddress((HINSTANCE)dll,#x);
  #else
    #define TURD(x) *(void **)&x = dlsym(dll,#x);
  #endif
    TURD(lame_get_lametag_frame)
    TURD(lame_close) 
    TURD(lame_init) 
    TURD(lame_set_in_samplerate) 
    TURD(lame_set_num_channels) 
    TURD(lame_set_out_samplerate) 
    TURD(lame_set_quality) 
    TURD(lame_set_mode)
    TURD(lame_set_brate) 
    TURD(lame_init_params)
    TURD(lame_get_framesize) 
    TURD(lame_encode_buffer_float) 
    TURD(lame_encode_flush)      
    TURD(lame_set_VBR)
    TURD(lame_set_VBR_q)
    TURD(lame_set_VBR_mean_bitrate_kbps)
    TURD(lame_set_VBR_min_bitrate_kbps)
    TURD(lame_set_VBR_max_bitrate_kbps)
  #undef TURD   
    }
  }
#endif
}

bool LameEncoder::CheckDLL() // returns true if dll is present
{
  InitDLL();
#ifdef USE_LAME_BLADE_API
  if (!beInitStream||!beCloseStream||!beEncodeChunkFloatS16NI||!beDeinitStream||!beVersion) return false;
#elif defined(DYNAMIC_LAME)
  if (!lame_close||!lame_init||!lame_set_in_samplerate||!lame_set_num_channels||!lame_set_out_samplerate||!lame_set_quality||!lame_set_mode
      ||!lame_set_brate||!lame_init_params||!lame_get_framesize||!lame_encode_buffer_float||!lame_encode_flush) return false;
#endif
  return true;
}

LameEncoder::LameEncoder(int srate, int nch, int bitrate, int stereomode, int quality, int vbrmethod, int vbrquality, int vbrmax, int abr)
{
  errorstat=0;
  InitDLL();
#ifdef USE_LAME_BLADE_API
  hbeStream=0;
  if (!CheckDLL())
  {
    errorstat=1;
    return;
  }
#else
  m_lamestate=0;
  if (!CheckDLL())
  {
    errorstat=1;
    return;
  }
  m_lamestate=lame_init();
  if (!m_lamestate)
  {
    errorstat=1; 
    return;
  }
#endif
  m_nch=nch;


#ifdef USE_LAME_BLADE_API

  m_encoder_nch = stereomode == BE_MP3_MODE_MONO ? 1 : nch;

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
  hbeStream=0;

  if (beInitStream(&beConfig, (DWORD*)&in_size_samples, (DWORD*)&out_size_bytes, (PHBE_STREAM) &hbeStream) != BE_ERR_SUCCESSFUL)
  {
    errorstat=2;
    return;
  }
  in_size_samples/=m_encoder_nch;
#else
  m_encoder_nch = stereomode == 3 ? 1 : m_nch;

  lame_set_in_samplerate(m_lamestate, srate);
  lame_set_num_channels(m_lamestate,m_encoder_nch);
  int outrate=srate;

  int maxbr = (vbrmethod != -1 ? vbrmax : bitrate);
  if (outrate>=32000 && maxbr <= 32*m_encoder_nch) outrate/=2;

  lame_set_out_samplerate(m_lamestate,outrate);
  lame_set_quality(m_lamestate,(quality>9 ||quality<0) ? 0 : quality);
  lame_set_mode(m_lamestate,(MPEG_mode) (m_encoder_nch==1?3 :stereomode ));
  lame_set_brate(m_lamestate,bitrate);
  
  //int vbrmethod (-1 no vbr), int vbrquality (nVBRQuality), int vbrmax, int abr
  if (vbrmethod != -1 && lame_set_VBR)
  {
    int vm=4; // mtrh
    if (vbrmethod == 4) vm = 3; //ABR
    lame_set_VBR(m_lamestate,vm);
    
    if (lame_set_VBR_q) lame_set_VBR_q(m_lamestate,vbrquality);
    
    if (vbrmethod == 4&&lame_set_VBR_mean_bitrate_kbps)
    {
      lame_set_VBR_mean_bitrate_kbps(m_lamestate,abr);
    }
    if (lame_set_VBR_max_bitrate_kbps)
    {
      lame_set_VBR_max_bitrate_kbps(m_lamestate,vbrmax);
    }
    if (lame_set_VBR_min_bitrate_kbps)
    {
      lame_set_VBR_min_bitrate_kbps(m_lamestate,bitrate);
    }
  }
  lame_init_params(m_lamestate);
  in_size_samples=lame_get_framesize(m_lamestate);
  int out_size_bytes=65536;
#endif

  outtmp.Resize(out_size_bytes);

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

#ifdef USE_LAME_BLADE_API
    DWORD dwo=0;
    if (beEncodeChunkFloatS16NI(hbeStream, a, (float*)spltmp[0].Get(), (float*)spltmp[m_encoder_nch > 1].Get(), 
                        (unsigned char*)outtmp.Get(), &dwo) != BE_ERR_SUCCESSFUL)
    {
      errorstat=3;
      return;
    }
//      printf("encoded to %d bytes (%d) %d\n",dwo, outtmp.GetSize(),in_size_samples);
#else
    int dwo=lame_encode_buffer_float(m_lamestate,(float *)spltmp[0].Get(),(float*)spltmp[m_encoder_nch>1].Get(),
         a,(unsigned char *)outtmp.Get(),outtmp.GetSize());
    //printf("encoded %d to %d\n",in_size_samples,dwo);
#endif
    outqueue.Add(outtmp.Get(),dwo);
    spltmp[0].Advance(a*sizeof(float));
    if (m_encoder_nch > 1) spltmp[1].Advance(a*sizeof(float));
  }

  if (in_spls<1)
  {
#ifdef USE_LAME_BLADE_API
    DWORD dwo=0;
    if (beDeinitStream(hbeStream, (unsigned char *)outtmp.Get(), &dwo) == BE_ERR_SUCCESSFUL && dwo>0)
      outqueue.Add(outtmp.Get(),dwo);

#else
    int a=lame_encode_flush(m_lamestate,(unsigned char *)outtmp.Get(),outtmp.GetSize());
    if (a>0) outqueue.Add(outtmp.Get(),a);
#endif
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
#ifdef USE_LAME_BLADE_API
  if (hbeStream) 
  {
    if (m_vbrfile.Get()[0] && beWriteInfoTag)
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

              beWriteInfoTag(hbeStream,tmpfn);
              hbeStream=0;

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


      if (hbeStream) beWriteInfoTag(hbeStream,m_vbrfile.Get());
    }
    else beCloseStream(hbeStream);
    hbeStream=0;
  }
#else
  if (m_lamestate)
  {
    if (m_vbrfile.Get()[0] && lame_get_lametag_frame)
    {
      unsigned char buf[16384];
      int a=lame_get_lametag_frame(m_lamestate,buf,sizeof(buf));
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
    lame_close(m_lamestate);
    m_lamestate=0;
  }
#endif
}

#ifdef USE_LAME_BLADE_API // todo: lame decoding on nonwin32

LameDecoder::LameDecoder()
{
  m_samples_remove=1152*2;
  m_samplesdec=0;
  decinst=0;
  errorstat=0;
  m_samples_used=0;
  m_srate=m_nch=0;
  LameEncoder::InitDLL();
  if (!InitMP3_Create||!ExitMP3_Delete||!decodeMP3_unclipped||!get_decode_info||!remove_buf)
  {
    errorstat=1;
    return;
  }

  decinst=InitMP3_Create();

}

void LameDecoder::DecodeWrote(int srclen)
{
  if (errorstat||!decinst) return;

  for (;;)
  {
    double buf[1152*2];
    int bout;
    //decodeMP3_unclipped(void *,unsigned char *inmemory,int inmemsize,char *outmemory,int outmemsize,int *done);
    int ret=decodeMP3_unclipped(decinst,(unsigned char *)srctmp.Get(),srclen,(char *)buf,sizeof(buf),&bout);
    if (ret == MP3_ERR) { errorstat=1; break; }
    if (ret == MP3_NEED_MORE) { break; }

    if (ret == MP3_OK)
    {
      if (!m_srate || !m_nch) get_decode_info(decinst,&m_nch,&m_srate);
      bout /= sizeof(double);

      int bout_pairs=bout/m_nch;
      double *bufptr=buf;


      if (bout > 0)
      {
        int x;
        int newsize=(m_samples_used+bout+4096)*sizeof(float);
        if (m_samples.GetSize() < newsize) m_samples.Resize(newsize);

        float *fbuf=(float *)m_samples.Get();
        fbuf += m_samples_used;

        m_samplesdec += bout_pairs;
        m_samples_used+=bout;

        for (x = 0; x < bout; x ++)
        {
          fbuf[x]=(float)(bufptr[x] * (1.0/32767.0));        
        }
      }
    }

    srclen=0;
  }
}

void LameDecoder::Reset()
{
  m_samples_remove=1152*2;
  m_samples_used=0; 
  m_samplesdec=0;

  //if (decinst) remove_buf(decinst);
/*
  if (decinst)
  {
    ExitMP3_Delete(decinst);
  }
  decinst=InitMP3_Create();
  */
}

LameDecoder::~LameDecoder()
{
  if (decinst)
  {
    ExitMP3_Delete(decinst);
  }
}
#endif
