#import "Controller.h"

#include "../../SWELL/swell.h"
#include "../../SWELL/swell-dlggen.h"

#include "main.cpp" // this would otherwise conflict in object name with main.m, which xcode made us.

@implementation Controller
-(void)awakeFromNib
{
  HWND h=CreateDialog(NULL,MAKEINTRESOURCE(IDD_DIALOG1),NULL,dlgProc);
  ShowWindow(h,SW_SHOW);
}
@end


// define our dialog box resource!

SWELL_DEFINE_DIALOG_RESOURCE_BEGIN(IDD_DIALOG1,SWELL_DLG_WS_RESIZABLE|SWELL_DLG_WS_FLIPPED,"LICE Test",400,300,1.8)
BEGIN
CONTROL         "",IDC_RECT,"Static",SS_BLACKRECT | NOT WS_VISIBLE | 
WS_DISABLED,7,23,384,239
COMBOBOX        IDC_COMBO1,7,7,181,170,CBS_DROPDOWNLIST | WS_VSCROLL | 
WS_TABSTOP

END
SWELL_DEFINE_DIALOG_RESOURCE_END(IDD_DIALOG1)