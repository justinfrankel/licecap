/*
  Cockos WDL - LICE - Lightweight Image Compositing Engine
  Copyright (C) 2007 and later, Cockos Incorporated
  File: lice_ico.cpp (Windows ICO icon file loading for LICE)
  See lice.h for license and other information

  This file contains some code from Microsoft's MSDN ICO loading sample
  see: http://msdn2.microsoft.com/en-us/library/ms997538.aspx
*/

#include "lice.h"

extern LICE_IBitmap *hbmToBit(HBITMAP hbm, LICE_IBitmap *bmp);

#ifndef GRPICONDIR
// #pragmas are used here to insure that the structure's
// packing in memory matches the packing of the EXE or DLL.
#pragma pack( push )
#pragma pack( 2 )
typedef struct
{
  BYTE   bWidth;               // Width, in pixels, of the image
  BYTE   bHeight;              // Height, in pixels, of the image
  BYTE   bColorCount;          // Number of colors in image (0 if >=8bpp)
  BYTE   bReserved;            // Reserved
  WORD   wPlanes;              // Color Planes
  WORD   wBitCount;            // Bits per pixel
  DWORD   dwBytesInRes;         // how many bytes in this resource?
  WORD   nID;                  // the ID
} GRPICONDIRENTRY, *LPGRPICONDIRENTRY;
#pragma pack( pop )

#pragma pack( push )
#pragma pack( 2 )
typedef struct 
{
  WORD            idReserved;   // Reserved (must be 0)
  WORD            idType;       // Resource type (1 for icons)
  WORD            idCount;      // How many images?
  GRPICONDIRENTRY   idEntries[1]; // The entries for each image
} GRPICONDIR, *LPGRPICONDIR;
#pragma pack( pop )
#endif

#ifndef ICONRESOURCE
// These next two structs represent how the icon information is stored
// in an ICO file.
typedef struct
{
	BYTE	bWidth;               // Width of the image
	BYTE	bHeight;              // Height of the image (times 2)
	BYTE	bColorCount;          // Number of colors in image (0 if >=8bpp)
	BYTE	bReserved;            // Reserved
	WORD	wPlanes;              // Color Planes
	WORD	wBitCount;            // Bits per pixel
	DWORD	dwBytesInRes;         // how many bytes in this resource?
	DWORD	dwImageOffset;        // where in the file is this image
} ICONDIRENTRY, *LPICONDIRENTRY;
typedef struct 
{
	WORD			idReserved;   // Reserved
	WORD			idType;       // resource type (1 for icons)
	WORD			idCount;      // how many images?
	ICONDIRENTRY	idEntries[1]; // the entries for each image
} ICONDIR, *LPICONDIR;

// The following two structs are for the use of this program in
// manipulating icons. They are more closely tied to the operation
// of this program than the structures listed above. One of the
// main differences is that they provide a pointer to the DIB
// information of the masks.
typedef struct
{
	UINT			Width, Height, Colors; // Width, Height and bpp
	LPBYTE			lpBits;                // ptr to DIB bits
	DWORD			dwNumBytes;            // how many bytes?
	LPBITMAPINFO	lpbi;                  // ptr to header
	LPBYTE			lpXOR;                 // ptr to XOR image bits
	LPBYTE			lpAND;                 // ptr to AND image bits
} ICONIMAGE, *LPICONIMAGE;
typedef struct
{
	BOOL		bHasChanged;                     // Has image changed?
	TCHAR		szOriginalICOFileName[MAX_PATH]; // Original name
	TCHAR		szOriginalDLLFileName[MAX_PATH]; // Original name
	UINT		nNumImages;                      // How many images?
	ICONIMAGE	IconImages[1];                   // Image entries
} ICONRESOURCE, *LPICONRESOURCE;
#endif

static UINT ReadICOHeader( HANDLE hFile )
{
  WORD    Input;
  DWORD	dwBytesRead;
  
  // Read the 'reserved' WORD
  if( ! ReadFile( hFile, &Input, sizeof( WORD ), &dwBytesRead, NULL ) )
    return (UINT)-1;
  // Did we get a WORD?
  if( dwBytesRead != sizeof( WORD ) )
    return (UINT)-1;
  // Was it 'reserved' ?   (ie 0)
  if( Input != 0 )
    return (UINT)-1;
  // Read the type WORD
  if( ! ReadFile( hFile, &Input, sizeof( WORD ), &dwBytesRead, NULL ) )
    return (UINT)-1;
  // Did we get a WORD?
  if( dwBytesRead != sizeof( WORD ) )
    return (UINT)-1;
  // Was it type 1?
  if( Input != 1 )
    return (UINT)-1;
  // Get the count of images
  if( ! ReadFile( hFile, &Input, sizeof( WORD ), &dwBytesRead, NULL ) )
    return (UINT)-1;
  // Did we get a WORD?
  if( dwBytesRead != sizeof( WORD ) )
    return (UINT)-1;
  // Return the count
  return Input;
}

static WORD DIBNumColors( LPSTR lpbi )
{
  WORD wBitCount;
  DWORD dwClrUsed;
  
  dwClrUsed = ((LPBITMAPINFOHEADER) lpbi)->biClrUsed;
  
  if (dwClrUsed)
    return (WORD) dwClrUsed;
  
  wBitCount = ((LPBITMAPINFOHEADER) lpbi)->biBitCount;
  
  switch (wBitCount)
  {
  case 1: return 2;
  case 4: return 16;
  case 8:	return 256;
  default:return 0;
  }
  return 0;
}

static WORD PaletteSize( LPSTR lpbi )
{
  return ( DIBNumColors( lpbi ) * sizeof( RGBQUAD ) );
}

static LPSTR FindDIBBits( LPSTR lpbi )
{
  return ( lpbi + *(LPDWORD)lpbi + PaletteSize( lpbi ) );
}

static DWORD BytesPerLine( LPBITMAPINFOHEADER lpBMIH )
{
#define WIDTHBYTES(bits)      ((((bits) + 31)>>5)<<2)
  return WIDTHBYTES(lpBMIH->biWidth * lpBMIH->biPlanes * lpBMIH->biBitCount);
}

static BOOL AdjustIconImagePointers( LPICONIMAGE lpImage )
{
  // Sanity check
  if( lpImage==NULL )
    return FALSE;
  // BITMAPINFO is at beginning of bits
  lpImage->lpbi = (LPBITMAPINFO)lpImage->lpBits;
  // Width - simple enough
  lpImage->Width = lpImage->lpbi->bmiHeader.biWidth;
  // Icons are stored in funky format where height is doubled - account for it
  lpImage->Height = (lpImage->lpbi->bmiHeader.biHeight)/2;
  // How many colors?
  lpImage->Colors = lpImage->lpbi->bmiHeader.biPlanes * lpImage->lpbi->bmiHeader.biBitCount;
  // XOR bits follow the header and color table
  lpImage->lpXOR = (LPBYTE)FindDIBBits((LPSTR)lpImage->lpbi);
  // AND bits follow the XOR bits
  lpImage->lpAND = lpImage->lpXOR + (lpImage->Height*BytesPerLine((LPBITMAPINFOHEADER)(lpImage->lpbi)));
  return TRUE;
}

static LPICONRESOURCE ReadIconFromICOFile( LPCTSTR szFileName )
{
  LPICONRESOURCE    	lpIR = NULL, lpNew = NULL;
  HANDLE            	hFile = NULL;
  UINT                i;
  DWORD            	dwBytesRead;
  LPICONDIRENTRY    	lpIDE = NULL;
  
  // Open the file
  if( (hFile = CreateFile( szFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL )) == INVALID_HANDLE_VALUE )
  {
    return NULL;
  }
  // Allocate memory for the resource structure
  if( (lpIR = (ICONRESOURCE *)malloc( sizeof(ICONRESOURCE) )) == NULL )
  {
    CloseHandle( hFile );
    return NULL;
  }
  // Read in the header
  if( (lpIR->nNumImages = ReadICOHeader( hFile )) == (UINT)-1 )
  {
    CloseHandle( hFile );
    free( lpIR );
    return NULL;
  }
  // Adjust the size of the struct to account for the images
  if( (lpNew = (ICONRESOURCE *)realloc( lpIR, sizeof(ICONRESOURCE) + ((lpIR->nNumImages-1) * sizeof(ICONIMAGE)) )) == NULL )
  {
    CloseHandle( hFile );
    free( lpIR );
    return NULL;
  }
  lpIR = lpNew;
  // Store the original name
  lstrcpy( lpIR->szOriginalICOFileName, szFileName );
  lstrcpy( lpIR->szOriginalDLLFileName, "" );
  // Allocate enough memory for the icon directory entries
  if( (lpIDE = (ICONDIRENTRY *)malloc( lpIR->nNumImages * sizeof( ICONDIRENTRY ) ) ) == NULL )
  {
    CloseHandle( hFile );
    free( lpIR );
    return NULL;
  }
  // Read in the icon directory entries
  if( ! ReadFile( hFile, lpIDE, lpIR->nNumImages * sizeof( ICONDIRENTRY ), &dwBytesRead, NULL ) )
  {
    CloseHandle( hFile );
    free( lpIR );
    return NULL;
  }
  if( dwBytesRead != lpIR->nNumImages * sizeof( ICONDIRENTRY ) )
  {
    CloseHandle( hFile );
    free( lpIR );
    return NULL;
  }
  // Loop through and read in each image
  for( i = 0; i < lpIR->nNumImages; i++ )
  {
    // Allocate memory for the resource
    if( (lpIR->IconImages[i].lpBits = (LPBYTE)malloc(lpIDE[i].dwBytesInRes)) == NULL )
    {
      CloseHandle( hFile );
      free( lpIR );
      free( lpIDE );
      return NULL;
    }
    lpIR->IconImages[i].dwNumBytes = lpIDE[i].dwBytesInRes;
    // Seek to beginning of this image
    if( SetFilePointer( hFile, lpIDE[i].dwImageOffset, NULL, FILE_BEGIN ) == 0xFFFFFFFF )
    {
      CloseHandle( hFile );
      free( lpIR );
      free( lpIDE );
      return NULL;
    }
    // Read it in
    if( ! ReadFile( hFile, lpIR->IconImages[i].lpBits, lpIDE[i].dwBytesInRes, &dwBytesRead, NULL ) )
    {
      CloseHandle( hFile );
      free( lpIR );
      free( lpIDE );
      return NULL;
    }
    if( dwBytesRead != lpIDE[i].dwBytesInRes )
    {
      CloseHandle( hFile );
      free( lpIDE );
      free( lpIR );
      return NULL;
    }
    // Set the internal pointers appropriately
    if( ! AdjustIconImagePointers( &(lpIR->IconImages[i]) ) )
    {
      CloseHandle( hFile );
      free( lpIDE );
      free( lpIR );
      return NULL;
    }
  }
  // Clean up	
  free( lpIDE );
  CloseHandle( hFile );
  return lpIR;
}

static LICE_IBitmap *createBitmapFromIcon(HICON ic, LICE_IBitmap *bmp)
{
  ICONINFO icf={0,};
  GetIconInfo(ic, &icf);
  
  LICE_IBitmap *ret = NULL;
  if(icf.hbmColor)
  {
    ret = hbmToBit(icf.hbmColor,bmp);
    DeleteObject(icf.hbmColor);
  }
  if(icf.hbmMask) 
  {
    if(ret)
    {
      //import the alpha mask
      LICE_IBitmap *mask = hbmToBit(icf.hbmMask, NULL);
      if(mask)
      {
        //invert the colors
        {
          LICE_pixel *p = mask->getBits();
          int l = mask->getRowSpan()*mask->getHeight();
          while(l--)
          {
            *p=~(*p);
            p++;
          }
        }
        //merge into final bitmap
        LICE_Blit(ret, mask, 0, 0, NULL, 1.0f, LICE_BLIT_MODE_CHANCOPY|LICE_PIXEL_R|(LICE_PIXEL_A<<2));
        delete mask;
      }
    }
    DeleteObject(icf.hbmMask);
  }
  return ret;
}

static void FreeIconResource(LPICONRESOURCE ir)
{
  UINT i;
  
  // Free all the bits
  for( i=0; i< ir->nNumImages; i++ )
  {
    if( ir->IconImages[i].lpBits != NULL )
      free( ir->IconImages[i].lpBits );
  }
  free( ir );
}

LICE_IBitmap *LICE_LoadIcon(const char *filename, int iconnb, LICE_IBitmap *bmp)
{
  LPICONRESOURCE ir = ReadIconFromICOFile(filename);
  if(!ir) return 0;

  if(iconnb>=ir->nNumImages)
  {
    FreeIconResource(ir);
    return 0;
  }

  LPICONIMAGE lpIcon = &ir->IconImages[iconnb];
  HICON        	hIcon = NULL;

  // Sanity Check
  if( lpIcon == NULL )
  {
    FreeIconResource(ir);
    return NULL;
  }
  if( lpIcon->lpBits == NULL )
  {
    FreeIconResource(ir);
    return NULL;
  }

  // Let the OS do the real work :)
  hIcon = CreateIconFromResourceEx( lpIcon->lpBits, lpIcon->dwNumBytes, TRUE, 0x00030000, 
    (*(LPBITMAPINFOHEADER)(lpIcon->lpBits)).biWidth, (*(LPBITMAPINFOHEADER)(lpIcon->lpBits)).biHeight/2, 0 );
  
  // It failed, odds are good we're on NT so try the non-Ex way
  if( hIcon == NULL )
  {
    // We would break on NT if we try with a 16bpp image
    if(lpIcon->lpbi->bmiHeader.biBitCount != 16)
    {	
      hIcon = CreateIconFromResource( lpIcon->lpBits, lpIcon->dwNumBytes, TRUE, 0x00030000 );
    }
  }

  LICE_IBitmap *ret = createBitmapFromIcon(hIcon, bmp);

  DeleteObject(hIcon);
  FreeIconResource(ir);
  return ret;
}

LICE_IBitmap *LICE_LoadIconFromResource(HINSTANCE hInst, int resid, int iconnb, LICE_IBitmap *bmp)
{
  HRSRC hRsrc = FindResource( hInst, MAKEINTRESOURCE( resid ), RT_GROUP_ICON );
  if(!hRsrc) return 0;

  HGLOBAL hGlobal = LoadResource( hInst, hRsrc );
  LPGRPICONDIR lpGrpIconDir = (LPGRPICONDIR)LockResource( hGlobal );
 
  if(iconnb>=lpGrpIconDir->idCount)
  {
    FreeResource(hGlobal);
    return 0;
  }

  hRsrc = FindResource( hInst, MAKEINTRESOURCE( lpGrpIconDir->idEntries[iconnb].nID ), RT_ICON );
  FreeResource(hGlobal);
  if(!hRsrc) return 0;
  hGlobal = LoadResource( hInst, hRsrc );
  void *lpIconImage = LockResource( hGlobal );
  
  HICON ic = CreateIconFromResource((PBYTE)lpIconImage, SizeofResource(hInst, hRsrc), TRUE, 0x00030000);
  if(!ic) 
  {
    FreeResource(hGlobal);
    return 0;
  }

  LICE_IBitmap *ret = createBitmapFromIcon(ic, bmp);

  DeleteObject(ic);
  FreeResource(hGlobal);
  return ret;
}
