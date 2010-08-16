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
  

    This file provides basic key and mouse cursor querying, as well as a mac key to windows key translation function.

  */

#ifndef SWELL_PROVIDED_BY_APP

#include "swell.h"
#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>



static int MacKeyCodeToVK(int code)
{
	switch (code)
	{
    case 51: return VK_BACK;
    case 65: return VK_DECIMAL;
    case 67: return VK_MULTIPLY;
    case 69: return VK_ADD;
    case 75: return VK_DIVIDE;
    case 78: return VK_SUBTRACT;
    case 81: return VK_SEPARATOR;
    case 82: return VK_NUMPAD0;
    case 83: return VK_NUMPAD1;
    case 84: return VK_NUMPAD2;
    case 85: return VK_NUMPAD3;
    case 86: return VK_NUMPAD4;
    case 87: return VK_NUMPAD5;
    case 88: return VK_NUMPAD6;
    case 89: return VK_NUMPAD7;
    case 91: return VK_NUMPAD8;
    case 92: return VK_NUMPAD9;
		case 115: return VK_HOME;
		case 116: return VK_PRIOR;
		case 119: return VK_END;
		case 121: return VK_NEXT;
		case 123: return VK_LEFT;
		case 124: return VK_RIGHT;
		case 125: return VK_DOWN;
		case 126: return VK_UP;
	}
	return 0;
}

int SWELL_MacKeyToWindowsKey(void *nsevent, int *flags)
{
  NSEvent *theEvent = (NSEvent *)nsevent;
	int mod=[theEvent modifierFlags];// & ( NSShiftKeyMask|NSControlKeyMask|NSAlternateKeyMask|NSCommandKeyMask);
                                   //	if ([theEvent isARepeat]) return;
    
    int flag=0;
    if (mod & NSShiftKeyMask) flag|=FSHIFT;
    if (mod & NSCommandKeyMask) flag|=FCONTROL; // todo: this should be command once we figure it out
    if (mod & NSAlternateKeyMask) flag|=FALT;
    
    int rawcode=[theEvent keyCode];
    int code=MacKeyCodeToVK(rawcode);
    if (!code)
    {
      NSString *str=[theEvent charactersIgnoringModifiers];
      const char *p=[str cStringUsingEncoding: NSASCIIStringEncoding];
      if (!p) return 0;
      code=toupper(*p);
      if (code == 25 && (flag&FSHIFT)) code=VK_TAB;
      if (isalnum(code)||code==' ' || code == '\r' || code == '\n' || code ==27 || code == VK_TAB) flag|=FVIRTKEY;
    }
    else flag|=FVIRTKEY;
    
    //if (code == ' ' && flag==(FVIRTKEY) && (mod&NSControlKeyMask)) flag|=FCONTROL;
    
    if (!(flag&FVIRTKEY)) flag&=~FSHIFT;
    if (!flag)
    {
      flag=FVIRTKEY|FSHIFT;
      switch (code)
      {
        case '!': code='1'; break;
        case '@': code='2'; break;
        case '#': code='3'; break;
        case '$': code='4'; break;
        case '%': code='5'; break;
        case '^': code='6'; break;
        case '&': code='7'; break;
        case '*': code='8'; break;
        case '(': code='9'; break;
        case ')': code='0'; break;
          
        default: flag=0; break;
      }
    }
 //   printf("rawcode=%d (%c), output code=%d (%c), flag=%d\n",rawcode,rawcode,code,code,flag);
    
    if (flags) *flags=flag;
    return code;
}

WORD GetAsyncKeyState(int key)
{
  NSEvent *evt=[NSApp currentEvent];
  if (!evt) return 0;
  if (key == MK_LBUTTON) return (GetCurrentEventButtonState()&1)?0x8000:0;
  if (key == MK_RBUTTON) return (GetCurrentEventButtonState()&2)?0x8000:0;
  if (key == MK_MBUTTON) return (GetCurrentEventButtonState()&4)?0x8000:0;
  int flags=[evt modifierFlags];
  if (key == VK_CONTROL) return (flags&NSCommandKeyMask)?0x8000:0;
  if (key == VK_MENU) return (flags&NSAlternateKeyMask)?0x8000:0;
  if (key == VK_SHIFT) return (flags&NSShiftKeyMask)?0x8000:0;
  return 0;
}

void GetCursorPos(POINT *pt)
{
	NSPoint localpt=[NSEvent mouseLocation];
  pt->x=(int)localpt.x;
  pt->y=(int)localpt.y;
}

DWORD GetMessagePos()
{
  NSPoint localpt=[NSEvent mouseLocation];
  return MAKELONG((int)localpt.x, (int)localpt.y);
}

HCURSOR SWELL_LoadCursor(int idx)
{
  switch (idx)
  {
    case IDC_SIZEALL: // todo
    case IDC_SIZEWE:
      return (HCURSOR)[NSCursor resizeLeftRightCursor];
    case IDC_SIZENS:
      return (HCURSOR)[NSCursor resizeUpDownCursor];
    case IDC_NO: // todo
    case IDC_ARROW:
      return (HCURSOR)[NSCursor arrowCursor];
    case IDC_HAND:
      return (HCURSOR)[NSCursor openHandCursor];
    case IDC_UPARROW:
      return (HCURSOR)[NSCursor resizeUpCursor];
  }
  return 0;
}

static HCURSOR m_last_setcursor;

void SWELL_SetCursor(HCURSOR curs)
{
  if (curs && [(id) curs isKindOfClass:[NSCursor class]])
  {
    m_last_setcursor=curs;
    [(NSCursor *)curs set];
  }
  else
  {
    m_last_setcursor=NULL;
    [[NSCursor arrowCursor] set];
  }
}

HCURSOR SWELL_GetCursor()
{
  return (HCURSOR)[NSCursor currentCursor];
}
HCURSOR SWELL_GetLastSetCursor()
{
  return m_last_setcursor;
}

#endif