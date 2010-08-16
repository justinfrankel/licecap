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
  

    This file provides basic windows APIs for handling windows, as well as the stubs to enable swell-dlggen to work.

  */



#import <Cocoa/Cocoa.h>
#include "swell.h"
#include "../mutex.h"
#include "../ptrlist.h"


@interface simpleDataHold : NSObject
{
  int m_data;
}
-(id) initWithVal:(int)val;
-(int) getValue;
@end
@implementation simpleDataHold
-(id) initWithVal:(int)val
{
  if ((self = [super init]))
  {
    m_data=val;
  }
  return self;
}
-(int) getValue
{
  return m_data;
}
@end

HWND GetDlgItem(HWND hwnd, int idx)
{
  if (!hwnd) return 0;
  NSView *v=0;
  id pid=(id)hwnd;
  if ([pid isKindOfClass:[NSWindow class]]) v=[((NSWindow *)pid) contentView];
  else if ([pid isKindOfClass:[NSView class]]) v=(NSView *)pid;
  
  if (!idx || !v) return (HWND)v;
  
  return (HWND) [v viewWithTag:idx];
}

void DestroyWindow(HWND hwnd)
{
  if (!hwnd) return;
  id pid=(id)hwnd;
  if ([pid isKindOfClass:[NSWindow class]])
  {
    [(NSWindow *)pid close];
  }
  else if ([pid isKindOfClass:[NSView class]])
  {
    [(NSView *)pid removeFromSuperview];
  }
}

void EnableWindow(HWND hwnd, int enable)
{
  if (!hwnd) return;
  id bla=(id)hwnd;
  if ([bla isKindOfClass:[NSControl class]])
    [bla setEnabled:(enable?YES:NO)];
}

void SetFocus(HWND hwnd) // these take NSWindow/NSView, and return NSView *
{
  id r=(id) hwnd;
  if (!r) return;
  
  if ([r isKindOfClass:[NSWindow class]])
  {
    [(NSWindow *)r makeKeyAndOrderFront:nil];
  }
  else if ([r isKindOfClass:[NSView class]])
  {
    NSWindow *wnd=[(NSView *)r window];
    if (wnd)
    {
      [wnd makeKeyAndOrderFront:nil];
      [wnd makeFirstResponder:r];
    }
  }
}

void ScreenToClient(HWND hwnd, POINT *p)
{
  if (!hwnd) return;
  
  id ch=(id)hwnd;
  if ([ch isKindOfClass:[NSWindow class]]) ch=[((NSWindow *)ch) contentView];
  if (!ch || ![ch isKindOfClass:[NSView class]]) return;
  
  NSWindow *window=[ch window];
  
  NSPoint wndpt = [window convertScreenToBase:NSMakePoint(p->x,p->y)];
  
  NSPoint po = [ch convertPoint:wndpt fromView:nil];
  
  p->x=(int)(po.x+0.5);
  p->y=(int)(po.y+0.5);
}

void ClientToScreen(HWND hwnd, POINT *p)
{
  if (!hwnd) return;
  
  id ch=(id)hwnd;
  if ([ch isKindOfClass:[NSWindow class]]) ch=[((NSWindow *)ch) contentView];
  if (!ch || ![ch isKindOfClass:[NSView class]]) return;
  
  NSWindow *window=[ch window];
  
  NSPoint wndpt = [ch convertPoint:NSMakePoint(p->x,p->y) toView:nil];
  
  NSPoint po = [window convertBaseToScreen:wndpt];
  
  p->x=(int)(po.x+0.5);
  p->y=(int)(po.y+0.5);
}

void GetWindowRect(HWND hwnd, RECT *r)
{
  r->left=r->top=r->right=r->bottom=0;
  if (!hwnd) return;
  
  id ch=(id)hwnd;
  if ([ch isKindOfClass:[NSWindow class]]) 
  {
    NSRect b=[ch frame];
    r->left=(int)(b.origin.x);
    r->top=(int)(b.origin.y);
    r->right = (int)(b.origin.x+b.size.width+0.5);
    r->bottom= (int)(b.origin.y+b.size.height+0.5);
    return;
  }
  if (![ch isKindOfClass:[NSView class]]) return;
  
  NSRect b=[ch bounds];
  r->left=(int)(b.origin.x);
  r->top=(int)(b.origin.y);
  r->right = (int)(b.origin.x+b.size.width+0.5);
  r->bottom= (int)(b.origin.y+b.size.height+0.5);
  RECT orect=*r;
  ClientToScreen(hwnd,(POINT *)r);
  r->right += r->left-orect.left;
  r->bottom += r->top-orect.top;
}

void GetClientRect(HWND hwnd, RECT *r)
{
  r->left=r->top=r->right=r->bottom=0;
  if (!hwnd) return;
  
  id ch=(id)hwnd;
  if ([ch isKindOfClass:[NSWindow class]]) ch=[((NSWindow *)ch) contentView];
  if (!ch || ![ch isKindOfClass:[NSView class]]) return;
  
  NSRect b=[ch bounds];
  r->left=(int)(b.origin.x);
  r->top=(int)(b.origin.y);
  r->right = (int)(b.origin.x+b.size.width+0.5);
  r->bottom= (int)(b.origin.y+b.size.height+0.5);
}

void SetWindowPos(HWND hwnd, HWND unused, int x, int y, int cx, int cy, int flags)
{
  if (!hwnd) return;
  
  id ch=(id)hwnd;
  if ([ch isKindOfClass:[NSWindow class]] || [ch isKindOfClass:[NSView class]]) 
  {
    NSRect f=[ch frame];
    bool repos=false;
    if (!(flags&SWP_NOMOVE))
    {
      f.origin.x=(float)x;
      f.origin.y=(float)y;
      repos=true;
    }
    if (!(flags&SWP_NOSIZE))
    {
      f.size.width=(float)cx;
      f.size.height=(float)cy;
      repos=true;
    }
    if (repos)
    {
      if ([ch isKindOfClass:[NSWindow class]])
        [ch setFrame:f display:YES];
      else
        [ch setFrame:f];
    }    
    return;
  }  
  
}


HWND GetParent(HWND hwnd)
{
  if (!hwnd || ![(id)hwnd isKindOfClass:[NSView class]]) return 0;
  return (HWND)[(NSView *)hwnd superview];
}

HWND SetParent(HWND hwnd, HWND newPar)
{
  NSView *v=(NSView *)hwnd;
  if (!v || ![(id)v isKindOfClass:[NSView class]]) return 0;
  HWND ret=(HWND) [v superview];
  if (ret) 
  {
    [v retain];
    [v removeFromSuperview];
  }
  NSView *np=(NSView *)newPar;
  if (np && [np isKindOfClass:[NSWindow class]]) np=[(NSWindow *)np contentView];
  
  if (np && [np isKindOfClass:[NSView class]])
  {
    [np addSubview:v];
    [v release];
  }
}


int IsChild(HWND hwndParent, HWND hwndChild)
{
  if (!hwndParent || !hwndChild || hwndParent == hwndChild) return 0;
  id par=(id)hwndParent;
  id ch=(id)hwndChild;
  if (![ch isKindOfClass:[NSView class]]) return 0;
  if ([par isKindOfClass:[NSWindow class]])
  {
    return [ch window] == par;
  }
  else if ([par isKindOfClass:[NSView class]])
  {
    return !![ch isDescendantOf:par];
  }
  return 0;
}

HWND GetFocus()
{
  NSWindow *window=[NSApp keyWindow];
  if (!window) return 0;
  id ret=[window firstResponder];
  if (ret && [ret isKindOfClass:[NSView class]]) return (HWND) ret;
  return 0;
}


// timer stuff
typedef struct
{
  int timerid;
  HWND hwnd;
  NSTimer *timer;
} TimerInfoRec;
static WDL_PtrList<TimerInfoRec> m_timerlist;
static WDL_Mutex m_timermutex;
void SetTimer(HWND hwnd, int timerid, int rate, unsigned long *notUsed)
{
  if (!hwnd) return;
  WDL_MutexLock lock(&m_timermutex);
  KillTimer(hwnd,timerid);
  TimerInfoRec *rec=(TimerInfoRec*)malloc(sizeof(TimerInfoRec));
  rec->timerid=timerid;
  rec->hwnd=hwnd;
  
  simpleDataHold *t=[[simpleDataHold alloc] initWithVal:timerid];
  rec->timer = [NSTimer scheduledTimerWithTimeInterval:1.0/max(rate,1) target:(id)hwnd selector:@selector(SWELL_Timer:) 
                                              userInfo:t repeats:YES];
  [t release];
  m_timerlist.Add(rec);
  
}
void KillTimer(HWND hwnd, int timerid)
{
  WDL_MutexLock lock(&m_timermutex);
  int x;
  for (x = 0; x < m_timerlist.GetSize(); x ++)
  {
    if (m_timerlist.Get(x)->timerid == timerid && m_timerlist.Get(x)->hwnd == hwnd)
    {
      [m_timerlist.Get(x)->timer invalidate];
      m_timerlist.Delete(x,true,free);
      break;
    }
  }
}



void SetDlgItemText(HWND hwnd, int idx, const char *text)
{
  NSView *poo=(NSView *)GetDlgItem(hwnd,idx);
  if (!poo) return;
  NSString *lbl=(NSString *)CFStringCreateWithCString(NULL,text,kCFStringEncodingUTF8);
  if ([poo isKindOfClass:[NSButton class]]) [(NSButton*)poo setTitle:lbl];
  else if ([poo isKindOfClass:[NSControl class]]) [(NSControl*)poo setStringValue:lbl];
  [lbl release];
}

void GetDlgItemText(HWND hwnd, int idx, char *text, int textlen)
{
  *text=0;
  NSView *poo=(NSView *)GetDlgItem(hwnd,idx);
  if (!poo) return;
  NSString *s;
  
  if ([poo isKindOfClass:[NSButton class]]) s=[((NSButton *)poo) title];
  else if ([poo isKindOfClass:[NSControl class]]) s=[((NSControl *)poo) stringValue];
  else return;
  
  if (s)
    [s getCString:text maxLength:textlen];
}

void CheckDlgButton(HWND hwnd, int idx, int check)
{
  NSView *poo=(NSView *)GetDlgItem(hwnd,idx);
  if (!poo) return;
  if ([poo isKindOfClass:[NSButton class]]) [(NSButton*)poo setState:(check&BST_CHECKED)?NSOnState:NSOffState];
}


int IsDlgButtonChecked(HWND hwnd, int idx)
{
  NSView *poo=(NSView *)GetDlgItem(hwnd,idx);
  if (poo && [poo isKindOfClass:[NSButton class]])
    return [(NSButton*)poo state]!=NSOffState;
  return 0;
}

void SWELL_TB_SetPos(HWND hwnd, int idx, int pos)
{
  NSSlider *p=(NSSlider *)GetDlgItem(hwnd,idx);
  if (!p || ![p isKindOfClass:[NSSlider class]]) return;
  
  [p setDoubleValue:(double)pos];
}

void SWELL_TB_SetRange(HWND hwnd, int idx, int low, int hi)
{
  NSSlider *p=(NSSlider *)GetDlgItem(hwnd,idx);
  if (!p || ![p isKindOfClass:[NSSlider class]]) return;
  [p setMinValue:low];
  [p setMaxValue:hi];
}

int SWELL_TB_GetPos(HWND hwnd, int idx)
{
  NSSlider *p=(NSSlider *)GetDlgItem(hwnd,idx);
  if (!p || ![p isKindOfClass:[NSSlider class]]) return 0;
  return (int) ([p doubleValue]+0.5);
}

void SWELL_TB_SetTic(HWND hwnd, int idx, int pos)
{
  NSSlider *p=(NSSlider *)GetDlgItem(hwnd,idx);
  if (!p || ![p isKindOfClass:[NSSlider class]]) return;
}


int SWELL_CB_AddString(HWND hwnd, int idx, const char *str)
{
  NSString *label=(NSString *)CFStringCreateWithCString(NULL,str,kCFStringEncodingUTF8);
  NSComboBox *p=(NSComboBox *)GetDlgItem(hwnd,idx);
  
  int ret=[p numberOfItems];
  if ([p isKindOfClass:[NSComboBox class]])
  {
    [p addItemWithObjectValue:label];
    [p setNumberOfVisibleItems:(ret+1)];
  }
  else
  {
    [(NSPopUpButton *)p addItemWithTitle:label];
  }
  [label release];
  return ret;
}

int SWELL_CB_GetCurSel(HWND hwnd, int idx)
{
  NSComboBox *p=(NSComboBox *)GetDlgItem(hwnd,idx);
  return [p indexOfSelectedItem];
}

void SWELL_CB_SetCurSel(HWND hwnd, int idx, int item)
{
  return [(NSComboBox *)GetDlgItem(hwnd,idx) selectItemAtIndex:item];
}

int SWELL_CB_GetNumItems(HWND hwnd, int idx)
{
  return [(NSComboBox *)GetDlgItem(hwnd,idx) numberOfItems];
}



void SWELL_CB_SetItemData(HWND hwnd, int idx, int item, int data)
{
  id cb=(id)GetDlgItem(hwnd,idx);
  if (!cb || ![cb isKindOfClass:[NSPopUpButton class]]) return;
  NSMenuItem *it=[cb itemAtIndex:item];
  if (!it) return;
  
  // todo: does this need to free the old one? thinking not
  simpleDataHold *p=[[simpleDataHold alloc] initWithVal:data];
  
  [it setRepresentedObject:p];
  
  [p release];
}

int SWELL_CB_GetItemData(HWND hwnd, int idx, int item)
{
  id cb=(id)GetDlgItem(hwnd,idx);
  if (!cb || ![cb isKindOfClass:[NSPopUpButton class]]) return 0;
  NSMenuItem *it=[cb itemAtIndex:item];
  if (!it) return 0;
  id p= [it representedObject];
  if (!p || ![p isKindOfClass:[simpleDataHold class]]) return 0;
  return [p getValue];
}

void SWELL_CB_Empty(HWND hwnd, int idx)
{
  id cb=(id)GetDlgItem(hwnd,idx);
  if (!cb || ![cb isKindOfClass:[NSPopUpButton class]]) return;  
  [cb removeAllItems];
}


void SetDlgItemInt(HWND hwnd, int idx, int val, int issigned)
{
  char buf[128];
  sprintf(buf,issigned?"%d":"%u",val);
  SetDlgItemText(hwnd,idx,buf);
}

int GetDlgItemInt(HWND hwnd, int idx, BOOL *translated, int issigned)
{
  char buf[128];
  GetDlgItemText(hwnd,idx,buf,sizeof(buf));
  char *p=buf;
  while (*p == ' ' || *p == '\t') p++;
  int a=atoi(p);
  if ((a<0 && !issigned) || (!a && p[0] != '0')) { if (translated) *translated=0; return 0; }
  if (translated) *translated=1;
  return a;
}


void ShowWindow(HWND hwnd, int cmd)
{
  id pid=(id)hwnd;
  if (!pid || ![pid isKindOfClass:[NSView class]]) return;
  
  switch (cmd)
  {
    case SW_SHOW:
    case SW_SHOWNA:
      [((NSView *)pid) setHidden:NO];
    break;
    case SW_HIDE:
      [((NSView *)pid) setHidden:YES];
    break;
  }
}

void *SWELL_ModalWindowStart(HWND hwnd)
{
  if (!hwnd) return 0;
  return (void *)[NSApp beginModalSessionForWindow:(NSWindow *)hwnd];
}

bool SWELL_ModalWindowRun(void *ctx, int *ret) // returns false and puts retval in *ret when done
{
  if (!ctx) return false;
  int r=[NSApp runModalSession:(NSModalSession)ctx];
  if (r==NSRunContinuesResponse) return true;
  if (ret) *ret=r;
  return false;
}

void SWELL_ModalWindowEnd(void *ctx)
{
  if (ctx) 
  {
    if ([NSApp runModalSession:(NSModalSession)ctx] == NSRunContinuesResponse)
    {    
      [NSApp stopModal];
      while ([NSApp runModalSession:(NSModalSession)ctx]==NSRunContinuesResponse) Sleep(10);
    }
    [NSApp endModalSession:(NSModalSession)ctx];
  }
}

void SWELL_CloseWindow(HWND hwnd)
{
  if (hwnd)
  {
    [((NSWindow*)hwnd) close];
  }
}


#include "swell-dlggen.h"

static NSView *m_parent;
static NSRect m_transform;
static float m_parent_h;
static bool m_doautoright;
static NSRect m_lastdoauto;
static int m_parentMode;

void SWELL_Make_SetMessageMode(int wantParentView)
{
  m_parentMode=wantParentView;
}
#define ACTIONTARGET (m_parentMode ? m_parent : [m_parent window])

void SWELL_MakeSetCurParms(float xscale, float yscale, float xtrans, float ytrans, HWND parent, bool doauto)
{
  m_parentMode=0;
  m_lastdoauto.origin.x = 0;
  m_lastdoauto.origin.y = -100;
  m_lastdoauto.size.width = 0;
  m_doautoright=doauto;
  m_transform.origin.x=xtrans;
  m_transform.origin.y=ytrans;
  m_transform.size.width=xscale;
  m_transform.size.height=yscale;
  m_parent=(NSView *)parent;
  m_parent_h=100.0;
  if ([(id)parent isKindOfClass:[NSView class]])
    m_parent_h=[(NSView *)parent bounds].size.height;
  else if ([(id)parent isKindOfClass:[NSWindow class]])
    m_parent_h=[((NSWindow *)parent) frame].size.height;
}

static void UpdateAutoCoords(NSRect r)
{
  m_lastdoauto.size.width=r.origin.x + r.size.width - m_lastdoauto.origin.x;
}

static NSRect MakeCoords(int x, int y, int w, int h, bool wantauto)
{
  float ysc=m_transform.size.height;
  NSRect ret= NSMakeRect((x+m_transform.origin.x)*m_transform.size.width,  
                         ysc >= 0.0 ? m_parent_h - ((y+m_transform.origin.y) + h)*ysc : 
                         ((y+m_transform.origin.y) )*-ysc
                         ,
                    w*m_transform.size.width,h*fabs(ysc));
  NSRect oret=ret;
  if (wantauto && m_doautoright)
  {
    float dx = ret.origin.x - m_lastdoauto.origin.x;
    if (fabs(dx<32) && m_lastdoauto.origin.y >= ret.origin.y && m_lastdoauto.origin.y < ret.origin.y + ret.size.height)
    {
      ret.origin.x += m_lastdoauto.size.width;
    }
    
    m_lastdoauto.origin.x = oret.origin.x + oret.size.width;
    m_lastdoauto.origin.y = ret.origin.y + ret.size.height*0.5;
    m_lastdoauto.size.width=0;
  }
  return ret;
}

/// these are for swell-dlggen.h
void SWELL_MakeButton(int def, const char *label, int idx, int x, int y, int w, int h)
{  
  NSButton *button=[[NSButton alloc] init];
  [button setTag:idx];
  [button setBezelStyle:NSRoundedBezelStyle ];
  [button setFrame:MakeCoords(x,y,w,h,true)];
  NSString *labelstr=(NSString *)CFStringCreateWithCString(NULL,label,kCFStringEncodingUTF8);
  [button setTitle:labelstr];
  [button setTarget:ACTIONTARGET];
  [button setAction:@selector(onCommand:)];
  [m_parent addSubview:button];
  if (m_doautoright) UpdateAutoCoords([button frame]);
  if (def) [[m_parent window] setDefaultButtonCell:(NSButtonCell*)button];
  [labelstr release];
  [button release];
}

void SWELL_MakeEditField(int idx, int x, int y, int w, int h, int flags)
{  
  NSTextField *obj=[[NSTextField alloc] init];
  [obj setEditable:(flags & ES_READONLY)?NO:YES];
  //if (flags & ES_WANTRETURN)
//    [obj 
  if (h < 20)
  {
    [[obj cell] setWraps:NO];
    [[obj cell] setScrollable:YES];
  }
  [obj setTag:idx];
  [obj setFrame:MakeCoords(x,y,w,h,true)];
  [obj setTarget:ACTIONTARGET];
  [obj setAction:@selector(onCommand:)];
  [obj setDelegate:ACTIONTARGET];
  [m_parent addSubview:obj];
  if (m_doautoright) UpdateAutoCoords([obj frame]);
  
  [obj release];
}

void SWELL_MakeLabel( int align, const char *label, int idx, int x, int y, int w, int h, int flags)
{
  NSTextField *obj=[[NSTextField alloc] init];
  [obj setEditable:NO];
  [obj setSelectable:NO];
  [obj setBordered:NO];
  [obj setBezeled:NO];
  [obj setDrawsBackground:NO];

  if (flags & SS_NOTFIY)
  {
    [obj setTarget:ACTIONTARGET];
    [obj setAction:@selector(onCommand:)];
  }
  
  NSString *labelstr=(NSString *)CFStringCreateWithCString(NULL,label,kCFStringEncodingUTF8);
  [obj setStringValue:labelstr];
  [obj setAlignment:(align<0?NSLeftTextAlignment:align>0?NSRightTextAlignment:NSCenterTextAlignment)];
  [obj setTag:idx];
  [obj setFrame:MakeCoords(x,y,w,h,true)];
  [m_parent addSubview:obj];
  [obj sizeToFit];
  if (m_doautoright) UpdateAutoCoords([obj frame]);
  [obj release];
  [labelstr release];
}


@interface TaggedProgressIndicator : NSProgressIndicator
{
  int m_tag;
}
@end
@implementation TaggedProgressIndicator
-(int) tag
{
  return m_tag;
}
-(void) setTag:(int)tag
{
  m_tag=tag;
}
@end

@interface TaggedScrollView : NSScrollView
{
  int m_tag;
}
@end

void SWELL_MakeControl(const char *cname, int idx, const char *classname, int style, int x, int y, int w, int h)
{
  if (!stricmp(classname, "SysListView32"))
  {
    NSTableView *obj=[[NSTableView alloc] init];
    [obj setAllowsMultipleSelection:YES];
    [obj setAllowsEmptySelection:YES];
    [obj setTag:idx];
    [obj setHidden:NO];
    [obj setTarget:ACTIONTARGET];
    [obj setAction:@selector(onCommand:)];
    NSScrollView *obj2=[[NSScrollView alloc] init];
    [obj2 setFrame:MakeCoords(x,y,w,h,false)];
    [obj2 setDocumentView:obj];
    [obj release];
    [m_parent addSubview:obj2];
    [obj2 release];
  }
  else if (!stricmp(classname, "msctls_progress32"))
  {
    TaggedProgressIndicator *obj=[[TaggedProgressIndicator alloc] init];
    [obj setStyle:NSProgressIndicatorBarStyle];
    [obj setIndeterminate:NO];
    [obj setTag:idx];
    [obj setFrame:MakeCoords(x,y,w,h,false)];
    [m_parent addSubview:obj];
    [obj release];
  }
  else if (!stricmp(classname,"Button"))
  {
    NSButton *button=[[NSButton alloc] init];
    [button setTag:idx];
    if (style & BS_AUTOCHECKBOX)
    {
      [button setButtonType:NSSwitchButton];
    }
    else if (style & BS_AUTORADIOBUTTON)
    {
      [button setButtonType:NSRadioButton];
    }
    [button setFrame:MakeCoords(x,y,w,h,true)];
    NSString *labelstr=(NSString *)CFStringCreateWithCString(NULL,cname,kCFStringEncodingUTF8);
    [button setTitle:labelstr];
    [button setTarget:ACTIONTARGET];
    [button setAction:@selector(onCommand:)];
    [m_parent addSubview:button];
    [button sizeToFit];
    if (m_doautoright) UpdateAutoCoords([button frame]);
    [labelstr release];
    [button release];
  }
  else if (!stricmp(classname,"REAPERhfader"))
  {
    NSSlider *obj=[[NSSlider alloc] init];
    [obj setTag:idx];
    [obj setMinValue:0.0];
    [obj setMaxValue:1000.0];
    [obj setFrame:MakeCoords(x,y,w,h,false)];
    [obj setTarget:ACTIONTARGET];
    [obj setAction:@selector(onCommand:)];
    [m_parent addSubview:obj];
    [obj release];
  }
}

void SWELL_MakeCombo(int idx, int x, int y, int w, int h, int flags)
{
  if (flags & CBS_DROPDOWNLIST)
  {
    NSPopUpButton *obj=[[NSPopUpButton alloc] init];
    [obj setTag:idx];
    [obj setFrame:MakeCoords(x,y-1,w,14,true)];
    [obj setAutoenablesItems:NO];
    [obj setTarget:ACTIONTARGET];
    [obj setAction:@selector(onCommand:)];
    [m_parent addSubview:obj];
    if (m_doautoright) UpdateAutoCoords([obj frame]);
    [obj release];
  }
  else
  {
    NSComboBox *obj=[[NSComboBox alloc] init];
    [obj setEditable:(flags & CBS_DROPDOWNLIST)?NO:YES];
    [obj setTag:idx];
    [obj setFrame:MakeCoords(x,y-1,w,14,true)];
    [obj setTarget:ACTIONTARGET];
    [obj setAction:@selector(onCommand:)];
    [obj setDelegate:ACTIONTARGET];
    [m_parent addSubview:obj];
    if (m_doautoright) UpdateAutoCoords([obj frame]);
    [obj release];
  }
}

@interface TaggedBox : NSBox
{
  int m_tag;
}
@end
@implementation TaggedBox
-(int) tag
{
  return m_tag;
}
-(void) setTag:(int)tag
{
  m_tag=tag;
}
@end

void SWELL_MakeGroupBox(const char *name, int idx, int x, int y, int w, int h)
{
  TaggedBox *obj=[[TaggedBox alloc] init];
//  [obj setTag:idx];
  NSString *labelstr=(NSString *)CFStringCreateWithCString(NULL,name,kCFStringEncodingUTF8);
  [obj setTitle:labelstr];
  [obj setTag:idx];
  [labelstr release];
  [obj setFrame:MakeCoords(x,y,w,h,false)];
  [m_parent addSubview:obj];
  [obj release];
}
