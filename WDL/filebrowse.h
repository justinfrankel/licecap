#ifndef _WDL_FILEBROWSE_H_
#define _WDL_FILEBROWSE_H_

#ifdef _WIN32
#include <windows.h>
#else
#include "swell/swell.h"
#endif


bool WDL_ChooseDirectory(HWND parent, const char *text, const char *initialdir, char *fn, int fnsize, bool preservecwd);
bool WDL_ChooseFileForSave(HWND parent, 
                                      const char *text, 
                                      const char *initialdir, 
                                      const char *initialfile, 
                                      const char *extlist,
                                      const char *defext,
                                      bool preservecwd,
                                      char *fn, 
                                      int fnsize,
                                      const char *dlgid=NULL, 
                                      void *dlgProc=NULL, 
#ifdef _WIN32
                                      HINSTANCE hInstance=NULL
#else
                                      struct SWELL_DialogResourceIndex *reshead=NULL
#endif
                                      );


char *WDL_ChooseFileForOpen(HWND parent,
                                        const char *text, 
                                        const char *initialdir,  
                                        const char *initialfile, 
                                        const char *extlist,
                                        const char *defext,

                                        bool preservecwd,
                                        bool allowmul, 

                                        const char *dlgid=NULL, 
                                        void *dlgProc=NULL, 
#ifdef _WIN32
                                        HINSTANCE hInstance=NULL
#else
                                        struct SWELL_DialogResourceIndex *reshead=NULL
#endif
                                        );



#endif