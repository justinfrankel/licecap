
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
  
  DialogBox emulation is here. To declare the resource at a global level, use (in any source file that includes this file and resource.h):

  
  #ifdef MAC


  SWELL_DEFINE_DIALOG_RESOURCE_BEGIN(IDD_SOMEDIALOG,0,"Dialog Box Title",222,55,1.8) // width, height, scale (1.8 is usually good)

  BEGIN
  DEFPUSHBUTTON   "OK",IDOK,117,33,47,14
  CONTROL         "Expand MIDI tracks to new REAPER tracks                    ",IDC_CHECK1,
  "Button",BS_AUTOCHECKBOX | WS_TABSTOP,4,7,214,10
  CONTROL         "Merge MIDI tempo map to project tempo map at                ",
  IDC_CHECK2,"Button",BS_AUTOCHECKBOX | WS_TABSTOP,4,19,
  214,10
  PUSHBUTTON      "Cancel",IDCANCEL,168,33,50,14
  END

  SWELL_DEFINE_DIALOG_RESOURCE_END(IDD_SOMEDIALOG)


  #endif


  This file also provides functions to dynamically create controls in a view from a win32 resource script.

  To create these controls, do something like the following:

  -createControls
  {
    NSView *view=self;

    SWELL_DLGGEN_DLGFOLLOWS(view,1.8);
    BEGIN
      LTEXT           "Bla bla bla",IDC_TEXT1,6,2,225,9
    END

    NSView *textview=(NSView *)GetDlgItem((HWND)self,IDC_TEXT1);
  }


  Not all controls are supported, and Cocoa can be picky about the ordering of controls, too.
  You can also use SWELL_DLGGEN_DLGFOLLOWS_EX to set things like separate scaling, translation, and autopositioning of items (to avoid overlapping)
  


*/




#ifndef _SWELL_DLGGEN_H_
#define _SWELL_DLGGEN_H_

// these will need implementing
void SWELL_MakeSetCurParms(float xscale, float yscale, float xtrans, float ytrans, HWND parent, bool doauto);
void SWELL_Make_SetMessageMode(int wantParentView);

void SWELL_MakeButton(int def, const char *label, int idx, int x, int y, int w, int h);
void SWELL_MakeEditField(int idx, int x, int y, int w, int h, int flags=0);                      
void SWELL_MakeLabel(int align, const char *label, int idx, int x, int y, int w, int h, int flags=0);
void SWELL_MakeControl(const char *cname, int idx, const char *classname, int style, int x, int y, int w, int h);
void SWELL_MakeCombo(int idx, int x, int y, int w, int h, int flags=0);                   
void SWELL_MakeGroupBox(const char *name, int idx, int x, int y, int w, int h);

#define SWELL_DLGGEN_DLGFOLLOWS(par, scale) SWELL_DLGGEN_DLGFOLLOWS_EX(par,scale,scale,0,0,false)
#define SWELL_DLGGEN_DLGFOLLOWS_EX(par, xscale, yscale, xtrans, ytrans, doauto) { SWELL_MakeSetCurParms(xscale,yscale,xtrans,ytrans,(HWND)par,doauto);
#define BEGIN (0
#define END ); SWELL_MakeSetCurParms(1.0,1.0,0,0,NULL,false); }
#define PUSHBUTTON ); SWELL_MakeButton(0,
#define DEFPUSHBUTTON ); SWELL_MakeButton(1,
#define EDITTEXT ); SWELL_MakeEditField(
#define CTEXT ); SWELL_MakeLabel(0,                                
#define LTEXT ); SWELL_MakeLabel(-1,
#define RTEXT ); SWELL_MakeLabel(1,
#define CONTROL ); SWELL_MakeControl(                               
#define COMBOBOX ); SWELL_MakeCombo(
#define GROUPBOX ); SWELL_MakeGroupBox(

#define NOT ~ // this may not work due to precedence, but worth a try                                    
                                    
// flags we may use
#define BS_AUTOCHECKBOX 1
#define BS_AUTORADIOBUTTON 2
#define CBS_DROPDOWNLIST 1
#define CBS_DROPDOWN 2
#define ES_READONLY 1
#define ES_NUMBER 2
#define ES_WANTRETURN 4
#define SS_NOTFIY 1

#define LVS_LIST 0
#define LVS_NOCOLUMNHEADER 1
#define LVS_REPORT 2
#define LVS_SINGLESEL 4
#define LVS_OWNERDATA 8
                                       
                                       
// things that should be implemented sooner
#define CBS_SORT 0
                                    
// flags we ignore
#define WS_GROUP 0
#define LVS_SHOWSELALWAYS 0
#define LVS_NOSORTHEADER 0         
#define LVS_SORTASCENDING 0
#define LVS_SHAREIMAGELISTS 0
#define SS_LEFTNOWORDWRAP 0
#define ES_AUTOHSCROLL 0
#define ES_MULTILINE 0
#define ES_AUTOVSCROLL 0
#define WS_TABSTOP 0
#define GROUP 0
#define WS_VSCROLL 0
#define WS_BORDER 0
#define WS_HSCROLL 0
#define PBS_SMOOTH 0
#define CBS_AUTOHSCROLL 0
#define TBS_NOTICKS 0
#define TBS_TOP 0
#define SS_NOTIFY 0
                     
                                       
#ifndef IDC_STATIC
#define IDC_STATIC 0
#endif

#ifndef WINAPI
#define WINAPI
#endif
                                       
#ifndef CALLBACK
#define CALLBACK
#endif

#ifndef MAKEINTRESOURCE
#define MAKEINTRESOURCE(x) (x)         
#endif                
                                       
#define DialogBox(hinst, resid, par, dlgproc) SWELL_DialogBox(resid,par,dlgproc,0)
#define DialogBoxParam(hinst, resid, par, dlgproc, param) SWELL_DialogBox(resid,par,dlgproc,param)
                                       
                                                                              
#ifndef LOWORD
#define LOWORD(x) ((x)&0xffff)
#endif
                                       
#ifndef HIWORD
#define HIWORD(x) (((x)>>16)&0xffff)
#endif
                                       
typedef unsigned int UINT;
typedef unsigned int WPARAM;
typedef long LPARAM;
                                       
typedef BOOL (*DLGPROC)(HWND, UINT, WPARAM, LPARAM);
                                       
int SWELL_DialogBox(int resid, HWND parent,  DLGPROC dlgproc, LPARAM param);  

                                       
void SWELL_RegisterDialogResource(int resid, void (*createFunc)(HWND createTo), bool isChildWindow, const char *title, int width, int height);                                    
#define SWELL_DEFINE_DIALOG_RESOURCE_BEGIN(resid, isch, title, width, height, scale) \
                                       class NewCustomResource_##resid { \
                                          public: \
                                            NewCustomResource_##resid () { SWELL_RegisterDialogResource(resid,cf,isch,title,(int)((width)*(scale)),(int)((height)*(scale))); } \
                                           static void cf(HWND view) { \
                                             SWELL_DLGGEN_DLGFOLLOWS(view,scale);
                           
#define SWELL_DEFINE_DIALOG_RESOURCE_END(resid ) } }; static NewCustomResource_##resid NewCustomResourceInst_##resid;                                       
     
#define EN_CHANGE           0x0300
#define STN_CLICKED         0
#define STN_DBLCLK          1
                                       
void EndDialog(HWND, int);            
                                       
#define WM_DESTROY                      0x0002
#define WM_INITDIALOG                   0x0110
#define WM_COMMAND                      0x0111
#define WM_TIMER                        0x0113
#define WM_CLOSE                        0x0010
#define WM_HSCROLL                      0x0114
#define WM_VSCROLL                      0x0115
       
                                
#endif