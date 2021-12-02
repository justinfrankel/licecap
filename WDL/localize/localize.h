#ifndef _WDL_LOCALIZE_H_
#define _WDL_LOCALIZE_H_

#include "../assocarray.h"

// normally onlySec_name is NULL and returns NULL (and updates global state)
// if onlySec_name is set, only loads/returns section in question and does not update global state.
WDL_AssocArray<WDL_UINT64, char *> *WDL_LoadLanguagePack(const char *buf, const char *onlySec_name);

WDL_AssocArray<WDL_UINT64, char *> *WDL_GetLangpackSection(const char *sec);

void WDL_SetLangpackFallbackEntry(const char *src_sec, WDL_UINT64 src_v, const char *dest_sec, WDL_UINT64 dest_v);

#define LOCALIZE_FLAG_VERIFY_FMTS 1 // verifies translated format-string (%d should match %d, etc)
#define LOCALIZE_FLAG_NOCACHE 2     // must use this if the string passed is not a persistent static string, or if in another thread
#define LOCALIZE_FLAG_PAIR 4        // one \0 in string needed -- this is not doublenull terminated but just a special case
#define LOCALIZE_FLAG_DOUBLENULL 8  // doublenull terminated string


#ifdef LOCALIZE_DISABLE
#define __LOCALIZE(str, ctx) str
#define __LOCALIZE_2N(str,ctx) str
#define __LOCALIZE_VERFMT(str, ctx) str
#define __LOCALIZE_NOCACHE(str, ctx) str
#define __LOCALIZE_VERFMT_NOCACHE(str, ctx) str
#define __LOCALIZE_LCACHE(str, ctx, pp) const char *pp = str
#define __LOCALIZE_VERFMT_LCACHE(str, ctx, pp) const char *pp = str
#else
#define __LOCALIZE(str, ctx) __localizeFunc("" str "" , "" ctx "",0)
#define __LOCALIZE_2N(str,ctx) __localizeFunc("" str "" , "" ctx "",LOCALIZE_FLAG_PAIR)
#define __LOCALIZE_VERFMT(str, ctx) __localizeFunc("" str "", "" ctx "",LOCALIZE_FLAG_VERIFY_FMTS)
#define __LOCALIZE_NOCACHE(str, ctx) __localizeFunc("" str "", "" ctx "",LOCALIZE_FLAG_NOCACHE)
#define __LOCALIZE_VERFMT_NOCACHE(str, ctx) __localizeFunc("" str "", "" ctx "",LOCALIZE_FLAG_VERIFY_FMTS|LOCALIZE_FLAG_NOCACHE)
#define __LOCALIZE_LCACHE(str, ctx, pp) static const char *pp; if (!pp) pp = __localizeFunc("" str "", "" ctx "",LOCALIZE_FLAG_NOCACHE)
#define __LOCALIZE_VERFMT_LCACHE(str, ctx, pp) static const char *pp; if (!pp) pp = __localizeFunc("" str "", "" ctx "",LOCALIZE_FLAG_VERIFY_FMTS|LOCALIZE_FLAG_NOCACHE)
#endif

#define __LOCALIZE_REG_ONLY(str, ctx) str

// localize a string
const char *__localizeFunc(const char *str, const char *subctx, int flags);

// localize a menu; rescat can be NULL, or a prefix
void __localizeMenu(const char *rescat, HMENU hMenu, LPCSTR lpMenuName);

void __localizeInitializeDialog(HWND hwnd, const char *desc); // only use for certain child windows that we can't control creation of, pass desc of DLG_xyz etc

// localize a dialog; rescat can be NULL, or a prefix
// if returns non-NULL, use retval as dlgproc and pass ptrs as LPARAM to dialogboxparam
// nptrs should be at least 4 (might increase someday, but this will only ever be used by utilfunc.cpp or localize-import.h)
DLGPROC __localizePrepareDialog(const char *rescat, HINSTANCE hInstance, const char *lpTemplate, DLGPROC dlgProc, LPARAM lParam, void **ptrs, int nptrs); 


#ifndef LOCALIZE_NO_DIALOG_MENU_REDEF
#undef DialogBox
#undef CreateDialog
#undef DialogBoxParam
#undef CreateDialogParam

#ifdef _WIN32
  #define DialogBoxParam(a,b,c,d,e) ((INT_PTR)__localizeDialog(a,b,c,d,e,1))
  #define CreateDialogParam(a,b,c,d,e) __localizeDialog(a,b,c,d,e,0)
#else
  #define DialogBoxParam(a,b,c,d,e) ((INT_PTR)__localizeDialog(NULL,b,c,d,e,1))
  #define CreateDialogParam(a,b,c,d,e) __localizeDialog(NULL,b,c,d,e,0)
#endif

#define DialogBox(hinst,lpt,hwndp,dlgproc) DialogBoxParam(hinst,lpt,hwndp,dlgproc,0)
#define CreateDialog(hinst,lpt,hwndp,dlgproc) CreateDialogParam(hinst,lpt,hwndp,dlgproc,0)


#undef LoadMenu
#define LoadMenu __localizeLoadMenu
#endif

HMENU __localizeLoadMenu(HINSTANCE hInstance, const char *lpMenuName);
HWND __localizeDialog(HINSTANCE hInstance, const char * lpTemplate, HWND hwndParent, DLGPROC dlgProc, LPARAM lParam, int mode);

#define __localizeDialogBoxParam(a,b,c,d,e) ((INT_PTR)__localizeDialog(a,b,c,d,e,1))
#define __localizeCreateDialogParam(a,b,c,d,e) __localizeDialog(a,b,c,d,e,0)

#endif
