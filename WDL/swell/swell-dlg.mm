#ifndef SWELL_PROVIDED_BY_APP


#include "swell.h"
#include "swell-dlggen.h"

#import <Cocoa/Cocoa.h>
static HMENU g_swell_defaultmenu,g_swell_defaultmenumodal;

void (*SWELL_DDrop_onDragLeave)();
void (*SWELL_DDrop_onDragOver)(POINT pt);
void (*SWELL_DDrop_onDragEnter)(void *hGlobal, POINT pt);

bool SWELL_owned_windows_levelincrease=false;

#include "swell-internal.h"


static NSColor *NSColorFromCGColor(CGColorRef ref)
{
  const float *cc = CGColorGetComponents(ref);
  if (cc) return [NSColor colorWithCalibratedRed:cc[0] green:cc[1] blue:cc[2] alpha:cc[3]];
  return NULL;
}

void SWELL_DoDialogColorUpdates(HWND hwnd, DLGPROC d, bool isUpdate)
{
  extern GDP_CTX *SWELL_GDP_CTX_NEW();
  NSArray *children = [(NSView *)hwnd subviews];
  
  if (!d || !children || ![children count]) return;

  int had_flags=0;

  NSColor *staticFg=NULL; // had_flags&1, WM_CTLCOLORSTATIC
  NSColor *editFg=NULL, *editBg=NULL; // had_flags&2, WM_CTLCOLOREDIT
  NSColor *buttonFg=NULL; // had_flags&4, WM_CTLCOLORBTN
      
  int x;
  for (x = 0; x < [children count]; x ++)
  {
    NSView *ch = [children objectAtIndex:x];
    if (ch)
    {
      if ([ch isKindOfClass:[NSButton class]] && [(NSButton *)ch image])
      {
        if (!(had_flags&4))
        {
          had_flags|=4;
          GDP_CTX *c = SWELL_GDP_CTX_NEW();
          if (c)
          {
            d(hwnd,WM_CTLCOLORBTN,(WPARAM)c,(LPARAM)ch);
            if (c->curcgtextcol) buttonFg=NSColorFromCGColor(c->curcgtextcol);
            else if (isUpdate) buttonFg = [NSColor textColor]; // todo some other col?              
            SWELL_DeleteGfxContext((HDC)c);
          }
        }
        if (buttonFg) [(NSTextField*)ch setTextColor:buttonFg]; // NSButton had this added below
      }
      else if ([ch isKindOfClass:[NSTextField class]] || [ch isKindOfClass:[NSBox class]])
      {
        bool isbox = ([ch isKindOfClass:[NSBox class]]);        
        if (!isbox && [(NSTextField *)ch isEditable])
        {
          if (!(had_flags&2))
          {
            had_flags|=2;
            GDP_CTX *c = SWELL_GDP_CTX_NEW();
            if (c)
            {
              d(hwnd,WM_CTLCOLOREDIT,(WPARAM)c,(LPARAM)ch);
              if (c->curcgtextcol) 
              {
                editFg=NSColorFromCGColor(c->curcgtextcol);
                editBg=[NSColor colorWithCalibratedRed:GetRValue(c->curbkcol)/255.0f green:GetGValue(c->curbkcol)/255.0f blue:GetBValue(c->curbkcol)/255.0f alpha:1.0f];
              }
              else if (isUpdate) 
              {
                editFg = [NSColor textColor]; 
                editBg = [NSColor textBackgroundColor];
              }
              SWELL_DeleteGfxContext((HDC)c);
            }
          }
          if (editFg) [(NSTextField*)ch setTextColor:editFg]; 
          if (editBg) [(NSTextField*)ch setBackgroundColor:editBg];
        }
        else // isbox or noneditable
        {
          if (!(had_flags&1))
          {
            had_flags|=1;
            GDP_CTX *c = SWELL_GDP_CTX_NEW();
            if (c)
            {
              d(hwnd,WM_CTLCOLORSTATIC,(WPARAM)c,(LPARAM)ch);
              if (c->curcgtextcol) 
              {
                staticFg=NSColorFromCGColor(c->curcgtextcol);
              }
              else if (isUpdate) 
              {
                staticFg = [NSColor textColor]; 
              }
              SWELL_DeleteGfxContext((HDC)c);
            }
          }
          if (staticFg)
          {
            if (isbox) 
            {
              [[(NSBox*)ch titleCell] setTextColor:staticFg];
              //[(NSBox*)ch setBorderColor:staticFg]; // see comment at SWELL_MakeGroupBox
            }
            else
            {
              [(NSTextField*)ch setTextColor:staticFg]; 
            }
          }
        } // noneditable           
      }  //nstextfield
    } // child
  }     // children
}  

static LRESULT SwellDialogDefaultWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  DLGPROC d=(DLGPROC)GetWindowLong(hwnd,DWL_DLGPROC);
  if (d) 
  {
    if (uMsg == WM_PAINT)
    {
        PAINTSTRUCT ps;
        if (BeginPaint(hwnd,&ps))
        {
          HBRUSH hbrush = (HBRUSH) d(hwnd,WM_CTLCOLORDLG,(WPARAM)ps.hdc,(LPARAM)hwnd);
          if (hbrush && hbrush != (HBRUSH)1)
          {
            FillRect(ps.hdc,&ps.rcPaint,hbrush);
          }
          else if ([[(NSView *)hwnd window] contentView] == (NSView *)hwnd && ![(NSView *)hwnd isOpaque]) // always draw content view, unless opaque (which implies it doesnt need it)
          {
            SWELL_FillDialogBackground(ps.hdc,&ps.rcPaint,3);
          }
          
          EndPaint(hwnd,&ps);
        }
    }
    
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


static int DelegateMouseMove(NSView *view, NSEvent *theEvent)
{
  static int __nofwd;
  if (__nofwd) return 0;
  
  NSPoint p=[theEvent locationInWindow];
  NSWindow *w=[theEvent window];
  if (!w) return 0;
  NSView *v=[[w contentView] hitTest:p];
  
  if (!v) 
  {
    if (![NSApp modalWindow])
    {
      p=[w convertBaseToScreen:p];
    
      POINT pt={(int)(p.x+0.5),(int)(p.y+0.5)};
      HWND h=WindowFromPoint(pt);
      if (h && [(id)h isKindOfClass:[SWELL_hwndChild class]])
      {
        NSWindow *nw = [(NSView *)h window];
        if (nw != w)
        {
          p = [nw convertScreenToBase:p];
          theEvent = [NSEvent mouseEventWithType:[theEvent type] 
                              location:p 
                            modifierFlags:[theEvent modifierFlags]
                            timestamp:[theEvent timestamp]
                            windowNumber:[nw windowNumber] 
                            context:[nw graphicsContext] 
                            eventNumber:[theEvent eventNumber] 
                            clickCount:[theEvent clickCount]
                            pressure:[theEvent pressure]];
        }
        v=(NSView *)h;      
      }
    }  
    
    if (!v) return !GetCapture(); // eat mouse move if not captured
  }
  if (v==view) return 0;
  
  __nofwd=1;
  [v mouseMoved:theEvent];
  __nofwd=0;
  
  return 1;
}




@implementation SWELL_hwndChild : NSView 

- (void)SWELL_Timer:(id)sender
{
  id uinfo=[sender userInfo];
  if ([uinfo respondsToSelector:@selector(getValue)]) 
  {
    WPARAM idx=(WPARAM)[(SWELL_DataHold*)uinfo getValue];
    m_wndproc((HWND)self,WM_TIMER,idx,0);
  }
}

- (int)swellCapChangeNotify { return YES; }

- (LRESULT)onSwellMessage:(UINT)msg p1:(WPARAM)wParam p2:(LPARAM)lParam
{
  if (msg==WM_DESTROY) 
  { 
    if (m_hashaddestroy) return 0; 
    m_hashaddestroy=true; 
    if (GetCapture()==(HWND)self) ReleaseCapture(); 
    SWELL_MessageQueue_Clear((HWND)self); 
  }
  else if (msg==WM_CAPTURECHANGED && m_hashaddestroy) return 0; 
  
  LRESULT ret=m_wndproc((HWND)self,msg,wParam,lParam);
  if (msg == WM_DESTROY) 
  {
    if ([[self window] contentView] == self && [[self window] respondsToSelector:@selector(swellDestroyAllOwnedWindows)])
      [(SWELL_ModelessWindow*)[self window] swellDestroyAllOwnedWindows];

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
           [(SWELL_hwndChild*)sv onSwellMessage:WM_DESTROY p1:0 p2:0];
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
- (void)comboBoxSelectionDidChange:(NSNotification *)notification
{
  id sender=[notification object];
  int code=CBN_SELCHANGE;
  m_wndproc((HWND)self,WM_COMMAND,([sender tag])|(code<<16),(LPARAM)sender);
}

- (void)textDidEndEditing:(NSNotification *)aNotification
{
  id sender=[aNotification object];
  int code=EN_CHANGE;
  if ([sender isKindOfClass:[NSComboBox class]]) return;
  m_wndproc((HWND)self,WM_COMMAND,([sender tag])|(code<<16),(LPARAM)sender);
}

- (void)controlTextDidChange:(NSNotification *)aNotification
{
  id sender=[aNotification object];
  int code=EN_CHANGE;
  if ([sender isKindOfClass:[NSComboBox class]]) code=CBN_EDITCHANGE;
  m_wndproc((HWND)self,WM_COMMAND,([sender tag])|(code<<16),(LPARAM)sender);
}

- (void)menuNeedsUpdate:(NSMenu *)menu
{
  m_wndproc((HWND)self,WM_INITMENUPOPUP,(WPARAM)menu,0);
}

-(void) swellOnControlDoubleClick:(id)sender
{
  if ([sender isKindOfClass:[NSTableView class]] && 
      [sender respondsToSelector:@selector(getSwellNotificationMode)] &&
      [(SWELL_ListView*)sender getSwellNotificationMode])
  {
    m_wndproc((HWND)self,WM_COMMAND,(LBN_DBLCLK<<16)|[sender tag],(LPARAM)sender);
  }
  else
  {   
    NMCLICK nm={{(HWND)sender,[sender tag],NM_DBLCLK}, }; 
    m_wndproc((HWND)self,WM_NOTIFY,[sender tag],(LPARAM)&nm);
  }
}

- (void)outlineViewSelectionDidChange:(NSNotification *)notification
{
  NSOutlineView *sender=[notification object];
  NMTREEVIEW nmhdr={{(HWND)sender,(int)[sender tag],TVN_SELCHANGED},0,};  // todo: better treeview notifications
  m_wndproc((HWND)self,WM_NOTIFY,(int)[sender tag],(LPARAM)&nmhdr);
}
- (void)tableViewSelectionDidChange:(NSNotification *)aNotification
{
  NSTableView *sender=[aNotification object];
      if ([sender respondsToSelector:@selector(getSwellNotificationMode)] && [(SWELL_ListView*)sender getSwellNotificationMode])
      {
        m_wndproc((HWND)self,WM_COMMAND,(int)[sender tag] | (LBN_SELCHANGE<<16),(LPARAM)sender);
      }
      else
      {
        NMLISTVIEW nmhdr={{(HWND)sender,(int)[sender tag],LVN_ITEMCHANGED},(int)[sender selectedRow],0}; 
        m_wndproc((HWND)self,WM_NOTIFY,(int)[sender tag],(LPARAM)&nmhdr);
        
      }
}

- (void)tableView:(NSTableView *)tableView didClickTableColumn:(NSTableColumn *)tableColumn
{
  if ([tableView isKindOfClass:[SWELL_ListView class]] && 
      ((SWELL_ListView *)tableView)->m_cols && 
      !((SWELL_ListView *)tableView)->m_lbMode &&
      !(((SWELL_ListView *)tableView)->style & LVS_NOSORTHEADER)
      )
  {
    int col=((SWELL_ListView *)tableView)->m_cols->Find(tableColumn);

    NMLISTVIEW hdr={{(HWND)tableView,[tableView tag],LVN_COLUMNCLICK},-1,col};
    m_wndproc((HWND)self,WM_NOTIFY,[tableView tag], (LPARAM) &hdr);
  }
}

-(void) onSwellCommand:(id)sender
{
  if ([sender isKindOfClass:[NSSlider class]])
  {
    m_wndproc((HWND)self,WM_HSCROLL,0,(LPARAM)sender);
    //  WM_HSCROLL, WM_VSCROLL
  }
  else if ([sender isKindOfClass:[NSTableView class]])
  {
  #if 0
    if ([sender isKindOfClass:[NSOutlineView class]])
    {
//      NMTREEVIEW nmhdr={{(HWND)sender,(int)[sender tag],TVN_SELCHANGED},0,};  // todo: better treeview notifications
  //    m_wndproc((HWND)self,WM_NOTIFY,(int)[sender tag],(LPARAM)&nmhdr);
    }
    else
    {
    
      if ([sender respondsToSelector:@selector(getSwellNotificationMode)] && [(SWELL_ListView*)sender getSwellNotificationMode])
      {
        m_wndproc((HWND)self,WM_COMMAND,(int)[sender tag] | (LBN_SELCHANGE<<16),(LPARAM)sender);
      }
      else
      {
        NMLISTVIEW nmhdr={{(HWND)sender,(int)[sender tag],LVN_ITEMCHANGED},(int)[sender clickedRow],0}; 
        m_wndproc((HWND)self,WM_NOTIFY,(int)[sender tag],(LPARAM)&nmhdr);
      }
    }
    #endif
  }
  else
  {
    int cw=0;
    if ([sender isKindOfClass:[NSComboBox class]]) return; // combo boxes will use delegate messages
    else if ([sender isKindOfClass:[NSPopUpButton class]]) cw=CBN_SELCHANGE;
    else if ([sender isKindOfClass:[SWELL_Button class]])
    {
      int rf;
      if ((rf=(int)[(SWELL_Button*)sender swellGetRadioFlags]))
      {
        NSView *par=(NSView *)GetParent((HWND)sender);
        if (par && [par isKindOfClass:[NSWindow class]]) par=[(NSWindow *)par contentView];
        if (par && [par isKindOfClass:[NSView class]])
        {
          NSArray *ar=[par subviews];
          if (ar)
          {
            int x=[ar indexOfObject:sender];
            if (x != NSNotFound)
            {
              int n=[ar count];
              int a=x;
              if (!(rf&2)) while (--a >= 0)
              {
                NSView *item=[ar objectAtIndex:a];
                if (!item || ![item isKindOfClass:[SWELL_Button class]]) break; // we may want to allow other controls in there, but for now if it's non-button we're done
                int bla=(int)[(SWELL_Button*)item swellGetRadioFlags]; 
                if (bla&1) if ([(NSButton *)item state]!=NSOffState) [(NSButton *)item setState:NSOffState];                
                if (bla&2) break;
              }
              a=x;
              while (++a < n)
              {
                NSView *item=[ar objectAtIndex:a];
                if (!item || ![item isKindOfClass:[SWELL_Button class]]) break; // we may want to allow other controls in there, but for now if it's non-button we're done
                int bla=(int)[(SWELL_Button*)item swellGetRadioFlags];
                if (bla&2) break;              
                if (bla&1) if ([(NSButton *)item state]!=NSOffState) [(NSButton *)item setState:NSOffState];                
              }
            }
          }
        }
      }
    }
    else if ([sender isKindOfClass:[NSControl class]])
    {
      NSEvent *evt=[NSApp currentEvent];
      int ty=evt?[evt type]:0;
      if (evt && (ty==NSLeftMouseDown || ty==NSLeftMouseUp) && [evt clickCount] > 1) cw=STN_DBLCLK;
    }
    m_wndproc((HWND)self,WM_COMMAND,[sender tag]|(cw<<16),(LPARAM)sender);
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
-(LONG)getSwellUserData { return m_userdata; }
-(void)setSwellUserData:(LONG)val {   m_userdata=val; }
-(LPARAM)getSwellExtraData:(int)idx { idx/=4; if (idx>=0&&idx<sizeof(m_extradata)/sizeof(m_extradata[0])) return m_extradata[idx]; return 0; }
-(void)setSwellExtraData:(int)idx value:(LPARAM)val { idx/=4; if (idx>=0&&idx<sizeof(m_extradata)/sizeof(m_extradata[0])) m_extradata[idx] = val; }
-(void)setSwellWindowProc:(WNDPROC)val { m_wndproc=val; }
-(WNDPROC)getSwellWindowProc { return m_wndproc; }
-(void)setSwellDialogProc:(DLGPROC)val { m_dlgproc=val; }
-(DLGPROC)getSwellDialogProc { return m_dlgproc; }
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

  [self setHidden:YES];
  
//  BOOL wasHid=[self isHidden];
  //if (!wasHid) [self setHidden:YES];
  
  bool isChild=false;
  
  if ([parent isKindOfClass:[NSSavePanel class]]||[parent isKindOfClass:[NSOpenPanel class]])
  {
    [(NSSavePanel *)parent setAccessoryView:self];
    [self setHidden:NO];
  }
  else if ([parent isKindOfClass:[NSWindow class]])
  {
    [(NSWindow *)parent setContentView:self];
  }
  else
  {
    isChild=[parent isKindOfClass:[NSView class]];
    [parent addSubview:self];
  }
  if (resstate) resstate->createFunc((HWND)self,resstate->windowTypeFlags);
  
  if (resstate) m_dlgproc=dlgproc;  
  else if (dlgproc) m_wndproc=(WNDPROC)dlgproc;
  
  if (resstate && (resstate->windowTypeFlags&SWELL_DLG_WS_DROPTARGET))
  {
    [self registerForDraggedTypes:[NSArray arrayWithObjects:NSFilenamesPboardType, nil]];
  }
  
  if (!resstate)
    m_wndproc((HWND)self,WM_CREATE,0,par);
    
  if (m_dlgproc)
  {
    HWND hFoc=0;
    NSArray *ar=[self subviews];
    if (ar && [ar count]>0)
    {
      int x;
      for (x = 0; x < [ar count] && !hFoc; x ++)
      {
        NSView *v=[ar objectAtIndex:x];
        if (v && [v isKindOfClass:[NSScrollView class]]) v=[(NSScrollView *)v documentView];
        if (v && [v acceptsFirstResponder]) hFoc=(HWND)v;
      }
    }
    if (!isChild && hFoc) SetFocus(hFoc); // if not child window, set focus anyway
    
    if (m_dlgproc((HWND)self,WM_INITDIALOG,(WPARAM)hFoc,par) && hFoc)
      SetFocus(hFoc);
    SWELL_DoDialogColorUpdates((HWND)self,m_dlgproc,false);
  }
  
//  if (!wasHid)
  //  [self setHidden:NO];
  
  return self;
}

-(void)setOpaque:(bool)isOpaque
{
  m_isopaque = isOpaque;
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
    if (m_wndproc((HWND)self,WM_KEYDOWN,code,flag)==69) [super keyDown:theEvent];
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
    if (m_wndproc((HWND)self,WM_KEYUP,code,flag)==69) [super keyUp:theEvent];
  }
}

/*
-(id)_recursiveDisplayAllDirtyWithLockFocus:(BOOL)lf visRect:(NSRect) rc
{
// hook drawing of children!
//  printf("turds\n");
  return [super _recursiveDisplayAllDirtyWithLockFocus:lf visRect:rc];
}

*/

-(void) drawRect:(NSRect)rect
{
  if (m_hashaddestroy) return;
  m_paintctx_hdc=SWELL_CreateGfxContext([NSGraphicsContext currentContext]);
  m_paintctx_rect=rect;
  m_paintctx_used=false;
  DoPaintStuff(m_wndproc,(HWND)self,m_paintctx_hdc,&m_paintctx_rect);
  
  
  SWELL_DeleteGfxContext(m_paintctx_hdc);
  m_paintctx_hdc=0;
  if (!m_paintctx_used) {
     /*[super drawRect:rect];*/
  }
  
#if 0
  // debug: show everything
  static CGColorSpaceRef cspace;
  if (!cspace) cspace=CGColorSpaceCreateDeviceRGB();
  float cols[4]={0.0f,1.0f,0.0f,0.8f};
  CGColorRef color=CGColorCreate(cspace,cols);
  
  CGContextRef ctx = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
  CGContextSetStrokeColorWithColor(ctx,color);
  CGContextStrokeRectWithWidth(ctx, CGRectMake(rect.origin.x,rect.origin.y,rect.size.width,rect.size.height), 1);

  CGColorRelease(color);
  
  cols[0]=1.0f;
  cols[1]=0.0f;
  cols[2]=0.0f;
  cols[3]=1.0f;
  color=CGColorCreate(cspace,cols);

  NSRect rect2=[self bounds];
  CGContextSetStrokeColorWithColor(ctx,color);
  CGContextStrokeRectWithWidth(ctx, CGRectMake(rect2.origin.x,rect2.origin.y,rect2.size.width,rect2.size.height), 1);
  
  
  CGColorRelease(color);
  
  cols[0]=0.0f;
  cols[1]=0.0f;
  cols[2]=1.0f;
  cols[3]=0.7f;
  color=CGColorCreate(cspace,cols);
  cols[3]=0.25;
  cols[2]=0.5;
  CGColorRef color2=CGColorCreate(cspace,cols);

  NSArray *ar = [self subviews];
  if (ar)
  {
    int x;
    for(x=0;x<[ar count];x++)  
    {
      NSView *v = [ar objectAtIndex:x];
      if (v && ![v isHidden])
      {
        NSRect rect = [v frame];
        CGContextSetStrokeColorWithColor(ctx,color);
        CGContextStrokeRectWithWidth(ctx, CGRectMake(rect.origin.x,rect.origin.y,rect.size.width,rect.size.height), 1);
        CGContextSetFillColorWithColor(ctx,color2);
        CGContextFillRect(ctx, CGRectMake(rect.origin.x,rect.origin.y,rect.size.width,rect.size.height));
      }
    }
    
  // draw children
  }
  CGColorRelease(color);
  CGColorRelease(color2);
  
#endif
  
  
  
}
- (void)rightMouseDragged:(NSEvent *)theEvent
{
  if (!m_enabled) return;
  [self mouseDragged:theEvent];
}
- (void)otherMouseDragged:(NSEvent *)theEvent
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
  if ([NSApp keyWindow] != [self window])
  {
    SetFocus((HWND)[self window]);
  }
  [self sendMouseMessage:([theEvent clickCount]>1 ? WM_RBUTTONDBLCLK : WM_RBUTTONDOWN) event:theEvent]; 
}  
- (void)otherMouseUp:(NSEvent *)theEvent
{
  if (!m_enabled) return;
  [self sendMouseMessage:WM_MBUTTONUP event:theEvent];  
}
- (void)otherMouseDown:(NSEvent *)theEvent
{
  if ([NSApp keyWindow] != [self window])
  {
    SetFocus((HWND)[self window]);
  }
  [self sendMouseMessage:([theEvent clickCount]>1 ? WM_MBUTTONDBLCLK : WM_MBUTTONDOWN) event:theEvent]; 
}  

-(LRESULT)sendMouseMessage:(int)msg event:(NSEvent*)theEvent
{
  NSView *capv=(NSView *)GetCapture();
  if (capv && capv != self && [capv window] == [self window] && [capv respondsToSelector:@selector(sendMouseMessage:event:)])
    return (LONG)[(SWELL_hwndChild *)capv sendMouseMessage:msg event:theEvent];
  
  if (m_hashaddestroy) return -1;
  
  NSPoint swellProcessMouseEvent(int msg, NSView *view, NSEvent *event);
  
	NSPoint p = swellProcessMouseEvent(msg,self,theEvent);
	unsigned short xpos=(int)(p.x); unsigned short ypos=(int)(p.y);
  
  LRESULT htc=HTCLIENT;
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
       if (msg==WM_MBUTTONUP) return m_wndproc((HWND)self,WM_NCMBUTTONUP,htc,p); 
       if (msg==WM_MBUTTONDOWN) return m_wndproc((HWND)self,WM_NCMBUTTONDOWN,htc,p); 
       if (msg==WM_MBUTTONDBLCLK) return m_wndproc((HWND)self,WM_NCMBUTTONDBLCLK,htc,p); 
     } 
  } 
  
  int l=0;
  if (msg==WM_MOUSEWHEEL)
  {
    float dy=[theEvent deltaY];
    if (!dy) dy=[theEvent deltaX]; // shift+mousewheel sends deltaX instead of deltaY
    l=(int) (dy*60.0);
    l<<=16;
  }
  
  // put todo: modifiers into low word of l?
  
  if (msg==WM_MOUSEWHEEL)
  {
    POINT p;
    GetCursorPos(&p);
    return m_wndproc((HWND)self,msg,l,(short)p.x + (((int)p.y)<<16));
  }
  
  LRESULT ret=m_wndproc((HWND)self,msg,l,xpos + (ypos<<16));

  if (msg==WM_LBUTTONUP || msg==WM_RBUTTONUP || msg==WM_MOUSEMOVE || msg==WM_MBUTTONUP) {
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

-(void)swellSetExtendedStyle:(LONG)st
{
  if (st&WS_EX_ACCEPTFILES) m_supports_ddrop=true;
  else m_supports_ddrop=false;
}
-(LONG)swellGetExtendedStyle
{
  LONG ret=0;
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
      SWELL_CFStringToCString(sv,text,sizeof(text));
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
      SWELL_CFStringToCString(sv,text,sizeof(text));
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
        if ([(SWELL_hwndChild *)v swellExtendedDragOp:sender retGlob:NO]) return YES;
      v=[v superview];
    }
  }
  return !![self swellExtendedDragOp:sender retGlob:NO];
}

-(unsigned int)swellCreateWindowFlags
{
  return m_create_windowflags;
}

@end





static HWND last_key_window;


#define SWELLDIALOGCOMMONIMPLEMENTS_WND(ISMODAL) \
-(BOOL)acceptsFirstResponder { return m_enabled?YES:NO; } \
- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent {	return m_enabled?YES:NO; } \
- (void)setFrame:(NSRect)frameRect display:(BOOL)displayFlag \
{ \
  [super setFrame:frameRect display:displayFlag]; \
    [(SWELL_hwndChild*)[self contentView] onSwellMessage:WM_SIZE p1:0 p2:0]; \
} \
- (void)windowDidMove:(NSNotification *)aNotification { \
    NSRect f=[self frame]; \
    [(SWELL_hwndChild*)[self contentView] onSwellMessage:WM_MOVE p1:0 p2:MAKELPARAM((int)f.origin.x,(int)f.origin.y)]; \
} \
-(void)swellDoDestroyStuff \
{ \
  if (last_key_window==(HWND)self) last_key_window=0; \
      OwnedWindowListRec *p=m_ownedwnds; m_ownedwnds=0; \
        while (p) \
        { \
          OwnedWindowListRec *next=p->_next;  \
            DestroyWindow((HWND)p->hwnd); \
              free(p); p=next;  \
        } \
  if (m_owner) [(SWELL_ModelessWindow*)m_owner swellRemoveOwnedWindow:self]; \
    m_owner=0;  \
} \
-(void)dealloc \
{ \
  [self swellDoDestroyStuff]; \
  [super dealloc]; \
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
  [(SWELL_hwndChild*)[self contentView] onSwellMessage:WM_ACTIVATE p1:WA_INACTIVE p2:(LPARAM)0]; \
  last_key_window=(HWND)self; \
} \
-(void)becomeKeyWindow \
{ \
  [super becomeKeyWindow]; \
  HWND foc=last_key_window ? (HWND)[(NSWindow *)last_key_window contentView] : 0; \
    HMENU menu=0; \
      if ([[self contentView] respondsToSelector:@selector(swellGetMenu)]) \
        menu = (HMENU) [[self contentView] swellGetMenu]; \
          if (!menu) menu=ISMODAL && g_swell_defaultmenumodal ? g_swell_defaultmenumodal : g_swell_defaultmenu; \
            if (menu && menu != [NSApp mainMenu])  [NSApp setMainMenu:(NSMenu *)menu]; \
  [(SWELL_hwndChild*)[self contentView] onSwellMessage:WM_ACTIVATE p1:WA_ACTIVE p2:(LPARAM)foc]; \
} \
-(BOOL)windowShouldClose:(id)sender \
{ \
  NSView *v=[self contentView]; \
    if ([v respondsToSelector:@selector(onSwellMessage:p1:p2:)]) \
      if (![(SWELL_hwndChild*)v onSwellMessage:WM_CLOSE p1:0 p2:0]) \
        [(SWELL_hwndChild*)v onSwellMessage:WM_COMMAND p1:IDCANCEL p2:0]; \
          return NO; \
} \
- (BOOL)canBecomeKeyWindow {   return !!m_enabled; } \
- (void **)swellGetOwnerWindowHead { return (void **)&m_ownedwnds; } \
- (void)swellAddOwnedWindow:(NSWindow*)wnd \
{ \
  OwnedWindowListRec *p=m_ownedwnds; \
    while (p) { \
      if (p->hwnd == wnd) return; \
        p=p->_next; \
    } \
    p=(OwnedWindowListRec*)malloc(sizeof(OwnedWindowListRec)); \
    p->hwnd=wnd; p->_next=m_ownedwnds; m_ownedwnds=p; \
    if ([wnd respondsToSelector:@selector(swellSetOwner:)]) [(SWELL_ModelessWindow*)wnd swellSetOwner:self];  \
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
  [(SWELL_hwndChild*)[self contentView] onSwellMessage:WM_GETMINMAXINFO p1:0 p2:(LPARAM)&mmi]; \
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
  [(SWELL_hwndChild*)[self contentView] onSwellMessage:WM_GETMINMAXINFO p1:0 p2:(LPARAM)&mmi]; \
  if (mmi.ptMaxTrackSize.x < 1000000) maxsz.width=mmi.ptMaxTrackSize.x; \
  if (mmi.ptMaxTrackSize.y < 1000000) maxsz.height=mmi.ptMaxTrackSize.y; \
  return maxsz; \
} \



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



@implementation SWELL_ModelessWindow : NSWindow

SWELLDIALOGCOMMONIMPLEMENTS_WND(0)


- (id)initModelessForChild:(HWND)child owner:(HWND)owner styleMask:(unsigned int)smask
{
  INIT_COMMON_VARS
  
  NSRect cr=[(NSView *)child bounds];
  NSRect contentRect=NSMakeRect(50,50,cr.size.width,cr.size.height);
  if (!(self = [super initWithContentRect:contentRect styleMask:smask backing:NSBackingStoreBuffered defer:NO])) return self;

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
      [(SWELL_ModelessWindow*)w swellAddOwnedWindow:self]; 
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
      [(SWELL_ModelessWindow*)w swellAddOwnedWindow:self]; 
    }
  }
  
  SWELL_hwndChild *ch=[[SWELL_hwndChild alloc] initChild:resstate Parent:(NSView *)self dlgProc:dlgproc Param:par];       // create a new child view class
  ch->m_create_windowflags=sf;

    
//  DOWINDOWMINMAXSIZES(ch)
  [ch release];

  [self display];
  
  return self;
}
-(int)level
{
  //if (SWELL_owned_windows_levelincrease) return NSNormalWindowLevel;
  return [super level];
}
@end




@implementation SWELL_ModalDialog : NSPanel

SWELLDIALOGCOMMONIMPLEMENTS_WND(1)



- (id)initDialogBox:(SWELL_DialogResourceIndex *)resstate Parent:(HWND)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par
{
  m_rv=0;
  INIT_COMMON_VARS
  
  NSRect contentRect=NSMakeRect(0,0,resstate->width,resstate->height);
  unsigned int sf=(NSTitledWindowMask|NSClosableWindowMask|((resstate->windowTypeFlags&SWELL_DLG_WS_RESIZABLE)? NSResizableWindowMask : 0));
  if (!(self = [super initWithContentRect:contentRect styleMask:sf backing:NSBackingStoreBuffered defer:NO])) return self;

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
      [(SWELL_ModelessWindow*)w swellAddOwnedWindow:self]; 
    }
  }
  if (resstate&&resstate->title) SetWindowText((HWND)self, resstate->title);
  
  SWELL_hwndChild *ch=[[SWELL_hwndChild alloc] initChild:resstate Parent:(NSView *)self dlgProc:dlgproc Param:par];       // create a new child view class
  ch->m_create_windowflags=sf;
  [ch setHidden:NO];
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
  if (!wnd) return;
  
  NSWindow *nswnd=NULL;
  NSView *nsview = NULL;
  if ([(id)wnd isKindOfClass:[NSView class]])
  {
    nsview = (NSView *)wnd;
    nswnd = [nsview window];
  }
  else if ([(id)wnd isKindOfClass:[NSWindow class]])
  {
    nswnd = (NSWindow *)wnd;
    nsview = [nswnd contentView];
  }
  if (!nswnd) return;
   
  if ([NSApp modalWindow] == nswnd)
  {
    if ([nswnd respondsToSelector:@selector(swellSetModalRetVal:)])
       [(SWELL_ModalDialog*)nswnd swellSetModalRetVal:ret];
    
    if ([nsview respondsToSelector:@selector(onSwellMessage:p1:p2:)])
      [(SWELL_hwndChild*)nsview onSwellMessage:WM_DESTROY p1:0 p2:0];
    
    NSEvent *evt=[NSApp currentEvent];
    if (evt && [evt window] == nswnd)
    {
      [NSApp stopModal];
    }
    
    [NSApp abortModal]; // always call this, otherwise if running in runModalForWindow: it can often require another even tto come through before things continue
    
    [nswnd close];
  }
}


int SWELL_DialogBox(SWELL_DialogResourceIndex *reshead, int resid, HWND parent,  DLGPROC dlgproc, LPARAM param)
{
  SWELL_DialogResourceIndex *p=resById(reshead,resid);
  if (!p||(p->windowTypeFlags&SWELL_DLG_WS_CHILD)) return -1;
  SWELL_ModalDialog *box = [[SWELL_ModalDialog alloc] initDialogBox:p Parent:parent dlgProc:dlgproc Param:param];      
     
  if (!box) return -1;
  
  if (![NSApp isActive]) // using this enables better background processing (i.e. if the app isnt active it still runs)
  {
    NSModalSession session = [NSApp beginModalSessionForWindow:box];
    for (;;) 
    {
      if ([NSApp runModalSession:session] != NSRunContinuesResponse) break;
      Sleep(1);
    }
    [NSApp endModalSession:session];
  }
  else
  {
    [NSApp runModalForWindow:box];
  }
  int ret=[box swellGetModalRetVal];
  [box release];
  return ret;
}

HWND SWELL_CreateModelessFrameForWindow(HWND childW, HWND ownerW, unsigned int windowFlags)
{
    SWELL_ModelessWindow *ch=[[SWELL_ModelessWindow alloc] initModelessForChild:childW owner:ownerW styleMask:windowFlags];
    return (HWND)ch;
}


HWND SWELL_CreateDialog(SWELL_DialogResourceIndex *reshead, int resid, HWND parent, DLGPROC dlgproc, LPARAM param)
{
  SWELL_DialogResourceIndex *p=resById(reshead,resid);
  if (!p&&resid) return 0;
  
  NSView *parview=NULL;
  if (parent && ([(id)parent isKindOfClass:[NSView class]] || [(id)parent isKindOfClass:[NSSavePanel class]] || [(id)parent isKindOfClass:[NSOpenPanel class]])) parview=(NSView *)parent;
  else if (parent && [(id)parent isKindOfClass:[NSWindow class]])  parview=(NSView *)[(id)parent contentView];
  
  if ((!p || (p->windowTypeFlags&SWELL_DLG_WS_CHILD)) && parview)
  {
    SWELL_hwndChild *ch=[[SWELL_hwndChild alloc] initChild:p Parent:parview dlgProc:dlgproc Param:param];       // create a new child view class
    ch->m_create_windowflags=(NSTitledWindowMask|NSMiniaturizableWindowMask|NSClosableWindowMask|NSResizableWindowMask);
    [ch release];
    return (HWND)ch;
  }
  else
  {
    SWELL_ModelessWindow *ch=[[SWELL_ModelessWindow alloc] initModeless:p Parent:parent dlgProc:dlgproc Param:param];
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
HMENU SWELL_GetDefaultModalWindowMenu() { return g_swell_defaultmenumodal; }
void SWELL_SetDefaultModalWindowMenu(HMENU menu)
{
  g_swell_defaultmenumodal=menu;
}



SWELL_DialogResourceIndex *SWELL_curmodule_dialogresource_head; // this eventually will go into a per-module stub file


#import <Carbon/Carbon.h>


#if 0
static void PrintAllHIViews(HIViewRef f, const char *bla)
{
  char tmp[4096];
  sprintf(tmp,"%s:%08x",bla,f);
  
  HIRect r;
  HIViewGetFrame(f,&r);
  printf("%s beg %f %f %f %f\n",tmp,r.origin.x,r.origin.y,r.size.width, r.size.height);
  HIViewRef a=HIViewGetFirstSubview(f);
  while (a)
  {
    PrintAllHIViews(a,tmp);
    a=HIViewGetNextView(a);  
  }
  printf("%s end\n",tmp);
}
#endif

// carbon event handler for carbon-in-cocoa
OSStatus CarbonEvtHandler(EventHandlerCallRef nextHandlerRef, EventRef event, void* userdata)
{
  SWELL_hwndCarbonHost* _this = (SWELL_hwndCarbonHost*)userdata;
  UInt32 evtkind = GetEventKind(event);

  switch (evtkind)
  {
    case kEventWindowActivated:
      [NSApp setMainMenu:nil];
    break;
    case kEventWindowGetClickActivation: 
    {
      ClickActivationResult car = kActivateAndHandleClick;
      SetEventParameter(event, kEventParamClickActivation, typeClickActivationResult, sizeof(ClickActivationResult), &car);
    }
    break;
    
    case kEventWindowHandleDeactivate:
    {
      if (_this)
      {
        void* SWELL_GetWindowFromCarbonWindowView(HWND);
        WindowRef wndref = (WindowRef)[_this->m_cwnd windowRef];
        if (wndref) ActivateWindow(wndref, true);
      }
    }
    break;
  
    case kEventControlBoundsChanged:
    {
      if (_this && !_this->m_whileresizing)
      {
        ControlRef ctl;
        GetEventParameter(event, kEventParamDirectObject, typeControlRef, 0, sizeof(ControlRef), 0, &ctl);
        
        Rect prevr, curr;
        GetEventParameter(event, kEventParamPreviousBounds, typeQDRectangle, 0, sizeof(Rect), 0, &prevr);
        GetEventParameter(event, kEventParamCurrentBounds, typeQDRectangle, 0, sizeof(Rect), 0, &curr);

        RECT parr;
        GetWindowRect((HWND)_this, &parr);
        parr.left += curr.left-prevr.left;
        parr.top += curr.top-prevr.top;
        parr.right += curr.right-prevr.right;
        parr.bottom += curr.bottom-prevr.bottom;        
        _this->m_whileresizing = true;
        SetWindowPos((HWND)_this, 0, parr.left, parr.right, parr.right-parr.left, parr.bottom-parr.top, SWP_NOZORDER|SWP_NOACTIVATE);
        _this->m_whileresizing = false;
      }
    }
    break;
  }
  return noErr;
}


@implementation SWELL_hwndCarbonHost

- (id)initCarbonChild:(NSView *)parent rect:(Rect*)r composit:(bool)wantComp
{
  if (!(self = [super initChild:nil Parent:parent dlgProc:nil Param:nil])) return self;

  WindowRef wndref=0;
  CreateNewWindow (kPlainWindowClass, (wantComp ? kWindowCompositingAttribute : 0) |  kWindowAsyncDragAttribute|kWindowStandardHandlerAttribute|kWindowNoShadowAttribute, r, &wndref);
  if (wndref)
  {
    // eventually we should set this and have the real NSWindow parent call ActivateWindow when activated/deactivated
    // SetWindowActivationScope( m_wndref, kWindowActivationScopeNone);    

    // adding a Carbon event handler to catch special stuff that NSWindow::initWithWindowRef
    // doesn't automatically redirect to a standard Cocoa window method
    
    ControlRef ctl=0;
    CreateRootControl(wndref, &ctl);  // creating root control here so callers must use GetRootControl

    EventTypeSpec winevts[] = 
    {
      { kEventClassWindow, kEventWindowActivated },
      { kEventClassWindow, kEventWindowGetClickActivation },
      { kEventClassWindow, kEventWindowHandleDeactivate },
    };
    int nwinevts = sizeof(winevts)/sizeof(EventTypeSpec);
          
    EventTypeSpec ctlevts[] = 
    {
      //{ kEventClassControl, kEventControlInitialize },
      //{ kEventClassControl, kEventControlDraw },            
      { kEventClassControl, kEventControlBoundsChanged },
    };
    int nctlevts = sizeof(ctlevts)/sizeof(EventTypeSpec);  
          
    EventHandlerRef wndhandler, ctlhandler;
    InstallWindowEventHandler(wndref, CarbonEvtHandler, nwinevts, winevts, self, &wndhandler);        
    InstallControlEventHandler(ctl, CarbonEvtHandler, nctlevts, ctlevts, self, &ctlhandler);
    m_wndhandler = wndhandler;
    m_ctlhandler = ctlhandler;
                    
    // initWithWindowRef does not retain // MAKE SURE THIS IS NOT BAD TO DO
    //CFRetain(wndref);

    m_cwnd = [[NSWindow alloc] initWithWindowRef:wndref];  
    [m_cwnd setDelegate:self];    
    
    ShowWindow(wndref);
    
    //[[parent window] addChildWindow:m_cwnd ordered:NSWindowAbove];
    //[self swellDoRepos]; 
    SetTimer((HWND)self,1,10,NULL);
  }  
  return self;
}

-(BOOL)swellIsCarbonHostingView { return YES; }

-(void)close
{
  KillTimer((HWND)self,1);
  
  if (m_wndhandler)
  {
    EventHandlerRef wndhandler = (EventHandlerRef)m_wndhandler;
    RemoveEventHandler(wndhandler);
    m_wndhandler = 0;
  }
  if (m_ctlhandler)
  {
    EventHandlerRef ctlhandler = (EventHandlerRef)m_ctlhandler;
    RemoveEventHandler(ctlhandler);
    m_ctlhandler = 0;
  }
  
  if (m_cwnd) 
  {
    if ([m_cwnd parentWindow]) [[m_cwnd parentWindow] removeChildWindow:m_cwnd];
    [m_cwnd orderOut:self];
    [m_cwnd close];   // this disposes the owned wndref
    m_cwnd=0;
  }
}

-(void)dealloc
{
  [self close];
  [super dealloc];  // ?!
}

- (void)SWELL_Timer:(id)sender
{
  id uinfo=[sender userInfo];
  if ([uinfo respondsToSelector:@selector(getValue)]) 
  {
    int idx=(int)[(SWELL_DataHold*)uinfo getValue];
    if (idx==1)
    {
      if (![self superview] || [[self superview] isHiddenOrHasHiddenAncestor])
      {
        NSWindow *oldw=[m_cwnd parentWindow];
        if (oldw)
        {
          [oldw removeChildWindow:(NSWindow *)m_cwnd];
          [m_cwnd orderOut:self];
        }
      }
      else
      {
        if (![m_cwnd parentWindow])
        {                
          NSWindow *par = [self window];
          if (par) 
          { 
            [par addChildWindow:m_cwnd ordered:NSWindowAbove];
            [self swellDoRepos];    
          }          
        }
        else 
        { 
          if (GetCurrentEventButtonState()&7)
          {
            if ([NSApp keyWindow] == [self window])
            {
              POINT p;
              GetCursorPos(&p);
              RECT r;
              GetWindowRect((HWND)self,&r);
              if (r.top>r.bottom)
              {
                int a=r.top;
                r.top=r.bottom;
                r.bottom=a;
              }
              if (m_cwnd && p.x >=r.left &&p.x < r.right && p.y >= r.top && p.y < r.bottom)
              {
                [(NSWindow *)m_cwnd makeKeyWindow];
              }
            }
          }
        }
      }
      return;
    }
    KillTimer((HWND)self,idx);
    return;
  }
}
- (LRESULT)onSwellMessage:(UINT)msg p1:(WPARAM)wParam p2:(LPARAM)lParam
{
  if (msg == WM_DESTROY)
  {
    if (m_cwnd) 
    {
      if ([NSApp keyWindow] == m_cwnd) // restore focus to highest window that is not us!
      {
        NSArray *ar = [NSApp orderedWindows];
        int x;
        for (x = 0; x < (ar ? [ar count] : 0); x ++)
        {
          NSWindow *w=[ar objectAtIndex:x];
          if (w && w != m_cwnd && [w isVisible]) { [w makeKeyWindow]; break; }
        }
      }
      
      [self close];
    }
  }
  return [super onSwellMessage:msg p1:wParam p2:lParam];
}
- (void)windowDidResignKey:(NSNotification *)aNotification
{
}
- (void)windowDidBecomeKey:(NSNotification *)aNotification
{
}


- (void)viewDidMoveToWindow
{
  [super viewDidMoveToWindow];
  if (m_cwnd)
  {
    // reparent m_cwnd to new owner
    NSWindow *neww=[self window];
    NSWindow *oldw=[m_cwnd parentWindow];
    if (neww != oldw)
    {
      if (oldw) [oldw removeChildWindow:m_cwnd];
    }
  }
}
-(void)swellDoRepos
{
  if (m_cwnd)
  {
    RECT r;
    GetWindowRect((HWND)self,&r);
    if (r.top>r.bottom)
    {
      int a=r.top;
      r.top=r.bottom;
      r.bottom=a;
    }
    
    // [m_cwnd setFrameOrigin:NSMakePoint(r.left,r.top)];
    
    {
      Rect bounds;
      bounds.left = r.left;
      bounds.top = CGRectGetHeight(CGDisplayBounds(kCGDirectMainDisplay))-r.bottom;
      // GetWindowBounds (m_wndref, kWindowContentRgn, &bounds);
      bounds.right = bounds.left + (r.right-r.left);
      bounds.bottom = bounds.top + (r.bottom-r.top);
      
      WindowRef wndref = (WindowRef)[m_cwnd windowRef];
      SetWindowBounds (wndref, kWindowContentRgn, &bounds); 

      // might make sense to only do this on initial show, but doesnt seem to hurt to do it often
      WindowAttributes wa=0;
      GetWindowAttributes(wndref,&wa);
      
      if (wa&kWindowCompositingAttribute)
      {
         HIViewRef ref = HIViewGetRoot(wndref);
         if (ref)
         {
            // PrintAllHIViews(ref,"");
            
            HIViewRef ref2=HIViewGetFirstSubview(ref);
            while  (ref2)
            {                              
            /*
              HIRect r3=CGRectMake(0,0,bounds.right-bounds.left,bounds.bottom-bounds.top);
             HIViewRef ref3=HIViewGetFirstSubview(ref2);
             while (ref3)
             {
              HIViewSetVisible(ref3,true);            
              HIViewSetNeedsDisplay(ref3,true);
              HIViewSetFrame(ref3,&r3);
              ref3=HIViewGetNextView(ref3);
             }
             */

              HIViewSetVisible(ref2,true);            
              HIViewSetNeedsDisplay(ref2,true);
              ref2=HIViewGetNextView(ref2);
            }
            HIViewSetVisible(ref,true);            
            HIViewSetNeedsDisplay(ref,true);
         }
      }
      else
      {
      
#if 0
        ControlRef rc=NULL;
        GetRootControl(m_wndref,&rc);
        if (rc)
        {
          RgnHandle rgn=NewRgn();
          GetControlRegion(rc,kControlEntireControl,rgn);
          UpdateControls(m_wndref,rgn);
          CloseRgn(rgn);
        }
#endif
        // Rect r={0,0,bounds.bottom-bounds.top,bounds.right-bounds.left};
        // InvalWindowRect(m_wndref,&r);
        
        // or we could just do: 
        DrawControls(wndref);
      }
    }
  }
}

- (void)viewDidMoveToSuperview
{
  [super viewDidMoveToSuperview];
  [self swellDoRepos];
}
- (void)setFrameSize:(NSSize)newSize
{
  [super setFrameSize:newSize];
  [self swellDoRepos];
}
- (void)setFrame:(NSRect)frameRect
{
  [super setFrame:frameRect];
  [self swellDoRepos];
}
- (void)setFrameOrigin:(NSPoint)newOrigin
{
  [super setFrameOrigin:newOrigin];
  [self swellDoRepos];
}


-(BOOL)isOpaque
{
  return NO;
}

@end


HWND SWELL_CreateCarbonWindowView(HWND viewpar, void **wref, RECT* r, bool wantcomp)  // window is created with a root control
{
  RECT wndr = *r;
  ClientToScreen(viewpar, (POINT*)&wndr);
  ClientToScreen(viewpar, (POINT*)&wndr+1);
  //Rect r2 = { wndr.top, wndr.left, wndr.bottom, wndr.right };
  Rect r2 = { wndr.bottom, wndr.left, wndr.top, wndr.right };
  SWELL_hwndCarbonHost *w = [[SWELL_hwndCarbonHost alloc] initCarbonChild:(NSView*)viewpar rect:&r2 composit:wantcomp];
  if (w) *wref = [w->m_cwnd windowRef];
  return (HWND)w;
}

void* SWELL_GetWindowFromCarbonWindowView(HWND cwv)
{
  SWELL_hwndCarbonHost* w = (SWELL_hwndCarbonHost*)cwv;
  if (w) return [w->m_cwnd windowRef];
  return 0;
}

void SWELL_AddCarbonPaneToView(HWND cwv, void* pane)  // not currently used
{
  SWELL_hwndCarbonHost* w = (SWELL_hwndCarbonHost*)cwv;
  if (w)
  {
    WindowRef wndref = (WindowRef)[w->m_cwnd windowRef];
    if (wndref)
    {
      EventTypeSpec ctlevts[] = 
      {
        //{ kEventClassControl, kEventControlInitialize },
        //{ kEventClassControl, kEventControlDraw },            
        { kEventClassControl, kEventControlBoundsChanged },
      };
      int nctlevts = sizeof(ctlevts)/sizeof(EventTypeSpec);  
          
      EventHandlerRef ctlhandler = (EventHandlerRef)w->m_ctlhandler;   
      InstallControlEventHandler((ControlRef)pane, CarbonEvtHandler, nctlevts, ctlevts, w, &ctlhandler);
    }
  }
}


@interface NSButton (TextColor)

- (NSColor *)textColor;
- (void)setTextColor:(NSColor *)textColor;

@end

@implementation NSButton (TextColor)

- (NSColor *)textColor
{
  NSAttributedString *attrTitle = [self attributedTitle];
  int len = [attrTitle length];
  NSRange range = NSMakeRange(0, MIN(len, 1)); // take color from first char
  NSDictionary *attrs = [attrTitle fontAttributesInRange:range];
  NSColor *textColor = [NSColor controlTextColor];
  if (attrs) {
    textColor = [attrs objectForKey:NSForegroundColorAttributeName];
  }
  return textColor;
}

- (void)setTextColor:(NSColor *)textColor
{
  NSMutableAttributedString *attrTitle = [[NSMutableAttributedString alloc] 
                                          initWithAttributedString:[self attributedTitle]];
  int len = [attrTitle length];
  NSRange range = NSMakeRange(0, len);
  [attrTitle addAttribute:NSForegroundColorAttributeName 
                    value:textColor 
                    range:range];
  [attrTitle fixAttributesInRange:range];
  [self setAttributedTitle:attrTitle];
  [attrTitle release];
}

@end







#endif
