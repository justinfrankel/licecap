#ifndef _WDLTYPES_
#define _WDLTYPES_

#ifdef _MSC_VER

typedef __int64 WDL_INT64;
typedef unsigned __int64 WDL_UINT64;

#else

typedef long long WDL_INT64;
typedef unsigned long long WDL_UINT64;

#endif

#if !defined(_MSC_VER) ||  _MSC_VER > 1200
#define WDL_DLGRET INT_PTR CALLBACK
#else
#define WDL_DLGRET BOOL CALLBACK
#endif


#ifdef _WIN32
#include <windows.h>
#else
#include <stdint.h>
typedef intptr_t INT_PTR;
#endif

#if defined(__ppc__) || !defined(__cplusplus)
typedef char WDL_bool;
#else
typedef bool WDL_bool;
#endif

#ifndef GWLP_USERDATA
#define GWLP_USERDATA GWL_USERDATA
#define GWLP_WNDPROC GWL_WNDPROC
#define GWLP_HINSTANCE GWL_HINSTANCE
#define GWLP_HWNDPARENT GWL_HWNDPARENT
#define DWLP_USER DWL_USER
#define DWLP_DLGPROC DWL_DLGPROC
#define DWLP_MSGRESULT DWL_MSGRESULT
#define SetWindowLongPtr(a,b,c) SetWindowLong(a,b,c)
#define GetWindowLongPtr(a,b) GetWindowLong(a,b)

#define GCLP_WNDPROC GCL_WNDPROC
#define GCLP_HICON GCL_HICON
#define GCLP_HICONSM GCL_HICONSM
#define SetClassLongPtr(a,b,c) SetClassLong(a,b,c)
#define GetClassLongPtr(a,b) GetClassLong(a,b)
#endif


// for structures that contain doubles, or doubles in structures that are after stuff of questionable alignment (for OSX/linux)
#ifdef _WIN32
#define WDL_FIXALIGN 
#else
#define WDL_FIXALIGN  __attribute__ ((aligned (8)))
#endif



#endif