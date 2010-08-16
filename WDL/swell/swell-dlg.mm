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
  void (*createFunc)(HWND);
  int width,height;
  
  ResourceIndex *_next;
}; 

static ResourceIndex *m_resources;

@interface SWELLDialogBox : NSPanel
-(void)setRetVal:(int)r;
-(int)getRetVal;
@end
@implementation SWELLDialogBox : NSPanel

DLGPROC m_dlgproc;
int m_rv;
int m_userdata;

-(int)getUserData
{
  return m_userdata;
}
-(void)setUserData:(int)val
{
  m_userdata=val;
}

-(int)getRetVal
{
  return m_rv;
}
  
  
- (id)initWithAuthoritay:(ResourceIndex *)resstate Parent:(HWND)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par
{
  m_rv=0;
  m_dlgproc=dlgproc;
  
  NSRect contentRect=NSMakeRect(0,0,resstate->width,resstate->height);
  if (!(self = [super initWithContentRect:contentRect styleMask:(NSTitledWindowMask|NSClosableWindowMask) backing:NSBackingStoreBuffered defer:NO])) return self;

  resstate->createFunc((HWND)[self contentView]);
    
  {
    NSString *st=(NSString *)SWELL_CStringToCFString(resstate->title);  
    [self setTitle:st];
    [st release];
  }    
  m_dlgproc((HWND)self,WM_INITDIALOG,0,par);
  return self;
}
- (void)SWELL_Timer:(id)sender
{
  int idx=(int)[[sender userInfo] getValue];
  m_dlgproc((HWND)self,WM_TIMER,idx,0);
}

- (void)controlTextDidChange:(NSNotification *)aNotification
{
  m_dlgproc((HWND)self,WM_COMMAND,([[aNotification object] tag])|(EN_CHANGE<<16),0);
}
-(void) onCommand:(id)sender
{
  
  if ([sender isKindOfClass:[NSSlider class]])
  {
    m_dlgproc((HWND)self,WM_HSCROLL,0,(LPARAM)sender);
    //  WM_HSCROLL, WM_VSCROLL
  }
  else
  {
    int cw=0;
    if ([sender isKindOfClass:[NSControl class]])
    {
      NSEvent *evt=[NSApp currentEvent];
      if (evt && [evt clickCount] > 1) cw=STN_DBLCLK;
    }
    m_dlgproc((HWND)self,WM_COMMAND,[sender tag]|(cw<<16),0);
  }
}
-(void)setRetVal:(int)r
{
  m_rv=r;
}

-(void) close
{
  if (!m_dlgproc((HWND)self,WM_CLOSE,0,0))
  {
     m_dlgproc((HWND)self,WM_COMMAND,IDCANCEL,0);
  }
}


-(void) dealloc
{
  m_dlgproc((HWND)self,WM_DESTROY,0,0);
  KillTimer((HWND)self,-1);
  [super dealloc];
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
  ResourceIndex *p=m_resources;
  
  while (p)
  {
    if (p->resid == resid) 
    {
      if (p->isChildWindow) return -1;
      
      
      SWELLDialogBox *box = [[SWELLDialogBox alloc] initWithAuthoritay:p Parent:parent dlgProc:dlgproc Param:param];      
     
      if (!box) return -1;
      
      [NSApp runModalForWindow:box];
      int ret=[box getRetVal];
      [box release];
      return ret;
    }
    p=p->_next;
  }
  return -1;
}

void SWELL_RegisterDialogResource(int resid, void (*createFunc)(HWND createTo), bool isChildWindow, const char *title, int width, int height)
{
  int x;
  ResourceIndex *p=new ResourceIndex;
  p->resid=resid;
  p->createFunc=createFunc;
  p->isChildWindow=isChildWindow;
  p->title=title;
  p->width=width;
  p->height=height;
  p->_next=m_resources;
  m_resources=p;
}