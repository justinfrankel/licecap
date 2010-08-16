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
#include "swell-dlggen.h"
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
    case 71: return VK_NUMLOCK;
    case 75: return VK_DIVIDE;
    case 76: return VK_RETURN|0x8000;
    case 27:
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
    case 96: return VK_F5;
    case 97: return VK_F6;
    case 98: return VK_F7;
    case 99: return VK_F3;
    case 100: return VK_F8;
    case 101: return VK_F9;
    case 109: return VK_F10;
    case 103: return VK_F11;
    case 105: return VK_SNAPSHOT;
    case 111: return VK_F12;
    case 114: return VK_INSERT;
		case 115: return VK_HOME;
    case 117: return VK_DELETE;
		case 116: return VK_PRIOR;
    case 118: return VK_F4;
		case 119: return VK_END;
    case 120: return VK_F2;
		case 121: return VK_NEXT;
    case 122: return VK_F1;
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
//    printf("rawcode %d\n",rawcode);
    
    int code=MacKeyCodeToVK(rawcode);
    if (!code)
    {
      NSString *str=[theEvent charactersIgnoringModifiers];
      const char *p=[str cStringUsingEncoding: NSASCIIStringEncoding];
      if (!p) 
      {
        return 0;
      }
      code=toupper(*p);
      if (code == 25 && (flag&FSHIFT)) code=VK_TAB;
      if (isalnum(code)||code==' ' || code == '\r' || code == '\n' || code ==27 || code == VK_TAB) flag|=FVIRTKEY;
    }
    else
    {
      flag|=FVIRTKEY;
      if (code==8) code='\b';
    }
    if (flag & FSHIFT)
    {
      if (code=='[') { code='{'; flag&=~(FSHIFT|FVIRTKEY); }
      else if (code==']') { code='}'; flag&=~(FSHIFT|FVIRTKEY); }
    }
    
    //if (code == ' ' && flag==(FVIRTKEY) && (mod&NSControlKeyMask)) flag|=FCONTROL;
    
    if (!(flag&FVIRTKEY)) flag&=~FSHIFT;
    if (!flag)
    {
    // todo: some OS X API for this?
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
    
    if (flags) *flags=flag;
    return code;
}

int SWELL_KeyToASCII(int wParam, int lParam, int *newflags)
{
  if (wParam >= '0' && wParam <= '9' && lParam == (FSHIFT|FVIRTKEY))
  {
  // todo: some OS X API for this?
    *newflags = lParam&~(FSHIFT|FVIRTKEY);
    switch (wParam) 
    {
      case '1': return '!';
      case '2': return '@';
      case '3': return '#';
      case '4': return '$';
      case '5': return '%';
      case '6': return '^';
      case '7': return '&';
      case '8': return '*';
      case '9': return '(';
      case '0': return ')';      
    }
  }
  return 0;
}


WORD GetAsyncKeyState(int key)
{
  if (key == VK_LBUTTON) return (GetCurrentEventButtonState()&1)?0x8000:0;
  if (key == VK_RBUTTON) return (GetCurrentEventButtonState()&2)?0x8000:0;
  if (key == VK_MBUTTON) return (GetCurrentEventButtonState()&4)?0x8000:0;
  NSEvent *evt=[NSApp currentEvent];
  if (!evt) return 0;
  if (key == VK_CONTROL) return ([evt modifierFlags]&NSCommandKeyMask)?0x8000:0;
  if (key == VK_MENU) return ([evt modifierFlags]&NSAlternateKeyMask)?0x8000:0;
  if (key == VK_SHIFT) return ([evt modifierFlags]&NSShiftKeyMask)?0x8000:0;
  return 0;
}


SWELL_CursorResourceIndex *SWELL_curmodule_cursorresource_head;

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
    case IDC_IBEAM: return (HCURSOR)[NSCursor IBeamCursor];
  }
  SWELL_CursorResourceIndex *p = SWELL_curmodule_cursorresource_head;
  while (p)
  {
    if (p->resid == idx)
    {
      if (p->cachedCursor) return p->cachedCursor;
      
      NSString *str = (NSString *)SWELL_CStringToCFString(p->resname);     
      NSImage *img = [NSImage imageNamed:str];
      [str release];
      if (img)
      {      
        return p->cachedCursor=(HCURSOR)[[NSCursor alloc] initWithImage:img hotSpot:NSMakePoint(p->hotspot.x,p->hotspot.y)];
      }
    }
    p=p->_next;
  }
  // search registered cursor list here
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


static bool g_swell_mouse_relmode;
static bool g_swell_mouse_relmode_synergydet; // only used when synergy is detected on hidden mouse mode
static bool g_swell_last_set_valid;
static POINT g_swell_last_set_pos;


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


NSPoint swellProcessMouseEvent(int msg, NSView *view, NSEvent *event)
{
  if (g_swell_mouse_relmode && msg==WM_MOUSEMOVE) // event will have relative coordinates
  {
    int idx=(int)[event deltaX];
    int idy=(int)[event deltaY];
    NSPoint localpt=[event locationInWindow];
    localpt=[view convertPoint:localpt fromView:nil];
    POINT p={(int)localpt.x,(int)localpt.y};
    ClientToScreen((HWND)view,&p);
    
     // if deltas set, and the cursor actually moved, then it must be synergy
    if (!g_swell_mouse_relmode_synergydet && g_swell_last_set_valid && (idx||idy) && g_swell_last_set_pos.x+idx == p.x && g_swell_last_set_pos.y-idy == p.y)
    {
      g_swell_mouse_relmode_synergydet=true;
    }
    
    if (g_swell_mouse_relmode_synergydet) idx=idy=0;      
    else if (idx||idy) SetCursorPos(p.x+idx,p.y-idy);
    return NSMakePoint(localpt.x+idx,localpt.y+idy);
  }
  
  NSPoint localpt=[event locationInWindow];
  return [view convertPoint:localpt fromView:nil];
}

static int m_curvis_cnt;
bool SWELL_IsCursorVisible()
{
  return m_curvis_cnt>=0;
}
int SWELL_ShowCursor(BOOL bShow)
{
  m_curvis_cnt += (bShow?1:-1);
  if (m_curvis_cnt==-1 && !bShow) 
  {
    CGDisplayHideCursor(kCGDirectMainDisplay);
    CGAssociateMouseAndMouseCursorPosition(false);
    g_swell_mouse_relmode=true;
  }
  if (m_curvis_cnt==0 && bShow) 
  {
    CGDisplayShowCursor(kCGDirectMainDisplay);
    CGAssociateMouseAndMouseCursorPosition(true);
    g_swell_mouse_relmode=false;
  }  
  g_swell_mouse_relmode_synergydet=false;
  g_swell_last_set_valid=false;
  return m_curvis_cnt;
}


BOOL SWELL_SetCursorPos(int X, int Y)
{  
  if (g_swell_mouse_relmode_synergydet) return false;

  g_swell_last_set_pos.x = X;
  g_swell_last_set_pos.y = Y;
  g_swell_last_set_valid=true;

  int h=CGDisplayPixelsHigh(CGMainDisplayID());
  CGPoint pos=CGPointMake(X,h-Y);
  return CGWarpMouseCursorPosition(pos)==kCGErrorSuccess;
}


#endif
