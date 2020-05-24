// used by plug-ins to access imported localization
// usage, in one file: 
// #define LOCALIZE_IMPORT_PREFIX "midi_" (should be the directory name with _)
// #include "localize-import.h"
// #include "localize.h"
// import function pointers in initialization
//
// other files can simply import localize.h as normal
// note that if the module might be unloaded, LOCALIZE_FLAG_NOCACHE is essential!
//

#ifdef _WDL_LOCALIZE_H_
#error you must include localize-import.h before localize.h, sorry
#endif

#include <stdio.h>
#include "../wdltypes.h"

// caller must import these 4 function pointers from the running app
static const char *(*importedLocalizeFunc)(const char *str, const char *subctx, int flags);
static void (*importedLocalizeMenu)(const char *rescat, HMENU hMenu, LPCSTR lpMenuName);
static DLGPROC (*importedLocalizePrepareDialog)(const char *rescat, HINSTANCE hInstance, const char *lpTemplate, DLGPROC dlgProc, LPARAM lPAram, void **ptrs, int nptrs);
static void (*importedLocalizeInitializeDialog)(HWND hwnd, const char *d);

const char *__localizeFunc(const char *str, const char *subctx, int flags)
{
  if (WDL_NORMALLY(importedLocalizeFunc)) return importedLocalizeFunc(str,subctx,flags);
  return str;
}

HMENU __localizeLoadMenu(HINSTANCE hInstance, const char *lpMenuName)
{
  HMENU menu = LoadMenu(hInstance,lpMenuName);
  if (menu && WDL_NORMALLY(importedLocalizeMenu)) importedLocalizeMenu(LOCALIZE_IMPORT_PREFIX,menu,lpMenuName);
  return menu;
}

HWND __localizeDialog(HINSTANCE hInstance, const char *lpTemplate, HWND hwndParent, DLGPROC dlgProc, LPARAM lParam, int mode)
{
  void *p[5];
  char tmp[256];
  if (mode == 1)
  {
    sprintf(tmp,"%.100s%d",LOCALIZE_IMPORT_PREFIX,(int)(INT_PTR)lpTemplate);
    p[4] = tmp;
  }
  else
    p[4] = NULL;

  if (WDL_NORMALLY(importedLocalizePrepareDialog))
  {
    DLGPROC newDlg  = importedLocalizePrepareDialog(LOCALIZE_IMPORT_PREFIX,hInstance,lpTemplate,dlgProc,lParam,p,sizeof(p)/sizeof(p[0]));
    if (newDlg)
    {
      dlgProc = newDlg;
      lParam = (LPARAM)(INT_PTR)p;
    }
  }
  switch (mode)
  {
    case 0: return CreateDialogParam(hInstance,lpTemplate,hwndParent,dlgProc,lParam);
    case 1: return (HWND) (INT_PTR)DialogBoxParam(hInstance,lpTemplate,hwndParent,dlgProc,lParam);
  }
  return 0;
}

void __localizeInitializeDialog(HWND hwnd, const char *d)
{
  if (WDL_NORMALLY(importedLocalizeInitializeDialog)) importedLocalizeInitializeDialog(hwnd,d);
}

static void localizeKbdSection(KbdSectionInfo *sec, const char *nm)
{
  if (WDL_NORMALLY(importedLocalizeFunc))
  {
    int x;
    if (sec->action_list) for (x=0;x<sec->action_list_cnt; x++)
    {
      KbdCmd *p = &sec->action_list[x];
      if (p->text) p->text = __localizeFunc(p->text,nm,2/*LOCALIZE_FLAG_NOCACHE*/);
    }
  }
}
