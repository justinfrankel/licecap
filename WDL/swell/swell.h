/* Cockos SWELL (Simple/Small Win32 Emulation Layer for Losers (who use OS X))
   Copyright (C) 2006-2007, Cockos, Inc.

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
  

    SWELL provides _EXTREMELY BASIC_ win32 wrapping for OS X. It is far from complete, too.

  */


#ifndef _WIN32


#ifndef _WDL_SWELL_H_ // here purely for apps/other libraries (dirscan.h uses it), each section actually has its own define
#define _WDL_SWELL_H_
#endif


// IF YOU ADD TO SWELL:
// Adding types, defines, etc: put them in _WDL_SWELL_H_TYPES_DEFINED_ block.
// Adding functions: put them in _WDL_SWELL_H_API_DEFINED_ block!
//   when adding APIs, add it using:
//      SWELL_API_DEFINE(void, function_name, (int parm, int parm2))
//   rather than:
//      void function_name(int parm, int parm2);



////////////////// SWELL INCLUDES, DEFINES AND TYPEDEFS


#ifndef _WDL_SWELL_H_TYPES_DEFINED_
#define _WDL_SWELL_H_TYPES_DEFINED_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>


#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#ifndef max
#define max(x,y) ((x)<(y)?(y):(x))
#define min(x,y) ((x)<(y)?(x):(y))
#endif

#ifndef S_OK
#define S_OK 0
#endif
#ifndef E_FAIL
#define E_FAIL (-1)
#endif

// the byte ordering of RGB() etc is different than on win32 
#define RGB(r,g,b) (((r)<<16)|((g)<<8)|(b))
#define GetRValue(x) (((x)>>16)&0xff)
#define GetGValue(x) (((x)>>8)&0xff)
#define GetBValue(x) ((x)&0xff)

// basic platform compat defines
#define stricmp(x,y) strcasecmp(x,y)
#define strnicmp(x,y,z) strncasecmp(x,y,z)

#define DeleteFile(x) (!unlink(x))
#define MoveFile(x,y) (!rename(x,y))
#define GetCurrentDirectory(sz,buf) (!getcwd(buf,sz))
#define CreateDirectory(x,y) (!mkdir((x),0755))
#define wsprintf sprintf

#ifndef LOWORD
#define MAKEWORD(a, b)      ((unsigned short)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#define MAKELONG(a, b)      ((long)(((unsigned short)(a)) | ((DWORD)((unsigned short)(b))) << 16))
#define MAKEWPARAM(l, h)      (WPARAM)MAKELONG(l, h)
#define MAKELPARAM(l, h)      (LPARAM)MAKELONG(l, h)
#define MAKELRESULT(l, h)     (LRESULT)MAKELONG(l, h)
#define LOWORD(l)           ((unsigned short)(l))
#define HIWORD(l)           ((unsigned short)(((unsigned long)(l) >> 16) & 0xFFFF))
#define LOBYTE(w)           ((BYTE)(w))
#define HIBYTE(w)           ((BYTE)(((unsigned short)(w) >> 8) & 0xFF))
#endif

#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))

#define UNREFERENCED_PARAMETER(P) (P)
#define _T(T) T

#define CallWindowProc(A,B,C,D,E) ((WNDPROC)A)(B,C,D,E)
#define GetSystemMetrics(x) (1)
#define OffsetRect WinOffsetRect  //to avoid OSX's OffsetRect function
#define SetRect WinSetRect        //to avoid OSX's SetRect function

// DLL loading mappings
#define FreeLibrary(x) dlclose(x)
#define GetProcAddress(h,v) dlsym(h,v)
#define LoadLibrary(x) dlopen(x,RTLD_LAZY)


// basic types
typedef unsigned int DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef signed char BOOL;
typedef DWORD COLORREF;
typedef unsigned int UINT;
typedef unsigned int WPARAM;
typedef long LPARAM;
typedef long LRESULT;
typedef void *LPVOID, *PVOID;
typedef long HRESULT;
typedef unsigned long ULONG;
typedef long LONG;
typedef int *LPINT;


typedef struct HWND__ { 
  int bla;
} *HWND;
typedef void *HANDLE, *HMENU, *HINSTANCE;

typedef struct 
{
  LONG x,y;
} POINT, *LPPOINT;

typedef struct 
{
  LONG left,top, right, bottom;
} RECT, *LPRECT;


typedef struct {
  unsigned char fVirt;
  unsigned short key,cmd;
} ACCEL, *LPACCEL;


typedef struct {
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
} FILETIME;

typedef struct _GUID {
  unsigned long  Data1;
  unsigned short Data2;
  unsigned short Data3;
  unsigned char  Data4[8];
} GUID;

typedef struct {
  HWND hwnd;
  UINT message;
  WPARAM wParam;
  LPARAM lParam;
  DWORD time;
  POINT pt;
} MSG, *LPMSG;

typedef void *HDC, *HBRUSH, *HPEN, *HFONT, *HICON, *HBITMAP, *HGDIOBJ, *HCURSOR, *HRGN;

typedef struct
{
  HWND  hwndFrom;
  UINT  idFrom;
  UINT  code;
} NMHDR, *LPNMHDR;


typedef struct {
  NMHDR   hdr;
  DWORD   dwItemSpec;
  DWORD   dwItemData;
  POINT   pt;
  DWORD   dwHitInfo;
} NMMOUSE, *LPNMMOUSE;
typedef NMMOUSE NMCLICK;
typedef LPNMMOUSE LPNMCLICK;

typedef struct 
{ 
  int mask, fmt,cx; 
  char *pszText; 
  int cchTextMax, iSubItem;
} LVCOLUMN;
typedef struct 
{ 
  int mask, iItem, iSubItem, state, stateMask; 
  char *pszText; 
  int cchTextMax, iImage, lParam;
} LVITEM;


typedef void *HIMAGELIST;

typedef struct
{
  POINT pt;
  UINT flags;
  int iItem;
  int iSubItem;    // this is was NOT in win95.  valid only for LVM_SUBITEMHITTEST
} LVHITTESTINFO, *LPLVHITTESTINFO;


typedef struct
{
  NMHDR   hdr;
  int     iItem;
  int     iSubItem;
  UINT    uNewState;
  UINT    uOldState;
  UINT    uChanged;
  POINT   ptAction;
  LPARAM  lParam;
} NMLISTVIEW, *LPNMLISTVIEW;

typedef struct {
  NMHDR hdr;
  LVITEM item;
} NMLVDISPINFO, *LPNMLVDISPINFO;

typedef struct TCITEM
{
  UINT mask;
  DWORD dwState;
  DWORD dwStateMask;
  char *pszText;
  int cchTextMax;
  int iImage;
  
  LPARAM lParam;
} TCITEM, *LPTCITEM;


typedef struct
{
  unsigned int cbSize, fMask, fType, fState, wID;
  HMENU hSubMenu;
  void *hbmpChecked,*hbmpUnchecked;
  DWORD dwItemData;
  char *dwTypeData;
  int cch;
} MENUITEMINFO;


typedef struct {
  POINT ptReserved, ptMaxSize, ptMaxPosition, ptMinTrackSize, ptMaxTrackSize;
} MINMAXINFO, *LPMINMAXINFO;


typedef struct
{
  long lfHeight, lfWidth, lfEscapement,lfOrientation, lfWeight;
  char lfItalic, lfUnderline, lfStrikeOut, lfCharSet, lfOutPrecision, lfClipPrecision, 
    lfQuality, lfPitchAndFamily;
  char lfFaceName[32];
} LOGFONT;
typedef struct
{
  LONG tmHeight; 
  LONG tmAscent; 
  LONG tmDescent; 
  LONG tmInternalLeading; 
  // todo: implement rest
} TEXTMETRIC;

typedef struct {
  HDC         hdc;
  BOOL        fErase;
  RECT        rcPaint;
} PAINTSTRUCT;

typedef struct
{
  UINT    cbSize;
  UINT    fMask;
  int     nMin;
  int     nMax;
  UINT    nPage;
  int     nPos;
  int     nTrackPos;
} SCROLLINFO, *LPSCROLLINFO;

typedef struct
{
  DWORD   styleOld;
  DWORD   styleNew;
} STYLESTRUCT, *LPSTYLESTRUCT;

typedef struct
{
  HWND    hwnd;
  HWND    hwndInsertAfter;
  int     x;
  int     y;
  int     cx;
  int     cy;
  UINT    flags;
} WINDOWPOS, *LPWINDOWPOS, *PWINDOWPOS;

typedef struct  
{
  RECT       rgrc[3];
  PWINDOWPOS lppos;
} NCCALCSIZE_PARAMS, *LPNCCALCSIZE_PARAMS;

typedef BOOL (*DLGPROC)(HWND, UINT, WPARAM, LPARAM);
typedef LRESULT (*WNDPROC)(HWND, UINT, WPARAM, LPARAM);

typedef BOOL (*PROPENUMPROCEX)(HWND hwnd, const char *lpszString, HANDLE hData, LPARAM lParam);

// swell specific type
typedef HWND (*SWELL_ControlCreatorProc)(HWND parent, const char *cname, int idx, const char *classname, int style, int x, int y, int w, int h);                                           


/*
 ** win32 specific constants
 */
#define MB_OK 0
#define MB_OKCANCEL 1
#define MB_YESNOCANCEL 3
#define MB_YESNO 4
#define MB_RETRYCANCEL 5
#define IDCANCEL 1
#define IDOK 2
#define IDNO 3
#define IDYES 4
#define IDRETRY 5

#define GW_HWNDFIRST        0
#define GW_HWNDLAST         1
#define GW_HWNDNEXT         2
#define GW_HWNDPREV         3
#define GW_OWNER            4
#define GW_CHILD            5

#define GWL_USERDATA        (-21)
#define GWL_ID              (-12)
#define GWL_STYLE           (-16) // only supported for BS_ for now I think
#define GWL_EXSTYLE         (-20)
#define GWL_WNDPROC         (-4)
#define DWL_DLGPROC         (-8)

#define CB_ADDSTRING                0x0143
#define CB_DELETESTRING             0x0144
#define CB_GETCOUNT                 0x0146
#define CB_GETCURSEL                0x0147
#define CB_GETLBTEXT                0x0148
#define CB_INSERTSTRING             0x014A
#define CB_RESETCONTENT             0x014B
#define CB_SETCURSEL                0x014E
#define CB_GETITEMDATA              0x0150
#define CB_SETITEMDATA              0x0151
#define CB_INITSTORAGE              0x0161

#define LB_ADDSTRING            0x0180
#define LB_INSERTSTRING         0x0181
#define LB_DELETESTRING         0x0182
#define LB_RESETCONTENT         0x0184
#define LB_SETSEL               0x0185
#define LB_SETCURSEL            0x0186
#define LB_GETSEL               0x0187
#define LB_GETCURSEL            0x0188
#define LB_GETCOUNT             0x018B
#define LB_GETSELCOUNT          0x0190
#define LB_GETITEMDATA          0x0199
#define LB_SETITEMDATA          0x019A

#define TBM_GETPOS              (WM_USER)
#define TBM_SETTIC              (WM_USER+4)
#define TBM_SETPOS              (WM_USER+5)
#define TBM_SETRANGE            (WM_USER+6)
#define TBM_SETSEL              (WM_USER+10)

#define PBM_SETRANGE            (WM_USER+1)
#define PBM_SETPOS              (WM_USER+2)
#define PBM_DELTAPOS            (WM_USER+3)

#define BM_GETIMAGE        0x00F6
#define BM_SETIMAGE        0x00F7
#define IMAGE_BITMAP 0
#define IMAGE_ICON 1

#define NM_FIRST                (0U-  0U)       // generic to all controls
#define NM_LAST                 (0U- 99U)
#define NM_CLICK                (NM_FIRST-2)    // uses NMCLICK struct
#define NM_DBLCLK               (NM_FIRST-3)
#define NM_RCLICK               (NM_FIRST-5)    // uses NMCLICK struct


#define LVSIL_STATE 1

#define LVIR_BOUNDS             0
#define LVIR_ICON               1
#define LVIR_LABEL              2
#define LVIR_SELECTBOUNDS       3

#define LVHT_NOWHERE            0x0001
#define LVHT_ONITEMICON         0x0002
#define LVHT_ONITEMLABEL        0x0004
#define LVHT_ONITEMSTATEICON    0x0008
#define LVHT_ONITEM             (LVHT_ONITEMICON | LVHT_ONITEMLABEL | LVHT_ONITEMSTATEICON)

#define LVHT_ABOVE              0x0010
#define LVHT_BELOW              0x0020
#define LVHT_TORIGHT            0x0040
#define LVHT_TOLEFT             0x0080

#define LVCF_TEXT 1
#define LVCF_WIDTH 2
#define LVIF_TEXT 1
#define LVIF_PARAM 2
#define LVIF_STATE 4
#define LVIS_SELECTED 1
#define LVIS_FOCUSED 2
#define LVNI_FOCUSED 1
#define INDEXTOSTATEIMAGEMASK(x) ((x)<<16)

#define LVN_FIRST               (0U-100U)       // listview
#define LVN_LAST                (0U-199U)
#define LVN_BEGINDRAG           (LVN_FIRST-9)
#define LVN_ITEMCHANGED         (LVN_FIRST-1)
#define LVN_ODFINDITEM          (LVN_FIRST-52)
#define LVN_GETDISPINFO         (LVN_FIRST-50)

// ignored for now
#define LVS_EX_FULLROWSELECT 0



#define TCIF_TEXT               0x0001
#define TCIF_IMAGE              0x0002
#define TCIF_PARAM              0x0008
//#define TCIF_STATE              0x0010



#define TCN_FIRST               (0U-550U)       // tab control
#define TCN_LAST                (0U-580U)
#define TCN_SELCHANGE           (TCN_FIRST - 1)



#define BS_AUTOCHECKBOX 1
#define BS_AUTORADIOBUTTON 2
#define BS_AUTO3STATE 4

#define BST_CHECKED 1
#define BST_UNCHECKED 0
#define BST_INDETERMINATE 2
#define SW_HIDE 0
#define SW_SHOWNA 1
#define SW_SHOW 2
#define SW_NORMAL 2
#define SW_SHOWNORMAL 2

#define SWP_NOMOVE 1
#define SWP_NOSIZE 2
#define SWP_NOZORDER 4
#define SWP_NOACTIVATE 8
#define HWND_TOP (HWND)0
#define HWND_TOPMOST (HWND)0
#define HWND_BOTTOM (HWND)0
#define HWND_NOTOPMOST (HWND)0

// most of these are ignored, actually, but TPM_NONOTIFY and TPM_RETURNCMD are now used
#define TPM_LEFTBUTTON  0x0000L
#define TPM_RIGHTBUTTON 0x0002L
#define TPM_LEFTALIGN   0x0000L
#define TPM_CENTERALIGN 0x0004L
#define TPM_RIGHTALIGN  0x0008L
#define TPM_TOPALIGN        0x0000L
#define TPM_VCENTERALIGN    0x0010L
#define TPM_BOTTOMALIGN     0x0020L
#define TPM_HORIZONTAL      0x0000L     /* Horz alignment matters more */
#define TPM_VERTICAL        0x0040L     /* Vert alignment matters more */
#define TPM_NONOTIFY        0x0080L     /* Don't send any notification msgs */
#define TPM_RETURNCMD       0x0100L

#define MIIM_ID 1
#define MIIM_STATE 2
#define MIIM_TYPE 4
#define MIIM_SUBMENU 8
#define MIIM_DATA 16
#define MFT_STRING 0
#define MFT_SEPARATOR 1
#define MFT_BITMAP 2

#define MF_UNCHECKED 0
#define MF_ENABLED 0
#define MF_GRAYED 1
#define MF_DISABLED 1
#define MF_CHECKED 4

#define MFS_UNCHECKED 0
#define MFS_ENABLED 0
#define MFS_GRAYED 1
#define MFS_CHECKED 4

#define MF_BYCOMMAND 0
#define MF_BYPOSITION 0x100


#define EN_CHANGE           0x0300
#define STN_CLICKED         0
#define STN_DBLCLK          1
#define WM_CREATE                       0x0001
#define WM_DESTROY                      0x0002
#define WM_MOVE                         0x0003
#define WM_SIZE                         0x0005
#define WM_PAINT                        0x000F
#define WM_CLOSE                        0x0010
#define WM_ERASEBKGND                   0x0014
#define WM_SETCURSOR                    0x0020
#define WM_GETMINMAXINFO                0x0024


#define WM_NOTIFY                       0x004E

#define WM_CONTEXTMENU                  0x007B
#define WM_STYLECHANGED                 0x007D
#define WM_NCDESTROY                    0x0082
#define WM_NCCALCSIZE                   0x0083
#define WM_NCHITTEST                    0x0084
#define WM_NCPAINT                      0x0085
#define WM_NCMOUSEMOVE                  0x00A0
#define WM_NCLBUTTONDOWN                0x00A1
#define WM_NCLBUTTONUP                  0x00A2
#define WM_NCLBUTTONDBLCLK              0x00A3
#define WM_NCRBUTTONDOWN                0x00A4
#define WM_NCRBUTTONUP                  0x00A5
#define WM_NCMBUTTONDOWN                0x00A7
#define WM_NCMBUTTONUP                  0x00A8
#define WM_KEYFIRST                     0x0100
#define WM_KEYDOWN                      0x0100
#define WM_KEYUP                        0x0101
#define WM_CHAR                         0x0102
#define WM_DEADCHAR                     0x0103
#define WM_SYSKEYDOWN                   0x0104
#define WM_SYSKEYUP                     0x0105
#define WM_SYSCHAR                      0x0106
#define WM_SYSDEADCHAR                  0x0107
#define WM_KEYLAST                      0x0108
#define WM_INITDIALOG                   0x0110
#define WM_COMMAND                      0x0111
#define WM_TIMER                        0x0113
#define WM_INITMENUPOPUP                0x0117
#define WM_HSCROLL                      0x0114
#define WM_VSCROLL                      0x0115
#define WM_MOUSEFIRST                   0x0200
#define WM_MOUSEMOVE                    0x0200
#define WM_LBUTTONDOWN                  0x0201
#define WM_LBUTTONUP                    0x0202
#define WM_LBUTTONDBLCLK                0x0203
#define WM_RBUTTONDOWN                  0x0204
#define WM_RBUTTONUP                    0x0205
#define WM_RBUTTONDBLCLK                0x0206
#define WM_MBUTTONDOWN                  0x0207
#define WM_MBUTTONUP                    0x0208
#define WM_MBUTTONDBLCLK                0x0209
#define WM_MOUSEWHEEL                   0x020A
#define WM_MOUSELAST                    0x020A
#define WM_CAPTURECHANGED               0x0215
#define WM_USER                         0x0400      

#define CBN_SELCHANGE       0
#define CBN_EDITCHANGE 1
#define CB_ERR (-1)


#define SB_HORZ             0
#define SB_VERT             1
#define SB_CTL              2
#define SB_BOTH             3

#define SB_LINEUP           0
#define SB_LINELEFT         0
#define SB_LINEDOWN         1
#define SB_LINERIGHT        1
#define SB_PAGEUP           2
#define SB_PAGELEFT         2
#define SB_PAGEDOWN         3
#define SB_PAGERIGHT        3
#define SB_THUMBPOSITION    4
#define SB_THUMBTRACK       5
#define SB_TOP              6
#define SB_LEFT             6
#define SB_BOTTOM           7
#define SB_RIGHT            7
#define SB_ENDSCROLL        8

#define DFCS_SCROLLUP           0x0000
#define DFCS_SCROLLDOWN         0x0001
#define DFCS_SCROLLLEFT         0x0002
#define DFCS_SCROLLRIGHT        0x0003
#define DFCS_SCROLLCOMBOBOX     0x0005
#define DFCS_SCROLLSIZEGRIP     0x0008
#define DFCS_SCROLLSIZEGRIPRIGHT 0x0010

#define DFCS_INACTIVE           0x0100
#define DFCS_PUSHED             0x0200
#define DFCS_CHECKED            0x0400
#define DFCS_FLAT               0x4000

#define DFCS_BUTTONPUSH         0x0010

#define DFC_SCROLL              3
#define DFC_BUTTON              4

#define ESB_ENABLE_BOTH     0x0000
#define ESB_DISABLE_BOTH    0x0003

#define ESB_DISABLE_LEFT    0x0001
#define ESB_DISABLE_RIGHT   0x0002

#define ESB_DISABLE_UP      0x0001
#define ESB_DISABLE_DOWN    0x0002

#define BDR_RAISEDOUTER 0x0001
#define BDR_SUNKENOUTER 0x0002
#define BDR_RAISEDINNER 0x0004
#define BDR_SUNKENINNER 0x0008

#define BDR_OUTER       0x0003
#define BDR_INNER       0x000c

#define EDGE_RAISED     (BDR_RAISEDOUTER | BDR_RAISEDINNER)
#define EDGE_SUNKEN     (BDR_SUNKENOUTER | BDR_SUNKENINNER)
#define EDGE_ETCHED     (BDR_SUNKENOUTER | BDR_RAISEDINNER)
#define EDGE_BUMP       (BDR_RAISEDOUTER | BDR_SUNKENINNER)

#define BF_ADJUST       0x2000
#define BF_FLAT         0x4000
#define BF_LEFT         0x0001
#define BF_TOP          0x0002
#define BF_RIGHT        0x0004
#define BF_BOTTOM       0x0008
#define BF_RECT         (BF_LEFT | BF_TOP | BF_RIGHT | BF_BOTTOM)

#define PATCOPY             (DWORD)0x00F00021

#define HTHSCROLL           6
#define HTVSCROLL           7

#define WS_EX_LEFTSCROLLBAR     0x00004000L

#define SIF_RANGE           0x0001
#define SIF_PAGE            0x0002
#define SIF_POS             0x0004
#define SIF_DISABLENOSCROLL 0x0008
#define SIF_TRACKPOS        0x0010
#define SIF_ALL             (SIF_RANGE | SIF_PAGE | SIF_POS | SIF_TRACKPOS)

#define SIZE_MINIMIZED (-1)
#define SIZE_MAXIMIZED (-2)

#ifndef WINAPI
#define WINAPI
#endif

#ifndef CALLBACK
#define CALLBACK
#endif

#ifndef MAKEINTRESOURCE
#define MAKEINTRESOURCE(x) (x)         
#endif                

#define FVIRTKEY  1
#define FSHIFT    0x04
#define FCONTROL  0x08
#define FALT      0x10


#define VK_BACK           0x08
#define VK_TAB            0x09

#define VK_CLEAR          0x0C
#define VK_RETURN         0x0D

#define VK_SHIFT          0x10
#define VK_CONTROL        0x11
#define VK_MENU           0x12
#define VK_PAUSE          0x13
#define VK_CAPITAL        0x14

#define VK_ESCAPE         0x1B

#define VK_SPACE          0x20
#define VK_PRIOR          0x21
#define VK_NEXT           0x22
#define VK_END            0x23
#define VK_HOME           0x24
#define VK_LEFT           0x25
#define VK_UP             0x26
#define VK_RIGHT          0x27
#define VK_DOWN           0x28
#define VK_SELECT         0x29
#define VK_PRINT          0x2A
#define VK_INSERT         0x2D
#define VK_DELETE         0x2E
#define VK_HELP           0x2F

#define VK_NUMPAD0        0x60
#define VK_NUMPAD1        0x61
#define VK_NUMPAD2        0x62
#define VK_NUMPAD3        0x63
#define VK_NUMPAD4        0x64
#define VK_NUMPAD5        0x65
#define VK_NUMPAD6        0x66
#define VK_NUMPAD7        0x67
#define VK_NUMPAD8        0x68
#define VK_NUMPAD9        0x69
#define VK_MULTIPLY       0x6A
#define VK_ADD            0x6B
#define VK_SEPARATOR      0x6C
#define VK_SUBTRACT       0x6D
#define VK_DECIMAL        0x6E
#define VK_DIVIDE         0x6F
#define VK_F1             0x70
#define VK_F2             0x71
#define VK_F3             0x72
#define VK_F4             0x73
#define VK_F5             0x74
#define VK_F6             0x75
#define VK_F7             0x76
#define VK_F8             0x77
#define VK_F9             0x78
#define VK_F10            0x79
#define VK_F11            0x7A
#define VK_F12            0x7B

#define MK_LBUTTON        0x10000
#define MK_MBUTTON        0x10001
#define MK_RBUTTON        0x10002


#define IDC_UPARROW -1004
#define IDC_NO -1003
#define IDC_SIZEALL -1002
#define IDC_SIZENS -1001
#define IDC_SIZEWE -1000
#define IDC_ARROW -999
#define IDC_HAND 32649




#define COLOR_3DSHADOW 0
#define COLOR_3DHILIGHT 1
#define COLOR_3DFACE 2
#define COLOR_BTNTEXT 3
#define COLOR_WINDOW 4
#define COLOR_SCROLLBAR 5
#define COLOR_3DDKSHADOW 6
#define COLOR_BTNFACE 7
#define COLOR_INFOBK 8
#define COLOR_INFOTEXT 9

#define SRCCOPY 0
#define PS_SOLID 0
#define DT_CALCRECT 1
#define DT_VCENTER 2
#define DT_CENTER 4
#define DT_END_ELLIPSIS 8
#define DT_BOTTOM 16
#define DT_RIGHT 32
#define DT_SINGLELINE 64
#define DT_NOPREFIX 128
#define DT_NOCLIP 256

#define DT_TOP 0
#define DT_LEFT 0

#define FW_DONTCARE         0
#define FW_THIN             100
#define FW_EXTRALIGHT       200
#define FW_LIGHT            300
#define FW_NORMAL           400
#define FW_MEDIUM           500
#define FW_SEMIBOLD         600
#define FW_BOLD             700
#define FW_EXTRABOLD        800
#define FW_HEAVY            900

#define FW_ULTRALIGHT       FW_EXTRALIGHT
#define FW_REGULAR          FW_NORMAL
#define FW_DEMIBOLD         FW_SEMIBOLD
#define FW_ULTRABOLD        FW_EXTRABOLD
#define FW_BLACK            FW_HEAVY

#define OUT_DEFAULT_PRECIS 0
#define CLIP_DEFAULT_PRECIS 0
#define DEFAULT_QUALITY 0
#define DRAFT_QUALITY 1
#define PROOF_QUALITY 2
#define NONANTIALIASED_QUALITY 3
#define ANTIALIASED_QUALITY 4
#define DEFAULT_PITCH 0
#define DEFAULT_CHARSET 0
#define ANSI_CHARSET 0
#define TRANSPARENT 0
#define OPAQUE 1

#define NULL_PEN 1
#define NULL_BRUSH 2

#define GMEM_ZEROINIT 1
#define GMEM_FIXED 0
#define GMEM_MOVEABLE 0
#define GMEM_DDESHARE 0
#define GMEM_DISCARDABLE 0
#define GMEM_SHARE 0
#define GMEM_LOWER 0
#define GHND (GMEM_MOVEABLE|GM_ZEROINIT)
#define GPTR (GMEM_FIXED|GMEM_ZEROINIT)



extern struct SWELL_DialogResourceIndex *SWELL_curmodule_dialogresource_head;
extern struct SWELL_MenuResourceIndex *SWELL_curmodule_menuresource_head;




#endif //_WDL_SWELL_H_TYPES_DEFINED_



#ifndef _WDL_SWELL_H_API_DEFINED_
#define _WDL_SWELL_H_API_DEFINED_

////////////////////////////////////////////
/////////// FUNCTIONS
////////////////////////////////////////////

#ifndef SWELL_API_DEFINE


#ifdef SWELL_PROVIDED_BY_APP
#define SWELL_API_DEFINE(ret,func,parms) extern ret (*func)parms;
#else
#define SWELL_API_DEFINE(ret,func,parms) ret func parms ;
#endif
#endif

/* 
** lstrcpyn: this is provided because strncpy is braindead (filling with zeroes, and not 
** NULL terminating if the destination buffer is too short? ASKING for trouble..)
** lstrpcyn always null terminates the string and doesnt fill anything extra.
*/
SWELL_API_DEFINE(char *, lstrcpyn, (char *dest, const char *src, int l))


/*
** MulDiv(): (parm1*parm2)/parm3
** Implemented using long longs.
*/
SWELL_API_DEFINE(int, MulDiv, (int, int, int))


/*
** Sleep() sleeps for specified milliseconds. This maps to usleep, with a ms value of 0 
** usleeping for 100 microseconds. 
*/
SWELL_API_DEFINE(void, Sleep,(int ms))

/*
** GetTickCount() and timeGetTime() give you ms level timings via gettimeofday(). 
** 
** NOTE: This doesn't map to time since system start (like in win32), so a wrap around 
** is slightly more likely (i.e. even if you booted your system an hour ago it could happen).
*/
SWELL_API_DEFINE(DWORD, GetTickCount,())
#ifndef timeGetTime
#define timeGetTime() GetTickCount()
#endif

/*
** GetFileTime() gets the file time of a file (FILE *), and converts it to the Windows time.
**
** NOTE: while it returns a 64 bit time value, it is only accurate to the second since thats 
** what fstat() returns. Takes a FILE * rather than a HANDLE.
*/
SWELL_API_DEFINE(BOOL, GetFileTime,(void *fh, FILETIME *lpCreationTime, FILETIME *lpLastAccessTime, FILETIME *lpLastWriteTime))

/*
** *PrivateProfileString/Int():
** These are mostly thread-safe, mostly inter-process safe, and mostly module safe 
** (i.e. writes from other modules) should be synchronized).
**
** NOTES:
**   the filename used MUST be the full filename, unlike on Windows where files without paths go to
**   C:/Windows, here they will be opened in the current directory.
**
**   It's probably not a good idea to push your luck with simultaneous writes from multiple
**   modules in different threads/processes, but in theory it should work.
*/
SWELL_API_DEFINE(BOOL, WritePrivateProfileString, (const char *appname, const char *keyname, const char *val, const char *fn))
SWELL_API_DEFINE(DWORD, GetPrivateProfileString, (const char *appname, const char *keyname, const char *def, char *ret, int retsize, const char *fn))
SWELL_API_DEFINE(int, GetPrivateProfileInt,(const char *appname, const char *keyname, int def, const char *fn))
SWELL_API_DEFINE(BOOL, GetPrivateProfileStruct,(const char *appname, const char *keyname, void *buf, int bufsz, const char *fn))
SWELL_API_DEFINE(BOOL, WritePrivateProfileStruct,(const char *appname, const char *keyname, const void *buf, int bufsz, const char *fn))


/*
** GetModuleFileName()
**
** This currently ignores the HINSTANCE parameter and returns the main bundle name of the
** application (i.e. /Applications/MyApp.app).
*/
SWELL_API_DEFINE(DWORD, GetModuleFileName,(HINSTANCE ignored, char *fn, DWORD nSize))

/*
** SWELL_CStringToCFString(): Creates a CFString/NSString * from a C string. This is mostly 
** used internally but you may wish to use it as well (though none of the SWELL APIs take 
** CFString/NSString.
*/
SWELL_API_DEFINE(void *,SWELL_CStringToCFString,(const char *str))


/*
** PtInRect() should hopefully function just like it's win32 equivelent.
** there is #define funkiness because some Mac system headers define PtInRect as well.
*/
#ifdef PtInRect
#undef PtInRect
#endif
#define PtInRect(r,p) SWELL_PtInRect(r,p)
SWELL_API_DEFINE(BOOL, SWELL_PtInRect,(RECT *r, POINT p))

/*
** ShellExecute(): 
** NOTE: currently action is ignored, and it only works on content1 being a URL beginning with http://.
** TODO: finish implementation
*/
SWELL_API_DEFINE(BOOL, ShellExecute,(HWND hwndDlg, const char *action,  const char *content1, const char *content2, const char *content3, int blah))

/*
** MessageBox().
*/
SWELL_API_DEFINE(int, MessageBox,(HWND hwndParent, const char *text, const char *caption, int type))


/*
** GetOpenFileName() / GetSaveFileName() 
** These are a different API because we didnt feel like reeimplimenting the full API.
** Extlist is something similar you'd pass getopenfilename, 
** initialdir and initialfile are optional (and NULL means not set).
*/

// free() the result of this, if non-NULL.
// if allowmul is set, the multiple files are specified the same way GetOpenFileName() returns.
SWELL_API_DEFINE(char *,BrowseForFiles,(const char *text, const char *initialdir, 
					const char *initialfile, bool allowmul, const char *extlist)) 

// returns TRUE if file was chosen.         
SWELL_API_DEFINE(bool, BrowseForSaveFile,(const char *text, const char *initialdir, const char *initialfile, const char *extlist,
			char *fn, int fnsize))

// returns TRUE if path was chosen.
SWELL_API_DEFINE(bool, BrowseForDirectory,(const char *text, const char *initialdir, char *fn, int fnsize))


// Note that window functions are generally NOT threadsafe.
// all of these treat HWND as NSView and/or NSWindow (usually smartish about it)


/* 
** GetDlgItem(hwnd,0) returns hwnd if hwnd is a NSView, or the contentview if hwnd is a NSWindow.
** Note that this GetDlgItem will actually search a view hierarchy for the tagged view, 
** unlike Win32 where it will only look at immediate children.
*/
SWELL_API_DEFINE(HWND, GetDlgItem,(HWND, int))

/*
** ShowWindow() works for hwnds that represent NSView and/or NSWindow.
** SW_SHOW, SW_SHOWNA, and SW_HIDE are defined, and some of the other common uses
** alias to these.
*/
SWELL_API_DEFINE(void, ShowWindow,(HWND, int))


/*
** DestroyWindow() works for both a NSWindow or NSView.
** Note that if the window is a fake window with a procedure 
** (created via CreateDialog or DialogBox below) then WM_DESTROY
** will be called immediately, though the window/view may be freed 
** sometime later via the autorelease pool.
*/
SWELL_API_DEFINE(void, DestroyWindow,(HWND hwnd)) 


/*
** These should all work like their Win32 versions, though if idx=0 it gets/sets the
** value for the window. Note that SetDlgItemText() for an edit control does NOT send
** a WM_COMMAND notification like on win32, so you will have to do this yourself.
*/
SWELL_API_DEFINE(void, SetDlgItemText,(HWND, int idx, const char *text))
SWELL_API_DEFINE(void, SetDlgItemInt,(HWND, int idx, int val, int issigned))
SWELL_API_DEFINE(int, GetDlgItemInt,(HWND, int idx, BOOL *translated, int issigned))
SWELL_API_DEFINE(void, GetDlgItemText,(HWND, int idx, char *text, int textlen))

#ifndef GetWindowText
#define GetWindowText(hwnd,text,textlen) GetDlgItemText(hwnd,0,text,textlen)
#define SetWindowText(hwnd,text) SetDlgItemText(hwnd,0,text)
#endif


SWELL_API_DEFINE(void, CheckDlgButton,(HWND hwnd, int idx, int check))
SWELL_API_DEFINE(int, IsDlgButtonChecked,(HWND hwnd, int idx))
SWELL_API_DEFINE(void, EnableWindow,(HWND hwnd, int enable))
SWELL_API_DEFINE(void, SetFocus,(HWND hwnd)) // these take NSWindow/NSView, and return NSView *
SWELL_API_DEFINE(HWND, GetFocus,())
SWELL_API_DEFINE(void, SetForegroundWindow,(HWND hwnd)) // these take NSWindow/NSView, and return NSView *
SWELL_API_DEFINE(HWND, GetForegroundWindow,())
#ifndef GetActiveWindow
#define GetActiveWindow() GetForegroundWindow()
#endif

/*
** GetCapture/SetCapture/ReleaseCapture are completely faked, with just an internal state.
** Mouse click+drag automatically captures the focus sending mouseDrag events in OSX, so
** these are as a porting aid.
**
** Updated: they actually send WM_CAPTURECHANGED messages now, if the window supports
** onSwellMessage:p1:p2: and swellCapChangeNotify (and swellCapChangeNotify returns YES).
**
** Note that any HWND that returns YES to swellCapChangeNotify should do the following on 
** destroy or dealloc: if (GetCapture()==(HWND)self) ReleaseCapture(); Failure to do so
** can cause a dealloc'd window to get messages sent to it.
*/
SWELL_API_DEFINE(HWND, SetCapture,(HWND hwnd))
SWELL_API_DEFINE(HWND, GetCapture,())
SWELL_API_DEFINE(void, ReleaseCapture,())

/*
** IsChild()
** Notes: hwndChild must be a NSView (if not then false is returned)
** hwndParent can be a NSWindow or NSView. 
** NSWindow level ownership/children are not detected.
*/
SWELL_API_DEFINE(int, IsChild,(HWND hwndParent, HWND hwndChild))


/*
** GetParent()
** Notes: if hwnd is a NSView, then gets the parent view (or NSWindow 
** if the parent view is the window's contentview). If hwnd is a NSWindow,
** then GetParent returns the owner window, if any. Note that the owner 
** window system is not part of OSX, but rather part of SWELL.
*/
SWELL_API_DEFINE(HWND, GetParent,(HWND hwnd))

/*
** SetParent()
** Notes: hwnd must be a NSView, newPar can be either NSView or NSWindow.
*/
SWELL_API_DEFINE(HWND, SetParent,(HWND hwnd, HWND newPar))

/*
** GetWindow()
** Most of the standard GW_CHILD etc work. Does not do anything to prevent you from 
** getting into infinite loops if you go changing the order on the fly etc.
*/
SWELL_API_DEFINE(HWND, GetWindow,(HWND hwnd, int what))

/*
** Notes: common win32 code like this:
**   RECT r;
**   GetWindowRect(hwnd,&r); 
**   ScreenToClient(otherhwnd,(LPPOINT)&r);
**   ScreenToClient(otherhwnd,((LPPOINT)&r)+1);
** does work, however be aware that in certain instances r.bottom may be less 
** than r.top, due to flipped coordinates. SetWindowPos and other functions 
** handle negative heights gracefully, and you should too.
*/
SWELL_API_DEFINE(void, ClientToScreen,(HWND hwnd, POINT *p))
SWELL_API_DEFINE(void, ScreenToClient,(HWND hwnd, POINT *p))
SWELL_API_DEFINE(void, GetWindowRect,(HWND hwnd, RECT *r))
SWELL_API_DEFINE(void, GetClientRect,(HWND hwnd, RECT *r))
SWELL_API_DEFINE(HWND, WindowFromPoint,(POINT p))
SWELL_API_DEFINE(BOOL, WinOffsetRect, (LPRECT lprc, int dx, int dy))
SWELL_API_DEFINE(BOOL, WinSetRect, (LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom))

/*
** SetWindowPos():
** Notes: Z ordering stuff is ignored, as are most flags. 
** SWP_NOMOVE and SWP_NOSIZE are the only flags used.
*/
SWELL_API_DEFINE(void, SetWindowPos,(HWND hwnd, HWND unused, int x, int y, int cx, int cy, int flags))

/*
** InvalidateRect()
** Notes: eraseBk is ignored, probably not threadsafe! hwnd can be NSWindow or NSView
*/
SWELL_API_DEFINE(void,InvalidateRect,(HWND hwnd, RECT *r, int eraseBk))

/*
** UpdateWindow()
** Notes: not currently implemented but provided here in case someday it is necessary
*/
SWELL_API_DEFINE(void,UpdateWindow,(HWND hwnd))


/*
** GetWindowLong()/SetWindowLong()
**
** GWL_ID is supported for all objects that support the 'tag'/'setTag' methods,
** which would be controls and SWELL created windows/dialogs/controls.
**
** GWL_USERDATA is supported by SWELL created windows/dialogs/controls, using 
** (int)getSwellUserData and setSwellUserData:(int).
** 
** GWL_WNDPROC is supported by SWELL created windows/dialogs/controls, using 
** (int)getSwellWindowProc and setSwellWindowProc:(int).
**
** DWL_DLGPROC is supported by SWELL created dialogs now (it might work in windows/controls but isnt recommended)
**
** GWL_STYLE is only supported for NSButton. Currently the only flags supported are
** BS_AUTO3STATE (BS_AUTOCHECKBOX is returned but also ignored).
**
** indices of >= 0 and < 128 (32 integers) are supported for SWELL created 
** windows/dialogs/controls, via (int)getSwellExtraData:(int)idx and 
** setSwellExtraData:(int)idx value:(int)val . 
*/
SWELL_API_DEFINE(int, GetWindowLong,(HWND hwnd, int idx))
SWELL_API_DEFINE(int, SetWindowLong,(HWND hwnd, int idx, int val))


/* 
** GetProp() SetProp() RemoveProp() EnumPropsEx()
** These should work like in win32. Free your props otherwise they will leak.
** Restriction on what you can do in the PROPENUMPROCEX is similar to win32 
** (you can remove only the called prop, and can't add props within it).
** if the prop name is < (void *)65536 then it is treated as a short identifier.
*/
SWELL_API_DEFINE(int, EnumPropsEx,(HWND, PROPENUMPROCEX, LPARAM))
SWELL_API_DEFINE(HANDLE, GetProp, (HWND, const char *))
SWELL_API_DEFINE(BOOL, SetProp, (HWND, const char *, HANDLE))
SWELL_API_DEFINE(HANDLE, RemoveProp, (HWND, const char *))
                

/*
** IsWindowVisible()
** if hwnd is a NSView, returns !isHiddenOrHasHiddenAncestor
** if hwnd is a NSWindow returns isVisible
** otherwise returns TRUE if non-null hwnd
*/
SWELL_API_DEFINE(bool, IsWindowVisible,(HWND hwnd))
#ifndef IsWindow
#define IsWindow(x) (!!(x)) // todo use isKindOf
#endif


/*
** SetTimer/KillTimer():
** Notes:
** The timer API may be threadsafe though it is highly untested.
** Note also that the mechanism for sending timers is SWELL_Timer:(id).
** The fourth parameter to SetTimer() is not supported and will be ignored, so you must
** receive your timers via a WM_TIMER (or SWELL_Timer:(id))
**
** You can kill all timers for a window using KillTimer(hwnd,-1);
** You MUST kill all timers for a window before destroying it. Note that SWELL created 
** windows/dialogs/controls automatically do this, but if you use SetTimer() on a NSView *
** or NSWindow * directly, then you should kill all timers in -dealloc.
*/
SWELL_API_DEFINE(int, SetTimer,(HWND hwnd, int timerid, int rate, unsigned long *notUsed))
SWELL_API_DEFINE(void, KillTimer,(HWND hwnd, int timerid))

/*
** These provide the interfaces for directly updating a combo box control. This is no longer
** required as SendMessage can now be used with CB_* etc.
** Combo boxes may be implemented using a NSComboBox or NSPopUpButton depending on the style.
**
** Notes: CB_SetItemData/CB_GetItemData only work for the non-user-editable version (using NSPopUpbutotn).
*/
SWELL_API_DEFINE(int, SWELL_CB_AddString,(HWND hwnd, int idx, const char *str))
SWELL_API_DEFINE(void, SWELL_CB_SetCurSel,(HWND hwnd, int idx, int sel))
SWELL_API_DEFINE(int, SWELL_CB_GetCurSel,(HWND hwnd, int idx))
SWELL_API_DEFINE(int, SWELL_CB_GetNumItems,(HWND hwnd, int idx))
SWELL_API_DEFINE(void, SWELL_CB_SetItemData,(HWND hwnd, int idx, int item, int data)) // these two only work for the combo list version for now
SWELL_API_DEFINE(int, SWELL_CB_GetItemData,(HWND hwnd, int idx, int item))
SWELL_API_DEFINE(void, SWELL_CB_Empty,(HWND hwnd, int idx))
SWELL_API_DEFINE(int, SWELL_CB_InsertString,(HWND hwnd, int idx, int pos, const char *str))
SWELL_API_DEFINE(int, SWELL_CB_GetItemText,(HWND hwnd, int idx, int item, char *buf, int bufsz))
SWELL_API_DEFINE(void, SWELL_CB_DeleteString,(HWND hwnd, int idx, int wh))

/*
** Trackbar API
** These provide the interfaces for directly updating a trackbar (implemented using NSSlider).
** Note that you can now use SendMessage with TBM_* instead.
*/
SWELL_API_DEFINE(void, SWELL_TB_SetPos,(HWND hwnd, int idx, int pos))
SWELL_API_DEFINE(void, SWELL_TB_SetRange,(HWND hwnd, int idx, int low, int hi))
SWELL_API_DEFINE(int, SWELL_TB_GetPos,(HWND hwnd, int idx))
SWELL_API_DEFINE(void, SWELL_TB_SetTic,(HWND hwnd, int idx, int pos))


/*
** ListView API. In owner data mode only LVN_GETDISPINFO is used (not ODFINDITEM etc).
** LVN_BEGINDRAG also should work as on windows. Imagelists state icons work as well.
*/
SWELL_API_DEFINE(void, ListView_SetExtendedListViewStyleEx,(HWND h, int flag, int mask))
SWELL_API_DEFINE(void, ListView_InsertColumn,(HWND h, int pos, const LVCOLUMN *lvc))
SWELL_API_DEFINE(int, ListView_InsertItem,(HWND h, const LVITEM *item))
SWELL_API_DEFINE(void, ListView_SetItemText,(HWND h, int ipos, int cpos, const char *txt))
SWELL_API_DEFINE(bool, ListView_SetItem,(HWND h, LVITEM *item))
SWELL_API_DEFINE(int, ListView_GetNextItem,(HWND h, int istart, int flags))
SWELL_API_DEFINE(bool, ListView_GetItem,(HWND h, LVITEM *item))
SWELL_API_DEFINE(int, ListView_GetItemState,(HWND h, int ipos, int mask))
SWELL_API_DEFINE(void, ListView_DeleteItem,(HWND h, int ipos))
SWELL_API_DEFINE(void, ListView_DeleteAllItems,(HWND h))
SWELL_API_DEFINE(int, ListView_GetSelectedCount,(HWND h))
SWELL_API_DEFINE(int, ListView_GetItemCount,(HWND h))
SWELL_API_DEFINE(int, ListView_GetSelectionMark,(HWND h))
SWELL_API_DEFINE(void, ListView_SetColumnWidth,(HWND h, int colpos, int wid))
SWELL_API_DEFINE(bool, ListView_SetItemState,(HWND h, int item, int state, int statemask))
SWELL_API_DEFINE(void, ListView_RedrawItems,(HWND h, int startitem, int enditem))
SWELL_API_DEFINE(void, ListView_SetItemCount,(HWND h, int cnt))
SWELL_API_DEFINE(void, ListView_EnsureVisible,(HWND h, int i, BOOL pok))
SWELL_API_DEFINE(bool, ListView_GetSubItemRect,(HWND h, int item, int subitem, int code, RECT *r))
SWELL_API_DEFINE(void, ListView_SetImageList,(HWND h, HIMAGELIST imagelist, int which)) 
SWELL_API_DEFINE(int, ListView_HitTest,(HWND h, LVHITTESTINFO *pinf))

#ifndef ImageList_Create
#define ImageList_Create(x,y,a,b,c) ImageList_CreateEx();
#endif
SWELL_API_DEFINE(HIMAGELIST, ImageList_CreateEx,())
SWELL_API_DEFINE(void, ImageList_ReplaceIcon,(HIMAGELIST list, int offset, HICON image))

/*
** TabCtrl api. 
*/
SWELL_API_DEFINE(int, TabCtrl_GetItemCount,(HWND hwnd))
SWELL_API_DEFINE(BOOL, TabCtrl_DeleteItem,(HWND hwnd, int idx))
SWELL_API_DEFINE(int, TabCtrl_InsertItem,(HWND hwnd, int idx, TCITEM *item))
SWELL_API_DEFINE(int, TabCtrl_SetCurSel,(HWND hwnd, int idx))
SWELL_API_DEFINE(int, TabCtrl_GetCurSel,(HWND hwnd))
SWELL_API_DEFINE(BOOL, TabCtrl_AdjustRect, (HWND hwnd, BOOL fLarger, RECT *r))

/*
** These are deprecated functions for launching a modal window but still running
** your own code. In general use DialogBox with a timer if needed instead.
*/
SWELL_API_DEFINE(void *, SWELL_ModalWindowStart,(HWND hwnd))
SWELL_API_DEFINE(bool, SWELL_ModalWindowRun,(void *ctx, int *ret)) // returns false and puts retval in *ret when done
SWELL_API_DEFINE(void, SWELL_ModalWindowEnd,(void *ctx))
SWELL_API_DEFINE(void, SWELL_CloseWindow,(HWND hwnd))


/*
** Menu functions
** HMENU is a NSMenu *.
*/
SWELL_API_DEFINE(HMENU, CreatePopupMenu,())
SWELL_API_DEFINE(HMENU, CreatePopupMenuEx,(const char *title))
SWELL_API_DEFINE(void, DestroyMenu,(HMENU hMenu))
SWELL_API_DEFINE(int, AddMenuItem,(HMENU hMenu, int pos, const char *name, int tagid))
SWELL_API_DEFINE(HMENU, GetSubMenu,(HMENU hMenu, int pos))
SWELL_API_DEFINE(int, GetMenuItemCount,(HMENU hMenu))
SWELL_API_DEFINE(int, GetMenuItemID,(HMENU hMenu, int pos))
SWELL_API_DEFINE(bool, SetMenuItemModifier,(HMENU hMenu, int idx, int flag, void *ModNSS, unsigned int mask))
SWELL_API_DEFINE(bool, SetMenuItemText,(HMENU hMenu, int idx, int flag, const char *text))
SWELL_API_DEFINE(bool, EnableMenuItem,(HMENU hMenu, int idx, int en))
SWELL_API_DEFINE(bool, DeleteMenu,(HMENU hMenu, int idx, int flag))
SWELL_API_DEFINE(bool, CheckMenuItem,(HMENU hMenu, int idx, int chk))
SWELL_API_DEFINE(void, InsertMenuItem,(HMENU hMenu, int pos, BOOL byPos, MENUITEMINFO *mi))
SWELL_API_DEFINE(BOOL, GetMenuItemInfo,(HMENU hMenu, int pos, BOOL byPos, MENUITEMINFO *mi))
SWELL_API_DEFINE(BOOL, SetMenuItemInfo,(HMENU hMenu, int pos, BOOL byPos, MENUITEMINFO *mi))



/*
** LoadMenu()
**  Loads a menu created with SWELL_DEFINE_MENU_RESOURCE_BEGIN(), see swell-menugen.h
**  Notes: the hinst parameter is ignored, the menu must have been defined in the same 
**  APP or DYLIB as the LoadMenu call. If you wish to load a menu from another module, 
**  you should somehow get its SWELL_curmodule_menuresource_head and pass it to SWELL_LoadMenu
**  directly.
*/
#ifndef LoadMenu
#define LoadMenu(hinst,resid) SWELL_LoadMenu(SWELL_curmodule_menuresource_head,(resid))
#endif
SWELL_API_DEFINE(HMENU, SWELL_LoadMenu,(struct SWELL_MenuResourceIndex *head, int resid))

/*
** TrackPopupMenu
** Notes: the rectangle and many flags are ignored, but TPM_NONOTIFY is used.
** It always returns the command selected, even if TPM_RETURNCMD is not specified.
*/
SWELL_API_DEFINE(int, TrackPopupMenu,(HMENU hMenu, int flags, int xpos, int ypos, int resvd, HWND hwnd, const RECT *r))

/* 
** SWELL_SetMenuDestination: set the action destination for all items in a menu 
** Notes:
**   TrackPopupMenu and SetMenu use this internally, but it may be useful.
*/
SWELL_API_DEFINE(void, SWELL_SetMenuDestination,(HMENU menu, HWND hwnd))

/*
** SWELL_DuplicateMenu:
** Copies an entire menu.
*/
SWELL_API_DEFINE(HMENU, SWELL_DuplicateMenu,(HMENU menu))  

/*
** SetMenu()/GetMenu()
** Notes: These work on SWELL created NSWindows, or objective C objects that respond to
** swellSetMenu:(HMENU) and swellGetMenu. 
** Setting these values doesnt mean anything, except that the SWELL windows will automatically
** set the application menu via NSApp setMainMenu: when activated.
** 
*/
SWELL_API_DEFINE(BOOL, SetMenu,(HWND hwnd, HMENU menu))
SWELL_API_DEFINE(HMENU, GetMenu,(HWND hwnd))

/*
** SWELL_SetDefaultWindowMenu()/SWELL_GetDefaultWindowMenu()
** these set an internal flag for the default window menu, which will be set 
** when switching to a window that has no menu set. Set this to your application's
** main menu.
*/
SWELL_API_DEFINE(HMENU, SWELL_GetDefaultWindowMenu,())
SWELL_API_DEFINE(void, SWELL_SetDefaultWindowMenu,(HMENU))




/* 
** SWELL dialog box/control/window/child dialog/etc creation
** DialogBox(), DialogBoxParam(), CreateDialog(), and CreateDialogParam()
**
** Notes:
** hInstance parameters are ignored. If you wish to load a dialog resource from another
** module (DYLIB), you should get its SWELL_curmodule_dialogresource_head via your own 
** mechanism and pass it as the first parameter of SWELL_DialogBox or whichever API.
** 
** If you are using CreateDialog() and creating a child window, you can use a resource ID of 
** 0, which creates an opaque child window. Instead of passing a DLGPROC, you should pass a 
** routine that retuns LRESULT (and cast it to DLGPROC).
** 
*/

#ifndef DialogBox
#define DialogBox(hinst, resid, par, dlgproc) SWELL_DialogBox(SWELL_curmodule_dialogresource_head,resid,par,dlgproc,0)
#define DialogBoxParam(hinst, resid, par, dlgproc, param) SWELL_DialogBox(SWELL_curmodule_dialogresource_head,resid,par,dlgproc,param)
#define CreateDialog(hinst,resid,par,dlgproc) SWELL_CreateDialog(SWELL_curmodule_dialogresource_head,resid,par,dlgproc,0)
#define CreateDialogParam(hinst,resid,par,dlgproc,param) SWELL_CreateDialog(SWELL_curmodule_dialogresource_head,resid,par,dlgproc,param)
#endif
SWELL_API_DEFINE(int, SWELL_DialogBox,(struct SWELL_DialogResourceIndex *reshead, int resid, HWND parent,  DLGPROC dlgproc, LPARAM param))  
SWELL_API_DEFINE(HWND, SWELL_CreateDialog,(struct SWELL_DialogResourceIndex *reshead, int resid, HWND parent, DLGPROC dlgproc, LPARAM param))


/*
** SWELL_RegisterCustomControlCreator(), SWELL_UnregisterCustomControlCreator()
** Notes:
** Pass these a callback function that can create custom controls based on classname.
** Todo: document.
*/

SWELL_API_DEFINE(void, SWELL_RegisterCustomControlCreator,(SWELL_ControlCreatorProc proc))
SWELL_API_DEFINE(void, SWELL_UnregisterCustomControlCreator,(SWELL_ControlCreatorProc proc))

/*
** DefWindowProc(). 
** Notes: Doesnt do much but call it anyway from any child windows created with CreateDialog 
** and a 0 resource-id window proc.
*/
SWELL_API_DEFINE(long, DefWindowProc,(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam))
                 
/*
** EndDialog():
** Notes: _ONLY_ use on a dialog created with DialogBox(). Will do nothing on other dialogs.
*/
SWELL_API_DEFINE(void, EndDialog,(HWND, int))            



/*
** SendMessage()
** Notes:
**   LIMITATION - SendMessage should only be used from the same thread that the window/view
**   was created in. Cross-thread use SHOULD BE AVOIDED. It may work, but it may blow up.
**   PostMessage  (below) can be used in certain instances for asynchronous notifications.
** 
** If the hwnd supports onSwellMessage:p1:p2: then the message is sent via this.
** Alternatively, buttons created via the dialog interface support BM_GETIMAGE/BM_SETIMAGE
** NSPopUpButton and NSComboBox controls support CB_*
** NSSlider controls support TBM_*
** If the receiver is a view and none of these work, the message goes to the window's onSwellMessage, if any
** If the receiver is a window and none of these work, the message goes to the window's contentview's onSwellMessage, if any
**
*/
SWELL_API_DEFINE(int, SendMessage,(HWND, UINT, WPARAM, LPARAM))  
#ifndef SendDlgItemMessage                                       
#define SendDlgItemMessage(hwnd,idx,msg,wparam,lparam) SendMessage(GetDlgItem(hwnd,idx),msg,wparam,lparam)
#endif


/*
** PostMessage()
** Notes:
**   Queues a message into the application message queue. Note that you should only ever
**   send messages to destinations that were created from the main thread. They will be 
**   processed later from a timer (in the main thread).
** When a window is destroyed any outstanding messages will be discarded for it.
*/
SWELL_API_DEFINE(BOOL, PostMessage,(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam))

/*
** SWELL_MessageQueue_Flush():
** Notes:
** Processes all messages in the message queue. ONLY call from the main thread.
*/
SWELL_API_DEFINE(void, SWELL_MessageQueue_Flush,())

/*
** SWELL_MessageQueue_Clear():
** Notes:
** Discards all messages from the message queue if h is NULL, otherwise discards all messages
** to h.
*/
SWELL_API_DEFINE(void, SWELL_MessageQueue_Clear,(HWND h))



/*
** keyboard/mouse support
*/

/*
** SWELL_MacKeyToWindowsKey()
** Pass a keyboard NSEvent *, and it will return a windows VK_ keycode (or ascii), and set flags, 
** including (possibly) FSHIFT, FCONTROL (apple key), FALT, and FVIRTKEY. The ctrl key is not checked,
** as SWELL generally encourages this to be used soley for a right mouse button (as modifier).
*/
SWELL_API_DEFINE(int, SWELL_MacKeyToWindowsKey,(void *nsevent, int *flags))

/*
** GetAsyncKeyState()
** Notes: only supports MK_LBUTTON, MK_RBUTTON, MK_MBUTTON, VK_SHIFT, VK_MENU, and VK_CONTROL (apple key) for now.
*/
SWELL_API_DEFINE(WORD, GetAsyncKeyState,(int key))

/*
** GetCursorPos(), GetMessagePos()
** Notes: GetMessagePos() currently returns the same coordinates as GetCursorPos(),
** this needs to be fixed.
*/
SWELL_API_DEFINE(void, GetCursorPos,(POINT *pt))
SWELL_API_DEFINE(DWORD, GetMessagePos,())

/*
** LoadCursor(). 
** Notes: hinstance parameter ignored, currently only supports loading some of the predefined values.
** (IDC_SIZEALL etc). If it succeeds value is a NSCursor *
*/
SWELL_API_DEFINE(HCURSOR, SWELL_LoadCursor,(int idx))
#ifndef LoadCursor
#define LoadCursor(a,x) SWELL_LoadCursor(x)
#endif

/*
** SetCursor()
** Sets a cursor as active (can be HCURSOR or NSCursor * cast as such).
*/
#ifdef SetCursor
#undef SetCursor
#endif
#define SetCursor(x) SWELL_SetCursor(x)
SWELL_API_DEFINE(void, SWELL_SetCursor,(HCURSOR curs))


#ifdef GetCursor
#undef GetCursor
#endif
#define GetCursor SWELL_GetCursor
/*
** GetCursor() gets the actual system cursor,
** SWELL_GetLastSetCursor() gets the last cursor set via SWELL (if they differ than some other window must have changed the cursor)
*/
SWELL_API_DEFINE(HCURSOR, SWELL_GetCursor,())
SWELL_API_DEFINE(HCURSOR, SWELL_GetLastSetCursor,())


/*
** SWELL_GetViewPort
** Gets screen information, for the screen that contains sourcerect. if wantWork is set
** it excluses the menu bar etc.
*/
SWELL_API_DEFINE(void, SWELL_GetViewPort,(RECT *r, RECT *sourcerect, bool wantWork))

/*
** Clipboard API emulation
** Notes: setting multiple types may not work right
** 
*/
SWELL_API_DEFINE(bool, OpenClipboard,(HWND hwndDlg))
SWELL_API_DEFINE(void, CloseClipboard,())
SWELL_API_DEFINE(HANDLE, GetClipboardData,(UINT type))

SWELL_API_DEFINE(void, EmptyClipboard,())
SWELL_API_DEFINE(void, SetClipboardData,(UINT type, HANDLE h))
SWELL_API_DEFINE(UINT, RegisterClipboardFormat,(const char *desc))
SWELL_API_DEFINE(UINT, EnumClipboardFormats,(UINT lastfmt))

/*
** GlobalAlloc*() 
** These are only currently used by the clipboard system,
** but should work normally. 
*/

SWELL_API_DEFINE(HANDLE, GlobalAlloc,(int flags, int sz))
SWELL_API_DEFINE(void *, GlobalLock,(HANDLE h))
SWELL_API_DEFINE(int, GlobalSize,(HANDLE h))
SWELL_API_DEFINE(void, GlobalUnlock,(HANDLE h))
SWELL_API_DEFINE(void, GlobalFree,(HANDLE h))



/*
** GDI functions.
** Everything should be all hunky dory, your windows may get WM_PAINT, call 
** GetDC()/ReleaseDC(), etc.
**
** Or, there are these helper functions:
*/

/*
** SWELL_CreateContext()
** pass a CGContextRef, and it will create a "HDC" for it.
*/

SWELL_API_DEFINE(HDC, SWELL_CreateContext,(void *))

/*
** SWELL_CreateMemContext()
** Creates a memory context (that you can get the bits for, below)
** hdc is currently ignored.
*/
SWELL_API_DEFINE(HDC, SWELL_CreateMemContext,(HDC hdc, int w, int h))

/*
** SWELL_DeleteContext()
** Deletes a context created with SWELL_CreateContext() or SWELL_CreateMemContext()
*/
SWELL_API_DEFINE(void, SWELL_DeleteContext,(HDC))

/*
** SWELL_GetCtxGC()
** Returns the CGContextRef of a HDC
*/
SWELL_API_DEFINE(void *, SWELL_GetCtxGC,(HDC ctx))


/*
** SWELL_GetCtxFrameBuffer()
** Gets the framebuffer of a memory context. NULL if none available.
*/
SWELL_API_DEFINE(void *, SWELL_GetCtxFrameBuffer,(HDC ctx))

/*
** SWELL_SyncCtxFrameBuffer()
** If you modify the framebuffer, and will BitBlt() from this memory DC,
** you will probably want to call this before blitting. Note that you may
** not always HAVE to (it may work if you don't), but you SHOULD.
*/
SWELL_API_DEFINE(void, SWELL_SyncCtxFrameBuffer,(HDC ctx))

/* 
** Some utility functions for pushing, setting, and popping the clip region. 
*/
SWELL_API_DEFINE(void, SWELL_PushClipRegion,(HDC ctx))
SWELL_API_DEFINE(void, SWELL_SetClipRegion,(HDC ctx, RECT *r))
SWELL_API_DEFINE(void, SWELL_PopClipRegion,(HDC ctx))


/* 
** GDI emulation functions
** todo: document
*/

SWELL_API_DEFINE(HFONT, CreateFontIndirect,(LOGFONT *))
SWELL_API_DEFINE(HFONT, CreateFont,(long lfHeight, long lfWidth, long lfEscapement, long lfOrientation, long lfWeight, char lfItalic, 
  char lfUnderline, char lfStrikeOut, char lfCharSet, char lfOutPrecision, char lfClipPrecision, 
         char lfQuality, char lfPitchAndFamily, const char *lfFaceName))

SWELL_API_DEFINE(HPEN, CreatePen,(int attr, int wid, int col))
SWELL_API_DEFINE(HBRUSH, CreateSolidBrush,(int col))
SWELL_API_DEFINE(HPEN, CreatePenAlpha,(int attr, int wid, int col, float alpha))
SWELL_API_DEFINE(HBRUSH, CreateSolidBrushAlpha,(int col, float alpha))
SWELL_API_DEFINE(HGDIOBJ, SelectObject,(HDC ctx, HGDIOBJ pen))
SWELL_API_DEFINE(HGDIOBJ, GetStockObject,(int wh))
SWELL_API_DEFINE(void, DeleteObject,(HGDIOBJ))
#ifndef DestroyIcon
#define DestroyIcon(x) DeleteObject(x)
#endif

#ifdef LineTo
#undef LineTo
#endif
#ifdef SetPixel
#undef SetPixel
#endif
#ifdef FillRect
#undef FillRect
#endif
#ifdef DrawText
#undef DrawText
#endif
#ifdef Polygon
#undef Polygon
#endif

#define DrawText SWELL_DrawText
#define FillRect SWELL_FillRect
#define LineTo SWELL_LineTo
#define SetPixel SWELL_SetPixel
#define Polygon(a,b,c) SWELL_Polygon(a,b,c)

SWELL_API_DEFINE(void, SWELL_FillRect,(HDC ctx, RECT *r, HBRUSH br))
SWELL_API_DEFINE(void, Rectangle,(HDC ctx, int l, int t, int r, int b))
SWELL_API_DEFINE(void, Ellipse,(HDC ctx, int l, int t, int r, int b))
SWELL_API_DEFINE(void, SWELL_Polygon,(HDC ctx, POINT *pts, int npts))
SWELL_API_DEFINE(void, MoveToEx,(HDC ctx, int x, int y, POINT *op))
SWELL_API_DEFINE(void, LineTo,(HDC ctx, int x, int y))
SWELL_API_DEFINE(void, SetPixel,(HDC ctx, int x, int y, int c))
SWELL_API_DEFINE(void, PolyBezierTo,(HDC ctx, POINT *pts, int np))
SWELL_API_DEFINE(int, SWELL_DrawText,(HDC ctx, const char *buf, int len, RECT *r, int align))
SWELL_API_DEFINE(void, SetTextColor,(HDC ctx, int col))
SWELL_API_DEFINE(void, SetBkColor,(HDC ctx, int col))
SWELL_API_DEFINE(void, SetBkMode,(HDC ctx, int col))

SWELL_API_DEFINE(void, RoundRect,(HDC ctx, int x, int y, int x2, int y2, int xrnd, int yrnd))
SWELL_API_DEFINE(void, PolyPolyline,(HDC ctx, POINT *pts, DWORD *cnts, int nseg))
SWELL_API_DEFINE(BOOL, GetTextMetrics,(HDC ctx, TEXTMETRIC *tm))
SWELL_API_DEFINE(void *, GetNSImageFromHICON,(HICON))
SWELL_API_DEFINE(HICON, LoadNamedImage,(const char *name, bool alphaFromMask))
SWELL_API_DEFINE(void, DrawImageInRect,(HDC ctx, HICON img, RECT *r))
SWELL_API_DEFINE(void, BitBlt,(HDC hdcOut, int x, int y, int w, int h, HDC hdcIn, int xin, int yin, int mode))
SWELL_API_DEFINE(void, StretchBlt,(HDC hdcOut, int x, int y, int w, int h, HDC hdcIn, int xin, int yin, int srcw, int srch, int mode))
SWELL_API_DEFINE(int, GetSysColor,(int idx))

SWELL_API_DEFINE(HDC, BeginPaint,(HWND, PAINTSTRUCT *))
SWELL_API_DEFINE(BOOL, EndPaint,(HWND, PAINTSTRUCT *))

SWELL_API_DEFINE(HDC, GetDC,(HWND)) // use these sparingly! they kinda work but shouldnt be overused!!
SWELL_API_DEFINE(HDC, GetWindowDC,(HWND)) 
SWELL_API_DEFINE(void, ReleaseDC,(HWND, HDC))
            





/*
** Functions used by swell-dlggen.h and swell-menugen.h
** No need to really dig into these unless you're working on swell or debugging..
*/

SWELL_API_DEFINE(void, SWELL_MakeSetCurParms,(float xscale, float yscale, float xtrans, float ytrans, HWND parent, bool doauto, bool dosizetofit))
SWELL_API_DEFINE(void, SWELL_Make_SetMessageMode,(int wantParentView))

SWELL_API_DEFINE(HWND, SWELL_MakeButton,(int def, const char *label, int idx, int x, int y, int w, int h, int flags))
SWELL_API_DEFINE(HWND, SWELL_MakeEditField,(int idx, int x, int y, int w, int h, int flags))
SWELL_API_DEFINE(HWND, SWELL_MakeLabel,(int align, const char *label, int idx, int x, int y, int w, int h, int flags))
SWELL_API_DEFINE(HWND, SWELL_MakeControl,(const char *cname, int idx, const char *classname, int style, int x, int y, int w, int h))
SWELL_API_DEFINE(HWND, SWELL_MakeCombo,(int idx, int x, int y, int w, int h, int flags))
SWELL_API_DEFINE(HWND, SWELL_MakeGroupBox,(const char *name, int idx, int x, int y, int w, int h))
SWELL_API_DEFINE(HWND, SWELL_MakeCheckBox,(const char *name, int idx, int x, int y, int w, int h))
SWELL_API_DEFINE(HWND, SWELL_MakeListBox,(int idx, int x, int y, int w, int h, int styles))

SWELL_API_DEFINE(void, SWELL_Menu_AddMenuItem,(HMENU hMenu, const char *name, int idx, int flags))




#endif // _WDL_SWELL_H_API_DEFINED_


#ifndef SWELL_PROVIDED_BY_APP
#ifndef _WDL_SWELL_H_UTIL_DEFINED_
#define _WDL_SWELL_H_UTIL_DEFINED_

// these should never be called directly!!! put SWELL_POSTMESSAGE_DELEGATE_IMPL in your nsapp delegate, and call SWELL_POSTMESSAGE_INIT at some point from there too
void SWELL_Internal_PostMessage_Init();
BOOL SWELL_Internal_PostMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void SWELL_Internal_PMQ_ClearAllMessages(HWND hwnd);
                 
#define SWELL_POSTMESSAGE_DELEGATE_IMPL \
                 -(bool)swellPostMessage:(HWND)dest msg:(int)message wp:(WPARAM)wParam lp:(LPARAM)lParam { \
                   return SWELL_Internal_PostMessage(dest,message,wParam,lParam); \
                 } \
                 -(void)swellPostMessageClearQ:(HWND)dest { \
                   SWELL_Internal_PMQ_ClearAllMessages(dest); \
                 } \
                 -(void)swellPostMessageTick:(id)sender { \
                   SWELL_MessageQueue_Flush(); \
                 } 
                 
#define SWELL_POSTMESSAGE_INIT SWELL_Internal_PostMessage_Init();


// if you use this then include swell-appstub.mm in your project
#define SWELL_APPAPI_DELEGATE_IMPL \
                -(void *)swellGetAPPAPIFunc { \
                    void *SWELLAPI_GetFunc(const char *name); \
                    return (void*)SWELLAPI_GetFunc; \
                }

#endif // _WDL_SWELL_H_UTIL_DEFINED_
#endif // !SWELL_PROVIDED_BY_APP

#endif // !_WIN32


#ifdef _WIN32 // deprecated SWELL_CB_ / SWELL_TB_ defines for windows

#ifndef SWELL_CB_InsertString

#define SWELL_CB_InsertString(hwnd, idx, pos, str) SendDlgItemMessage(hwnd,idx,CB_INSERTSTRING,(pos),(LPARAM)(str))
#define SWELL_CB_AddString(hwnd, idx, str) SendDlgItemMessage(hwnd,idx,CB_ADDSTRING,0,(LPARAM)(str))
#define SWELL_CB_SetCurSel(hwnd,idx,val) SendDlgItemMessage(hwnd,idx,CB_SETCURSEL,(WPARAM)(val),0)
#define SWELL_CB_GetNumItems(hwnd,idx) SendDlgItemMessage(hwnd,idx,CB_GETCOUNT,0,0)
#define SWELL_CB_GetCurSel(hwnd,idx) SendDlgItemMessage(hwnd,idx,CB_GETCURSEL,0,0)
#define SWELL_CB_SetItemData(hwnd,idx,item,val) SendDlgItemMessage(hwnd,idx,CB_SETITEMDATA,(item),(val))
#define SWELL_CB_GetItemData(hwnd,idx,item) SendDlgItemMessage(hwnd,idx,CB_GETITEMDATA,(item),0)
#define SWELL_CB_GetItemText(hwnd,idx,item,buf,bufsz) SendDlgItemMessage(hwnd,idx,CB_GETLBTEXT,(item),(LPARAM)(buf))
#define SWELL_CB_Empty(hwnd,idx) SendDlgItemMessage(hwnd,idx,CB_RESETCONTENT,0,0)
#define SWELL_CB_DeleteString(hwnd,idx,str) SendDlgItemMessage(hwnd,idx,CB_DELETESTRING,str,0)

#define SWELL_TB_SetPos(hwnd, idx, pos) SendDlgItemMessage(hwnd,idx, TBM_SETPOS,TRUE,(pos))
#define SWELL_TB_SetRange(hwnd, idx, low, hi) SendDlgItemMessage(hwnd,idx,TBM_SETRANGE,TRUE,(LPARAM)MAKELONG((low),(hi)))
#define SWELL_TB_GetPos(hwnd, idx) SendDlgItemMessage(hwnd,idx,TBM_GETPOS,0,0)
#define SWELL_TB_SetTic(hwnd, idx, pos) SendDlgItemMessage(hwnd,idx,TBM_SETTIC,0,(pos))

#endif

#endif//_WIN32




#ifndef WDL_GDP_CTX                // stupid GDP compatibility layer, deprecated

#define WDL_GDP_CreateContext(x) SWELL_CreateContext(x)
#define WDL_GDP_CreateMemContext(hdc,w,h) SWELL_CreateMemContext(hdc,w,h)
#define WDL_GDP_DeleteContext(x) SWELL_DeleteContext(x)

#define WDL_GDP_CTX HDC
#define WDL_GDP_PEN HPEN
#define WDL_GDP_BRUSH HBRUSH
#define WDL_GDP_CreatePen(col, wid) (WDL_GDP_PEN)CreatePen(PS_SOLID,(wid),(col))
#define WDL_GDP_DeletePen(pen) DeleteObject((HGDIOBJ)(pen))
#define WDL_GDP_SetPen(ctx, pen) ((WDL_GDP_PEN)SelectObject(ctx,(HGDIOBJ)(pen)))
#define WDL_GDP_SetBrush(ctx, brush) ((WDL_GDP_BRUSH)SelectObject(ctx,(HGDIOBJ)(brush)))
#define WDL_GDP_CreateBrush(col) (WDL_GDP_BRUSH)CreateSolidBrush(col)
#define WDL_GDP_DeleteBrush(brush) DeleteObject((HGDIOBJ)(brush))
#define WDL_GDP_FillRectWithBrush(hdc,r,br) FillRect(hdc,r,(HBRUSH)(br))
#define WDL_GDP_Rectangle(hdc,l,t,r,b) Rectangle(hdc,l,t,r,b)
#define WDL_GDP_Polygon(hdc,pts,n) Polygon(hdc,pts,n)
#define WDL_GDP_MoveToEx(hdc,x,y,op) MoveToEx(hdc,x,y,op)
#define WDL_GDP_LineTo(hdc,x,y) LineTo(hdc,x,y)
#define WDL_GDP_PutPixel(hdc,x,y,c) SetPixel(hdc,x,y,c)
#define WDL_GDP_PolyBezierTo(hdc,p,np) PolyBezierTo(hdc,p,np)

#endif



