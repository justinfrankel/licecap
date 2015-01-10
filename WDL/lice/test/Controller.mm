#import "Controller.h"

#include "../../SWELL/swell.h"
#include "../../SWELL/swell-dlggen.h"

#include "main.cpp" // this would otherwise conflict in object name with main.m, which xcode made us.

static HWND ccontrolCreator(HWND parent, const char *cname, int idx, const char *classname, int style, int x, int y, int w, int h)
{
  if (!stricmp(classname,"TestRenderingClass"))
  {
    HWND hw=CreateDialog(NULL,0,parent,(DLGPROC)testRenderDialogProc);
    SetWindowLong(hw,GWL_ID,idx);
    SetWindowPos(hw,HWND_TOP,x,y,w,h,SWP_NOZORDER|SWP_NOACTIVATE);
    ShowWindow(hw,SW_SHOWNA);
    return hw;
  }
  return 0;
}

@implementation Controller
-(void)awakeFromNib
{
  SWELL_RegisterCustomControlCreator(ccontrolCreator);
  HWND h=CreateDialog(NULL,MAKEINTRESOURCE(IDD_DIALOG1),NULL,dlgProc);
  ShowWindow(h,SW_SHOW);
}
@end


// define our dialog box resource!

SWELL_DEFINE_DIALOG_RESOURCE_BEGIN(IDD_DIALOG1,SWELL_DLG_WS_RESIZABLE|SWELL_DLG_WS_FLIPPED,"LICE Test",400,300,1.8)
BEGIN
CONTROL         "",IDC_RECT,"TestRenderingClass",0,7,23,384,239 // we arae creating a custom control here because it will be opaque and therefor a LOT faster drawing
COMBOBOX        IDC_COMBO1,7,7,181,170,CBS_DROPDOWNLIST | WS_VSCROLL | 
WS_TABSTOP

END
SWELL_DEFINE_DIALOG_RESOURCE_END(IDD_DIALOG1)