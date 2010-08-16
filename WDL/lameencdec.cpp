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
#include <stdio.h>

#pragma pack(push)
#pragma pack(1)

#ifdef	__cplusplus
extern "C" {
#endif

/* encoding formats */

#define		BE_CONFIG_MP3			0										
#define		BE_CONFIG_LAME			256		

/* type definitions */

typedef		unsigned long			HBE_STREAM;
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

#ifndef _BLADEDLL

typedef BE_ERR	(*BEINITSTREAM)			(PBE_CONFIG, PDWORD, PDWORD, PHBE_STREAM);
typedef BE_ERR	(*BEENCODECHUNK)		(HBE_STREAM, DWORD, PSHORT, PBYTE, PDWORD);
// added for floating point audio  -- DSPguru, jd
typedef BE_ERR	(*BEENCODECHUNKFLOATS16NI)	(HBE_STREAM, DWORD, PFLOAT, PFLOAT, PBYTE, PDWORD);
typedef BE_ERR	(*BEDEINITSTREAM)		(HBE_STREAM, PBYTE, PDWORD);
typedef BE_ERR	(*BECLOSESTREAM)		(HBE_STREAM);
typedef VOID	(*BEVERSION)			(PBE_VERSION);
typedef VOID	(*BEWRITEVBRHEADER)		(LPCSTR);

#define	TEXT_BEINITSTREAM		"beInitStream"
#define	TEXT_BEENCODECHUNK		"beEncodeChunk"
#define	TEXT_BEENCODECHUNKFLOATS16NI	"beEncodeChunkFloatS16NI"
#define	TEXT_BEDEINITSTREAM		"beDeinitStream"
#define	TEXT_BECLOSESTREAM		"beCloseStream"
#define	TEXT_BEVERSION			"beVersion"
#define	TEXT_BEWRITEVBRHEADER	"beWriteVBRHeader"



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



#else

__declspec(dllexport) BE_ERR	beInitStream(PBE_CONFIG pbeConfig, PDWORD dwSamples, PDWORD dwBufferSize, PHBE_STREAM phbeStream);
__declspec(dllexport) BE_ERR	beEncodeChunk(HBE_STREAM hbeStream, DWORD nSamples, PSHORT pSamples, PBYTE pOutput, PDWORD pdwOutput);
// added for floating point audio  -- DSPguru, jd
__declspec(dllexport) BE_ERR	beEncodeChunkFloatS16NI(HBE_STREAM hbeStream, DWORD nSamples, PFLOAT buffer_l, PFLOAT buffer_r, PBYTE pOutput, PDWORD pdwOutput);
__declspec(dllexport) BE_ERR	beDeinitStream(HBE_STREAM hbeStream, PBYTE pOutput, PDWORD pdwOutput);
__declspec(dllexport) BE_ERR	beCloseStream(HBE_STREAM hbeStream);
__declspec(dllexport) VOID		beVersion(PBE_VERSION pbeVersion);
__declspec(dllexport) BE_ERR	beWriteVBRHeader(LPCSTR lpszFileName);

#endif

#pragma pack(pop)

#ifdef	__cplusplus
}
#endif
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#endif // end _WIN32



#include "lameencdec.h"


//#define LATENCY_GAP_HACK



#ifdef _WIN32

static HINSTANCE hlamedll;
static BEINITSTREAM     beInitStream;
static BECLOSESTREAM    beCloseStream;
static BEENCODECHUNKFLOATS16NI    beEncodeChunkFloatS16NI;
static BEDEINITSTREAM   beDeinitStream;
static BEWRITEVBRHEADER beWriteVBRHeader;
static BEVERSION        beVersion;
static void *(*InitMP3_Create)();
static void (*ExitMP3_Delete)(void *);
static int (*decodeMP3_unclipped)(void *,unsigned char *inmemory,int inmemsize,char *outmemory,int outmemsize,int *done);
static void (*get_decode_info)(void *,int *nch, int *srate);//JF> added for querying the decode stats
static void (*remove_buf)(void *);


#endif

#ifdef __APPLE__
#include <Carbon/Carbon.h>
#include <dlfcn.h>
#define DYNAMIC_LAME
#endif

#ifdef DYNAMIC_LAME

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
static int (*lame_encode_buffer_float)(lame_t,
        const float     buffer_l [],       /* PCM data for left channel     */
        const float     buffer_r [],       /* PCM data for right channel    */
        const int           nsamples,      /* number of samples per channel */
        unsigned char*      mp3buf,        /* pointer to encoded MP3 stream */
        const int           mp3buf_size ); 
static int (*lame_encode_flush)(lame_t,unsigned char*       mp3buf,  int                  size);

#endif


static void initdll()
{
#ifdef _WIN32
  if (!hlamedll) 
  {
    char me[1024];
    GetModuleFileName(NULL,me,sizeof(me)-1);

    char *p=me;
    while (*p) p++;
    while(p>=me && *p!='\\') p--;

    strcpy(++p,"lame_enc.dll");

    hlamedll=LoadLibrary(me);
    if (!hlamedll) hlamedll=LoadLibrary("lame_enc.dll");//try cur dir
    if (hlamedll)
    {
		  *((void**)&beInitStream) = (void *) GetProcAddress(hlamedll, "beInitStream");
		  *((void**)&beCloseStream) = (void *) GetProcAddress(hlamedll, "beCloseStream");
		  *((void**)&beEncodeChunkFloatS16NI) = (void *) GetProcAddress(hlamedll, "beEncodeChunkFloatS16NI");
		  *((void**)&beDeinitStream)	= (void *) GetProcAddress(hlamedll, "beDeinitStream");
		  *((void**)&beVersion) = (void *) GetProcAddress(hlamedll, "beVersion");
      *((void**)&beWriteVBRHeader) = (void*) GetProcAddress(hlamedll, "beWriteVBRHeader");
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
  if (!dll&&!a)
  {
    a=1;
    if (!dll) dll=dlopen("libmp3lame.dylib",RTLD_LAZY);
    if (!dll) dll=dlopen("/usr/local/lib/libmp3lame.dylib",RTLD_LAZY);
    if (!dll) dll=dlopen("/usr/lib/libmp3lame.dylib",RTLD_LAZY);
#ifdef __APPLE__
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
              *p=0;
              strcpy(p,"/libmp3lame.dylib");
              if (!dll) dll=dlopen(buf,RTLD_LAZY);
              strcpy(p,"/Plugins/libmp3lame.dylib");
              if (!dll) dll=dlopen(buf,RTLD_LAZY);
            }          
          }
          CFRelease(url);
        }
      }
#endif
    }
     
    if (dll)
    {
    #define TURD(x) *(void **)&x = dlsym(dll,#x);
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
    }
  }
#endif
}

bool LameEncoder::CheckDLL() // returns true if dll is present
{
  initdll();
#ifdef _WIN32
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
  initdll();
#ifdef _WIN32
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



#ifdef _WIN32

  BE_CONFIG beConfig;
  memset(&beConfig,0,sizeof(beConfig));
	beConfig.dwConfig = BE_CONFIG_LAME;

	beConfig.format.LHV1.dwStructVersion	= 1;
	beConfig.format.LHV1.dwStructSize		= sizeof(beConfig);		
	beConfig.format.LHV1.dwSampleRate		= srate;
  if (nch == 1) beConfig.format.LHV1.nMode = BE_MP3_MODE_MONO;
  else beConfig.format.LHV1.nMode = stereomode; //bitrate >= 192 ? BE_MP3_MODE_STEREO : BE_MP3_MODE_JSTEREO;

  beConfig.format.LHV1.dwBitrate=bitrate;
  beConfig.format.LHV1.dwMaxBitrate=(vbrmethod!=-1?vbrmax:bitrate);
  beConfig.format.LHV1.dwReSampleRate	= srate;

  // if mpeg 1, and bitrate is less than 32kbps per channel, switch to mpeg 2
  if (beConfig.format.LHV1.dwReSampleRate >= 32000 && beConfig.format.LHV1.dwMaxBitrate/nch <= 32)
    beConfig.format.LHV1.dwReSampleRate/=2;

  beConfig.format.LHV1.dwMpegVersion		= beConfig.format.LHV1.dwReSampleRate < 32000 ? MPEG2 : MPEG1;

  beConfig.format.LHV1.nPreset=quality;

  beConfig.format.LHV1.bNoRes	= 1;

  beConfig.format.LHV1.bWriteVBRHeader= 1;
  beConfig.format.LHV1.bEnableVBR= vbrmethod!=-1;
  beConfig.format.LHV1.nVBRQuality= vbrquality;
  beConfig.format.LHV1.dwVbrAbr_bps = (vbrmethod!=-1?1000*abr:0);
  beConfig.format.LHV1.nVbrMethod=(VBRMETHOD)vbrmethod;

  int out_size_bytes=0;
  if (beInitStream(&beConfig, (DWORD*)&in_size_samples, (DWORD*)&out_size_bytes, (PHBE_STREAM) &hbeStream) != BE_ERR_SUCCESSFUL)
  {
    errorstat=2;
    return;
  }
  in_size_samples/=m_nch;
#else
  lame_set_in_samplerate(m_lamestate, srate);
  lame_set_num_channels(m_lamestate,nch);
  int outrate=srate;
  if (outrate>=32000 && bitrate <= 32*nch) outrate/=2;
  lame_set_out_samplerate(m_lamestate,outrate);
  lame_set_quality(m_lamestate,(quality>9 ||quality<0) ? 0 : quality);
  lame_set_mode(m_lamestate,(MPEG_mode) (nch==1?3 :stereomode ));
  lame_set_brate(m_lamestate,bitrate);
  // todo: vbr modes etc
  // lame_set_VBR etc
  lame_init_params(m_lamestate);
  in_size_samples=lame_get_framesize(m_lamestate);
  int out_size_bytes=65536;
#endif

  outtmp.Resize(out_size_bytes);

#ifdef _WIN32

#ifdef LATENCY_GAP_HACK
    {
      WDL_HeapBuf w;
      w.Resize(in_size_samples*sizeof(float));
      memset(w.Get(),0,in_size_samples*sizeof(float));
      DWORD dwo=0;
      if (beEncodeChunkFloatS16NI(hbeStream, in_size_samples, (float*)w.Get(), (float*)w.Get(), 
                          (unsigned char*)outtmp.Get(), &dwo) == BE_ERR_SUCCESSFUL && dwo > 0)
      {
        outqueue.Add(outtmp.Get(),dwo);
      }
    }
#endif
#endif

}

void LameEncoder::Encode(float *in, int in_spls, int spacing)
{

  if (errorstat) return;

  if (in_spls > 0)
  {
    if (m_nch > 1) // deinterleave
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
    while (spltmp[0].Available() >= (int) (in_size_samples*sizeof(float)))
    {
#ifdef _WIN32
      DWORD dwo=0;
      if (beEncodeChunkFloatS16NI(hbeStream, in_size_samples, (float*)spltmp[0].Get(), (float*)spltmp[m_nch > 1].Get(), 
                          (unsigned char*)outtmp.Get(), &dwo) != BE_ERR_SUCCESSFUL)
      {
        errorstat=3;
        return;
      }
//      printf("encoded to %d bytes (%d) %d\n",dwo, outtmp.GetSize(),in_size_samples);
#else
      int dwo=lame_encode_buffer_float(m_lamestate,(float *)spltmp[0].Get(),(float*)spltmp[m_nch>1].Get(),
           in_size_samples,(unsigned char *)outtmp.Get(),outtmp.GetSize());
      //printf("encoded %d to %d\n",in_size_samples,dwo);
#endif
      outqueue.Add(outtmp.Get(),dwo);
      spltmp[0].Advance(in_size_samples*sizeof(float));
      if (m_nch > 1) spltmp[1].Advance(in_size_samples*sizeof(float));
    }

  }
  else if (1)
  {
    if (spltmp[0].Available() && spltmp[0].Available() < (int) (in_size_samples*sizeof(float)))
    {
      float buf[1152]={0,};
      int l=in_size_samples*sizeof(float)-spltmp[0].Available();
      spltmp[0].Add(buf,l);
      if (m_nch>1) spltmp[1].Add(buf,l);

#ifdef _WIN32
      DWORD dwo=0;
      if (beEncodeChunkFloatS16NI(hbeStream, in_size_samples, (float*)spltmp[0].Get(), (float*)spltmp[m_nch > 1].Get(), 
                          (unsigned char*)outtmp.Get(), &dwo) != BE_ERR_SUCCESSFUL)
      {
        //printf("error calling encode\n");
        //errorstat=3;
        return;
      }

      outqueue.Add(outtmp.Get(),dwo);
#else
      int dwo=lame_encode_buffer_float(m_lamestate,(float *)spltmp[0].Get(),(float*)spltmp[m_nch>1].Get(),
           in_size_samples,(unsigned char *)outtmp.Get(),outtmp.GetSize());
      if (dwo>0) outqueue.Add(outtmp.Get(),dwo);

      unsigned char tmp[8192];
      int a=lame_encode_flush(m_lamestate,tmp,sizeof(tmp));
      if (a>0) outqueue.Add(tmp,a);
#endif

      spltmp[0].Advance(in_size_samples*sizeof(float));
      if (m_nch > 1) spltmp[1].Advance(in_size_samples*sizeof(float));
    }
    // encode a spare blank frame to divide
#ifdef LATENCY_GAP_HACK
    {
      WDL_HeapBuf w;
      w.Resize(in_size_samples*sizeof(float));
      memset(w.Get(),0,in_size_samples*sizeof(float));
      int x;
      for(x=0;x<2;x++) // 2 spare frames!!
      {
      DWORD dwo=0;
      if (beEncodeChunkFloatS16NI(hbeStream, in_size_samples, (float*)w.Get(), (float*)w.Get(), 
                          (unsigned char*)outtmp.Get(), &dwo) == BE_ERR_SUCCESSFUL && dwo > 0)
      {
        outqueue.Add(outtmp.Get(),dwo);
      }
      }
    }
#endif
  }



  spltmp[0].Compact();
  spltmp[1].Compact();

}

LameEncoder::~LameEncoder()
{
#ifdef _WIN32
  if (hbeStream) beCloseStream(hbeStream);
  if (m_vbrfile.Get()[0] && beWriteVBRHeader)
  {
    beWriteVBRHeader(m_vbrfile.Get());
  }
#else
  if (m_lamestate)
  {
    lame_close(m_lamestate);
    m_lamestate=0;
  }
#endif
}

#ifdef _WIN32 // todo: lame decoding on nonwin32

LameDecoder::LameDecoder()
{
  m_samples_remove=1152*2;
  m_samplesdec=0;
  decinst=0;
  errorstat=0;
  m_samples_used=0;
  m_srate=m_nch=0;
  initdll();
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

#ifdef LATENCY_GAP_HACK
      if (m_samples_remove>0)
      {
        int rem=min(m_samples_remove,bout_pairs);
        bout -= rem*m_nch;
        bufptr+=rem*m_nch;
        bout_pairs -= rem;
        m_samples_remove -= rem;
      }
#endif

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
