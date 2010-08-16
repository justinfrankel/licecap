#ifndef SWELL_PROVIDED_BY_APP


#include "swell.h"
#include "swell-dlggen.h"

#import <Cocoa/Cocoa.h>
static HMENU g_swell_defaultmenu;

void (*SWELL_DDrop_onDragLeave)();
void (*SWELL_DDrop_onDragOver)(POINT pt);
void (*SWELL_DDrop_onDragEnter)(void *hGlobal, POINT pt);

bool SWELL_owned_windows_levelincrease=false;

#define simpleDataHold __SWELL_PREFIX_CLASSNAME(_simpleDataHold)
#define SWELLDialogChild __SWELL_PREFIX_CLASSNAME(_dlgchild) 
#define SWELLModelessDialog __SWELL_PREFIX_CLASSNAME(_mldlg)
#define SWELLDialogBox __SWELL_PREFIX_CLASSNAME(_sldb)

@class simpleDataHold;

static LRESULT SwellDialogDefaultWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  DLGPROC d=(DLGPROC)GetWindowLong(hwnd,DWL_DLGPROC);
  if (d) 
  {
    LRESULT r=(LRESULT) d(hwnd,uMsg,wParam,lParam);
    if (r) return r; 
  }
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

static void DoPaintStuff(WNDPROC wndproc, HWND hwnd, HDC hdc, NSRect *modrect)
{
  RECT r,r2;
  GetWindowRect(hwnd,&r);
  if (r.top>r.bottom) { int tmp=r.top; r.top=r.bottom; r.bottom=tmp; }
  r2=r;
  wndproc(hwnd,WM_NCCALCSIZE,FALSE,(LPARAM)&r);
  wndproc(hwnd,WM_NCPAINT,(WPARAM)1,0);
  modrect->origin.x += r.left-r2.left;
  modrect->origin.y += r.top-r2.top;
    
  if (modrect->size.width >= 1 && modrect->size.height >= 1)
  {
    int a=0;
    if (memcmp(&r,&r2,sizeof(r)))
    {
      RECT tr;
      SWELL_PushClipRegion(hdc);
      GetClientRect(hwnd,&tr);
      SWELL_SetClipRegion(hdc,&tr);
      a++;
    }
    wndproc(hwnd,WM_PAINT,(WPARAM)hdc,0);
    if (a) SWELL_PopClipRegion(hdc);
  }
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

static int DelegateMouseMove(NSView *view, NSEvent *theEvent)
{
  static int __nofwd;
  if (__nofwd) return 0;
  
  NSPoint p=[theEvent locationInWindow];
  NSWindow *w=[theEvent window];
  if (!w) return 0;
  NSView *v=[[w contentView] hitTest:p];
  if (!v || v==view) return 0;
  
  __nofwd=1;
  [v mouseMoved:theEvent];
  __nofwd=0;
  
  return 1;
}



@interface SWELLDialogChild : NSView
{
  BOOL m_enabled;
  DLGPROC m_dlgproc;
  WNDPROC m_wndproc;
  int m_userdata;
  char m_extradata[128];
  int m_tag;
  int m_isfakerightmouse;
  bool m_hashaddestroy;
  HMENU m_menu;
  BOOL m_flip;
  bool m_supports_ddrop;
  bool m_paintctx_used;
  HDC m_paintctx_hdc;
  WindowPropRec *m_props;
  NSRect m_paintctx_rect;
  BOOL m_isopaque;
  char m_titlestr[1024];
}
- (id)initChild:(SWELL_DialogResourceIndex *)resstate Parent:(NSView *)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par;
-(int)sendMouseMessage:(int)msg event:(NSEvent*)theEvent;
@end

@implementation SWELLDialogChild : NSView 

- (void)SWELL_Timer:(id)sender
{
  id uinfo=[sender userInfo];
    if ([uinfo respondsToSelector:@selector(getValue)]) 
    {
      int idx=(int)[(simpleDataHold*)uinfo getValue];
        m_wndproc((HWND)self,WM_TIMER,idx,0);
    }
}

- (int)swellCapChangeNotify { return YES; }

- (int)onSwellMessage:(UINT)msg p1:(WPARAM)wParam p2:(LPARAM)lParam
{
  if (msg==WM_DESTROY) 
  { 
    if (m_hashaddestroy) return 0; 
    m_hashaddestroy=true; 
    if (GetCapture()==(HWND)self) ReleaseCapture(); 
    SWELL_MessageQueue_Clear((HWND)self); 
  }
  else if (msg==WM_CAPTURECHANGED && m_hashaddestroy) return 0; 
  
  int ret=m_wndproc((HWND)self,msg,wParam,lParam);
  if (msg == WM_DESTROY) 
  {
    if ([[self window] contentView] == self && [[self window] respondsToSelector:@selector(swellDestroyAllOwnedWindows)])
      [[self window] swellDestroyAllOwnedWindows];

    if (GetCapture()==(HWND)self) ReleaseCapture(); 
    SWELL_MessageQueue_Clear((HWND)self); 
    
    if (m_menu) 
    {
      if ([NSApp mainMenu] == m_menu) [NSApp setMainMenu:nil];
      [(NSMenu *)m_menu release]; 
      m_menu=0;
    }
    NSView *v=self;
    NSArray *ar;
    if (v && [v isKindOfClass:[NSView class]] && (ar=[v subviews]) && [ar count]>0) 
    {
      int x; 
      for (x = 0; x < [ar count]; x ++) 
      {
        NSView *sv=[ar objectAtIndex:x]; 
        if (sv && [sv respondsToSelector:@selector(onSwellMessage:p1:p2:)]) 
           [sv onSwellMessage:WM_DESTROY p1:0 p2:0];
      }
    }
    KillTimer((HWND)self,-1);
  }
  return ret;
}

- (void) setEnabled:(BOOL)en
{ 
  m_enabled=en; 
} 

- (void)textDidEndEditing:(NSNotification *)aNotification
{
  m_wndproc((HWND)self,WM_COMMAND,([[aNotification object] tag])|(EN_CHANGE<<16),0);
}
- (void)controlTextDidChange:(NSNotification *)aNotification
{
  m_wndproc((HWND)self,WM_COMMAND,([[aNotification object] tag])|(EN_CHANGE<<16),0);
}

- (void)menuNeedsUpdate:(NSMenu *)menu
{
  m_wndproc((HWND)self,WM_INITMENUPOPUP,(WPARAM)menu,0);
}

-(void) onControlDoubleClick:(id)sender
{
  if ([sender isKindOfClass:[NSTableView class]] && 
      [sender respondsToSelector:@selector(getSwellNotificationMode)] &&
      [sender getSwellNotificationMode])
  {
    m_wndproc((HWND)self,WM_COMMAND,(LBN_DBLCLK<<16)|[sender tag],0);
  }
  else
  {   
    NMCLICK nm={{(HWND)sender,[sender tag],NM_DBLCLK}, }; 
    m_wndproc((HWND)self,WM_NOTIFY,[sender tag],(LPARAM)&nm);
  }
}


-(void) onCommand:(id)sender
{
  if ([sender isKindOfClass:[NSSlider class]])
  {
    m_wndproc((HWND)self,WM_HSCROLL,0,(LPARAM)sender);
    //  WM_HSCROLL, WM_VSCROLL
  }
  else if ([sender isKindOfClass:[NSTableView class]])
  {
    if ([sender isKindOfClass:[NSOutlineView class]])
    {
      NMTREEVIEW nmhdr={{(HWND)sender,(int)[sender tag],TVN_SELCHANGED},0,};  // todo: better treeview notifications
      m_wndproc((HWND)self,WM_NOTIFY,(int)[sender tag],(LPARAM)&nmhdr);
    }
    else
    {
    
      if ([sender respondsToSelector:@selector(getSwellNotificationMode)] && [sender getSwellNotificationMode])
      {
        m_wndproc((HWND)self,WM_COMMAND,(int)[sender tag] | (LBN_SELCHANGE<<16),0);
      }
      else
      {
        NMLISTVIEW nmhdr={{(HWND)sender,(int)[sender tag],LVN_ITEMCHANGED},(int)[sender clickedRow],0}; 
        m_wndproc((HWND)self,WM_NOTIFY,(int)[sender tag],(LPARAM)&nmhdr);
      }
    }
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
    m_wndproc((HWND)self,WM_COMMAND,[sender tag]|(cw<<16),0);
  }

}
-(void) dealloc
{
  KillTimer((HWND)self,-1);
  [self onSwellMessage:WM_DESTROY p1:0 p2:0];
  if (GetCapture()==(HWND)self) ReleaseCapture();
  [super dealloc];
}

-(int)tag { return m_tag; }
-(void)setTag:(int)t { m_tag=t; }
-(int)getSwellUserData { return m_userdata; }
-(void)setSwellUserData:(int)val {   m_userdata=val; }
-(int)getSwellExtraData:(int)idx { if (idx>=0&&idx<sizeof(m_extradata)-3) return  *(int*)(m_extradata+idx); return 0; }
-(void)setSwellExtraData:(int)idx value:(int)val {  if (idx>=0&&idx<sizeof(m_extradata)-3) *(int*)(m_extradata+idx) = val; }
-(void)setSwellWindowProc:(int)val { m_wndproc=(WNDPROC)val; }
-(int)getSwellWindowProc { return (int)m_wndproc; }
-(void)setSwellDialogProc:(int)val { m_dlgproc=(DLGPROC)val; }
-(int)getSwellDialogProc { return (int)m_dlgproc; }
-(BOOL)isFlipped {   return m_flip; }
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

-(bool)swellCanPostMessage { return !m_hashaddestroy; }
-(int)swellEnumProps:(PROPENUMPROCEX)proc lp:(LPARAM)lParam 
{
  WindowPropRec *p=m_props;
  if (!p) return -1;
  while (p) 
  {
    WindowPropRec *ps=p;
    p=p->_next;
    if (!proc((HWND)self, ps->name, ps->data, lParam)) return 0;
  }
  return 1;
}

-(void *)swellGetProp:(const char *)name wantRemove:(BOOL)rem 
{
  WindowPropRec *p=m_props, *lp=NULL;
  while (p) 
  {
    if (p->name < (void *)65536) 
    {
      if (name==p->name) break;
    }
    else if (name >= (void *)65536) 
    {
      if (!strcmp(name,p->name)) break;
    }
    lp=p; p=p->_next;
  }
  if (!p) return NULL;
  void *ret=p->data;
  if (rem) 
  {
    if (lp) lp->_next=p->_next; else m_props=p->_next; 
    free(p);
  }
  return ret;
}

-(int)swellSetProp:(const char *)name value:(void *)val 
{
  WindowPropRec *p=m_props;
  while (p) 
  {
    if (p->name < (void *)65536) 
    {
      if (name==p->name) { p->data=val; return TRUE; };
    }
    else if (name >= (void *)65536) 
    {
      if (!strcmp(name,p->name)) { p->data=val; return TRUE; };
    }
    p=p->_next;
  }
  p=(WindowPropRec*)malloc(sizeof(WindowPropRec));
  p->name = (name<(void*)65536) ? (char *)name : strdup(name);
  p->data = val; p->_next=m_props; m_props=p;
  return TRUE;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent {	return m_enabled?YES:NO; }
-(HMENU)swellGetMenu {   return m_menu; }
-(void)swellSetMenu:(HMENU)menu {   m_menu=menu; }


- (id)initChild:(SWELL_DialogResourceIndex *)resstate Parent:(NSView *)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par
{
  m_enabled=TRUE;
  m_dlgproc=NULL;
  m_wndproc=NULL;
  m_userdata=0;
  memset(&m_extradata,0,sizeof(m_extradata));
  m_tag=0;
  m_isfakerightmouse=0;
  m_hashaddestroy=false;
  m_menu=0;
  m_flip=0;
  m_supports_ddrop=0;
  m_paintctx_used=0;
  m_paintctx_hdc=0;
  m_props=0;
                                
  m_titlestr[0]=0;
  
  m_wndproc=SwellDialogDefaultWindowProc;
  
  m_isopaque = !resstate || (resstate->windowTypeFlags&SWELL_DLG_WS_OPAQUE);
  m_flip = !resstate || (resstate->windowTypeFlags&SWELL_DLG_WS_FLIPPED);
  m_supports_ddrop = resstate && (resstate->windowTypeFlags&SWELL_DLG_WS_DROPTARGET);
  NSRect contentRect=NSMakeRect(0,0,resstate ? resstate->width : 300,resstate ? resstate->height : 200);
  if (!(self = [super initWithFrame:contentRect])) return self;
    
  if ([parent isKindOfClass:[NSWindow class]])
  {
    [parent setContentView:self];
  }
  else
    [parent addSubview:self];

  if (resstate) resstate->createFunc((HWND)self,resstate->windowTypeFlags);
  
  if (resstate) m_dlgproc=dlgproc;  
  else m_wndproc=(WNDPROC)dlgproc;
  
  if (resstate && (resstate->windowTypeFlags&SWELL_DLG_WS_DROPTARGET))
  {
    [self registerForDraggedTypes:[NSArray arrayWithObjects:NSFilenamesPboardType, nil]];
  }
  
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
  if (self == [[self window] contentView] && [theEvent keyCode]==53)
  {
    if (!m_wndproc((HWND)self,WM_CLOSE,0,0))
      m_wndproc((HWND)self,WM_COMMAND,IDCANCEL,0);
  }
  else
  {
    int flag,code=SWELL_MacKeyToWindowsKey(theEvent,&flag);
    if (!m_wndproc((HWND)self,WM_KEYDOWN,code,0)) [super keyDown:theEvent];
  }
}
- (void)keyUp:(NSEvent *)theEvent
{
  if (self == [[self window] contentView] && [theEvent keyCode]==53)
  {
  }
  else
  {
    int flag,code=SWELL_MacKeyToWindowsKey(theEvent,&flag);
    if (!m_wndproc((HWND)self,WM_KEYUP,code,0)) [super keyUp:theEvent];
  }
}



-(void) drawRect:(NSRect)rect
{
  if (m_hashaddestroy) return;
  m_paintctx_hdc=WDL_GDP_CreateContext([[NSGraphicsContext currentContext] graphicsPort]);
  m_paintctx_rect=rect;
  m_paintctx_used=false;
  DoPaintStuff(m_wndproc,(HWND)self,m_paintctx_hdc,&m_paintctx_rect);
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
  if (DelegateMouseMove(self,theEvent)) return;
  
  if (m_enabled) if (!GetCapture() || GetCapture()==(HWND)self) { 
    [self sendMouseMessage:WM_MOUSEMOVE event:theEvent];
  }
//  [super mouseMoved:theEvent];
}
- (void)mouseUp:(NSEvent *)theEvent
{
  if (!m_enabled) return;
	if (m_isfakerightmouse) [self rightMouseUp:theEvent];
  else [self sendMouseMessage:WM_LBUTTONUP event:theEvent];
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
- (void)rightMouseUp:(NSEvent *)theEvent
{
  if (!m_enabled) return;
  m_isfakerightmouse=0;
  [self sendMouseMessage:WM_RBUTTONUP event:theEvent];  
}
- (void)rightMouseDown:(NSEvent *)theEvent
{
  m_isfakerightmouse=0;
  [self sendMouseMessage:([theEvent clickCount]>1 ? WM_RBUTTONDBLCLK : WM_RBUTTONDOWN) event:theEvent]; 
}  


-(int)sendMouseMessage:(int)msg event:(NSEvent*)theEvent
{
  NSView *capv=(NSView *)GetCapture();
  if (capv && capv != self && [capv window] == [self window] && [capv respondsToSelector:@selector(sendMouseMessage:event:)])
    return (int)[capv sendMouseMessage:msg event:theEvent];
  
  if (m_hashaddestroy) return -1;
	NSPoint localpt=[theEvent locationInWindow];
	NSPoint p = [self convertPoint:localpt fromView:nil];
	unsigned short xpos=(int)(p.x); unsigned short ypos=(int)(p.y);
  int htc=HTCLIENT;
  if (msg != WM_MOUSEWHEEL && !capv) { 
     DWORD p=GetMessagePos(); 
     htc=m_wndproc((HWND)self,WM_NCHITTEST,0,p); 
     if (htc!=HTCLIENT) 
     { 
       if (msg==WM_MOUSEMOVE) return m_wndproc((HWND)self,WM_NCMOUSEMOVE,htc,p); 
       if (msg==WM_LBUTTONUP) return m_wndproc((HWND)self,WM_NCLBUTTONUP,htc,p); 
       if (msg==WM_LBUTTONDOWN) return m_wndproc((HWND)self,WM_NCLBUTTONDOWN,htc,p); 
       if (msg==WM_LBUTTONDBLCLK) return m_wndproc((HWND)self,WM_NCLBUTTONDBLCLK,htc,p); 
       if (msg==WM_RBUTTONUP) return m_wndproc((HWND)self,WM_NCRBUTTONUP,htc,p); 
       if (msg==WM_RBUTTONDOWN) return m_wndproc((HWND)self,WM_NCRBUTTONDOWN,htc,p); 
       if (msg==WM_RBUTTONDBLCLK) return m_wndproc((HWND)self,WM_NCRBUTTONDBLCLK,htc,p); 
     } 
  } 
  
  int l=0;
  if (msg==WM_MOUSEWHEEL)
  {
    l=(int) ([theEvent deltaY]*60.0);
    l<<=16;
  }
  
  // put todo: modifiers into low work of l
  
  if (msg==WM_MOUSEWHEEL)
  {
    POINT p;
    GetCursorPos(&p);
    return m_wndproc((HWND)self,msg,l,(short)p.x + (((int)p.y)<<16));
  }
  
  int ret=m_wndproc((HWND)self,msg,l,xpos + (ypos<<16));

  if (msg==WM_LBUTTONUP || msg==WM_RBUTTONUP || msg==WM_MOUSEMOVE) {
    if (!GetCapture() && !m_wndproc((HWND)self,WM_SETCURSOR,(WPARAM)self,htc | (msg<<16))) {
      NSCursor *arr= [NSCursor arrowCursor];
        if (GetCursor() != (HCURSOR)arr) SetCursor((HCURSOR)arr);
    }
  }
   return ret;
} 
- (const char *)onSwellGetText { return m_titlestr; }
-(void)onSwellSetText:(const char *)buf { lstrcpyn(m_titlestr,buf,sizeof(m_titlestr)); }


/*
- (BOOL)becomeFirstResponder 
{
  if (!m_enabled) return NO;
  HWND foc=GetFocus();
  if (![super becomeFirstResponder]) return NO;
  [self onSwellMessage:WM_ACTIVATE p1:WA_ACTIVE p2:(LPARAM)foc];
  return YES;
}

- (BOOL)resignFirstResponder
{
  HWND foc=GetFocus();
  if (![super resignFirstResponder]) return NO;
  [self onSwellMessage:WM_ACTIVATE p1:WA_INACTIVE p2:(LPARAM)foc];
  return YES;
}
*/

- (BOOL)acceptsFirstResponder 
{
  return m_enabled?YES:NO;
}

-(void)swellSetExtendedStyle:(int)st
{
  if (st&WS_EX_ACCEPTFILES) m_supports_ddrop=true;
  else m_supports_ddrop=false;
}
-(int)swellGetExtendedStyle
{
  int ret=0;
  if (m_supports_ddrop) ret|=WS_EX_ACCEPTFILES;
  return ret;
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender 
{
  if (!m_supports_ddrop) return NSDragOperationNone;
  
  if (SWELL_DDrop_onDragEnter)
  {
    HANDLE h = (HANDLE)[self swellExtendedDragOp:sender retGlob:YES];
    if (h)
    {
      NSPoint p=[[self window] convertBaseToScreen:[sender draggingLocation]];
      POINT pt={(int)(p.x+0.5),(int)(p.y+0.5)};
      SWELL_DDrop_onDragEnter(h,pt);
      GlobalFree(h);
    }
  }
    
  return NSDragOperationGeneric;
}
- (NSDragOperation)draggingUpdated:(id <NSDraggingInfo>)sender 
{
  if (!m_supports_ddrop) return NSDragOperationNone;
  
  if (SWELL_DDrop_onDragOver)
  {
    NSPoint p=[[self window] convertBaseToScreen:[sender draggingLocation]];
    POINT pt={(int)(p.x+0.5),(int)(p.y+0.5)};
    SWELL_DDrop_onDragOver(pt);
  }
  
  return NSDragOperationGeneric;
  
} 
- (void)draggingExited:(id <NSDraggingInfo>)sender
{
  if (m_supports_ddrop && SWELL_DDrop_onDragLeave) SWELL_DDrop_onDragLeave();
}

-(HANDLE)swellExtendedDragOp:(id <NSDraggingInfo>)sender retGlob:(BOOL)retG
{
  if (!m_supports_ddrop) return 0;
  
  NSPasteboard *pboard;
  NSDragOperation sourceDragMask;
  sourceDragMask = [sender draggingSourceOperationMask];
  pboard = [sender draggingPasteboard];
 
  if (! [[pboard types] containsObject:NSFilenamesPboardType] )  return 0;


  int sz=sizeof(DROPFILES);
  NSArray *files = [pboard propertyListForType:NSFilenamesPboardType];
  int x;
  for (x = 0; x < [files count]; x ++)
  {
    NSString *sv=[files objectAtIndex:x]; 
    if (sv)
    {
      char text[4096];
      text[0]=0;
      [sv getCString:text maxLength:sizeof(text)];
      sz+=strlen(text)+1;
    }
  }

  NSPoint tpt = [self convertPoint:[sender draggingLocation] fromView:nil];  
  
  HANDLE gobj=GlobalAlloc(0,sz+1);
  DROPFILES *df=(DROPFILES*)gobj;
  df->pFiles=sizeof(DROPFILES);
  df->pt.x = (int)(tpt.x+0.5);
  df->pt.y = (int)(tpt.y+0.5);
  df->fNC = FALSE;
  df->fWide = FALSE;
  char *pout = (char *)(df+1);
  for (x = 0; x < [files count]; x ++)
  {
    NSString *sv=[files objectAtIndex:x]; 
    if (sv)
    {
      char text[4096];
      text[0]=0;
      [sv getCString:text maxLength:sizeof(text)];
      strcpy(pout,text);
      pout+=strlen(pout)+1;
    }
  }
  *pout=0;
  
  if (!retG)
  {
    [self onSwellMessage:WM_DROPFILES p1:(WPARAM)gobj p2:0];
    GlobalFree(gobj);
  }
  
  return gobj;
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
  if (m_supports_ddrop && SWELL_DDrop_onDragLeave) SWELL_DDrop_onDragLeave();
  
  NSView *v=[self hitTest:[[self superview] convertPoint:[sender draggingLocation] fromView:nil]];
  if (v && [v isDescendantOf:self])
  {
    while (v && v!=self)
    {
      if ([v respondsToSelector:@selector(swellExtendedDragOp:retGlob:)])
        if ([v swellExtendedDragOp:sender retGlob:NO]) return YES;
      v=[v superview];
    }
  }
  return !![self swellExtendedDragOp:sender retGlob:NO];
}



@end





static HWND last_key_window;


#define SWELLDIALOGCOMMONIMPLEMENTS_WND \
-(BOOL)acceptsFirstResponder { return m_enabled?YES:NO; } \
- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent {	return m_enabled?YES:NO; } \
- (void)setFrame:(NSRect)frameRect display:(BOOL)displayFlag \
{ \
  [super setFrame:frameRect display:displayFlag]; \
    [[self contentView] onSwellMessage:WM_SIZE p1:0 p2:0]; \
} \
- (void)windowDidMove:(NSNotification *)aNotification { \
    NSRect f=[self frame]; \
    [[self contentView] onSwellMessage:WM_MOVE p1:0 p2:MAKELPARAM((int)f.origin.x,(int)f.origin.y)]; \
} \
-(void)dealloc \
{ \
  if (last_key_window==(HWND)self) last_key_window=0; \
  if (m_owner) [m_owner swellRemoveOwnedWindow:self]; \
    m_owner=0;  \
      OwnedWindowListRec *p=m_ownedwnds; m_ownedwnds=0; \
        while (p) \
        { \
          OwnedWindowListRec *next=p->_next;  \
            DestroyWindow((HWND)p->hwnd); \
              free(p); p=next;  \
        } \
} \
- (void)swellDestroyAllOwnedWindows \
{ \
  OwnedWindowListRec *p=m_ownedwnds; m_ownedwnds=0; \
    while (p) \
    { \
      OwnedWindowListRec *next=p->_next;  \
        DestroyWindow((HWND)p->hwnd); \
          free(p); p=next;  \
    } \
} \
- (void)resignKeyWindow { \
  [super resignKeyWindow]; \
  [[self contentView] onSwellMessage:WM_ACTIVATE p1:WA_INACTIVE p2:(LPARAM)0]; \
  last_key_window=(HWND)self; \
} \
-(void)becomeKeyWindow \
{ \
  [super becomeKeyWindow]; \
  HWND foc=last_key_window ? (HWND)[(NSWindow *)last_key_window contentView] : 0; \
    HMENU menu=0; \
      if ([[self contentView] respondsToSelector:@selector(swellGetMenu)]) \
        menu = (HMENU) [[self contentView] swellGetMenu]; \
          if (!menu) menu=g_swell_defaultmenu; \
            if (menu && menu != [NSApp mainMenu])  [NSApp setMainMenu:(NSMenu *)menu]; \
  [[self contentView] onSwellMessage:WM_ACTIVATE p1:WA_ACTIVE p2:(LPARAM)foc]; \
} \
-(BOOL)windowShouldClose:(id)sender \
{ \
  NSView *v=[self contentView]; \
    if ([v respondsToSelector:@selector(onSwellMessage:p1:p2:)]) \
      if (![v onSwellMessage:WM_CLOSE p1:0 p2:0]) \
        [v onSwellMessage:WM_COMMAND p1:IDCANCEL p2:0]; \
          return NO; \
} \
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
    if (SWELL_owned_windows_levelincrease) if ([wnd isKindOfClass:[NSWindow class]]) [wnd setLevel:[self level]+1]; \
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
- (void)swellResetOwnedWindowLevels { \
  if (SWELL_owned_windows_levelincrease) { OwnedWindowListRec *p=m_ownedwnds; \
  int l=[self level]+1; \
    while (p) { \
      if (p->hwnd) { \
        [(NSWindow *)p->hwnd setLevel:l]; \
        if ([(id)p->hwnd respondsToSelector:@selector(swellResetOwnedWindowLevels)]) \
          [(id)p->hwnd swellResetOwnedWindowLevels]; \
      } \
      p=p->_next; \
    } \
  } \
} \
- (void)swellSetOwner:(id)owner { m_owner=owner; } \
- (id)swellGetOwner { return m_owner; }  \
- (NSSize)minSize \
{ \
  MINMAXINFO mmi={0}; \
  NSSize minsz=(NSSize)[super minSize]; \
  mmi.ptMinTrackSize.x=(int)minsz.width; mmi.ptMinTrackSize.y=(int)minsz.height; \
  [[self contentView] onSwellMessage:WM_GETMINMAXINFO p1:0 p2:(LPARAM)&mmi]; \
  minsz.width=mmi.ptMinTrackSize.x; minsz.height=mmi.ptMinTrackSize.y; \
  return minsz; \
} \
- (NSSize)maxSize \
{ \
  MINMAXINFO mmi={0}; \
  NSSize maxsz=(NSSize)[super maxSize]; NSSize tmp=maxsz;\
  if (tmp.width<1)tmp.width=1; else if (tmp.width > 1000000.0) tmp.width=1000000.0; \
  if (tmp.height<1)tmp.height=1; else if (tmp.height > 1000000.0) tmp.height=1000000.0; \
  mmi.ptMaxTrackSize.x=(int)tmp.width; mmi.ptMaxTrackSize.y=(int)tmp.height; \
  [[self contentView] onSwellMessage:WM_GETMINMAXINFO p1:0 p2:(LPARAM)&mmi]; \
  if (mmi.ptMaxTrackSize.x < 1000000) maxsz.width=mmi.ptMaxTrackSize.x; \
  if (mmi.ptMaxTrackSize.y < 1000000) maxsz.height=mmi.ptMaxTrackSize.y; \
  return maxsz; \
} \



#define DECLARE_COMMON_VARS \
  BOOL m_enabled; \
  id m_owner; \
  OwnedWindowListRec *m_ownedwnds; \

#define INIT_COMMON_VARS \
  m_enabled=TRUE; \
  m_owner=0; \
  m_ownedwnds=0;


#if 0
#define DOWINDOWMINMAXSIZES(ch) \
{ \
  MINMAXINFO mmi={0}; \
    NSSize minsz=(NSSize)[super contentMinSize]; \
      mmi.ptMinTrackSize.x=(int)minsz.width; mmi.ptMinTrackSize.y=(int)minsz.height; \
        [ch onSwellMessage:WM_GETMINMAXINFO p1:0 p2:(LPARAM)&mmi]; \
          minsz.width=mmi.ptMinTrackSize.x; minsz.height=mmi.ptMinTrackSize.y; \
            [super setContentMinSize:minsz];  \
}

#endif

@interface SWELLModelessDialog : NSWindow
{
  
  DECLARE_COMMON_VARS
  
}
- (id)initModeless:(SWELL_DialogResourceIndex *)resstate Parent:(HWND)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par;
- (id)initModelessForChild:(HWND)child owner:(HWND)owner;
@end


@implementation SWELLModelessDialog : NSWindow

SWELLDIALOGCOMMONIMPLEMENTS_WND


- (id)initModelessForChild:(HWND)child owner:(HWND)owner
{
  INIT_COMMON_VARS
  
  NSRect cr=[(NSView *)child bounds];
  NSRect contentRect=NSMakeRect(50,50,cr.size.width,cr.size.height);
  if (!(self = [super initWithContentRect:contentRect styleMask:(NSTitledWindowMask|NSMiniaturizableWindowMask|NSClosableWindowMask|NSResizableWindowMask) backing:NSBackingStoreBuffered defer:NO])) return self;

  [self setDelegate:self];
  [self setAcceptsMouseMovedEvents:YES];
  [self setContentView:(NSView *)child];
  [self useOptimizedDrawing:YES];
    
  if (owner && [(id)owner respondsToSelector:@selector(swellAddOwnedWindow:)])
  {
    [(id)owner swellAddOwnedWindow:self]; 
  }
  else if (owner && [(id)owner isKindOfClass:[NSView class]])
  {
    NSWindow *w=[(id)owner window];
    if (w && [w respondsToSelector:@selector(swellAddOwnedWindow:)])
    {
      [w swellAddOwnedWindow:self]; 
    }
  }
    
  [self display];
  return self;
}

- (id)initModeless:(SWELL_DialogResourceIndex *)resstate Parent:(HWND)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par
{
  INIT_COMMON_VARS
  
  NSRect contentRect=NSMakeRect(50,50,resstate ? resstate->width : 10,resstate ? resstate->height : 10);
  int sf=0;
  
  if (resstate)
  {
    sf |= NSTitledWindowMask|NSMiniaturizableWindowMask|NSClosableWindowMask;
    if (resstate->windowTypeFlags&SWELL_DLG_WS_RESIZABLE) sf |= NSResizableWindowMask;
  }
  
  if (!(self = [super initWithContentRect:contentRect styleMask:sf backing:NSBackingStoreBuffered defer:NO])) return self;
  
  [self setAcceptsMouseMovedEvents:YES];
  [self useOptimizedDrawing:YES];
  [self setDelegate:self];
  
  if (resstate&&resstate->title) SetWindowText((HWND)self, resstate->title);
  
  
  if (parent && [(id)parent respondsToSelector:@selector(swellAddOwnedWindow:)])
  {
    [(id)parent swellAddOwnedWindow:self]; 
  }
  else if (parent && [(id)parent isKindOfClass:[NSView class]])
  {
    NSWindow *w=[(id)parent window];
    if (w && [w respondsToSelector:@selector(swellAddOwnedWindow:)])
    {
      [w swellAddOwnedWindow:self]; 
    }
  }
  
  SWELLDialogChild *ch=[[SWELLDialogChild alloc] initChild:resstate Parent:self dlgProc:dlgproc Param:par];       // create a new child view class
    
//  DOWINDOWMINMAXSIZES(ch)
  [ch release];

  [self display];
  
  return self;
}

@end



@interface SWELLDialogBox : NSPanel
{
  DECLARE_COMMON_VARS
  
  int m_rv;
}
- (id)initDialogBox:(SWELL_DialogResourceIndex *)resstate Parent:(HWND)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par;
@end

@implementation SWELLDialogBox : NSPanel

SWELLDIALOGCOMMONIMPLEMENTS_WND



- (id)initDialogBox:(SWELL_DialogResourceIndex *)resstate Parent:(HWND)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par
{
  m_rv=0;
  INIT_COMMON_VARS
  
  NSRect contentRect=NSMakeRect(0,0,resstate->width,resstate->height);
  if (!(self = [super initWithContentRect:contentRect styleMask:(NSTitledWindowMask|NSClosableWindowMask|((resstate->windowTypeFlags&SWELL_DLG_WS_RESIZABLE)? NSResizableWindowMask : 0)) backing:NSBackingStoreBuffered defer:NO])) return self;

  [self setAcceptsMouseMovedEvents:YES];
  [self useOptimizedDrawing:YES];
  [self setDelegate:self];

  if (parent && [(id)parent respondsToSelector:@selector(swellAddOwnedWindow:)])
  {
    [(id)parent swellAddOwnedWindow:self]; 
  }
  else if (parent && [(id)parent isKindOfClass:[NSView class]])
  {
    NSWindow *w=[(id)parent window];
    if (w && [w respondsToSelector:@selector(swellAddOwnedWindow:)])
    {
      [w swellAddOwnedWindow:self]; 
    }
  }
  if (resstate&&resstate->title) SetWindowText((HWND)self, resstate->title);
  
  SWELLDialogChild *ch=[[SWELLDialogChild alloc] initChild:resstate Parent:self dlgProc:dlgproc Param:par];       // create a new child view class
  
//  DOWINDOWMINMAXSIZES(ch)
  [ch release];

  [self display];
  
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
  NSWindow *nswnd=(NSWindow *)wnd;
  if (wnd && ([NSApp modalWindow] == nswnd ||
              ([nswnd isKindOfClass:[NSView class]] && [NSApp modalWindow] == (nswnd=[(NSView *)nswnd window]))))
  {
    if ([nswnd respondsToSelector:@selector(swellSetModalRetVal:)])
       [nswnd swellSetModalRetVal:ret];
    
    if ([(id)wnd respondsToSelector:@selector(onSwellMessage:p1:p2:)])
      [(id)wnd onSwellMessage:WM_DESTROY p1:0 p2:0];
    
    NSEvent *evt=[NSApp currentEvent];
    if (evt && [evt window] == nswnd)
      [NSApp stopModal];
    else
      [NSApp abortModal];
    
    if ([nswnd isKindOfClass:[NSWindow class]])
      [nswnd close];
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

HWND SWELL_CreateModelessFrameForWindow(HWND childW, HWND ownerW)
{
    SWELLModelessDialog *ch=[[SWELLModelessDialog alloc] initModelessForChild:childW owner:ownerW];
    return (HWND)ch;
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
  else
  {
    SWELLModelessDialog *ch=[[SWELLModelessDialog alloc] initModeless:p Parent:parent dlgProc:dlgproc Param:param];
    if (!ch) return 0;
    return (HWND)[ch contentView];
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