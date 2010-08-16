#include "swell.h"
#include "swell-dlggen.h"

#import <Cocoa/Cocoa.h>

class ResourceIndex
{
public:
  ResourceIndex() { resid=0; title=NULL; windowTypeFlags=0; createFunc=NULL; _next=0; }
  ~ResourceIndex() { }
  int resid;
  const char *title;
  int windowTypeFlags;
  void (*createFunc)(HWND, int);
  int width,height;
  
  ResourceIndex *_next;
}; 

static ResourceIndex *m_resources;

static ResourceIndex *resById(int resid)
{
  ResourceIndex *p=m_resources;
  while (p)
  {
    if (p->resid == resid) return p;
    p=p->_next;
  }
  return 0;
}



static void HandleCommand(id wnd, DLGPROC dlgproc, id sender)
{
  if ([sender isKindOfClass:[NSSlider class]])
  {
    dlgproc((HWND)wnd,WM_HSCROLL,0,(LPARAM)sender);
    //  WM_HSCROLL, WM_VSCROLL
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
    dlgproc((HWND)wnd,WM_COMMAND,[sender tag]|(cw<<16),0);
  }
}

#define CALLDLGPROC(msg,wp,lp) [self CallDlgProc:(msg) wParam:(wp) lParam:(lp)]

@class simpleDataHold;
#define SWELLDIALOGCOMMONIMPLEMENTS \
- (int)CallDlgProc:(UINT)msg wParam:(WPARAM)wp lParam:(LPARAM)lp \
{ \
  if (m_dlgproc_isfullret) { \
    int (*func)(HWND,UINT,WPARAM,LPARAM); *(void**)&func = (void *)m_dlgproc; \
      return func((HWND)self,msg,wp,lp); \
  }  \
  return m_dlgproc((HWND)self,msg,wp,lp); \
} \
- (void)SWELL_Timer:(id)sender \
{ \
  id uinfo=[sender userInfo]; \
  if ([uinfo respondsToSelector:@selector(getValue)]) { \
    int idx=(int)[(simpleDataHold*)uinfo getValue]; \
    CALLDLGPROC(WM_TIMER,idx,0); \
  } \
} \
- (int)onSwellMessage:(UINT)msg p1:(WPARAM)wParam p2:(LPARAM)lParam \
{ \
  if (msg==WM_DESTROY) { if (m_hashaddestroy) return 0; m_hashaddestroy=true; }   \
  int ret=CALLDLGPROC(msg,wParam,lParam);  \
  if (msg == WM_DESTROY) { \
    /* send all children WM_DESTROY */ \
    KillTimer((HWND)self,-1); \
  } \
  return ret; \
} \
- (void)controlTextDidChange:(NSNotification *)aNotification \
{ \
  CALLDLGPROC(WM_COMMAND,([[aNotification object] tag])|(EN_CHANGE<<16),0); \
} \
- (void)menuNeedsUpdate:(NSMenu *)menu \
{ \
  CALLDLGPROC(WM_INITMENUPOPUP,(WPARAM)menu,0); \
} \
-(void) onCommand:(id)sender \
{ \
  HandleCommand(self,m_dlgproc,sender);   \
} \
-(void) dealloc \
{ \
  KillTimer((HWND)self,-1); \
  [self onSwellMessage:WM_DESTROY p1:0 p2:0]; \
    if (m_menu) [(NSMenu *)m_menu release]; \
      [super dealloc]; \
} \
-(int)tag { return m_tag; } \
-(void)setTag:(int)t { m_tag=t; } \
-(int)getSwellUserData { return m_userdata; } \
-(void)setSwellUserData:(int)val {   m_userdata=val; } \
-(int)getSwellExtraData:(int)idx { if (idx>=0&&idx<sizeof(m_extradata)-3) return  *(int*)(m_extradata+idx); return 0; } \
-(void)setSwellExtraData:(int)idx value:(int)val {  if (idx>=0&&idx<sizeof(m_extradata)-3) *(int*)(m_extradata+idx) = val; }  \
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
}


#define SWELLDIALOGCOMMONIMPLEMENTS_WND \
- (void)setFrame:(NSRect)frameRect display:(BOOL)displayFlag \
{ \
  [super setFrame:frameRect display:displayFlag]; \
  CALLDLGPROC(WM_SIZE,0,0); \
  [self display]; \
} \
-(HMENU)swellGetMenu {   return m_menu; } \
-(void)swellSetMenu:(HMENU)menu {   m_menu=menu; } \
-(void)becomeKeyWindow \
{ \
  [super becomeKeyWindow]; \
    if (m_menu && m_menu != [NSApp mainMenu])  [NSApp setMainMenu:(NSMenu *)m_menu]; \
} \
-(BOOL)windowShouldClose:(id)sender \
{ \
  if (!CALLDLGPROC(WM_CLOSE,0,0)) CALLDLGPROC(WM_COMMAND,IDCANCEL,0); \
  return NO; \
} \
- (void)keyDown:(NSEvent *)theEvent \
{ \
  if ([theEvent keyCode]==53) { \
    if (!CALLDLGPROC(WM_CLOSE,0,0)) \
      CALLDLGPROC(WM_COMMAND,IDCANCEL,0); \
  } \
  else [super keyDown:theEvent]; \
} \
-(int)sendMouseMessage:(int)msg event:(NSEvent*)theEvent \
{ \
	NSPoint p=[theEvent locationInWindow]; \
	unsigned short xpos=(int)(p.x); unsigned short ypos=(int)(p.y); \
  int l=0; \
  if (msg==WM_MOUSEWHEEL) \
  { \
    l=(int) ([theEvent deltaY]*60.0); \
    l<<=16; \
  } \
  int ret=(int)CALLDLGPROC(msg,l,xpos + (ypos<<16)); \
  if (msg==WM_RBUTTONUP && !ret) { \
    p=[self convertBaseToScreen:p]; \
    CALLDLGPROC(WM_CONTEXTMENU,(WPARAM)self,(int)p.x + (((int)p.y)<<16)); \
  } \
  return ret; \
} \
-(void) SwellWnd_drawRect:(NSRect)rect \
{ \
  m_paintctx_hdc=WDL_GDP_CreateContext([[NSGraphicsContext currentContext] graphicsPort]); \
    m_paintctx_rect=rect; \
      m_paintctx_used=false; \
        CALLDLGPROC(WM_PAINT,(WPARAM)m_paintctx_hdc,0); \
          WDL_GDP_DeleteContext(m_paintctx_hdc); \
            m_paintctx_hdc=0; \
              if (!m_paintctx_used) { \
                /*[super drawRect:rect];*/ \
              } \
} \
- (void)SwellWnd_rightMouseDragged:(NSEvent *)theEvent \
{ \
  [self mouseDragged:theEvent]; \
} \
- (void)SwellWnd_mouseDragged:(NSEvent *)theEvent \
{ \
  [self sendMouseMessage:WM_MOUSEMOVE event:theEvent]; \
} \
- (void)SwellWnd_mouseUp:(NSEvent *)theEvent \
{ \
	if (m_isfakerightmouse) [self rightMouseUp:theEvent]; \
  else [self sendMouseMessage:WM_LBUTTONUP event:theEvent]; \
} \
- (void)SwellWnd_scrollWheel:(NSEvent *)theEvent \
{ \
  [self sendMouseMessage:WM_MOUSEWHEEL event:theEvent]; \
} \
- (void)SwellWnd_mouseDown:(NSEvent *)theEvent \
{ \
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
  m_isfakerightmouse=0; \
  return [self sendMouseMessage:WM_RBUTTONUP event:theEvent]; \
}  \
- (void)SwellWnd_rightMouseDown:(NSEvent *)theEvent \
{ \
  m_isfakerightmouse=0; \
  [self sendMouseMessage:([theEvent clickCount]>1 ? WM_RBUTTONDBLCLK : WM_RBUTTONDOWN) event:theEvent]; \
}  





@interface SWELLDialogChild : NSView
{
  DLGPROC m_dlgproc;
  bool m_dlgproc_isfullret;
  int m_userdata; 
  char m_extradata[128];
  int m_tag;
  int m_isfakerightmouse;
  bool m_hashaddestroy;
  HMENU m_menu;
  
  BOOL m_flip;
  bool m_paintctx_used;
  HDC m_paintctx_hdc;
  NSRect m_paintctx_rect;
}
- (id)initChild:(ResourceIndex *)resstate Parent:(NSView *)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par;
-(int)sendMouseMessage:(int)msg event:(NSEvent*)theEvent;
@end

@implementation SWELLDialogChild : NSView 

SWELLDIALOGCOMMONIMPLEMENTS


- (id)initChild:(ResourceIndex *)resstate Parent:(NSView *)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par
{
  m_userdata=0;
  memset(&m_extradata,0,sizeof(m_extradata)); 
  m_menu=0;
  m_hashaddestroy=false;
  m_isfakerightmouse=0;
  m_tag=0;
  m_paintctx_hdc=0;
  m_paintctx_used=0;
  m_dlgproc=dlgproc;
  m_dlgproc_isfullret=!resstate;
  m_flip = !resstate || (resstate->windowTypeFlags&SWELL_DLG_WS_FLIPPED);
  NSRect contentRect=NSMakeRect(0,0,resstate ? resstate->width : 300,resstate ? resstate->height : 200);
  if (!(self = [super initWithFrame:contentRect])) return self;
  
  if (resstate) resstate->createFunc((HWND)self,1);
  
  [parent addSubview:self];
  if (!resstate)
    CALLDLGPROC(WM_CREATE,0,par);
  CALLDLGPROC(WM_INITDIALOG,0,par);
    
  return self;
}

-(void) drawRect:(NSRect)rect
{
  m_paintctx_hdc=WDL_GDP_CreateContext([[NSGraphicsContext currentContext] graphicsPort]);
  m_paintctx_rect=rect;
  m_paintctx_used=false;
  CALLDLGPROC(WM_PAINT,(WPARAM)m_paintctx_hdc,0);
  WDL_GDP_DeleteContext(m_paintctx_hdc);
  m_paintctx_hdc=0;
  if (!m_paintctx_used) {
     /*[super drawRect:rect];*/
  }
}
- (void)rightMouseDragged:(NSEvent *)theEvent
{
  [self mouseDragged:theEvent];
}
- (void)mouseDragged:(NSEvent *)theEvent
{
  [self sendMouseMessage:WM_MOUSEMOVE event:theEvent];
}
- (void)mouseUp:(NSEvent *)theEvent
{
	if (m_isfakerightmouse) return [self rightMouseUp:theEvent];
    [self sendMouseMessage:WM_LBUTTONUP event:theEvent];
}
- (void)scrollWheel:(NSEvent *)theEvent
{
  [self sendMouseMessage:WM_MOUSEWHEEL event:theEvent];
}
- (void)mouseDown:(NSEvent *)theEvent
{
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
  m_isfakerightmouse=0;
  return [self sendMouseMessage:WM_RBUTTONUP event:theEvent];
}
- (void)rightMouseDown:(NSEvent *)theEvent
{
  m_isfakerightmouse=0;
  [self sendMouseMessage:([theEvent clickCount]>1 ? WM_RBUTTONDBLCLK : WM_RBUTTONDOWN) event:theEvent]; 
}  


-(int)sendMouseMessage:(int)msg event:(NSEvent*)theEvent
{
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
  
  int ret=CALLDLGPROC(msg,l,xpos + (ypos<<16));
  if (msg==WM_RBUTTONUP && !ret && !m_dlgproc_isfullret) 
  {
    localpt=[[self window] convertBaseToScreen:localpt];
    CALLDLGPROC(WM_CONTEXTMENU,(WPARAM)self,(int)localpt.x + (((int)localpt.y)<<16));
  }
} 

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
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


@end

@interface SWELLModelessDialog : NSWindow
{
  DLGPROC m_dlgproc;
  int m_dlgproc_isfullret;
  int m_userdata; 
  char m_extradata[128];
  int m_tag;
  int m_isfakerightmouse;
  BOOL m_flip;
  bool m_hashaddestroy;
  HMENU m_menu;
  bool m_paintctx_used;
  HDC m_paintctx_hdc;
  NSRect m_paintctx_rect;
}
- (id)initModeless:(ResourceIndex *)resstate Parent:(HWND)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par;
-(int)sendMouseMessage:(int)msg event:(NSEvent*)theEvent;
@end


#define DOWINDOWMINMAXSIZES \
{ \
  MINMAXINFO mmi={0}; \
  NSSize minsz=(NSSize)[super contentMinSize]; \
  mmi.ptMinTrackSize.x=(int)minsz.width; mmi.ptMinTrackSize.y=(int)minsz.height; \
  CALLDLGPROC(WM_GETMINMAXINFO,0,(LPARAM)&mmi); \
  minsz.width=mmi.ptMinTrackSize.x; minsz.height=mmi.ptMinTrackSize.y; \
  [super setContentMinSize:minsz];  \
} 


@implementation SWELLModelessDialog : NSWindow

SWELLDIALOGCOMMONIMPLEMENTS
SWELLDIALOGCOMMONIMPLEMENTS_WND


- (id)initModeless:(ResourceIndex *)resstate Parent:(HWND)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par
{
  m_menu=0;
  m_hashaddestroy=false;
  m_tag=0;
  m_userdata=0;
  memset(&m_extradata,0,sizeof(m_extradata));
  m_dlgproc=dlgproc;
  m_dlgproc_isfullret=0;
  m_isfakerightmouse=0;
  m_paintctx_hdc=0;
  m_paintctx_used=0;
  
  m_flip = !resstate || (resstate->windowTypeFlags&SWELL_DLG_WS_FLIPPED);

  NSRect contentRect=NSMakeRect(50,50,resstate->width,resstate->height);
  if (!(self = [super initWithContentRect:contentRect styleMask:(NSTitledWindowMask|NSMiniaturizableWindowMask|NSClosableWindowMask|((resstate->windowTypeFlags&SWELL_DLG_WS_RESIZABLE) ? NSResizableWindowMask : 0)) backing:NSBackingStoreBuffered defer:NO])) return self;
  
  SWELL_ReplacementContentView *ncv = [[SWELL_ReplacementContentView alloc] initWithFrame:NSMakeRect(0,0,20,20)];
  [ncv setFlipped:m_flip];
  [self setContentView:ncv];
  [ncv release];
  resstate->createFunc((HWND)[self contentView],0);
  if (resstate&&resstate->title) SetWindowText((HWND)self, resstate->title);
  
 /*
  if (parent)
  {
    if ([(id)parent isKindOfClass:[NSView class]])
      [self setParentWindow:[(id)parent window]];
    else
      if ([(id)parent isKindOfClass:[NSWindow  class]])
        [self setParentWindow:(id)parent];
  }
  */
  
  CALLDLGPROC(WM_INITDIALOG,0,par);
    
  DOWINDOWMINMAXSIZES
  
  return self;
}

@end



@interface SWELLDialogBox : NSPanel
{
  DLGPROC m_dlgproc;
  int m_dlgproc_isfullret;
  int m_userdata;
  char m_extradata[128];
  int m_tag;
  int m_rv;
  BOOL m_flip;
  bool m_hashaddestroy;
  int m_isfakerightmouse;
  HMENU m_menu;
  bool m_paintctx_used;
  HDC m_paintctx_hdc;
  NSRect m_paintctx_rect;
}
- (id)initDialogBox:(ResourceIndex *)resstate Parent:(HWND)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par;
-(int)sendMouseMessage:(int)msg event:(NSEvent*)theEvent;
@end

@implementation SWELLDialogBox : NSPanel



SWELLDIALOGCOMMONIMPLEMENTS
SWELLDIALOGCOMMONIMPLEMENTS_WND

  
- (id)initDialogBox:(ResourceIndex *)resstate Parent:(HWND)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par
{
  m_menu=0;
  m_hashaddestroy=false;
  m_userdata=0;
  memset(&m_extradata,0,sizeof(m_extradata)); 
  m_isfakerightmouse=0;
  m_paintctx_hdc=0;
  m_paintctx_used=0;
  m_tag=0;
  m_rv=0;
  m_dlgproc=dlgproc;
  m_dlgproc_isfullret=0;
  m_flip = !resstate || (resstate->windowTypeFlags&SWELL_DLG_WS_FLIPPED);
  
  NSRect contentRect=NSMakeRect(0,0,resstate->width,resstate->height);
  if (!(self = [super initWithContentRect:contentRect styleMask:(NSTitledWindowMask|NSClosableWindowMask|((resstate->windowTypeFlags&SWELL_DLG_WS_RESIZABLE)? NSResizableWindowMask : 0)) backing:NSBackingStoreBuffered defer:NO])) return self;

  SWELL_ReplacementContentView *ncv = [[SWELL_ReplacementContentView alloc] initWithFrame:NSMakeRect(0,0,20,20)];
  [ncv setFlipped:m_flip];
  [self setContentView:ncv];
  [ncv release];
  
  resstate->createFunc((HWND)[self contentView],0);
    
  if (resstate&&resstate->title) SetWindowText((HWND)self, resstate->title);
  CALLDLGPROC(WM_INITDIALOG,0,par);
  
  DOWINDOWMINMAXSIZES
  
  return self;
}


-(void)setRetVal:(int)r
{
  m_rv=r;
}
-(int)getRetVal
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
      [(SWELLDialogBox*)wnd setRetVal:ret];
      if ([(id)wnd respondsToSelector:@selector(onSwellMessage:p1:p2:)])
        [(id)wnd onSwellMessage:WM_DESTROY p1:0 p2:0];
    }
    [NSApp abortModal];
  }
}


int SWELL_DialogBox(int resid, HWND parent,  DLGPROC dlgproc, LPARAM param)
{
  ResourceIndex *p=resById(resid);
  if (!p||(p->windowTypeFlags&SWELL_DLG_WS_CHILD)) return -1;
  SWELLDialogBox *box = [[SWELLDialogBox alloc] initDialogBox:p Parent:parent dlgProc:dlgproc Param:param];      
     
  if (!box) return -1;
      
  [NSApp runModalForWindow:box];
  int ret=[box getRetVal];
  [box release];
  return ret;
}

HWND SWELL_CreateDialog(int resid, HWND parent, DLGPROC dlgproc, LPARAM param)
{
  ResourceIndex *p=resById(resid);
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



void SWELL_RegisterDialogResource(int resid, void (*createFunc)(HWND createTo, int ischild), int windowTypeFlags, const char *title, int width, int height)
{
  bool doadd=false;
  ResourceIndex *p;
  
  if (!(p=resById(resid)))
  {
    doadd=true;
    p=new ResourceIndex;
  }
  p->resid=resid;
  p->createFunc=createFunc;
  p->windowTypeFlags=windowTypeFlags;
  p->title=title;
  p->width=width;
  p->height=height;
  if (doadd)
  {
    p->_next=m_resources;
    m_resources=p;
  }
}