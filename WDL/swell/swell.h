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


#ifndef _WDL_SWELL_H_
#define _WDL_SWELL_H_
#ifndef _WIN32

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define RGB(r,g,b) (((r)<<16)|((g)<<8)|(b))
#define GetRValue(x) (((x)>>16)&0xff)
#define GetGValue(x) (((x)>>8)&0xff)
#define GetBValue(x) ((x)&0xff)

#define stricmp(x,y) strcasecmp(x,y)
#define strnicmp(x,y,z) strncasecmp(x,y,z)

#define DeleteFile(x) (!unlink(x))
#define MoveFile(x,y) (!rename(x,y))
#define GetCurrentDirectory(sz,buf) (!getcwd(buf,sz))
#define CreateDirectory(x,y) (!mkdir((x),0755))
#define wsprintf sprintf

#ifndef max
#define max(x,y) ((x)<(y)?(y):(x))
#define min(x,y) ((x)<(y)?(x):(y))
#endif



typedef struct 
{
  int x,y;
} POINT, *LPPOINT;

typedef struct 
{
  int left,top;
  int right, bottom;
} RECT, *LPRECT;


typedef struct {
  unsigned char fVirt;
  unsigned short key,cmd;
} ACCEL, *LPACCEL;

typedef struct {
  int bla;
} MSG, *LPMSG;

typedef struct {
  int bla;
} __HWND;
typedef __HWND *HWND;

typedef unsigned int DWORD;
typedef unsigned short WORD;
typedef unsigned char BYTE;
typedef signed char BOOL;
typedef void *WNDPROC;
typedef void *HANDLE;
typedef void *HMENU;
typedef void *HINSTANCE;
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

#ifdef MAC
#include <dlfcn.h>
#define FreeLibrary(x) dlclose(x)
#define GetProcAddress(h,v) dlsym(h,v)
#define LoadLibrary(x) dlopen(x,RTLD_LAZY)
#endif

char *lstrcpyn(char *dest, const char *src, int l);
void Sleep(int ms);
DWORD GetTickCount();
#define timeGetTime() GetTickCount()
BOOL GetFileTime(void *fh, FILETIME *lpCreationTime, FILETIME *lpLastAccessTime, FILETIME *lpLastWriteTime);

BOOL WritePrivateProfileString(const char *appname, const char *keyname, const char *val, const char *fn);
DWORD GetPrivateProfileString(const char *appname, const char *keyname, const char *def, char *ret, int retsize, const char *fn);
int GetPrivateProfileInt(const char *appname, const char *keyname, int def, const char *fn);
BOOL GetPrivateProfileStruct(const char *appname, const char *keyname, void *buf, int bufsz, const char *fn);
BOOL WritePrivateProfileStruct(const char *appname, const char *keyname, const void *buf, int bufsz, const char *fn);


/*
 ** swell-miscdlg.mm
 */
#define MB_OK 0
#define MB_OKCANCEL 1
#define MB_YESNOCANCEL 3
#define MB_YESNO 4
#define MB_RETRYCANCEL 5
#define IDCANCEL 0
#define IDOK 1
#define IDNO 2
#define IDYES 3
#define IDRETRY 4
#define MessageBox(a,b,c,d) SWELL_MessageBox(b,c,d)
int SWELL_MessageBox(const char *text, const char *caption, int type);

// extlist is something similar you'd pass getopenfilename, initialdir and initialfile are optional
char *BrowseForFiles(const char *text, const char *initialdir, 
					const char *initialfile, bool allowmul, const char *extlist); // free() the result of this

bool BrowseForSaveFile(const char *text, const char *initialdir, const char *initialfile, const char *extlist,
			char *fn, int fnsize);

bool BrowseForDirectory(const char *text, const char *initialdir, char *fn, int fnsize);

/*
 ** swell-wnd.mm
 */

// all of these treat HWND as NSView 
HWND GetDlgItem(HWND, int);
void ShowWindow(HWND, int);
void DestroyWindow(HWND hwnd); // can also take NSWindow *
void SetDlgItemText(HWND, int idx, const char *text);
void SetDlgItemInt(HWND, int idx, int val, int issigned);
int GetDlgItemInt(HWND, int idx, BOOL *translated, int issigned);
void GetDlgItemText(HWND, int idx, char *text, int textlen);
#define GetWindowText(hwnd,text,textlen) GetDlgItemText(hwnd,0,text,textlen)
void CheckDlgButton(HWND hwnd, int idx, int check);
int IsDlgButtonChecked(HWND hwnd, int idx);
void EnableWindow(HWND hwnd, int enable);
void SetFocus(HWND hwnd); // these take NSWindow/NSView, and return NSView *
HWND GetFocus();
int IsChild(HWND hwndParent, HWND hwndChild);
HWND GetParent(HWND hwnd);
HWND SetParent(HWND hwnd, HWND newPar);
void ClientToScreen(HWND hwnd, POINT *p);
void ScreenToClient(HWND hwnd, POINT *p);
void GetWindowRect(HWND hwnd, RECT *r);
void GetClientRect(HWND hwnd, RECT *r);
void SetWindowPos(HWND hwnd, HWND unused, int x, int y, int cx, int cy, int flags);
int GetWindowLong(HWND hwnd, int idx);
#define GWL_ID -1000

void SetTimer(HWND hwnd, int timerid, int rate, unsigned long *notUsed);
void KillTimer(HWND hwnd, int timerid);

int SWELL_CB_AddString(HWND hwnd, int idx, const char *str);
void SWELL_CB_SetCurSel(HWND hwnd, int idx, int sel);
int SWELL_CB_GetCurSel(HWND hwnd, int idx);
int SWELL_CB_GetNumItems(HWND hwnd, int idx);
void SWELL_CB_SetItemData(HWND hwnd, int idx, int item, int data); // these two only work for the combo list version for now
int SWELL_CB_GetItemData(HWND hwnd, int idx, int item);
void SWELL_CB_Empty(HWND hwnd, int idx);
int SWELL_CB_InsertString(HWND hwnd, int idx, int pos, const char *str);
int SWELL_CB_GetItemText(HWND hwnd, int idx, int item, char *buf, int bufsz);

void SWELL_TB_SetPos(HWND hwnd, int idx, int pos);
void SWELL_TB_SetRange(HWND hwnd, int idx, int low, int hi);
int SWELL_TB_GetPos(HWND hwnd, int idx);
void SWELL_TB_SetTic(HWND hwnd, int idx, int pos);


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

void ListView_SetExtendedListViewStyleEx(HWND h, int flag, int mask);
void ListView_InsertColumn(HWND h, int pos, const LVCOLUMN *lvc);
int ListView_InsertItem(HWND h, const LVITEM *item);
void ListView_SetItemText(HWND h, int ipos, int cpos, const char *txt);
int ListView_GetNextItem(HWND h, int istart, int flags);
bool ListView_GetItem(HWND h, LVITEM *item);
int ListView_GetItemState(HWND h, int ipos, int mask);
void ListView_DeleteItem(HWND h, int ipos);
void ListView_DeleteAllItems(HWND h);
int ListView_GetSelectedCount(HWND h);
int ListView_GetItemCount(HWND h);
int ListView_GetSelectionMark(HWND h);
void ListView_SetColumnWidth(HWND h, int colpos, int wid);

#define LVCF_TEXT 1
#define LVCF_WIDTH 2
#define LVIF_TEXT 1
#define LVIF_PARAM 2
#define LVIF_STATE 4
#define LVIS_SELECTED 1
#define LVIS_FOCUSED 2
#define LVNI_FOCUSED 1

// ignored for now
#define LVS_EX_FULLROWSELECT 0



#define BST_CHECKED 1
#define BST_UNCHECKED 0
#define SW_HIDE 0
#define SW_SHOWNA 1
#define SW_SHOW 2

#define SWP_NOMOVE 1
#define SWP_NOSIZE 2
#define SWP_NOZORDER 4
#define SWP_NOACTIVATE 8
#define HWND_TOP (HWND)0
#define HWND_TOPMOST (HWND)0
#define HWND_BOTTOM (HWND)0
#define HWND_NOTOPMOST (HWND)0

void *SWELL_ModalWindowStart(HWND hwnd);
bool SWELL_ModalWindowRun(void *ctx, int *ret); // returns false and puts retval in *ret when done
void SWELL_ModalWindowEnd(void *ctx);

void SWELL_CloseWindow(HWND hwnd);

/*
 ** swell-menu.mm
 */
HMENU CreatePopupMenu();
void DestroyMenu(HMENU hMenu);
int AddMenuItem(HMENU hMenu, int pos, const char *name, int tagid);
int SWELL_TrackPopupMenu(HMENU hMenu, int xpos, int ypos, HWND hwnd);
#define TrackPopupMenu(menu, flags, xpos, ypos, resvd, parent, rect) SWELL_TrackPopupMenu(menu,xpos,ypos,parent)
HMENU GetSubMenu(HMENU hMenu, int pos);
int GetMenuItemCount(HMENU hMenu);
int GetMenuItemID(HMENU hMenu, int pos);
bool SetMenuItemModifier(HMENU hMenu, int idx, int flag, void *ModNSS, unsigned int mask);
bool SetMenuItemText(HMENU hMenu, int idx, int flag, const char *text);
bool EnableMenuItem(HMENU hMenu, int idx, int en);
bool DeleteMenu(HMENU hMenu, int idx, int flag);

bool CheckMenuItem(HMENU hMenu, int idx, int chk);
typedef struct
{
  unsigned int cbSize, fMask, fType, fState, wID;
  HMENU hSubMenu;
  void *hbmpChecked,*hbmpUnchecked;
  DWORD dwItemData;
  char *dwTypeData;
  int cch;
} MENUITEMINFO;
void InsertMenuItem(HMENU hMenu, int pos, BOOL byPos, MENUITEMINFO *mi);
BOOL GetMenuItemInfo(HMENU hMenu, int pos, BOOL byPos, MENUITEMINFO *mi);
BOOL SetMenuItemInfo(HMENU hMenu, int pos, BOOL byPos, MENUITEMINFO *mi);
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
#define MF_CHECKED 4

#define MFS_UNCHECKED 0
#define MFS_ENABLED 0
#define MFS_GRAYED 1
#define MFS_CHECKED 4

#define MF_BYCOMMAND 0
#define MF_BYPOSITION 0x100




/*
 ** swell-kb.mm
 */
int MacKeyToWindowsKey(void *nsevent, int *flags);
WORD GetAsyncKeyState(int key);
void GetCursorPos(POINT *pt);

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

/*
 ** swell-gdi.mm
 */

typedef void *HDC;
typedef void *HBRUSH;
typedef void *HPEN;
typedef void *HFONT;
typedef void *HICON;
typedef void *HBITMAP;
typedef void *HGDIOBJ;
typedef void *HCURSOR;
typedef struct
{
    long lfHeight, lfWidth, lfEscapement,lfOrientation, lfWeight;
    char lfItalic, lfUnderline, lfStrikeOut, lfCharSet, lfOutPrecision, lfClipPrecision, 
         lfQuality, lfPitchAndFamily;
    char lfFaceName[32];
} LOGFONT;
typedef struct
{
  int tmAscent,tmInternalLeading;
  // todo: full struct
} TEXTMETRIC;

HDC WDL_GDP_CreateContext(void *);
HDC WDL_GDP_CreateMemContext(HDC hdc, int w, int h);
void WDL_GDP_DeleteContext(HDC);
HFONT CreateFontIndirect(LOGFONT *);
HPEN CreatePen(int attr, int wid, int col);
HBRUSH CreateSolidBrush(int col);
HGDIOBJ SelectObject(HDC ctx, HGDIOBJ pen);
void DeleteObject(HGDIOBJ);
void FillRect(HDC ctx, RECT *r, HBRUSH br);
void Rectangle(HDC ctx, int l, int t, int r, int b);
void SWELL_Polygon(HDC ctx, POINT *pts, int npts);
void MoveToEx(HDC ctx, int x, int y, POINT *op);
void LineTo(HDC ctx, int x, int y);
void SetPixel(HDC ctx, int x, int y, int c);
void PolyBezierTo(HDC ctx, POINT *pts, int np);
void DrawText(HDC ctx, const char *buf, int len, RECT *r, int align);
void SetTextColor(HDC ctx, int col);
void SetBkColor(HDC ctx, int col);
void SetBkMode(HDC ctx, int col);

void RoundRect(HDC ctx, int x, int y, int x2, int y2, int xrnd, int yrnd);
void PolyPolyline(HDC ctx, POINT *pts, DWORD *cnts, int nseg);
BOOL GetTextMetrics(HDC ctx, TEXTMETRIC *tm);
void *GetNSImageFromHICON(HICON);
HICON LoadNamedImage(const char *name, bool alphaFromMask);
void DrawImageInRect(HDC ctx, HICON img, RECT *r);
void SWELL_PushClipRegion(HDC ctx);
void SWELL_SetClipRegion(HDC ctx, RECT *r);
void SWELL_PopClipRegion(HDC ctx);
void *SWELL_GetCtxFrameBuffer(HDC ctx);
void SWELL_SyncCtxFrameBuffer(HDC ctx);
void BitBlt(HDC hdcOut, int x, int y, int w, int h, HDC hdcIn, int xin, int yin, int mode);
#define DestroyIcon(x) DeleteObject(x)
int GetSysColor(int idx);
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
#define DT_SINGLELINE 0
#define DT_NOPREFIX 0
#define DT_TOP 0
#define DT_LEFT 0
#define FW_NORMAL 100
#define FW_BOLD 400
#define OUT_DEFAULT_PRECIS 0
#define CLIP_DEFAULT_PRECIS 0
#define DEFAULT_QUALITY 0
#define DEFAULT_PITCH 0
#define DEFAULT_CHARSET 0
#define Polygon(a,b,c) SWELL_Polygon(a,b,c)
#define TRANSPARENT 0
#define OPAQUE 1

#else

#define SWELL_CB_InsertString(hwnd, idx, pos, str) SendDlgItemMessage(hwnd,idx,CB_INSERTSTRING,pos,(LPARAM)str)
#define SWELL_CB_AddString(hwnd, idx, str) SendDlgItemMessage(hwnd,idx,CB_ADDSTRING,0,(LPARAM)str)
#define SWELL_CB_SetCurSel(hwnd,idx,val) SendDlgItemMessage(hwnd,idx,CB_SETCURSEL,(WPARAM)val,0)
#define SWELL_CB_GetNumItems(hwnd,idx) SendDlgItemMessage(hwnd,idx,CB_GETCOUNT,0,0)
#define SWELL_CB_GetCurSel(hwnd,idx) SendDlgItemMessage(hwnd,idx,CB_GETCURSEL,0,0)
#define SWELL_CB_SetItemData(hwnd,idx,item,val) SendDlgItemMessage(hwnd,idx,CB_SETITEMDATA,item,val)
#define SWELL_CB_GetItemData(hwnd,idx,item) SendDlgItemMessage(hwnd,idx,CB_GETITEMDATA,item,0)
#define SWELL_CB_GetItemText(hwnd,idx,item,buf,bufsz) SendDlgItemMessage(hwnd,idx,CB_GETLBTEXT,item,(LPARAM)buf)
#define SWELL_CB_Empty(hwnd,idx) SendDlgItemMessage(hwnd,idx,CB_RESETCONTENT,0,0)

#define SWELL_TB_SetPos(hwnd, idx, pos) SendDlgItemMessage(hwnd,idx, TBM_SETPOS,TRUE,pos)
#define SWELL_TB_SetRange(hwnd, idx, low, hi) SendDlgItemMessage(hwnd,idx,TBM_SETRANGE,TRUE,(LPARAM)MAKELONG(low,hi))
#define SWELL_TB_GetPos(hwnd, idx) SendDlgItemMessage(hwnd,idx,TBM_GETPOS,0,0)
#define SWELL_TB_SetTic(hwnd, idx, pos) SendDlgItemMessage(hwnd,idx,TBM_SETTIC,0,pos)

#endif//_WIN32


// stupid GDP compatibility layer, deprecated
#define WDL_GDP_CTX HDC
#define WDL_GDP_PEN HPEN
#define WDL_GDP_BRUSH HBRUSH
#define WDL_GDP_CreatePen(col, wid) (WDL_GDP_PEN)CreatePen(PS_SOLID,wid,col)
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


#endif//_WDL_SWELL_H_
