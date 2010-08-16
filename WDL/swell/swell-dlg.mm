#ifndef SWELL_PROVIDED_BY_APP


#include "swell.h"
#include "swell-dlggen.h"

#import <Cocoa/Cocoa.h>
static HMENU g_swell_defaultmenu;


static LRESULT SwellDialogDefaultWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  DLGPROC d=(DLGPROC)GetWindowLong(hwnd,DWL_DLGPROC);
  if (d) return (LRESULT) d(hwnd,uMsg,wParam,lParam);
  return DefWindowProc(hwnd,uMsg,wParam,lParam);
}

static SWELL_DialogResourceIndex *resById(SWELL_DialogResourceIndex *reshead, int resid)
{
  SWELL_DialogResourceIndex *p=reshead;
  while (p)
  {
    if (p->resid == resid) return p;
    p=p->_next;
  }
  return 0;
}

typedef struct OwnedWindowListRec
{
  NSWindow *hwnd;
  struct OwnedWindowListRec *_next;
} OwnedWindowListRec;

typedef struct WindowPropRec
{
  char *name; // either <64k or a strdup'd name
  void *data;
  struct WindowPropRec *_next;
} WindowPropRec;



static void HandleCommand(id wnd, WNDPROC wndproc, id sender)
{
  if ([sender isKindOfClass:[NSSlider class]])
  {
    wndproc((HWND)wnd,WM_HSCROLL,0,(LPARAM)sender);
    //  WM_HSCROLL, WM_VSCROLL
  }
  else if ([sender isKindOfClass:[NSTableView class]])
  {
    NMLISTVIEW nmhdr={{(HWND)sender,(int)[sender tag],LVN_ITEMCHANGED},(int)[sender clickedRow],0}; 
    wndproc((HWND)wnd,WM_NOTIFY,(int)[sender tag],(LPARAM)&nmhdr);
  }
  else
  {
    int cw=0;
    if ([sender isKindOfClass:[NSControl class]])
    {
      NSEvent *evt=[NSApp currentEvent];
      int ty=evt?[evt type]:0;
      if (evt && (ty==NSLeftMouseDown || ty==NSLeftMouseUp) && [evt clickCount] > 1) cw=STN_DBLCLK;
    }
    wndproc((HWND)wnd,WM_COMMAND,[sender tag]|(cw<<16),0);
  }
}

@class simpleDataHold;
#define SWELLDIALOGCOMMONIMPLEMENTS \
- (void)SWELL_Timer:(id)sender \
{ \
  id uinfo=[sender userInfo]; \
  if ([uinfo respondsToSelector:@selector(getValue)]) { \
    int idx=(int)[(simpleDataHold*)uinfo getValue]; \
    m_wndproc((HWND)self,WM_TIMER,idx,0); \
  } \
} \
- (int)swellCapChangeNotify { return YES; } \
- (int)onSwellMessage:(UINT)msg p1:(WPARAM)wParam p2:(LPARAM)lParam \
{ \
  if (msg==WM_DESTROY) { if (m_hashaddestroy) return 0; m_hashaddestroy=true; if (GetCapture()==(HWND)self) ReleaseCapture(); SWELL_MessageQueue_Clear((HWND)self); }   \
  if (msg==WM_CAPTURECHANGED && m_hashaddestroy) return 0; \
  int ret=m_wndproc((HWND)self,msg,wParam,lParam);  \
  if (msg == WM_DESTROY) { \
    if (m_owner) [m_owner swellRemoveOwnedWindow:self]; \
      m_owner=0; \
      OwnedWindowListRec *p=m_ownedwnds; m_ownedwnds=0; \
      while (p) \
      {\
        OwnedWindowListRec *next=p->_next; \
        DestroyWindow((HWND)p->hwnd); \
        free(p); p=next; \
      } \
        if (m_menu) {\
          if ([NSApp mainMenu] == m_menu) [NSApp setMainMenu:nil]; \
            [(NSMenu *)m_menu release]; m_menu=0;  \
        }\
        NSView *v=[self isKindOfClass:[NSWindow class]] ? (NSView *)[self contentView] : (NSView *)self; \
        NSArray *ar; \
        if (v && [v isKindOfClass:[NSView class]] && (ar=[v subviews]) && [ar count]>0) { \
          int x; for (x = 0; x < [ar count]; x ++) { \
              NSView *sv=[ar objectAtIndex:x]; if (sv && [sv respondsToSelector:@selector(onSwellMessage:p1:p2:)]) \
                [sv onSwellMessage:WM_DESTROY p1:0 p2:0]; \
            } \
        } \
    KillTimer((HWND)self,-1); \
  } \
  return ret; \
} \
- (void) setEnabled:(BOOL)en \
{ m_enabled=en; } \
- (void)controlTextDidChange:(NSNotification *)aNotification \
{ \
  m_wndproc((HWND)self,WM_COMMAND,([[aNotification object] tag])|(EN_CHANGE<<16),0); \
} \
- (void)menuNeedsUpdate:(NSMenu *)menu \
{ \
  m_wndproc((HWND)self,WM_INITMENUPOPUP,(WPARAM)menu,0); \
} \
-(void) onControlDoubleClick:(id)sender \
{ \
  NMCLICK nm={{(HWND)sender,[sender tag],NM_DBLCLK}, }; \
  m_wndproc((HWND)self,WM_NOTIFY,[sender tag],(LPARAM)&nm); \
} \
-(void) onCommand:(id)sender \
{ \
  HandleCommand(self,m_wndproc,sender);   \
} \
-(void) dealloc \
{ \
  KillTimer((HWND)self,-1); \
  [self onSwellMessage:WM_DESTROY p1:0 p2:0]; \
      [super dealloc]; \
} \
-(int)tag { return m_tag; } \
-(void)setTag:(int)t { m_tag=t; } \
-(int)getSwellUserData { return m_userdata; } \
-(void)setSwellUserData:(int)val {   m_userdata=val; } \
-(int)getSwellExtraData:(int)idx { if (idx>=0&&idx<sizeof(m_extradata)-3) return  *(int*)(m_extradata+idx); return 0; } \
-(void)setSwellExtraData:(int)idx value:(int)val {  if (idx>=0&&idx<sizeof(m_extradata)-3) *(int*)(m_extradata+idx) = val; }  \
-(void)setSwellWindowProc:(int)val { m_wndproc=(WNDPROC)val; } \
-(int)getSwellWindowProc { return (int)m_wndproc; } \
-(void)setSwellDialogProc:(int)val { m_dlgproc=(DLGPROC)val; } \
-(int)getSwellDialogProc { return (int)m_dlgproc; } \
-(BOOL)isFlipped {   return m_flip; } \
-(void) getSwellPaintInfo:(PAINTSTRUCT *)ps \
{ \
  if (m_paintctx_hdc) \
  { \
    m_paintctx_used=1; \
    ps->hdc = m_paintctx_hdc; \
    ps->fErase=false; \
    ps->rcPaint.left = (int)m_paintctx_rect.origin.x; \
    ps->rcPaint.right = (int)ceil(m_paintctx_rect.origin.x+m_paintctx_rect.size.width); \
    ps->rcPaint.top = (int)m_paintctx_rect.origin.y; \
    ps->rcPaint.bottom  = (int)ceil(m_paintctx_rect.origin.y+m_paintctx_rect.size.height);    \
  } \
} \
-(bool)swellCanPostMessage { return !m_hashaddestroy; } \
-(int)swellEnumProps:(PROPENUMPROCEX)proc lp:(LPARAM)lParam { \
  WindowPropRec *p=m_props; \
  if (!p) return -1; \
    while (p) { \
      WindowPropRec *ps=p;  \
      p=p->_next; \
      if (!proc((HWND)self, ps->name, ps->data, lParam)) return 0; \
    } \
  return 1; \
} \
-(void *)swellGetProp:(const char *)name wantRemove:(BOOL)rem { \
  WindowPropRec *p=m_props, *lp=NULL; \
  while (p) { \
      if (p->name < (void *)65536) { \
        if (name==p->name) break; \
      } \
      else if (name >= (void *)65536) { \
        if (!strcmp(name,p->name)) break; \
      } \
      lp=p; p=p->_next; \
  } \
  if (!p) return NULL; \
  void *ret=p->data; \
  if (rem) { \
    if (lp) lp->_next=p->_next; else m_props=p->_next;  \
    free(p); \
  } \
  return ret; \
} \
-(int)swellSetProp:(const char *)name value:(void *)val { \
  WindowPropRec *p=m_props; \
  while (p) { \
    if (p->name < (void *)65536) { \
      if (name==p->name) { p->data=val; return TRUE; }; \
    } \
    else if (name >= (void *)65536) { \
      if (!strcmp(name,p->name)) { p->data=val; return TRUE; }; \
    } \
    p=p->_next; \
  } \
  p=(WindowPropRec*)malloc(sizeof(WindowPropRec)); \
  p->name = (name<(void*)65536) ? (char *)name : strdup(name); \
  p->data = val; p->_next=m_props; m_props=p; \
  return TRUE; \
}


#define SWELLDIALOGCOMMONIMPLEMENTS_WND \
- (void)setFrame:(NSRect)frameRect display:(BOOL)displayFlag \
{ \
  [super setFrame:frameRect display:displayFlag]; \
  m_wndproc((HWND)self,WM_SIZE,0,0); \
  [self display]; \
} \
-(HMENU)swellGetMenu {   return m_menu; } \
-(void)swellSetMenu:(HMENU)menu {   m_menu=menu; } \
-(void)becomeKeyWindow \
{ \
  [super becomeKeyWindow]; \
    HMENU menu=m_menu?m_menu:g_swell_defaultmenu; \
    if (menu && menu != [NSApp mainMenu])  [NSApp setMainMenu:(NSMenu *)menu]; \
} \
-(BOOL)windowShouldClose:(id)sender \
{ \
  if (!m_wndproc((HWND)self,WM_CLOSE,0,0)) m_wndproc((HWND)self,WM_COMMAND,IDCANCEL,0); \
  return NO; \
} \
- (void)keyDown:(NSEvent *)theEvent \
{ \
  if ([theEvent keyCode]==53) { \
    if (!m_wndproc((HWND)self,WM_CLOSE,0,0)) \
      m_wndproc((HWND)self,WM_COMMAND,IDCANCEL,0); \
  } \
  else [super keyDown:theEvent]; \
} \
-(int)sendMouseMessage:(int)msg event:(NSEvent*)theEvent \
{ \
  if (m_hashaddestroy) return -1; \
	NSPoint p=[theEvent locationInWindow]; \
	unsigned short xpos=(int)(p.x); unsigned short ypos=(int)(p.y); \
  int l=0; \
  if (msg==WM_MOUSEWHEEL) \
  { \
    l=(int) ([theEvent deltaY]*60.0); \
    l<<=16; \
  } \
  int ret=(int)m_wndproc((HWND)self,msg,l,xpos + (ypos<<16)); \
  if (msg==WM_SETCURSOR && !ret) { \
      NSCursor *arr= [NSCursor arrowCursor]; \
        if (GetCursor() != (HCURSOR)arr) SetCursor((HCURSOR)arr); \
    } \
  if (msg==WM_RBUTTONUP && !ret) { \
    p=[self convertBaseToScreen:p]; \
    m_wndproc((HWND)self,WM_CONTEXTMENU,(WPARAM)self,(int)p.x + (((int)p.y)<<16)); \
  } \
  return ret; \
} \
-(void) SwellWnd_drawRect:(NSRect)rect \
{ \
  if (m_hashaddestroy) return; \
  m_paintctx_hdc=WDL_GDP_CreateContext([[NSGraphicsContext currentContext] graphicsPort]); \
    m_paintctx_rect=rect; \
      m_paintctx_used=false; \
        m_wndproc((HWND)self,WM_NCPAINT,(WPARAM)1,0); \
          m_wndproc((HWND)self,WM_PAINT,(WPARAM)m_paintctx_hdc,0); \
          WDL_GDP_DeleteContext(m_paintctx_hdc); \
            m_paintctx_hdc=0; \
              if (!m_paintctx_used) { \
                /*[super drawRect:rect];*/ \
              } \
} \
- (void)SwellWnd_rightMouseDragged:(NSEvent *)theEvent \
{ \
  [self SwellWnd_mouseDragged:theEvent]; \
} \
- (void)SwellWnd_mouseDragged:(NSEvent *)theEvent \
{ \
  if (!m_enabled) return; \
  [self sendMouseMessage:WM_MOUSEMOVE event:theEvent]; \
  if (SWELL_GetLastSetCursor()!=GetCursor()) SetCursor(SWELL_GetLastSetCursor()); \
} \
- (void)SwellWnd_mouseMoved:(NSEvent *)theEvent \
{ \
  if (!m_enabled) return; \
  if (!GetCapture() || GetCapture()==(HWND)self) { \
    [self sendMouseMessage:WM_MOUSEMOVE event:theEvent]; \
    [self sendMouseMessage:WM_SETCURSOR event:theEvent]; \
  } \
} \
- (void)SwellWnd_mouseUp:(NSEvent *)theEvent \
{ \
  if (!m_enabled) return; \
	if (m_isfakerightmouse) [self rightMouseUp:theEvent]; \
  else  \
  { \
    [self sendMouseMessage:WM_LBUTTONUP event:theEvent]; \
    [self sendMouseMessage:WM_SETCURSOR event:theEvent]; \
  } \
} \
- (void)SwellWnd_scrollWheel:(NSEvent *)theEvent \
{ \
  if (!m_enabled) return; \
  [self sendMouseMessage:WM_MOUSEWHEEL event:theEvent]; \
} \
- (void)SwellWnd_mouseDown:(NSEvent *)theEvent \
{ \
  if (!m_enabled) return; \
	m_isfakerightmouse=0; \
    if ([theEvent modifierFlags] & NSControlKeyMask) \
    { \
      [self rightMouseDown:theEvent]; \
        if ([theEvent clickCount]<2) m_isfakerightmouse=1; \
          return; \
    } \
    [self sendMouseMessage:([theEvent clickCount]>1 ? WM_LBUTTONDBLCLK : WM_LBUTTONDOWN) event:theEvent]; \
}	\
- (int)SwellWnd_rightMouseUp:(NSEvent *)theEvent \
{ \
  if (!m_enabled) return 0; \
  m_isfakerightmouse=0; \
  int ret=[self sendMouseMessage:WM_RBUTTONUP event:theEvent]; \
  [self sendMouseMessage:WM_SETCURSOR event:theEvent]; \
  return ret; \
}  \
- (void)SwellWnd_rightMouseDown:(NSEvent *)theEvent \
{ \
  if (!m_enabled) return; \
  m_isfakerightmouse=0; \
  [self sendMouseMessage:([theEvent clickCount]>1 ? WM_RBUTTONDBLCLK : WM_RBUTTONDOWN) event:theEvent]; \
}   \
- (void)swellAddOwnedWindow:(NSWindow*)wnd \
{ \
  OwnedWindowListRec *p=m_ownedwnds; \
  while (p) { \
    if (p->hwnd == wnd) return; \
    p=p->_next; \
  } \
  p=(OwnedWindowListRec*)malloc(sizeof(OwnedWindowListRec)); \
  p->hwnd=wnd; p->_next=m_ownedwnds; m_ownedwnds=p; \
  if ([wnd respondsToSelector:@selector(swellSetOwner:)]) [wnd swellSetOwner:self];  \
}  \
- (void)swellRemoveOwnedWindow:(NSWindow *)wnd \
{ \
  OwnedWindowListRec *p=m_ownedwnds, *lp=NULL; \
  while (p) { \
    if (p->hwnd == wnd) { \
      if (lp) lp->_next=p->_next; \
      else m_ownedwnds=p->_next; \
      free(p); \
      return; \
    } \
    lp=p; \
    p=p->_next; \
  } \
} \
- (void)swellSetOwner:(id)owner { m_owner=owner; } \
- (id)swellGetOwner { return m_owner; } \
- (BOOL)SwellWnd_isOpaque { return m_isopaque; }


#define DECLARE_COMMON_VARS \
  BOOL m_enabled; \
  DLGPROC m_dlgproc; \
  WNDPROC m_wndproc; \
  int m_userdata;  \
  char m_extradata[128]; \
  int m_tag; \
  int m_isfakerightmouse; \
  bool m_hashaddestroy; \
  HMENU m_menu; \
  BOOL m_flip; \
  bool m_paintctx_used; \
  HDC m_paintctx_hdc; \
  id m_owner; \
  OwnedWindowListRec *m_ownedwnds; \
  WindowPropRec *m_props; \
  NSRect m_paintctx_rect; \
  BOOL m_isopaque;

#define INIT_COMMON_VARS \
  m_enabled=TRUE; \
  m_dlgproc=NULL;  \
  m_wndproc=NULL; \
  m_userdata=0; \
  memset(&m_extradata,0,sizeof(m_extradata));  \
  m_tag=0; \
  m_isfakerightmouse=0; \
  m_hashaddestroy=false; \
  m_menu=0; \
  m_flip=0; \
  m_paintctx_used=0; \
  m_paintctx_hdc=0; \
  m_owner=0; \
  m_ownedwnds=0; \
  m_props=0;

@interface SWELLDialogChild : NSView
{
  DECLARE_COMMON_VARS
}
- (id)initChild:(SWELL_DialogResourceIndex *)resstate Parent:(NSView *)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par;
-(int)sendMouseMessage:(int)msg event:(NSEvent*)theEvent;
@end

@implementation SWELLDialogChild : NSView 

SWELLDIALOGCOMMONIMPLEMENTS


- (id)initChild:(SWELL_DialogResourceIndex *)resstate Parent:(NSView *)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par
{
  INIT_COMMON_VARS
  if (resstate)
  {
    m_wndproc=SwellDialogDefaultWindowProc;
    m_dlgproc=dlgproc;  
  }
  else
    m_wndproc=(WNDPROC)dlgproc;
  
  m_isopaque = !resstate || (resstate->windowTypeFlags&SWELL_DLG_WS_OPAQUE);
  m_flip = !resstate || (resstate->windowTypeFlags&SWELL_DLG_WS_FLIPPED);
  NSRect contentRect=NSMakeRect(0,0,resstate ? resstate->width : 300,resstate ? resstate->height : 200);
  if (!(self = [super initWithFrame:contentRect])) return self;
  
  if (resstate) resstate->createFunc((HWND)self,1);
  
  [parent addSubview:self];
  if (!resstate)
    m_wndproc((HWND)self,WM_CREATE,0,par);
  m_wndproc((HWND)self,WM_INITDIALOG,0,par);
    
  return self;
}

-(BOOL)isOpaque
{
  return m_isopaque;
}

- (void)setFrame:(NSRect)frameRect 
{
  [super setFrame:frameRect];
    m_wndproc((HWND)self,WM_SIZE,0,0); 
} 
- (void)keyDown:(NSEvent *)theEvent
{
  int flag,code=SWELL_MacKeyToWindowsKey(theEvent,&flag);
  if (!m_wndproc((HWND)self,WM_KEYDOWN,code,0)) [super keyDown:theEvent];
}
- (void)keyUp:(NSEvent *)theEvent
{
  int flag,code=SWELL_MacKeyToWindowsKey(theEvent,&flag);
  if (!m_wndproc((HWND)self,WM_KEYUP,code,0)) [super keyUp:theEvent];
}


-(void) drawRect:(NSRect)rect
{
  if (m_hashaddestroy) return;
  m_paintctx_hdc=WDL_GDP_CreateContext([[NSGraphicsContext currentContext] graphicsPort]);
  m_paintctx_rect=rect;
  m_paintctx_used=false;
  m_wndproc((HWND)self,WM_NCPAINT,(WPARAM)1,0);
  m_wndproc((HWND)self,WM_PAINT,(WPARAM)m_paintctx_hdc,0);
  WDL_GDP_DeleteContext(m_paintctx_hdc);
  m_paintctx_hdc=0;
  if (!m_paintctx_used) {
     /*[super drawRect:rect];*/
  }
}
- (void)rightMouseDragged:(NSEvent *)theEvent
{
  if (!m_enabled) return;
  [self mouseDragged:theEvent];
}
- (void)mouseDragged:(NSEvent *)theEvent
{
  if (!m_enabled) return;
  [self sendMouseMessage:WM_MOUSEMOVE event:theEvent];
  if (SWELL_GetLastSetCursor()!=GetCursor()) SetCursor(SWELL_GetLastSetCursor());
}
- (void)mouseMoved:(NSEvent *)theEvent
{
  if (!m_enabled) return;
  if (!GetCapture() || GetCapture()==(HWND)self) { 
    [self sendMouseMessage:WM_MOUSEMOVE event:theEvent];
    [self sendMouseMessage:WM_SETCURSOR event:theEvent];
  }
}
- (void)mouseUp:(NSEvent *)theEvent
{
  if (!m_enabled) return;
	if (m_isfakerightmouse) return [self rightMouseUp:theEvent];
    [self sendMouseMessage:WM_LBUTTONUP event:theEvent];
    [self sendMouseMessage:WM_SETCURSOR event:theEvent];
}
- (void)scrollWheel:(NSEvent *)theEvent
{
  if (!m_enabled) return;
  [self sendMouseMessage:WM_MOUSEWHEEL event:theEvent];
}
- (void)mouseDown:(NSEvent *)theEvent
{
  if (!m_enabled) return;
	m_isfakerightmouse=0;
  if ([theEvent modifierFlags] & NSControlKeyMask)
  {
    [self rightMouseDown:theEvent];
    if ([theEvent clickCount]<2) m_isfakerightmouse=1;
        return;
  }
  [self sendMouseMessage:([theEvent clickCount]>1 ? WM_LBUTTONDBLCLK : WM_LBUTTONDOWN) event:theEvent];
}
- (int)rightMouseUp:(NSEvent *)theEvent
{
  if (!m_enabled) return 0;
  m_isfakerightmouse=0;
  int ret=[self sendMouseMessage:WM_RBUTTONUP event:theEvent];
  
  [self sendMouseMessage:WM_SETCURSOR event:theEvent];
  return ret;
}
- (void)rightMouseDown:(NSEvent *)theEvent
{
  m_isfakerightmouse=0;
  [self sendMouseMessage:([theEvent clickCount]>1 ? WM_RBUTTONDBLCLK : WM_RBUTTONDOWN) event:theEvent]; 
}  


-(int)sendMouseMessage:(int)msg event:(NSEvent*)theEvent
{
  if (m_hashaddestroy) return -1;
	NSPoint localpt=[theEvent locationInWindow];
	NSPoint p = [self convertPoint:localpt fromView:nil];
	unsigned short xpos=(int)(p.x); unsigned short ypos=(int)(p.y);
  
  int l=0;
  if (msg==WM_MOUSEWHEEL)
  {
    l=(int) ([theEvent deltaY]*60.0);
    l<<=16;
  }
  
  // put todo: modifiers into low work of l
  
  int ret=m_wndproc((HWND)self,msg,l,xpos + (ypos<<16));
  if (msg==WM_RBUTTONUP && !ret && m_dlgproc) 
  {
    localpt=[[self window] convertBaseToScreen:localpt];
    m_wndproc((HWND)self,WM_CONTEXTMENU,(WPARAM)self,(int)localpt.x + (((int)localpt.y)<<16));
  }
  
  if (msg==WM_SETCURSOR && !ret) 
  {
    NSCursor *arr= [NSCursor arrowCursor];
    if (GetCursor() != (HCURSOR)arr) SetCursor((HCURSOR)arr);
   } 
   return ret;
} 

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
  if (!m_enabled) return NO;
	return YES;
}

@end


@interface SWELL_ReplacementContentView : NSView
{
  bool m_flip;
}
@end
@implementation SWELL_ReplacementContentView : NSView
-(void)setFlipped:(BOOL)fl
{
  m_flip=fl;
}
-(BOOL)isFlipped
{
  return YES;
}
- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
	return YES;
}

-(void) drawRect:(NSRect)rect
{ 
  NSWindow *wnd=[self window];
  if (wnd && [wnd respondsToSelector:@selector(SwellWnd_drawRect:)]) [wnd SwellWnd_drawRect:rect];
}

- (void)rightMouseDragged:(NSEvent *)theEvent
{ 
  [[self window] SwellWnd_rightMouseDragged:theEvent];
}

- (void)mouseDragged:(NSEvent *)theEvent
{
  [[self window] SwellWnd_mouseDragged:theEvent];
}

-(void)mouseMoved:(NSEvent *)theEvent
{
  [[self window] SwellWnd_mouseMoved:theEvent];
}

- (void)scrollWheel:(NSEvent *)theEvent
{
  [[self window] SwellWnd_scrollWheel:theEvent];
}

- (void)mouseDown:(NSEvent *)theEvent
{
  [[self window] SwellWnd_mouseDown:theEvent];  
}
- (void)mouseUp:(NSEvent *)theEvent
{
  [[self window] SwellWnd_mouseUp:theEvent];
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
  [[self window] SwellWnd_rightMouseDown:theEvent];
}  

- (int)rightMouseUp:(NSEvent *)theEvent
{
  return (int)[[self window] SwellWnd_rightMouseUp:theEvent];
}  

- (BOOL)isOpaque
{
  return (BOOL) (int)[[self window] SwellWnd_isOpaque];
}  

@end

@interface SWELLModelessDialog : NSWindow
{
  
  DECLARE_COMMON_VARS
  
}
- (id)initModeless:(SWELL_DialogResourceIndex *)resstate Parent:(HWND)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par;
-(int)sendMouseMessage:(int)msg event:(NSEvent*)theEvent;
@end


#define DOWINDOWMINMAXSIZES \
{ \
  MINMAXINFO mmi={0}; \
  NSSize minsz=(NSSize)[super contentMinSize]; \
  mmi.ptMinTrackSize.x=(int)minsz.width; mmi.ptMinTrackSize.y=(int)minsz.height; \
  m_wndproc((HWND)self,WM_GETMINMAXINFO,0,(LPARAM)&mmi); \
  minsz.width=mmi.ptMinTrackSize.x; minsz.height=mmi.ptMinTrackSize.y; \
  [super setContentMinSize:minsz];  \
} 


@implementation SWELLModelessDialog : NSWindow

SWELLDIALOGCOMMONIMPLEMENTS
SWELLDIALOGCOMMONIMPLEMENTS_WND


- (id)initModeless:(SWELL_DialogResourceIndex *)resstate Parent:(HWND)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par
{
  INIT_COMMON_VARS
  
  m_wndproc=SwellDialogDefaultWindowProc;
  m_dlgproc=dlgproc;
  m_flip = !resstate || (resstate->windowTypeFlags&SWELL_DLG_WS_FLIPPED);
  m_isopaque = resstate && (resstate->windowTypeFlags&SWELL_DLG_WS_OPAQUE);

  NSRect contentRect=NSMakeRect(50,50,resstate->width,resstate->height);
  if (!(self = [super initWithContentRect:contentRect styleMask:(NSTitledWindowMask|NSMiniaturizableWindowMask|NSClosableWindowMask|((resstate->windowTypeFlags&SWELL_DLG_WS_RESIZABLE) ? NSResizableWindowMask : 0)) backing:NSBackingStoreBuffered defer:NO])) return self;
  
  [self setAcceptsMouseMovedEvents:YES];
  SWELL_ReplacementContentView *ncv = [[SWELL_ReplacementContentView alloc] initWithFrame:NSMakeRect(0,0,20,20)];
  [ncv setFlipped:m_flip];
  [self setContentView:ncv];
  [ncv release];
  resstate->createFunc((HWND)[self contentView],0);
  if (resstate&&resstate->title) SetWindowText((HWND)self, resstate->title);
  
  
//  if (parent && [(id)parent isKindOfClass:[NSWindow class]]) [self setLevel:[(NSWindow *)parent level]+1];
  
  if (parent && [(id)parent respondsToSelector:@selector(swellAddOwnedWindow:)])
  {
    [(id)parent swellAddOwnedWindow:self]; 
  }
  else if (parent && [(id)parent isKindOfClass:[NSView class]])
  {
    NSWindow *w=[(id)parent window];
//    if (w) [self setLevel:[w level]+1];
    if (w && [w respondsToSelector:@selector(swellAddOwnedWindow:)])
    {
      [w swellAddOwnedWindow:self]; 
    }
  }
  
  m_wndproc((HWND)self,WM_INITDIALOG,0,par);
    
  DOWINDOWMINMAXSIZES
  
  return self;
}

@end



@interface SWELLDialogBox : NSPanel
{
  DECLARE_COMMON_VARS
  
  int m_rv;
}
- (id)initDialogBox:(SWELL_DialogResourceIndex *)resstate Parent:(HWND)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par;
-(int)sendMouseMessage:(int)msg event:(NSEvent*)theEvent;
@end

@implementation SWELLDialogBox : NSPanel



SWELLDIALOGCOMMONIMPLEMENTS
SWELLDIALOGCOMMONIMPLEMENTS_WND

  
- (id)initDialogBox:(SWELL_DialogResourceIndex *)resstate Parent:(HWND)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par
{
  m_rv=0;
  INIT_COMMON_VARS
  
  m_wndproc=SwellDialogDefaultWindowProc;
  m_dlgproc=dlgproc;
  m_flip = !resstate || (resstate->windowTypeFlags&SWELL_DLG_WS_FLIPPED);
  m_isopaque = resstate && (resstate->windowTypeFlags&SWELL_DLG_WS_OPAQUE);

  NSRect contentRect=NSMakeRect(0,0,resstate->width,resstate->height);
  if (!(self = [super initWithContentRect:contentRect styleMask:(NSTitledWindowMask|NSClosableWindowMask|((resstate->windowTypeFlags&SWELL_DLG_WS_RESIZABLE)? NSResizableWindowMask : 0)) backing:NSBackingStoreBuffered defer:NO])) return self;

  [self setAcceptsMouseMovedEvents:YES];

  SWELL_ReplacementContentView *ncv = [[SWELL_ReplacementContentView alloc] initWithFrame:NSMakeRect(0,0,20,20)];
  [ncv setFlipped:m_flip];
  [self setContentView:ncv];
  [ncv release];
  
  resstate->createFunc((HWND)[self contentView],0);
  
//  if (parent && [(id)parent isKindOfClass:[NSWindow class]]) [self setLevel:[(NSWindow *)parent level]+1];
  
  if (parent && [(id)parent respondsToSelector:@selector(swellAddOwnedWindow:)])
  {
    [(id)parent swellAddOwnedWindow:self]; 
  }
  else if (parent && [(id)parent isKindOfClass:[NSView class]])
  {
    NSWindow *w=[(id)parent window];
//    if (w) [self setLevel:[w level]+1];
    if (w && [w respondsToSelector:@selector(swellAddOwnedWindow:)])
    {
      [w swellAddOwnedWindow:self]; 
    }
  }
  
  if (resstate&&resstate->title) SetWindowText((HWND)self, resstate->title);
  m_wndproc((HWND)self,WM_INITDIALOG,0,par);
  
  DOWINDOWMINMAXSIZES
  
  return self;
}


-(void)swellSetModalRetVal:(int)r
{
  m_rv=r;
}
-(int)swellGetModalRetVal
{
  return m_rv;
}

@end

void EndDialog(HWND wnd, int ret)
{
  if (wnd && ([NSApp modalWindow] == (id)wnd ||
              ([(id)wnd isKindOfClass:[NSView class]] && [NSApp modalWindow] == [(id) wnd window])))
  {
    if ([(id)wnd isKindOfClass:[SWELLDialogBox class]])
    {
      [(SWELLDialogBox*)wnd swellSetModalRetVal:ret];
      if ([(id)wnd respondsToSelector:@selector(onSwellMessage:p1:p2:)])
        [(id)wnd onSwellMessage:WM_DESTROY p1:0 p2:0];
    }
    [NSApp abortModal];
  }
}


int SWELL_DialogBox(SWELL_DialogResourceIndex *reshead, int resid, HWND parent,  DLGPROC dlgproc, LPARAM param)
{
  SWELL_DialogResourceIndex *p=resById(reshead,resid);
  if (!p||(p->windowTypeFlags&SWELL_DLG_WS_CHILD)) return -1;
  SWELLDialogBox *box = [[SWELLDialogBox alloc] initDialogBox:p Parent:parent dlgProc:dlgproc Param:param];      
     
  if (!box) return -1;
      
  [NSApp runModalForWindow:box];
  int ret=[box swellGetModalRetVal];
  [box release];
  return ret;
}

HWND SWELL_CreateDialog(SWELL_DialogResourceIndex *reshead, int resid, HWND parent, DLGPROC dlgproc, LPARAM param)
{
  SWELL_DialogResourceIndex *p=resById(reshead,resid);
  if (!p&&resid) return 0;
  
  NSView *parview=NULL;
  if (parent && [(id)parent isKindOfClass:[NSView class]])
    parview=(NSView *)parent;
  else if (parent && [(id)parent isKindOfClass:[NSWindow class]])
    parview=(NSView *)[(id)parent contentView];
  
  if ((!p || (p->windowTypeFlags&SWELL_DLG_WS_CHILD)) && parview)
  {
    SWELLDialogChild *ch=[[SWELLDialogChild alloc] initChild:p Parent:parview dlgProc:dlgproc Param:param];       // create a new child view class
    [ch release];
    return (HWND)ch;
  }
  else if (p)
  {
    SWELLModelessDialog *ch=[[SWELLModelessDialog alloc] initModeless:p Parent:parent dlgProc:dlgproc Param:param];
    if (!ch) return 0;
    return (HWND)ch;
  }
  
  return 0;
}


HMENU SWELL_GetDefaultWindowMenu() { return g_swell_defaultmenu; }
void SWELL_SetDefaultWindowMenu(HMENU menu)
{
  g_swell_defaultmenu=menu;
}


SWELL_DialogResourceIndex *SWELL_curmodule_dialogresource_head; // this eventually will go into a per-module stub file


#endif