#include "swell.h"
#include "swell-dlggen.h"

#import <Cocoa/Cocoa.h>

class ResourceIndex
{
public:
  ResourceIndex() { resid=0; title=NULL; isChildWindow=false; createFunc=NULL; _next=0; }
  ~ResourceIndex() { }
  int resid;
  const char *title;
  bool isChildWindow;
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

#define SWELLDIALOGCOMMONIMPLEMENTS \
- (void)SWELL_Timer:(id)sender \
{ \
  int idx=(int)[[sender userInfo] getValue]; \
    m_dlgproc((HWND)self,WM_TIMER,idx,0); \
} \
- (int)onSwellMessage:(UINT)msg p1:(WPARAM)wParam p2:(LPARAM)lParam \
{ \
  return m_dlgproc((HWND)self,msg,wParam,lParam); \
} \
- (void)controlTextDidChange:(NSNotification *)aNotification \
{ \
  m_dlgproc((HWND)self,WM_COMMAND,([[aNotification object] tag])|(EN_CHANGE<<16),0); \
} \
-(void) onCommand:(id)sender \
{ \
  HandleCommand(self,m_dlgproc,sender);   \
} \
-(void) dealloc \
{ \
  KillTimer((HWND)self,-1); \
  m_dlgproc((HWND)self,WM_DESTROY,0,0); \
      [super dealloc]; \
} \
-(int)getSwellUserData { return m_userdata; } \
-(void)setSwellUserData:(int)val {   m_userdata=val; }


@interface SWELLDialogChild : NSView
{
  DLGPROC m_dlgproc;
  int m_userdata; 
  
  bool m_paintctx_used;
  HDC m_paintctx_hdc;
  NSRect m_paintctx_rect;
}
- (id)initChild:(ResourceIndex *)resstate Parent:(NSView *)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par;
@end

@implementation SWELLDialogChild : NSView 

SWELLDIALOGCOMMONIMPLEMENTS



- (id)initChild:(ResourceIndex *)resstate Parent:(NSView *)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par
{
  m_paintctx_hdc=0;
  m_paintctx_used=0;
  m_dlgproc=dlgproc;
  
  NSRect contentRect=NSMakeRect(0,0,resstate->width,resstate->height);
  if (!(self = [super initWithFrame:contentRect])) return self;
  
  
  resstate->createFunc((HWND)self,1);
  
  [parent addSubview:self];
  m_dlgproc((HWND)self,WM_INITDIALOG,0,par);
  return self;
}
-(void) getSwellPaintInfo:(PAINTSTRUCT *)ps
{
  if (m_paintctx_hdc)
  {
    m_paintctx_used=1;
    ps->hdc = m_paintctx_hdc;
    ps->fErase=false;
    ps->rcPaint.left = (int)m_paintctx_rect.origin.x;
    ps->rcPaint.right = (int)ceil(m_paintctx_rect.origin.x+m_paintctx_rect.size.width);
    ps->rcPaint.top = (int)m_paintctx_rect.origin.y;
    ps->rcPaint.bottom  = (int)ceil(m_paintctx_rect.origin.y+m_paintctx_rect.size.height);   
  }
}

-(void) drawRect:(NSRect)rect
{
  m_paintctx_hdc=WDL_GDP_CreateContext([[NSGraphicsContext currentContext] graphicsPort]);
  m_paintctx_rect=rect;
  m_paintctx_used=false;
  
  m_dlgproc((HWND)self,WM_PAINT,(WPARAM)m_paintctx_hdc,0);
  
  WDL_GDP_DeleteContext(m_paintctx_hdc);
  m_paintctx_hdc=0;
  
  if (!m_paintctx_used)
  {
    [super drawRect:rect];
  }
    
}

@end


@interface SWELLModelessDialog : NSWindow
{
  DLGPROC m_dlgproc;
  int m_userdata; 
}
- (id)initModeless:(ResourceIndex *)resstate Parent:(HWND)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par;
@end

@implementation SWELLModelessDialog : NSWindow

SWELLDIALOGCOMMONIMPLEMENTS



- (id)initModeless:(ResourceIndex *)resstate Parent:(HWND)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par
{
  m_userdata=0;
  m_dlgproc=dlgproc;
  
  NSRect contentRect=NSMakeRect(50,50,resstate->width,resstate->height);
  if (!(self = [super initWithContentRect:contentRect styleMask:(NSTitledWindowMask|NSClosableWindowMask) backing:NSBackingStoreBuffered defer:NO])) return self;
  
 resstate->createFunc((HWND)[self contentView],0);
  
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
  
  m_dlgproc((HWND)self,WM_INITDIALOG,0,par);
  return self;
}
-(BOOL)windowShouldClose:(id)sender
{
  if (!m_dlgproc((HWND)self,WM_CLOSE,0,0))
  {
    m_dlgproc((HWND)self,WM_COMMAND,IDCANCEL,0);
  }
  return NO;
}
- (void)keyDown:(NSEvent *)theEvent
{
  if ([theEvent keyCode]==53)
  {
    if (!m_dlgproc((HWND)self,WM_CLOSE,0,0))
    {
      m_dlgproc((HWND)self,WM_COMMAND,IDCANCEL,0);
    }
  }
  else [super keyDown:theEvent];
}

@end



@interface SWELLDialogBox : NSPanel
{
  DLGPROC m_dlgproc;
  int m_userdata; 
  int m_rv;
}
- (id)initDialogBox:(ResourceIndex *)resstate Parent:(HWND)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par;
@end

@implementation SWELLDialogBox : NSPanel



SWELLDIALOGCOMMONIMPLEMENTS


  
- (id)initDialogBox:(ResourceIndex *)resstate Parent:(HWND)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par
{
  m_rv=0;
  m_dlgproc=dlgproc;
  
  NSRect contentRect=NSMakeRect(0,0,resstate->width,resstate->height);
  if (!(self = [super initWithContentRect:contentRect styleMask:(NSTitledWindowMask|NSClosableWindowMask) backing:NSBackingStoreBuffered defer:NO])) return self;

  resstate->createFunc((HWND)[self contentView],0);
    
  {
    NSString *st=(NSString *)SWELL_CStringToCFString(resstate->title);  
    [self setTitle:st];
    [st release];
  }    
  m_dlgproc((HWND)self,WM_INITDIALOG,0,par);
  return self;
}

- (void)keyDown:(NSEvent *)theEvent
{
  if ([theEvent keyCode]==53)
  {
    if (!m_dlgproc((HWND)self,WM_CLOSE,0,0))
    {
      m_dlgproc((HWND)self,WM_COMMAND,IDCANCEL,0);
    }
  }
  else [super keyDown:theEvent];
}



-(BOOL) windowShouldClose:(id)sender
{
  if (!m_dlgproc((HWND)self,WM_CLOSE,0,0))
  {
    m_dlgproc((HWND)self,WM_COMMAND,IDCANCEL,0);
  }
  return NO;
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
    }
    [NSApp abortModal];
  }
}


int SWELL_DialogBox(int resid, HWND parent,  DLGPROC dlgproc, LPARAM param)
{
  ResourceIndex *p=resById(resid);
  if (!p||p->isChildWindow) return -1;
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
  if (!p) return 0;
  
  NSView *parview=NULL;
  if (parent && [(id)parent isKindOfClass:[NSView class]])
    parview=(NSView *)parent;
  else if (parent && [(id)parent isKindOfClass:[NSWindow class]])
    parview=(NSView *)[(id)parent contentView];
  
  if (p->isChildWindow && parview)
  {
    SWELLDialogChild *ch=[[SWELLDialogChild alloc] initChild:p Parent:parview dlgProc:dlgproc Param:param];       // create a new child view class
    [ch release];
    return (HWND)ch;
  }
  else
  {
    SWELLModelessDialog *ch=[[SWELLModelessDialog alloc] initModeless:p Parent:parent dlgProc:dlgproc Param:param];
    if (!ch) return 0;
    return (HWND)ch;
  }
  
  return 0;
}



void SWELL_RegisterDialogResource(int resid, void (*createFunc)(HWND createTo, int ischild), bool isChildWindow, const char *title, int width, int height)
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
  p->isChildWindow=isChildWindow;
  p->title=title;
  p->width=width;
  p->height=height;
  if (doadd)
  {
    p->_next=m_resources;
    m_resources=p;
  }
}