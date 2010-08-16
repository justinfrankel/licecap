
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
  

  This file provides functions to dynamically create controls in a view from a win32 resource script.

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
                     
                                       
#ifndef IDC_STATIC
#define IDC_STATIC 0
#endif
                                
#endif