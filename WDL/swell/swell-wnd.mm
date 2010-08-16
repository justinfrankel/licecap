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


#ifndef SWELL_PROVIDED_BY_APP

#import <Cocoa/Cocoa.h>
#include "swell.h"
#include "../mutex.h"
#include "../ptrlist.h"
#include "../queue.h"

#include "swell-dlggen.h"
#include "swell-gdi-int.h"

void *SWELL_CStringToCFString(const char *str)
{
  if (!str) str="";
  void *ret;
  
 // ret=(void *)CFStringCreateWithCString(NULL,str,kCFStringEncodingUTF8);
//  if (ret) return ret;
  ret=(void*)CFStringCreateWithCString(NULL,str,kCFStringEncodingASCII);
  return ret;
}





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


@interface SWELL_TabView : NSTabView
{
  int m_tag;
  id m_dest;
}
@end
@implementation SWELL_TabView
-(void)setNotificationWindow:(id)dest
{
  m_dest=dest;
}
-(id)getNotificationWindow
{
  return m_dest;
}
-(int) tag
{
  return m_tag;
}
-(void) setTag:(int)tag
{
  m_tag=tag;
}
- (void)tabView:(NSTabView *)tabView didSelectTabViewItem:(NSTabViewItem *)tabViewItem
{
  if (m_dest)
  {
    NMHDR nm={(HWND)self,[self tag],TCN_SELCHANGE};
    SendMessage((HWND)m_dest,WM_NOTIFY,[self tag],(LPARAM)&nm);
  }
}
@end

@interface SWELL_TaggedProgressIndicator : NSProgressIndicator
{
  int m_tag;
}
@end
@implementation SWELL_TaggedProgressIndicator
-(int) tag
{
  return m_tag;
}
-(void) setTag:(int)tag
{
  m_tag=tag;
}
-(int)onSwellMessage:(UINT)msg p1:(WPARAM)wParam p2:(LPARAM)lParam
{
  if (msg == PBM_SETRANGE)
  {
    [self setMinValue:LOWORD(lParam)];
    [self setMaxValue:HIWORD(lParam)];
  }
  else if (msg==PBM_SETPOS)
  {
    [self setDoubleValue:(double)wParam];
  }
  else if (msg==PBM_DELTAPOS)
  {
    [self incrementBy:(double)wParam];
  }
  return 0;
}

@end

@interface SWELL_StatusCell : NSCell
{
  NSImage *status;
}
@end
@implementation SWELL_StatusCell
-(id)initNewCell
{
  self=[super initTextCell:@""];
  status=0;
  return self;
}
-(void)setStatusImage:(NSImage *)img
{
  status=img;
}
- (void)drawWithFrame:(NSRect)cellFrame inView:(NSView *)controlView
{
  if (status)
  {
    [controlView lockFocus];
    [status drawInRect:NSMakeRect(cellFrame.origin.x,cellFrame.origin.y,cellFrame.size.height,cellFrame.size.height) fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0];
    [controlView unlockFocus];
  }
  cellFrame.origin.x += cellFrame.size.height + 2.0;
  cellFrame.size.width -= cellFrame.size.height + 2.0;
  [super drawWithFrame:cellFrame inView:controlView];
}
@end

class SWELL_TableView_Row
{
public:
  SWELL_TableView_Row()
  {
    m_imageidx=0;
    m_param=0;
  }
  ~SWELL_TableView_Row()
  {
    m_vals.Empty(true,free);
  }
  WDL_PtrList<char> m_vals;
  int m_param;
  int m_imageidx;
};

@interface SWELL_TableViewWithData : NSTableView
{
  int m_leftmousemovecnt;
  bool m_fakerightmouse;
  @public
  int style;
  int ownermode_cnt;
  int m_start_item;
  int m_start_item_clickmode;
  WDL_PtrList<SWELL_TableView_Row> *m_items;
  WDL_PtrList<NSTableColumn> *m_cols;
  WDL_PtrList<void> *m_status_imagelist;
}
@end
@implementation SWELL_TableViewWithData
-(id) init
{
  id ret=[super init];
  ownermode_cnt=0;
  m_status_imagelist=0;
  m_leftmousemovecnt=0;
  m_fakerightmouse=false;
  m_start_item=-1;
  m_start_item_clickmode=0;
  m_cols = new WDL_PtrList<NSTableColumn>;
  m_items=new WDL_PtrList<SWELL_TableView_Row>;
  return ret;
}
-(void) dealloc
{
  if (m_items) m_items->Empty(true);
  delete m_items;
  delete m_cols;
  m_cols=0;
  m_items=0;
  [super dealloc];
}

- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
  return (style & LVS_OWNERDATA) ? ownermode_cnt : (m_items ? m_items->GetSize():0);
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex
{
  NSString *str=NULL;
  int image_idx=0;
  
  if (style & LVS_OWNERDATA)
  {
    HWND tgt=(HWND)[self target];

    char buf[1024];
    NMLVDISPINFO nm={{(HWND)self, [self tag], LVN_GETDISPINFO}};
    nm.item.mask=LVIF_TEXT|LVIF_STATE;
    nm.item.iItem=rowIndex;
    nm.item.iSubItem=m_cols->Find(aTableColumn);
    nm.item.pszText=buf;
    nm.item.cchTextMax=sizeof(buf)-1;
    SendMessage(tgt,WM_NOTIFY,[self tag],(LPARAM)&nm);
    image_idx=(nm.item.state>>16)&0xff;
    
    str=(NSString *)SWELL_CStringToCFString(nm.item.pszText); 
  }
  else
  {
    char *p=NULL;
    SWELL_TableView_Row *r=0;
    if (m_items && m_cols && (r=m_items->Get(rowIndex))) 
    {
      p=r->m_vals.Get(m_cols->Find(aTableColumn));
      image_idx=r->m_imageidx;
    }
    
    str=(NSString *)SWELL_CStringToCFString(p);    
  
  }
  
  if (m_status_imagelist)
  {
    NSCell *cell=[aTableColumn dataCell];
    if ([cell isKindOfClass:[SWELL_StatusCell class]])
    {
      HICON icon=m_status_imagelist->Get(image_idx-1);      
      NSImage *img=NULL;
      if (icon)  img=(NSImage *)GetNSImageFromHICON(icon);
      [cell setStatusImage:img];
    }
  }
  
  return [str autorelease];

}
-(void)mouseDown:(NSEvent *)theEvent
{
  if (([theEvent modifierFlags] & NSControlKeyMask))
  {
    m_fakerightmouse=1;  
    m_start_item=-1;
  }
  else 
  {
    
    NMCLICK nmlv={{(HWND)self,[self tag], NM_CLICK},};
    SendMessage((HWND)[self target],WM_NOTIFY,[self tag],(LPARAM)&nmlv);
    
    m_leftmousemovecnt=0;
    m_fakerightmouse=0;
    
    NSPoint pt=[theEvent locationInWindow];
    pt=[self convertPoint:pt fromView:nil];
    m_start_item=[self rowAtPoint:pt];
    m_start_item_clickmode=0;
    
    if (m_start_item>=0 && m_status_imagelist && pt.x <= [self rowHeight]) // in left area
    {
      m_start_item_clickmode=1;
    }
    
    // 
  }
}

-(void)mouseDragged:(NSEvent *)theEvent
{
  if (++m_leftmousemovecnt==4)
  {
    if (m_start_item>=0&&!m_start_item_clickmode)
    {
      NMLISTVIEW hdr={{(HWND)self,[self tag],LVN_BEGINDRAG},m_start_item,};
      SendMessage((HWND)[self target],WM_NOTIFY,[self tag], (LPARAM) &hdr);
    }
  }
  else if (m_leftmousemovecnt>4&&!m_start_item_clickmode)
  {
    HWND tgt=(HWND)[self target];
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(tgt,&p);
    
    SendMessage(tgt,WM_MOUSEMOVE,0,(int)p.x + (((int)p.y)<<16));
  }
}

-(void)mouseUp:(NSEvent *)theEvent
{
  if (m_fakerightmouse||([theEvent modifierFlags] & NSControlKeyMask))
    [self rightMouseUp:theEvent];
  else if (!m_start_item_clickmode)
  {
    if (m_leftmousemovecnt>=0 && m_leftmousemovecnt<4)
    {
      [super mouseDown:theEvent];
      [super mouseUp:theEvent];
    }
    else if (m_leftmousemovecnt>=4)
    {
      HWND tgt=(HWND)[self target];
      POINT p;
      GetCursorPos(&p);
      ScreenToClient(tgt,&p);
      
      SendMessage(tgt,WM_LBUTTONUP,0,(int)p.x + (((int)p.y)<<16));
      
    }
  }
}
- (void)rightMouseUp:(NSEvent *)theEvent
{
  NMCLICK nmlv={{(HWND)self,[self tag], NM_RCLICK},};
  SendMessage((HWND)[self target],WM_NOTIFY,[self tag],(LPARAM)&nmlv);
  m_fakerightmouse=0;
}

-(int)onSwellMessage:(UINT)msg p1:(WPARAM)wParam p2:(LPARAM)lParam
{
  HWND hwnd=(HWND)self;
    switch (msg)
    {
      case LB_RESETCONTENT:
          ownermode_cnt=0;
          if (m_items) 
          {
            m_items->Empty(true);
            [self reloadData];
          }
      return 0;
      case LB_ADDSTRING:
      case LB_INSERTSTRING:
      {
        int cnt=ListView_GetItemCount(hwnd);
        if (msg==LB_ADDSTRING) wParam=cnt;
        else if (wParam < 0) wParam=0;
        else if (wParam > cnt) wParam=cnt;
        LVITEM lvi={LVIF_TEXT,wParam,0,0,0,(char *)lParam};
        ListView_InsertItem(hwnd,&lvi);
      }
      return wParam;
      case LB_GETCOUNT: return ListView_GetItemCount(hwnd);
      case LB_SETSEL:
        ListView_SetItemState(hwnd, lParam,wParam ? LVIS_SELECTED : 0,LVIS_SELECTED);
        return 0;
      case LB_GETSEL:
        return !!(ListView_GetItemState(hwnd,wParam,LVIS_SELECTED)&LVIS_SELECTED);
      case LB_GETCURSEL:
        return [self selectedRow];
      case LB_SETCURSEL:
      {
        if (lParam>=0&&lParam<ListView_GetItemCount(hwnd))
        {
          [self selectRow:lParam byExtendingSelection:NO];        
        }
        else
        {
          [self deselectAll:self];
        }
      }
        return 0;
      case LB_GETITEMDATA:
      {
        if (m_items)
        {
          SWELL_TableView_Row *row=m_items->Get(wParam);
          if (row) return row->m_param;
        }
      }
        return 0;
      case LB_SETITEMDATA:
      {
        if (m_items)
        {
          SWELL_TableView_Row *row=m_items->Get(wParam);
          if (row) row->m_param=lParam;
        }
      }
        return 0;
      case LB_GETSELCOUNT:
        return [[self selectedRowIndexes] count];
    }
  return 0;
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


int SetWindowLong(HWND hwnd, int idx, int val)
{
  if (!hwnd) return 0;
  id pid=(id)hwnd;
  
  if (idx==GWL_USERDATA && [pid respondsToSelector:@selector(setSwellUserData:)])
  {
    int ret=(int)[pid getSwellUserData];
    [pid setSwellUserData:(int)val];
    return ret;
  }
  if (idx==GWL_USERDATA && [pid isKindOfClass:[NSView class]])
  {
    id wnd=[pid window];
    if (wnd && [wnd contentView]==pid && [wnd respondsToSelector:@selector(setSwellUserData:)])
    {
      int ret=(int)[wnd getSwellUserData];
      [wnd setSwellUserData:(int)val];
      return ret;
    }
  }

  
  if (idx==GWL_ID && [pid respondsToSelector:@selector(tag)] && [pid respondsToSelector:@selector(setTag:)])
  {
    int ret=[pid tag];
    [pid setTag:val];
    return ret;
  }
  
  if (idx==GWL_WNDPROC && [pid respondsToSelector:@selector(setSwellWindowProc:)])
  {
    int ov=(int)[pid getSwellWindowProc];
    [pid setSwellWindowProc:(int)val];
    return ov;
  }
  if (idx==DWL_DLGPROC && [pid respondsToSelector:@selector(setSwellDialogProc:)])
  {
    int ov=(int)[pid getSwellDialogProc];
    [pid setSwellDialogProc:(int)val];
    return ov;
  }
  
  if (idx==GWL_STYLE)
  {
    if ([pid isKindOfClass:[NSButton class]]) 
    {
      int ret=GetWindowLong(hwnd,idx);
      
      if (val & BS_AUTO3STATE)
      {
        [pid setButtonType:NSSwitchButton];
        [pid setAllowsMixedState:YES];
      }    
      else if (val & BS_AUTOCHECKBOX)
      {
        [pid setButtonType:NSSwitchButton];
        [pid setAllowsMixedState:NO];
      }
      else if (val & BS_AUTORADIOBUTTON)
      {
        [pid setButtonType:NSRadioButton];
      }               
      
      return ret;
    }
    return 0;
  }

  
  if ([pid respondsToSelector:@selector(setSwellExtraData:value:)])
  {
    int ov=0;
    if ([pid respondsToSelector:@selector(getSwellExtraData:)]) ov=(int)[pid getSwellExtraData:idx];

    [pid setSwellExtraData:idx value:val];
    
    return ov;
  }
   
  return 0;
}

int GetWindowLong(HWND hwnd, int idx)
{
  if (!hwnd) return 0;
  id pid=(id)hwnd;
  
  
  if (idx==GWL_USERDATA && [pid respondsToSelector:@selector(getSwellUserData)])
  {
    return (int)[pid getSwellUserData];
  }
  if (idx==GWL_USERDATA && [pid isKindOfClass:[NSView class]])
  {
    id wnd=[pid window];
    if (wnd && [wnd contentView]==pid && [wnd respondsToSelector:@selector(getSwellUserData)])
    {
      return (int)[wnd getSwellUserData];
    }
  }

  if (idx==GWL_ID && [pid respondsToSelector:@selector(tag)])
    return [pid tag];
  
  
  if (idx==GWL_WNDPROC && [pid respondsToSelector:@selector(getSwellWindowProc)])
  {
    return (int)[pid getSwellWindowProc];
  }
  if (idx==DWL_DLGPROC && [pid respondsToSelector:@selector(getSwellDialogProc)])
  {
    return (int)[pid getSwellDialogProc];
  }  
  if (idx==GWL_STYLE)
  {
    if ([pid isKindOfClass:[NSButton class]]) 
    {
      int ret=0;
      if ([pid allowsMixedState]) ret |= BS_AUTO3STATE|BS_AUTOCHECKBOX;
      else ret |= BS_AUTOCHECKBOX; // todo: support querying radio buttons
      return ret;
    }
    return 0;
  }
  if ([pid respondsToSelector:@selector(getSwellExtraData:)])
  {
    return (int)[pid getSwellExtraData:idx];
  }
  
  return 0;
}

bool IsWindowVisible(HWND hwnd)
{
  if (!hwnd) return false;

  id turd=(id)hwnd;
  if ([turd isKindOfClass:[NSView class]])
  {
    return ![turd isHiddenOrHasHiddenAncestor];
  }
  if ([turd isKindOfClass:[NSWindow class]])
  {
    return !![turd isVisible];
  }
  return true;
}

static void *__GetNSImageFromHICON(HICON ico) // local copy to not be link dependent on swell-gdi.mm
{
  GDP_OBJECT *i = (GDP_OBJECT *)ico;
  if (!i || i->type != TYPE_BITMAP) return 0;
  return i->bitmapptr;
}


@interface Swell_Button : NSButton
{
  LPARAM m_swellGDIimage;
}
@end
@implementation Swell_Button : NSButton

-(id) init {
  self = [super init];
  if (self != nil) {
    m_swellGDIimage=0;
  }
  return self;
}

-(void)setSwellGDIImage:(LPARAM)par
{
  m_swellGDIimage=par;
}
-(LPARAM)getSwellGDIImage
{
  return m_swellGDIimage;
}

@end

int SendMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (!hwnd) return 0;
  id turd=(id)hwnd;
  if ([turd respondsToSelector:@selector(onSwellMessage:p1:p2:)])
  {
    return (int) [turd onSwellMessage:msg p1:wParam p2:lParam];
  }
  else 
  {
    if ((msg==BM_GETIMAGE || msg == BM_SETIMAGE) && [turd isKindOfClass:[Swell_Button class]])
    {
      if (wParam != IMAGE_BITMAP && wParam != IMAGE_ICON) return 0; // ignore unknown types
      int ret=(int)[turd getSwellGDIImage];
      if (msg==BM_SETIMAGE)
      {
        NSImage *img=NULL;
        if (lParam) img=(NSImage *)__GetNSImageFromHICON((HICON)lParam);
        [turd setImage:img];
        [turd setSwellGDIImage:img?lParam:0];
      }
      return ret;
    }
    else if (msg >= CB_ADDSTRING && msg <= CB_INITSTORAGE && ([turd isKindOfClass:[NSPopUpButton class]] || [turd isKindOfClass:[NSComboBox class]]))
    {
        switch (msg)
        {
          case CB_ADDSTRING: return SWELL_CB_AddString(hwnd,0,(char*)lParam); 
          case CB_DELETESTRING: SWELL_CB_DeleteString(hwnd,0,wParam); return 1;
          case CB_GETCOUNT: return SWELL_CB_GetNumItems(hwnd,0);
          case CB_GETCURSEL: return SWELL_CB_GetCurSel(hwnd,0);
          case CB_GETLBTEXT: return SWELL_CB_GetItemText(hwnd,0,wParam,(char *)lParam, 1024);  
          case CB_INSERTSTRING: return SWELL_CB_InsertString(hwnd,0,wParam,(char *)lParam);
          case CB_RESETCONTENT: SWELL_CB_Empty(hwnd,0); return 0;
          case CB_SETCURSEL: SWELL_CB_SetCurSel(hwnd,0,wParam); return 0;
          case CB_GETITEMDATA: return SWELL_CB_GetItemData(hwnd,0,wParam);
          case CB_SETITEMDATA: SWELL_CB_SetItemData(hwnd,0,wParam,lParam);
          case CB_INITSTORAGE: return 0;                                                      
        }
        return 0;
    }
    else if (msg >= TBM_GETPOS && msg <= TBM_SETRANGE && ([turd isKindOfClass:[NSSlider class]]))
    {
        switch (msg)
        {
          case TBM_GETPOS: return SWELL_TB_GetPos(hwnd,0);
          case TBM_SETTIC: SWELL_TB_SetTic(hwnd,0,lParam); return 1;
          case TBM_SETPOS: SWELL_TB_SetPos(hwnd,0,lParam); return 1;
          case TBM_SETRANGE: SWELL_TB_SetRange(hwnd,0,LOWORD(lParam),HIWORD(lParam)); return 1;
        }
        return 0;
    }
    else
    {
      NSWindow *w;
      NSView *v;
      if ([turd isKindOfClass:[NSView class]] && (w=[turd window]) && [w respondsToSelector:@selector(onSwellMessage:p1:p2:)])
      {
        return (int) [w onSwellMessage:msg p1:wParam p2:lParam];
      }
      else if ([turd isKindOfClass:[NSWindow class]] && (v=[turd contentView]) && [v respondsToSelector:@selector(onSwellMessage:p1:p2:)])
      {
        return (int) [v onSwellMessage:msg p1:wParam p2:lParam];
      }
    }
  }
  return 0;
}

void DestroyWindow(HWND hwnd)
{
  if (!hwnd) return;
  id pid=(id)hwnd;
  if ([pid isKindOfClass:[NSWindow class]])
  {
    KillTimer(hwnd,-1);
    if ([(id)pid respondsToSelector:@selector(onSwellMessage:p1:p2:)])
      [(id)pid onSwellMessage:WM_DESTROY p1:0 p2:0];
    NSWindow *par=[(NSWindow*)pid parentWindow];
    if (par)
    {
      [par removeChildWindow:(NSWindow*)pid];
    }
    [(NSWindow *)pid close]; // this is probably bad, but close takes too long to close!
  }
  else if ([pid isKindOfClass:[NSView class]])
  {
    KillTimer(hwnd,-1);
    if ([(id)pid respondsToSelector:@selector(onSwellMessage:p1:p2:)])
      [(id)pid onSwellMessage:WM_DESTROY p1:0 p2:0];
    [(NSView *)pid removeFromSuperview];
  }
}

void EnableWindow(HWND hwnd, int enable)
{
  if (!hwnd) return;
  id bla=(id)hwnd;
  if ([bla respondsToSelector:@selector(setEnabled:)]) [bla setEnabled:(enable?YES:NO)];
}

void SetForegroundWindow(HWND hwnd)
{
  SetFocus(hwnd);
}

void SetFocus(HWND hwnd) // these take NSWindow/NSView, and return NSView *
{
  id r=(id) hwnd;
  if (!r) return;
  
  if ([r isKindOfClass:[NSWindow class]])
  {
    [(NSWindow *)r makeKeyAndOrderFront:nil];
    [(NSWindow *)r makeFirstResponder:[(NSWindow *)r contentView]]; 
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

void SWELL_GetViewPort(RECT *r, RECT *sourcerect, bool wantWork)
{
  NSArray *ar=[NSScreen screens];
  
  int cnt=[ar count];
  int x;
  int cx=0;
  int cy=0;
  if (sourcerect)
  {
    cx=(sourcerect->left+sourcerect->right)/2;
    cy=(sourcerect->top+sourcerect->bottom)/2;
  }
  for (x = 0; x < cnt; x ++)
  {
    NSScreen *sc=[ar objectAtIndex:x];
    if (sc)
    {
      NSRect tr=wantWork ? [sc visibleFrame] : [sc frame];
      if (!x || (cx >= tr.origin.x && cx < tr.origin.x+tr.size.width  &&
                cy >= tr.origin.y && cy < tr.origin.y+tr.size.height))
      {
        r->left=(int)tr.origin.x;
        r->right=(int)(tr.origin.x+tr.size.width+0.5);
        r->top=(int)tr.origin.y;
        r->bottom=(int)(tr.origin.y+tr.size.height+0.5);
      }
    }
  }
  if (!cnt)
  {
    r->left=r->top=0;
    r->right=1600;
    r->bottom=1200;
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

static NSView *NavigateUpScrollClipViews(NSView *ch)
{
  NSView *par=[ch superview];
  if (par && [par isKindOfClass:[NSClipView class]]) 
  {
    par=[par superview];
    if (par && [par isKindOfClass:[NSScrollView class]])
    {
      ch=par;
    }
  }
  return ch;
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
  ch=NavigateUpScrollClipViews(ch);
  NSRect b=[ch bounds];
  r->left=(int)(b.origin.x);
  r->top=(int)(b.origin.y);
  r->right = (int)(b.origin.x+b.size.width+0.5);
  r->bottom= (int)(b.origin.y+b.size.height+0.5);
  ClientToScreen(hwnd,(POINT *)r);
  ClientToScreen(hwnd,((POINT *)r)+1);

}

void GetClientRect(HWND hwnd, RECT *r)
{
  r->left=r->top=r->right=r->bottom=0;
  if (!hwnd) return;
  
  id ch=(id)hwnd;
  if ([ch isKindOfClass:[NSWindow class]]) ch=[((NSWindow *)ch) contentView];
  if (!ch || ![ch isKindOfClass:[NSView class]]) return;
  ch=NavigateUpScrollClipViews(ch);
  
  NSRect b=[ch bounds];
  r->left=(int)(b.origin.x);
  r->top=(int)(b.origin.y);
  r->right = (int)(b.origin.x+b.size.width+0.5);
  r->bottom= (int)(b.origin.y+b.size.height+0.5);
}

BOOL WinOffsetRect(LPRECT lprc, int dx, int dy)
{
  if(!lprc) return 0;
  lprc->left+=dx;
  lprc->top+=dy;
  lprc->right+=dx;
  lprc->bottom+=dy;
  return TRUE;
}

BOOL WinSetRect(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom)
{
  if(!lprc) return 0;
  lprc->left = xLeft;
  lprc->top = yTop;
  lprc->right = xRight;
  lprc->bottom = yBottom;
}

void SetWindowPos(HWND hwnd, HWND unused, int x, int y, int cx, int cy, int flags)
{
  if (!hwnd) return;
 
 
  id ch=(id)hwnd;
  bool isview=false;
  if ([ch isKindOfClass:[NSWindow class]] || (isview=[ch isKindOfClass:[NSView class]])) 
  {
    if (isview)
    {
      ch=NavigateUpScrollClipViews(ch);
    }    
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
      if (f.size.height<0)f.size.height=-f.size.height;
      repos=true;
    }
    if (repos)
    {
      if (!isview)
        [ch setFrame:f display:YES];
      else
      {
        // this doesnt seem to actually be a good idea anymore
  //      if ([[ch window] contentView] != ch && ![[ch superview] isFlipped])
//          f.origin.y -= f.size.height;
        [ch setFrame:f];
      }
    }    
    return;
  }  
  
}


HWND GetWindow(HWND hwnd, int what)
{
  if (!hwnd) return 0;
  if ([(id)hwnd isKindOfClass:[NSWindow class]]) hwnd=(HWND)[(id)hwnd contentView];
  if (!hwnd || ![(id)hwnd isKindOfClass:[NSView class]]) return 0;
  
  NSView *v=(NSView *)hwnd;
  if (what == GW_CHILD)
  {
    NSArray *ar=[v subviews];
    if (ar && [ar count]>0)
    {
      return (HWND)[ar objectAtIndex:0];
    }
    return 0;
  }
  if (what == GW_OWNER)
  {
    v=NavigateUpScrollClipViews(v);
    if ([[v window] contentView] == v)
    {
      if ([[v window] respondsToSelector:@selector(swellGetOwner)])
      {
        return (HWND)[[v window] swellGetOwner];
      }
      return 0;
    }
    return (HWND)[v superView];
  }
  
  if (what >= GW_HWNDFIRST && what <= GW_HWNDPREV)
  {
    v=NavigateUpScrollClipViews(v);
    if ([[v window] contentView] == v)
    {
      if (what <= GW_HWNDLAST) return (HWND)hwnd; // content view is only window
      
      return 0; // we're the content view so cant do next/prev
    }
    NSView *par=[v superview];
    if (par)
    {
      NSArray *ar=[par subviews];
      int cnt;
      if (ar && (cnt=[ar count]) > 0)
      {
        if (what == GW_HWNDFIRST)
          return (HWND)[ar objectAtIndex:0];
        if (what == GW_HWNDLAST)
          return (HWND)[ar objectAtIndex:(cnt-1)];
        
        int idx=[ar indexOfObjectIdenticalTo:v];
        if (idx == NSNotFound) return 0;

        if (what==GW_HWNDNEXT) idx++;
        else if (what==GW_HWNDPREV) idx--;
        
        if (idx<0 || idx>=cnt) return 0;
        
        return (HWND)[ar objectAtIndex:idx];
      }
    }
    return 0;
  }
  return 0;
}


HWND GetParent(HWND hwnd)
{
  if (hwnd && [(id)hwnd isKindOfClass:[NSView class]])
  {
    hwnd=(HWND)NavigateUpScrollClipViews((NSView *)hwnd);

    NSView *cv=[[(NSView *)hwnd window] contentView];
    if (cv == (NSView *)hwnd) hwnd=(HWND)[(NSView *)hwnd window]; // passthrough to get window parent
    else
    {
      HWND h=(HWND)[(NSView *)hwnd superview];
      if (h==(HWND)cv) return (HWND)[(NSView *)hwnd window];
      return h;
    }
  }
  
  if (hwnd && [(id)hwnd isKindOfClass:[NSWindow class]]) 
  {
    HWND h= (HWND)[(NSWindow *)hwnd parentWindow];
    if (h) return h;
  }
  
  if (hwnd && [(id)hwnd respondsToSelector:@selector(swellGetOwner)])
  {
    return (HWND)[(id)hwnd swellGetOwner];
  }
  
  return 0;
}

HWND SetParent(HWND hwnd, HWND newPar)
{
  NSView *v=(NSView *)hwnd;
  if (!v || ![(id)v isKindOfClass:[NSView class]]) return 0;
  v=NavigateUpScrollClipViews(v);
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

HWND GetForegroundWindow()
{
  NSWindow *window=[NSApp keyWindow];
  if (!window) return 0;
  id ret=[window firstResponder];
  if (ret && [ret isKindOfClass:[NSView class]]) 
  {
    if (ret == [window contentView]) return (HWND) window;
    return (HWND) ret;
  }
  return (HWND)window;
}

HWND GetFocus()
{
  NSWindow *window=[NSApp keyWindow];
  if (!window) return 0;
  id ret=[window firstResponder];
  if (ret && [ret isKindOfClass:[NSView class]]) 
  {
    if (ret == [window contentView]) return (HWND) window;
    return (HWND) ret;
  }
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
int SetTimer(HWND hwnd, int timerid, int rate, unsigned long *notUsed)
{
  if (!hwnd) return 0;
  WDL_MutexLock lock(&m_timermutex);
  KillTimer(hwnd,timerid);
  TimerInfoRec *rec=(TimerInfoRec*)malloc(sizeof(TimerInfoRec));
  rec->timerid=timerid;
  rec->hwnd=hwnd;
  
  simpleDataHold *t=[[simpleDataHold alloc] initWithVal:timerid];
  rec->timer = [NSTimer scheduledTimerWithTimeInterval:(max(rate,1)*0.001) target:(id)hwnd selector:@selector(SWELL_Timer:) 
                                              userInfo:t repeats:YES];
  
  [[NSRunLoop currentRunLoop] addTimer:rec->timer forMode:(NSString*)kCFRunLoopCommonModes];
  [t release];
  m_timerlist.Add(rec);
  
  return timerid;
}
void KillTimer(HWND hwnd, int timerid)
{
  WDL_MutexLock lock(&m_timermutex);
  int x;
  for (x = 0; x < m_timerlist.GetSize(); x ++)
  {
    if ((timerid==-1 || m_timerlist.Get(x)->timerid == timerid) && m_timerlist.Get(x)->hwnd == hwnd)
    {
      [m_timerlist.Get(x)->timer invalidate];
      m_timerlist.Delete(x--,true,free);
      if (timerid!=-1) break;
    }
  }
}



void SetDlgItemText(HWND hwnd, int idx, const char *text)
{
  NSView *poo=(NSView *)(idx ? GetDlgItem(hwnd,idx) : hwnd);
  if (!poo) return;
  NSString *lbl=(NSString *)SWELL_CStringToCFString(text);
  if ([poo isKindOfClass:[NSWindow class]] || [poo isKindOfClass:[NSButton class]]) [(NSButton*)poo setTitle:lbl];
  else if ([poo isKindOfClass:[NSControl class]]) 
  {
    [(NSControl*)poo setStringValue:lbl];
/*    NSView *parv=[(NSControl*)poo superview];
    if (parv && [parv isKindOfClass:[NSClipView class]] &&
        [[parv superview] isKindOfClass:[NSScrollView class]])
          [(NSControl *)poo sizeToFit];
    */
  }
  
  [lbl release];
}

void GetDlgItemText(HWND hwnd, int idx, char *text, int textlen)
{
  *text=0;
  NSView *poo=(NSView *)(idx?GetDlgItem(hwnd,idx) : hwnd);
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
  if ([poo isKindOfClass:[NSButton class]]) [(NSButton*)poo setState:(check&BST_INDETERMINATE)?NSMixedState:((check&BST_CHECKED)?NSOnState:NSOffState)];
}


int IsDlgButtonChecked(HWND hwnd, int idx)
{
  NSView *poo=(NSView *)GetDlgItem(hwnd,idx);
  if (poo && [poo isKindOfClass:[NSButton class]])
  {
    int a=[(NSButton*)poo state];
    if (a==NSMixedState) return BST_INDETERMINATE;
    return a!=NSOffState;
  }
  return 0;
}

void SWELL_TB_SetPos(HWND hwnd, int idx, int pos)
{
  NSSlider *p=(NSSlider *)GetDlgItem(hwnd,idx);
  if (p  && [p isKindOfClass:[NSSlider class]]) 
  {
    [p setDoubleValue:(double)pos];
  }
  else if (p && [p respondsToSelector:@selector(onSwellMessage:p1:p2:)])
  {
    [p onSwellMessage:TBM_SETPOS p1:1 p2:pos];
  }
}

void SWELL_TB_SetRange(HWND hwnd, int idx, int low, int hi)
{
  NSSlider *p=(NSSlider *)GetDlgItem(hwnd,idx);
  if (p && [p isKindOfClass:[NSSlider class]])
  {
    [p setMinValue:low];
    [p setMaxValue:hi];
  }
  else if (p && [p respondsToSelector:@selector(onSwellMessage:p1:p2:)])
  {
    [p onSwellMessage:TBM_SETRANGE p1:1 p2:(low|(hi<<16))];
  }
  
}

int SWELL_TB_GetPos(HWND hwnd, int idx)
{
  NSSlider *p=(NSSlider *)GetDlgItem(hwnd,idx);
  if (p && [p isKindOfClass:[NSSlider class]]) 
  {
    return (int) ([p doubleValue]+0.5);
  }
  else if (p && [p respondsToSelector:@selector(onSwellMessage:p1:p2:)])
  {
    return (int) [p onSwellMessage:TBM_GETPOS p1:0 p2:0];
  }
  return 0;
}

void SWELL_TB_SetTic(HWND hwnd, int idx, int pos)
{
  NSSlider *p=(NSSlider *)GetDlgItem(hwnd,idx);
  if (p && [p respondsToSelector:@selector(onSwellMessage:p1:p2:)])
  {
    [p onSwellMessage:TBM_SETTIC p1:0 p2:pos];
  }
}

void SWELL_CB_DeleteString(HWND hwnd, int idx, int wh)
{
  NSComboBox *p=(NSComboBox *)GetDlgItem(hwnd,idx);
  if (!p) return;
  if (idx>=0 && idx<SWELL_CB_GetNumItems(hwnd,idx))
  {
    if ([p isKindOfClass:[NSComboBox class]] || [p isKindOfClass:[NSPopUpButton class]])
        [p removeItemAtIndex:idx];
  }
}


int SWELL_CB_GetItemText(HWND hwnd, int idx, int item, char *buf, int bufsz)
{
  NSComboBox *p=(NSComboBox *)GetDlgItem(hwnd,idx);

  *buf=0;
  if (!p) return 0;
  int ni=[p numberOfItems];
  if (item < 0 || item >= ni) return 0;
  
  if ([p isKindOfClass:[NSComboBox class]])
  {
    NSString *s=[p itemObjectValueAtIndex:item];
    if (s)
    {
      [s getCString:buf maxLength:bufsz];
      return 1;
    }
  }
  else 
  {
    NSMenuItem *i=[p itemAtIndex:item];
    if (i)
    {
      NSString *s=[i title];
      if (s)
      {
        [s getCString:buf maxLength:bufsz];
        return 1;
      }
    }
  }
  return 0;
}

int SWELL_CB_InsertString(HWND hwnd, int idx, int pos, const char *str)
{
  NSString *label=(NSString *)SWELL_CStringToCFString(str);
  NSComboBox *p=(NSComboBox *)GetDlgItem(hwnd,idx);
  if (!p) return 0;
  
  int ni=[p numberOfItems];
  if (pos == -1000) pos=ni;
  else if (pos < 0) pos=0;
  else if (pos > ni) pos=ni;
   
  if ([p isKindOfClass:[NSComboBox class]])
  {
    if (pos==ni)
      [p addItemWithObjectValue:label];
    else
      [p insertItemWithObjectValue:label atIndex:pos];
    [p setNumberOfVisibleItems:(ni+1)];
  }
  else
  {
    if (pos==ni)
      [(NSPopUpButton *)p addItemWithTitle:label];
    else
      [(NSPopUpButton *)p insertItemWithTitle:label atIndex:pos];
  }
  [label release];
  return pos;
  
}

int SWELL_CB_AddString(HWND hwnd, int idx, const char *str)
{
  return SWELL_CB_InsertString(hwnd,idx,-1000,str);
}

int SWELL_CB_GetCurSel(HWND hwnd, int idx)
{
  NSComboBox *p=(NSComboBox *)GetDlgItem(hwnd,idx);
  if (!p) return -1;
  return [p indexOfSelectedItem];
}

void SWELL_CB_SetCurSel(HWND hwnd, int idx, int item)
{
  if (item>=0 && item<SWELL_CB_GetNumItems(hwnd,idx))
    [(NSComboBox *)GetDlgItem(hwnd,idx) selectItemAtIndex:item];
}

int SWELL_CB_GetNumItems(HWND hwnd, int idx)
{
  NSComboBox *p=(NSComboBox *)GetDlgItem(hwnd,idx);
  if (!p) return 0;
  return [p numberOfItems];
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
  
  if (pid && [pid isKindOfClass:[NSWindow class]])
  {
    if (cmd==SW_SHOW)
    {
      [pid makeKeyAndOrderFront:pid];
    }
    else if (cmd==SW_SHOWNA)
    {
      [pid orderFront:pid];
    }
    else if (cmd==SW_HIDE)
    {
      [pid orderOut:pid];
    }
    return;
  }
  if (!pid || ![pid isKindOfClass:[NSView class]]) return;
  
  pid=NavigateUpScrollClipViews(pid);
  
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
static bool m_sizetofits;

void SWELL_Make_SetMessageMode(int wantParentView)
{
  m_parentMode=wantParentView;
}
#define ACTIONTARGET (m_parentMode ? (id)m_parent : (id)[m_parent window])

void SWELL_MakeSetCurParms(float xscale, float yscale, float xtrans, float ytrans, HWND parent, bool doauto, bool dosizetofit)
{
  m_sizetofits=dosizetofit;
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
  {
    m_parent_h=[(NSView *)parent bounds].size.height;
    if (m_transform.size.height > 0 && [(id)parent isFlipped])
      m_transform.size.height*=-1;
  }
  else if ([(id)parent isKindOfClass:[NSWindow class]])
  {
    m_parent_h=[((NSWindow *)parent) frame].size.height;
    if (m_transform.size.height > 0 && [(id)parent respondsToSelector:@selector(isFlipped)] && [(id)parent isFlipped])
      m_transform.size.height*=-1;
  }
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

static const double minwidfontadjust=1.81;
static const double minwidfontscale=6.0;
/// these are for swell-dlggen.h
HWND SWELL_MakeButton(int def, const char *label, int idx, int x, int y, int w, int h, int flags)
{  
  Swell_Button *button=[[Swell_Button alloc] init];
  
  if (m_transform.size.width < minwidfontadjust)
  {
    [button setFont:[NSFont systemFontOfSize:m_transform.size.width*minwidfontscale]];
  }
  
  [button setTag:idx];
  [button setBezelStyle:NSRoundedBezelStyle ];
  [button setFrame:MakeCoords(x,y,w,h,true)];
  NSString *labelstr=(NSString *)SWELL_CStringToCFString(label);
  [button setTitle:labelstr];
  [button setTarget:ACTIONTARGET];
  [button setAction:@selector(onCommand:)];
  [m_parent addSubview:button];
  if (m_doautoright) UpdateAutoCoords([button frame]);
  if (def) [[m_parent window] setDefaultButtonCell:(NSButtonCell*)button];
  [labelstr release];
  [button release];
  return (HWND) button;
}

HWND SWELL_MakeEditField(int idx, int x, int y, int w, int h, int flags)
{  
  NSTextField *obj=[[NSTextField alloc] init];
  [obj setEditable:(flags & ES_READONLY)?NO:YES];
  //if (flags & ES_WANTRETURN)
//    [obj 
  if (m_transform.size.width < minwidfontadjust)
    [obj setFont:[NSFont systemFontOfSize:m_transform.size.width*minwidfontscale]];
  if (h < 20)
  {
    [[obj cell] setWraps:NO];
    [[obj cell] setScrollable:YES];
  }
  [obj setTag:idx];
  [obj setTarget:ACTIONTARGET];
  [obj setAction:@selector(onCommand:)];
  [obj setDelegate:ACTIONTARGET];
  
  [obj setFrame:MakeCoords(x,y,w,h,true)];
/*  if (flags&WS_VSCROLL)
  {
    NSScrollView *obj2=[[NSScrollView alloc] init];
    [obj2 setFrame:MakeCoords(x,y,w,h,false)];
    [obj2 setDocumentView:obj];
    [obj2 setHasVerticalScroller:YES];
    [obj release];
    [m_parent addSubview:obj2];
    if (m_doautoright) UpdateAutoCoords([obj2 frame]);
    [obj2 release];
    obj=obj2;
  }  
  else */
  {
    [m_parent addSubview:obj];
    if (m_doautoright) UpdateAutoCoords([obj frame]);
    [obj release];
  }
  return (HWND)obj;
}

HWND SWELL_MakeLabel( int align, const char *label, int idx, int x, int y, int w, int h, int flags)
{
  NSTextField *obj=[[NSTextField alloc] init];
  [obj setEditable:NO];
  [obj setSelectable:NO];
  [obj setBordered:NO];
  [obj setBezeled:NO];
  [obj setDrawsBackground:NO];
  if (m_transform.size.width < minwidfontadjust)
    [obj setFont:[NSFont systemFontOfSize:m_transform.size.width*minwidfontscale]];

  if (flags & SS_NOTFIY)
  {
    [obj setTarget:ACTIONTARGET];
    [obj setAction:@selector(onCommand:)];
  }
  
  NSString *labelstr=(NSString *)SWELL_CStringToCFString(label);
  [obj setStringValue:labelstr];
  [obj setAlignment:(align<0?NSLeftTextAlignment:align>0?NSRightTextAlignment:NSCenterTextAlignment)];
  [obj setTag:idx];
  [obj setFrame:MakeCoords(x,y,w,h,true)];
  [m_parent addSubview:obj];
  if (m_sizetofits)[obj sizeToFit];
  if (m_doautoright) UpdateAutoCoords([obj frame]);
  [obj release];
  [labelstr release];
  return (HWND)obj;
}


HWND SWELL_MakeCheckBox(const char *name, int idx, int x, int y, int w, int h)
{
  return SWELL_MakeControl(name,idx,"Button",BS_AUTOCHECKBOX,x,y,w,h);
}

HWND SWELL_MakeListBox(int idx, int x, int y, int w, int h, int styles)
{
  HWND hw=SWELL_MakeControl("",idx,"SysListView32",((styles&LBS_EXTENDEDSEL) ? 0 : LVS_SINGLESEL)|LVS_NOCOLUMNHEADER,x,y,w,h);
/*  if (hw)
  {
    LVCOLUMN lvc={0,};
    RECT r;
    GetClientRect(hw,&r);
    lvc.cx=300;//yer.right-r.left;
    lvc.pszText="";
    ListView_InsertColumn(hw,0,&lvc);
  }
  */
  return hw;
}


typedef struct ccprocrec
{
  SWELL_ControlCreatorProc proc;
  int cnt;
  struct ccprocrec *next;
} ccprocrec;

static ccprocrec *m_ccprocs;

void SWELL_RegisterCustomControlCreator(SWELL_ControlCreatorProc proc)
{
  if (!proc) return;
  
  ccprocrec *p=m_ccprocs;
  while (p && p->next)
  {
    if (p->proc == proc)
    {
      p->cnt++;
      return;
    }
    p=p->next;
  }
  ccprocrec *ent = (ccprocrec*)malloc(sizeof(ccprocrec));
  ent->proc=proc;
  ent->cnt=1;
  ent->next=0;
  
  if (p) p->next=ent;
  else m_ccprocs=ent;
}

void SWELL_UnregisterCustomControlCreator(SWELL_ControlCreatorProc proc)
{
  if (!proc) return;
  
  ccprocrec *lp=NULL;
  ccprocrec *p=m_ccprocs;
  while (p)
  {
    if (p->proc == proc)
    {
      if (--p->cnt <= 0)
      {
        if (lp) lp->next=p->next;
        else m_ccprocs=p->next;
        free(p);
      }
      return;
    }
    lp=p;
    p=p->next;
  }
}


HWND SWELL_MakeControl(const char *cname, int idx, const char *classname, int style, int x, int y, int w, int h)
{
  if (m_ccprocs)
  {
    NSRect poo=MakeCoords(x,y,w,h,false);
    ccprocrec *p=m_ccprocs;
    while (p)
    {
      HWND h=p->proc((HWND)m_parent,cname,idx,classname,style,(int)(poo.origin.x+0.5),(int)(poo.origin.y+0.5),(int)(poo.size.width+0.5),(int)(poo.size.height+0.5));
      if (h) return h;
      p=p->next;
    }
  }
  if (!stricmp(classname,"SysTabControl32"))
  {
    SWELL_TabView *obj=[[SWELL_TabView alloc] init];
    [obj setTag:idx];
    [obj setDelegate:obj];
    [obj setAllowsTruncatedLabels:YES];
    [obj setNotificationWindow:ACTIONTARGET];
    [obj setHidden:NO];
    [obj setFrame:MakeCoords(x,y,w,h,false)];
    [m_parent addSubview:obj];
    [obj release];
    return (HWND)obj;
  }
  else if (!stricmp(classname, "SysListView32"))
  {
    SWELL_TableViewWithData *obj = [[SWELL_TableViewWithData alloc] init];
    [obj setDataSource:obj];
    obj->style=style;
    if ((style & LVS_NOCOLUMNHEADER) || !(style & LVS_REPORT))
      [obj setHeaderView:nil];
    
    [obj setAllowsColumnReordering:NO];
    [obj setAllowsMultipleSelection:!(style & LVS_SINGLESEL)];
    [obj setAllowsEmptySelection:YES];
    [obj setTag:idx];
    [obj setHidden:NO];
    id target=ACTIONTARGET;
    [obj setTarget:target];
    [obj setAction:@selector(onCommand:)];
    if ([target respondsToSelector:@selector(onControlDoubleClick:)])
      [obj setDoubleAction:@selector(onControlDoubleClick:)];
    else
      [obj setDoubleAction:@selector(onCommand:)];
    NSScrollView *obj2=[[NSScrollView alloc] init];
    NSRect tr=MakeCoords(x,y,w,h,false);
    [obj2 setFrame:tr];
    [obj2 setDocumentView:obj];
    [obj2 setHasVerticalScroller:YES];
    [obj release];
    [m_parent addSubview:obj2];
    [obj2 release];
    
    if ( !(style & LVS_REPORT))
    {
      LVCOLUMN lvc={0,};
      lvc.cx=(int)ceil(max(tr.size.width,300.0));
      lvc.pszText="";
      ListView_InsertColumn((HWND)obj,0,&lvc);
    }
    
    return (HWND)obj;
  }
  else if (!stricmp(classname, "msctls_progress32"))
  {
    SWELL_TaggedProgressIndicator *obj=[[SWELL_TaggedProgressIndicator alloc] init];
    [obj setStyle:NSProgressIndicatorBarStyle];
    [obj setIndeterminate:NO];
    [obj setTag:idx];
    [obj setFrame:MakeCoords(x,y,w,h,false)];
    [m_parent addSubview:obj];
    [obj release];
    return (HWND)obj;
  }
  else if (!stricmp(classname,"Edit"))
  {
    return SWELL_MakeEditField(idx,x,y,w,h,style);
  }
  else if (!stricmp(classname, "static"))
  {
    NSTextField *obj=[[NSTextField alloc] init];
    [obj setEditable:NO];
    [obj setSelectable:NO];
    [obj setBordered:NO];
    [obj setBezeled:NO];
    [obj setDrawsBackground:NO];
    
    [obj setTag:idx];
    [obj setFrame:MakeCoords(x,y,w,h,true)];
    [m_parent addSubview:obj];
    if (style & SS_BLACKRECT)
    {
      [obj setHidden:YES];
    }
    [obj release];
    return (HWND)obj;
  }
  else if (!stricmp(classname,"Button"))
  {
    NSButton *button=[[NSButton alloc] init];
    [button setTag:idx];
    if (style & BS_AUTO3STATE)
    {
      [button setButtonType:NSSwitchButton];
      [button setAllowsMixedState:YES];
    }    
    else if (style & BS_AUTOCHECKBOX)
    {
      [button setButtonType:NSSwitchButton];
      [button setAllowsMixedState:NO];
    }
    else if (style & BS_AUTORADIOBUTTON)
    {
      [button setButtonType:NSRadioButton];
    }
    if (m_transform.size.width < minwidfontadjust)
      [button setFont:[NSFont systemFontOfSize:m_transform.size.width*minwidfontscale]];
    [button setFrame:MakeCoords(x,y,w,h,true)];
    NSString *labelstr=(NSString *)SWELL_CStringToCFString(cname);
    [button setTitle:labelstr];
    [button setTarget:ACTIONTARGET];
    [button setAction:@selector(onCommand:)];
    [m_parent addSubview:button];
    if (m_sizetofits) [button sizeToFit];
    if (m_doautoright) UpdateAutoCoords([button frame]);
    [labelstr release];
    [button release];
    return (HWND)button;
  }
  else if (!stricmp(classname,"REAPERhfader")||!stricmp(classname,"msctls_trackbar32"))
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
    return (HWND)obj;
  }
  return 0;
}

HWND SWELL_MakeCombo(int idx, int x, int y, int w, int h, int flags)
{
  if (flags & CBS_DROPDOWNLIST)
  {
    NSPopUpButton *obj=[[NSPopUpButton alloc] init];
    [obj setTag:idx];
    if (m_transform.size.width < minwidfontadjust)
      [obj setFont:[NSFont systemFontOfSize:m_transform.size.width*minwidfontscale]];
    [obj setFrame:MakeCoords(x,y-1,w,14,true)];
    [obj setAutoenablesItems:NO];
    [obj setTarget:ACTIONTARGET];
    [obj setAction:@selector(onCommand:)];
    [m_parent addSubview:obj];
    if (m_doautoright) UpdateAutoCoords([obj frame]);
    [obj release];
    return (HWND)obj;
  }
  else
  {
    NSComboBox *obj=[[NSComboBox alloc] init];
    if (m_transform.size.width < minwidfontadjust)
      [obj setFont:[NSFont systemFontOfSize:m_transform.size.width*minwidfontscale]];
    [obj setEditable:(flags & CBS_DROPDOWNLIST)?NO:YES];
    [obj setTag:idx];
    [obj setFrame:MakeCoords(x,y-1,w,14,true)];
    [obj setTarget:ACTIONTARGET];
    [obj setAction:@selector(onCommand:)];
    [obj setDelegate:ACTIONTARGET];
    [m_parent addSubview:obj];
    if (m_doautoright) UpdateAutoCoords([obj frame]);
    [obj release];
    return (HWND)obj;
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

HWND SWELL_MakeGroupBox(const char *name, int idx, int x, int y, int w, int h)
{
  TaggedBox *obj=[[TaggedBox alloc] init];
//  [obj setTag:idx];
  NSString *labelstr=(NSString *)SWELL_CStringToCFString(name);
  [obj setTitle:labelstr];
  [obj setTag:idx];
  [labelstr release];
  [obj setFrame:MakeCoords(x,y,w,h,false)];
  [m_parent addSubview:obj];
  [obj release];
  return (HWND)obj;
}


int TabCtrl_GetItemCount(HWND hwnd)
{
  if (!hwnd || ![(id)hwnd isKindOfClass:[SWELL_TabView class]]) return 0;
  SWELL_TabView *tv=(SWELL_TabView*)hwnd;
  return [tv numberOfTabViewItems];
}

BOOL TabCtrl_AdjustRect(HWND hwnd, BOOL fLarger, RECT *r)
{
  if (!r || !hwnd || ![(id)hwnd isKindOfClass:[SWELL_TabView class]]) return FALSE;
  
  int sign=fLarger?-1:1;
  r->left+=sign*7; // todo: correct this?
  r->right-=sign*7;
  r->top+=sign*36;
  r->bottom-=sign*3;
  return TRUE;
}


BOOL TabCtrl_DeleteItem(HWND hwnd, int idx)
{
  if (!hwnd || ![(id)hwnd isKindOfClass:[SWELL_TabView class]]) return 0;
  SWELL_TabView *tv=(SWELL_TabView*)hwnd;
  if (idx<0 || idx>= [tv numberOfTabViewItems]) return 0;
  [tv removeTabViewItem:[tv tabViewItemAtIndex:idx]];
  return TRUE;
}

int TabCtrl_InsertItem(HWND hwnd, int idx, TCITEM *item)
{
  if (!item || !hwnd || ![(id)hwnd isKindOfClass:[SWELL_TabView class]]) return -1;
  if (!(item->mask & TCIF_TEXT) || !item->pszText) return -1;
  SWELL_TabView *tv=(SWELL_TabView*)hwnd;
  if (idx<0) idx=0;
  else if (idx>[tv numberOfTabViewItems]) idx=[tv numberOfTabViewItems];
  
  NSTabViewItem *tabitem=[[NSTabViewItem alloc] init];
  NSString *str=(NSString *)SWELL_CStringToCFString(item->pszText);  
  [tabitem setLabel:str];
  [str release];
  id turd=[tv getNotificationWindow];
  [tv setNotificationWindow:nil];
  [tv insertTabViewItem:tabitem atIndex:idx];
  [tv setNotificationWindow:turd];
  [tabitem release];
  return idx;
}

int TabCtrl_SetCurSel(HWND hwnd, int idx)
{
  if (!hwnd || ![(id)hwnd isKindOfClass:[SWELL_TabView class]]) return -1;
  SWELL_TabView *tv=(SWELL_TabView*)hwnd;
  int ret=TabCtrl_GetCurSel(hwnd);
  if (idx>=0 && idx < [tv numberOfTabViewItems])
  {
    [tv selectTabViewItemAtIndex:idx];
  }
  return ret;
}

int TabCtrl_GetCurSel(HWND hwnd)
{
  if (!hwnd || ![(id)hwnd isKindOfClass:[SWELL_TabView class]]) return 0;
  SWELL_TabView *tv=(SWELL_TabView*)hwnd;
  NSTabViewItem *item=[tv selectedTabViewItem];
  if (!item) return 0;
  return [tv indexOfTabViewItem:item];
}

void ListView_SetExtendedListViewStyleEx(HWND h, int flag, int mask)
{
}

void ListView_SetImageList(HWND h, HIMAGELIST imagelist, int which)
{
  if (!h || !imagelist || which != LVSIL_STATE) return;
  
  SWELL_TableViewWithData *v=(SWELL_TableViewWithData *)h;
  
  v->m_status_imagelist=(WDL_PtrList<void> *)imagelist;
  if (v->m_cols && v->m_cols->GetSize()>0)
  {
    NSTableColumn *col=(NSTableColumn*)v->m_cols->Get(0);
    if (![col isKindOfClass:[SWELL_StatusCell class]])
    {
      SWELL_StatusCell *cell=[[SWELL_StatusCell alloc] initNewCell];
      [cell setWraps:NO];
      [col setDataCell:cell];
      [cell release];
    }
  }  
}


void ListView_InsertColumn(HWND h, int pos, const LVCOLUMN *lvc)
{
  if (!h || !lvc) return;
  if (![(id)h isKindOfClass:[SWELL_TableViewWithData class]]) return;
  SWELL_TableViewWithData *v=(SWELL_TableViewWithData *)h;
  NSTableColumn *col=[[NSTableColumn alloc] init];
  [col setWidth:lvc->cx];
  [col setEditable:NO];
  if (!(v->style & LVS_NOCOLUMNHEADER))
  {
    NSString *lbl=(NSString *)SWELL_CStringToCFString(lvc->pszText);  
    [[col headerCell] setStringValue:lbl];
    [lbl release];
  }
  
  if (!pos && v->m_status_imagelist) 
  {
    SWELL_StatusCell *cell=[[SWELL_StatusCell alloc] initNewCell];
    [cell setWraps:NO];
    [col setDataCell:cell];
    [cell release];
  }
  else
  {  
    [[col dataCell] setWraps:NO];
  }

  [v addTableColumn:col];
  v->m_cols->Add(col);
  [col release];
}

int ListView_InsertItem(HWND h, const LVITEM *item)
{
  if (!h || !item || item->iSubItem) return 0;
  if (![(id)h isKindOfClass:[SWELL_TableViewWithData class]]) return 0;
  
  SWELL_TableViewWithData *tv=(SWELL_TableViewWithData*)h;
  if (tv->style & LVS_OWNERDATA) return -1;
  if (!tv->m_items) return -1;
  int a=item->iItem;
  if (a<0)a=0;
  else if (a > tv->m_items->GetSize()) a=tv->m_items->GetSize();
  
  SWELL_TableView_Row *nr=new SWELL_TableView_Row;
  nr->m_vals.Add(strdup((item->mask & LVIF_TEXT) ? item->pszText : ""));
  if (item->mask & LVIF_PARAM) nr->m_param = item->lParam;
  tv->m_items->Insert(a,nr);
  

  
  if ((item->mask&LVIF_STATE) && (item->stateMask & (0xff<<16)))
  {
    nr->m_imageidx=(item->state>>16)&0xff;
  }
  
  [tv reloadData];
  
  if (a < tv->m_items->GetSize()-1)
  {
    NSIndexSet *sel=[tv selectedRowIndexes];
    if (sel && [sel count])
    {
      NSMutableIndexSet *ms = [[NSMutableIndexSet alloc] initWithIndexSet:sel];
      [ms shiftIndexesStartingAtIndex:a by:1];
      [tv selectRowIndexes:ms byExtendingSelection:NO];
      [ms release];
    }
  }
  
  if (item->mask & LVIF_STATE)
  {
    if (item->stateMask & LVIS_SELECTED)
    {
      if (item->state&LVIS_SELECTED)
      {
        [tv selectRow:a byExtendingSelection:(tv->style&LVS_SINGLESEL)?NO:YES];        
      }
    }
  }
  
  return a;
}

void ListView_SetItemText(HWND h, int ipos, int cpos, const char *txt)
{
  if (!h || cpos < 0 || cpos >= 32) return;
  if (![(id)h isKindOfClass:[SWELL_TableViewWithData class]]) return;
  
  SWELL_TableViewWithData *tv=(SWELL_TableViewWithData*)h;
  if (tv->style & LVS_OWNERDATA) return;
  if (!tv->m_items) return;
  
  SWELL_TableView_Row *p=tv->m_items->Get(ipos);
  if (!p) return;
  int x;
  for (x = p->m_vals.GetSize(); x < cpos; x ++)
  {
    p->m_vals.Add(strdup(""));
  }
  if (cpos < p->m_vals.GetSize())
  {
    free(p->m_vals.Get(cpos));
    p->m_vals.Set(cpos,strdup(txt));
  }
  else p->m_vals.Add(strdup(txt));
    
  [tv reloadData];
}

int ListView_GetNextItem(HWND h, int istart, int flags)
{
  if (flags==LVNI_FOCUSED)
  {
    if (!h) return 0;
    if (![(id)h isKindOfClass:[SWELL_TableViewWithData class]]) return 0;
    
    SWELL_TableViewWithData *tv=(SWELL_TableViewWithData*)h;
    return [tv selectedRow];
  }
  return -1;
}

bool ListView_SetItem(HWND h, LVITEM *item)
{
  if (!item) return false;
  if (!h) return false;
  if (![(id)h isKindOfClass:[SWELL_TableViewWithData class]]) return false;
    
  SWELL_TableViewWithData *tv=(SWELL_TableViewWithData*)h;
  if (!(tv->style & LVS_OWNERDATA))
  {
    if (!tv->m_items) return false;
    SWELL_TableView_Row *row=tv->m_items->Get(item->iItem);
    if (!row) return false;  
  
    if (item->mask & LVIF_PARAM) row->m_param=item->lParam;
    if (item->mask & LVIF_TEXT) if (item->pszText) ListView_SetItemText(h,item->iItem,item->iSubItem,item->pszText);
  }
  if (item->mask & LVIF_STATE) if (item->stateMask)
    ListView_SetItemState(h,item->iItem,item->state,item->stateMask); 

  return true;
}

bool ListView_GetItem(HWND h, LVITEM *item)
{
  if (!item) return false;
  if ((item->mask&LVIF_TEXT)&&item->pszText && item->cchTextMax > 0) item->pszText[0]=0;
  item->state=0;
  if (!h) return false;
  if (![(id)h isKindOfClass:[SWELL_TableViewWithData class]]) return false;
  
  
  SWELL_TableViewWithData *tv=(SWELL_TableViewWithData*)h;
  if (!(tv->style & LVS_OWNERDATA))
  {
    if (!tv->m_items) return false;
    
    SWELL_TableView_Row *row=tv->m_items->Get(item->iItem);
    if (!row) return false;  
  
    if (item->mask & LVIF_PARAM) item->lParam=row->m_param;
    if (item->mask & LVIF_TEXT) if (item->pszText && item->cchTextMax>0)
    {
      char *p=row->m_vals.Get(item->iSubItem);
      lstrcpyn(item->pszText,p?p:"",item->cchTextMax);
    }
      if (item->mask & LVIF_STATE)
      {
        if (item->stateMask & (0xff<<16))
        {
          item->state|=row->m_imageidx<<16;
        }
      }
  }
  else
  {
    if (item->iItem <0 || item->iItem >= tv->ownermode_cnt) return false;
  }
  if (item->mask & LVIF_STATE)
  {
     if ((item->stateMask&LVIS_SELECTED) && [tv isRowSelected:item->iItem]) item->state|=LVIS_SELECTED;
     if ((item->stateMask&LVIS_FOCUSED) && [tv selectedRow] == item->iItem) item->state|=LVIS_FOCUSED;
  }

  return true;
}
int ListView_GetItemState(HWND h, int ipos, int mask)
{
  if (!h || ![(id)h isKindOfClass:[SWELL_TableViewWithData class]]) return 0;
  SWELL_TableViewWithData *tv=(SWELL_TableViewWithData*)h;
  int flag=0;
  if (!(tv->style & LVS_OWNERDATA))
  {
    if (!tv->m_items) return 0;
    SWELL_TableView_Row *row=tv->m_items->Get(ipos);
    if (!row) return 0;  
    if (mask & (0xff<<16))
    {
      flag|=row->m_imageidx<<16;
    }
  }
  else
  {
    if (ipos<0 || ipos >= tv->ownermode_cnt) return 0;
  }
  
  if ((mask&LVIS_SELECTED) && [tv isRowSelected:ipos]) flag|=LVIS_SELECTED;
  if ((mask&LVIS_FOCUSED) && [tv selectedRow]==ipos) flag|=LVIS_FOCUSED;
  return flag;  
}

bool ListView_SetItemState(HWND h, int ipos, int state, int statemask)
{
  if (!h || ![(id)h isKindOfClass:[SWELL_TableViewWithData class]]) return false;
  SWELL_TableViewWithData *tv=(SWELL_TableViewWithData*)h;

  if (!(tv->style & LVS_OWNERDATA))
  {
    if (!tv->m_items) return false;
    SWELL_TableView_Row *row=tv->m_items->Get(ipos);
    if (!row) return false;  
    if (statemask & (0xff<<16))
    {
      row->m_imageidx=(state>>16)&0xff;
    }
  }
  else
  {
    if (ipos<0 || ipos >= tv->ownermode_cnt) return 0;
  }
  if (statemask & LVIS_SELECTED)
  {
    if (state & LVIS_SELECTED)
    {      
      if (![tv isRowSelected:ipos]) [tv selectRow:ipos byExtendingSelection:(tv->style&LVS_SINGLESEL)?NO:YES];
    }
    else
    {
      if ([tv isRowSelected:ipos]) [tv deselectRow:ipos]; 
    }
  }
  if (statemask & LVIS_FOCUSED)
  {
    if (state&LVIS_FOCUSED)
    {
      // hmm hosed focused is nonexistent in osx gayness?
    }
    else
    {
      
    }
  }
  return true;
}
void ListView_RedrawItems(HWND h, int startitem, int enditem)
{
  if (!h || ![(id)h isKindOfClass:[SWELL_TableViewWithData class]]) return;
  SWELL_TableViewWithData *tv=(SWELL_TableViewWithData*)h;
  if (!tv->m_items) return;
  [tv reloadData];
}

void ListView_DeleteItem(HWND h, int ipos)
{
  if (!h) return;
  if (![(id)h isKindOfClass:[SWELL_TableViewWithData class]]) return;
  
  SWELL_TableViewWithData *tv=(SWELL_TableViewWithData*)h;
  if (!tv->m_items) return;
  
  if (ipos >=0 && ipos < tv->m_items->GetSize())
  {
    if (ipos != tv->m_items->GetSize()-1)
    {
      NSIndexSet *sel=[tv selectedRowIndexes];
      if (sel && [sel count])
      {
        NSMutableIndexSet *ms = [[NSMutableIndexSet alloc] initWithIndexSet:sel];
        [ms shiftIndexesStartingAtIndex:ipos+1 by:-1];
        [tv selectRowIndexes:ms byExtendingSelection:NO];
        [ms release];
      }
    }
    tv->m_items->Delete(ipos,true);
    
    [tv reloadData];
    
  }
}

void ListView_DeleteAllItems(HWND h)
{
  if (!h) return;
  if (![(id)h isKindOfClass:[SWELL_TableViewWithData class]]) return;
  
  SWELL_TableViewWithData *tv=(SWELL_TableViewWithData*)h;
  tv->ownermode_cnt=0;
  if (tv->m_items) tv->m_items->Empty(true);
  
  [tv reloadData];
}

int ListView_GetSelectedCount(HWND h)
{
  if (!h) return 0;
  if (![(id)h isKindOfClass:[SWELL_TableViewWithData class]]) return 0;
  
  SWELL_TableViewWithData *tv=(SWELL_TableViewWithData*)h;
  return [tv numberOfSelectedRows];
}

int ListView_GetItemCount(HWND h)
{
  if (!h) return 0;
  if (![(id)h isKindOfClass:[SWELL_TableViewWithData class]]) return 0;
  
  SWELL_TableViewWithData *tv=(SWELL_TableViewWithData*)h;
  if (!(tv->style & LVS_OWNERDATA))
  {
    if (!tv->m_items) return 0;
  
    return tv->m_items->GetSize();
  }
  return tv->ownermode_cnt;
}

int ListView_GetSelectionMark(HWND h)
{
  if (!h) return 0;
  if (![(id)h isKindOfClass:[SWELL_TableViewWithData class]]) return 0;
  
  SWELL_TableViewWithData *tv=(SWELL_TableViewWithData*)h;
  return [tv selectedRow];
}

void ListView_SetColumnWidth(HWND h, int colpos, int wid)
{
}

int ListView_HitTest(HWND h, LVHITTESTINFO *pinf)
{
  if (!h) return -1;
  if (![(id)h isKindOfClass:[SWELL_TableViewWithData class]]) return -1;
  
  SWELL_TableViewWithData *tv=(SWELL_TableViewWithData*)h;
  // return index
  pinf->flags=0;
  pinf->iItem=-1;
  
  NSPoint pt={pinf->pt.x,pinf->pt.y};
    
  pinf->iItem=[(NSTableView *)h rowAtPoint:pt];
  if (pinf->iItem >= 0 && tv->m_status_imagelist)
  {
    pinf->flags=LVHT_ONITEMLABEL;
    float rh = [tv rowHeight];
    if (pinf->pt.x <= rh)
    {      
      pinf->flags=LVHT_ONITEMSTATEICON;
    }
  }
  else
  {
    pinf->flags=LVHT_NOWHERE;
    
    NSRect fr=[(NSTableView *)h bounds];
    if (pt.y < 0) pinf->flags=LVHT_ABOVE;
    else pinf->flags=LVHT_BELOW;
    
  }
  
  return pinf->iItem;
}

void ListView_SetItemCount(HWND h, int cnt)
{
  if (!h) return;
  if (![(id)h isKindOfClass:[SWELL_TableViewWithData class]]) return;
  
  SWELL_TableViewWithData *tv=(SWELL_TableViewWithData*)h;
  if (tv->style & LVS_OWNERDATA)
  {
    tv->ownermode_cnt=cnt;
  }
}

void ListView_EnsureVisible(HWND h, int i, BOOL pok)
{
  if (!h) return;
  if (![(id)h isKindOfClass:[SWELL_TableViewWithData class]]) return;
  
  SWELL_TableViewWithData *tv=(SWELL_TableViewWithData*)h;
  
  if (i<0)i=0;
  if (tv->style & LVS_OWNERDATA)
  {
    if (i >=tv->ownermode_cnt-1) i=tv->ownermode_cnt-1;
  }
  else
  {
    if (tv->m_items && i >= tv->m_items->GetSize()) i=tv->m_items->GetSize()-1;
  }
  if (i>=0)
  {
    [tv scrollRowToVisible:i];
  }  
}
bool ListView_GetSubItemRect(HWND h, int item, int subitem, int code, RECT *r)
{
  if (!h) return false;
  if (![(id)h isKindOfClass:[SWELL_TableViewWithData class]]) return false;
  if (item < 0 || item > ListView_GetItemCount(h)) return false;
  SWELL_TableViewWithData *tv=(SWELL_TableViewWithData*)h;
  
  if (!tv->m_cols || subitem<0 || subitem >= tv->m_cols->GetSize()) return false;
  
  NSRect ar=[tv frameOfCellAtColumn:subitem row:item];
  r->left=(int)ar.origin.x;
  r->top=(int)ar.origin.y;
  r->right=r->left + (int)ar.size.width;
  r->bottom=r->top+(int)ar.size.height;
  return true;
}


HWND WindowFromPoint(POINT p)
{
  NSArray *windows=[NSApp orderedWindows];
  if (!windows) return 0;
  
  NSWindow *bestwnd=0;
  int x;
  for (x = 0; x < [windows count]; x ++)
  {
    NSWindow *wnd=[windows objectAtIndex:x];
    if (wnd)
    {
      NSRect fr=[wnd frame];
      if (p.x >= fr.origin.x && p.x < fr.origin.x + fr.size.width &&
          p.y >= fr.origin.y && p.y < fr.origin.y + fr.size.height)
      {
        bestwnd=wnd;
        break;
      }    
    }
  }
  
  if (!bestwnd) return 0;
  NSPoint pt={p.x,p.y};
  NSPoint lpt=[bestwnd convertScreenToBase:pt];
  NSView *v=[[bestwnd contentView] hitTest:lpt];
  if (v) return (HWND)v;
  return (HWND)bestwnd; // todo: implement
}

void UpdateWindow(HWND hwnd)
{
  // todo: implement
}

void InvalidateRect(HWND hwnd, RECT *r, int eraseBk)
{ 
  if (!hwnd) return;
  id view=(id)hwnd;
  if ([view isKindOfClass:[NSWindow class]]) view=[view contentView];
  if ([view isKindOfClass:[NSView class]]) 
  {
    if (r)
    {
      [view setNeedsDisplayInRect:NSMakeRect(r->left,r->top,r->right-r->left,r->bottom-r->top)]; 
    }
    else [view setNeedsDisplay:YES];
  }
}

static HWND m_fakeCapture;
static BOOL m_capChangeNotify;
HWND GetCapture()
{

  return m_fakeCapture;
}

HWND SetCapture(HWND hwnd)
{
  HWND oc=m_fakeCapture;
  int ocn=m_capChangeNotify;
  m_fakeCapture=hwnd;
  m_capChangeNotify = hwnd && [(id)hwnd respondsToSelector:@selector(swellCapChangeNotify)] && [(id)hwnd swellCapChangeNotify];

  if (ocn && oc) SendMessage(oc,WM_CAPTURECHANGED,0,(LPARAM)hwnd);
  return oc;
}

void ReleaseCapture()
{
  HWND h=m_fakeCapture;
  m_fakeCapture=NULL;
  if (m_capChangeNotify && h)
  {
    SendMessage(h,WM_CAPTURECHANGED,0,0);
  }
}


HDC BeginPaint(HWND hwnd, PAINTSTRUCT *ps)
{
  if (!ps) return 0;
  memset(ps,0,sizeof(PAINTSTRUCT));
  if (!hwnd) return 0;
  id turd = (id)hwnd;
  if (![turd respondsToSelector:@selector(getSwellPaintInfo:)]) return 0;

  [turd getSwellPaintInfo:(PAINTSTRUCT *)ps];
  return ps->hdc;
}

BOOL EndPaint(HWND hwnd, PAINTSTRUCT *ps)
{
  return TRUE;
}

long DefWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (msg==WM_RBUTTONUP)
  {
    POINT p={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
    ClientToScreen(hwnd,&p);
    SendMessage(hwnd,WM_CONTEXTMENU,(WPARAM)hwnd,((short)p.x)|(p.y<<16));
    return 1;
  }
  else if (msg==WM_CONTEXTMENU)
  {
    HWND par=GetParent(hwnd);
    if (par) SendMessage(par,WM_CONTEXTMENU,wParam,lParam);
  }
  return 0;
}

















///////////////// clipboard compatability (NOT THREAD SAFE CURRENTLY)

typedef struct
{
  int sz;
  int refcnt;
  void *buf;
} GLOBAL_REC;


void *GlobalLock(HANDLE h)
{
  if (!h) return 0;
  GLOBAL_REC *rec=(GLOBAL_REC*)h;
  rec->refcnt++;
  return rec->buf;
}
int GlobalSize(HANDLE h)
{
  if (!h) return 0;
  GLOBAL_REC *rec=(GLOBAL_REC*)h;
  return rec->sz;
}

void GlobalUnlock(HANDLE h)
{
  if (!h) return;
  GLOBAL_REC *rec=(GLOBAL_REC*)h;
  rec->refcnt--;
}
void GlobalFree(HANDLE h)
{
  if (!h) return;
  GLOBAL_REC *rec=(GLOBAL_REC*)h;
  if (rec->refcnt)
  {
    // note error freeing locked ram
  }
  free(rec->buf);
  free(rec);
  
}
HANDLE SWELL_GlobalAlloc(int sz)
{
  GLOBAL_REC *rec=(GLOBAL_REC*)malloc(sizeof(GLOBAL_REC));
  rec->sz=sz;
  rec->buf=malloc(sz);
  rec->refcnt=0;
  return rec;
}













static WDL_PtrList<GLOBAL_REC> m_clip_recs;
static WDL_PtrList<NSString> m_clip_fmts;
static WDL_PtrList<void> m_clip_curfmts;
bool OpenClipboard(HWND hwndDlg)
{
  NSPasteboard *pasteboard = [NSPasteboard pasteboardWithName:@"SWELL_APP"];
  m_clip_curfmts.Empty();
  NSArray *ar=[pasteboard types];

  
  if (ar && [ar count])
  {
    int x;
    
    for (x = 0; x < [ar count]; x ++)
    {
      NSString *s=[ar objectAtIndex:x];
      if (!s) continue;
      int y;
      for (y = 0; y < m_clip_fmts.GetSize(); y ++)
      {
        if ([s compare:(NSString *)m_clip_fmts.Get(y)]==NSOrderedSame)
        {
          if (m_clip_curfmts.Find((void*)(y+1))<0)
            m_clip_curfmts.Add((void*)(y+1));
          break;
        }
      }
      
    }
  }
  return true;
}

void CloseClipboard() // frees any remaining items in clipboard
{
  m_clip_recs.Empty(true,GlobalFree);
}

UINT EnumClipboardFormats(UINT lastfmt)
{
  if (!m_clip_curfmts.GetSize()) return 0;
  if (lastfmt == 0) return (UINT)m_clip_curfmts.Get(0);
  int x;
  for (x = m_clip_curfmts.GetSize()-2; x >= 0; x--) // scan backwards to avoid dupes causing infinite loops
  {
    if (m_clip_curfmts.Get(x) == (void*)lastfmt)
      return (UINT)m_clip_curfmts.Get(x+1);
  }
  return 0;
}

HANDLE GetClipboardData(UINT type)
{
  NSString *fmt=m_clip_fmts.Get(type-1);
  if (!fmt) return 0;
  NSPasteboard *pasteboard = [NSPasteboard pasteboardWithName:@"SWELL_APP"];

	NSData *data=[pasteboard dataForType:fmt];
	if (!data) return 0; 
  int l=[data length];
  HANDLE h=GlobalAlloc(0,l);  
	memcpy(GlobalLock(h),[data bytes],l);
  GlobalUnlock(h);
  m_clip_recs.Add((GLOBAL_REC*)h);
  
	return h;
}


void EmptyClipboard()
{
}

void SetClipboardData(UINT type, HANDLE h)
{
  NSString *fmt=m_clip_fmts.Get(type-1);
  if (fmt)
  {
    NSPasteboard *pasteboard = [NSPasteboard pasteboardWithName:@"SWELL_APP"];
    [pasteboard declareTypes:[NSArray arrayWithObject:fmt] owner:nil];
    void *buf=GlobalLock(h);
    int bufsz=GlobalSize(h);
    NSData *data=[NSData dataWithBytes:buf length:bufsz];
    [pasteboard setData:data forType:fmt];
    GlobalUnlock(h);
  }
  GlobalFree(h);
  
}

UINT RegisterClipboardFormat(const char *desc)
{
  NSString *s=(NSString*)SWELL_CStringToCFString(desc);
  int x;
  for (x = 0; x < m_clip_fmts.GetSize(); x ++)
  {
    NSString *ts=m_clip_fmts.Get(x);
    if ([ts compare:s]==NSOrderedSame)
    {
      [s release];
      return x+1;
    }
  }
  m_clip_fmts.Add(s);
  return m_clip_fmts.GetSize();
}


HIMAGELIST ImageList_CreateEx()
{
  return new WDL_PtrList<void>;
}

void ImageList_ReplaceIcon(HIMAGELIST list, int offset, HICON image)
{
  if (!image || !list) return;
  WDL_PtrList<void> *l=(WDL_PtrList<void> *)list;
  if (offset<0||offset>=l->GetSize()) l->Add(image);
  else
  {
    HICON old=l->Get(offset);
    l->Set(offset,image);
    if (old) DestroyIcon(old);
  }
}





///////// PostMessage emulation

BOOL PostMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  id del=[NSApp delegate];
  if (del && [del respondsToSelector:@selector(swellPostMessage:msg:wp:lp:)])
    return (BOOL)!![del swellPostMessage:hwnd msg:message wp:wParam lp:lParam];
  return FALSE;
}

void SWELL_MessageQueue_Clear(HWND h)
{
  id del=[NSApp delegate];
  if (del && [del respondsToSelector:@selector(swellPostMessageClearQ:)])
    [del swellPostMessageClearQ:h];
}



// implementation of postmessage stuff



typedef struct PMQ_rec
{
  HWND hwnd;
  UINT msg;
  WPARAM wParam;
  LPARAM lParam;

  struct PMQ_rec *next;
} PMQ_rec;

static WDL_Mutex *m_pmq_mutex;
static PMQ_rec *m_pmq, *m_pmq_empty, *m_pmq_tail;
static int m_pmq_size;
static id m_pmq_timer;
#define MAX_POSTMESSAGE_SIZE 1024

void SWELL_Internal_PostMessage_Init()
{
  if (m_pmq_mutex) return;
  id del = [NSApp delegate];
  if (!del || ![del respondsToSelector:@selector(swellPostMessageTick:)]) return;
  
  m_pmq_mutex = new WDL_Mutex;
  
  m_pmq_timer = [NSTimer scheduledTimerWithTimeInterval:0.05 target:(id)del selector:@selector(swellPostMessageTick:) userInfo:nil repeats:YES];
  [[NSRunLoop currentRunLoop] addTimer:m_pmq_timer forMode:(NSString*)kCFRunLoopCommonModes];
//  [ release];
  // set a timer to the delegate
}

void SWELL_MessageQueue_Flush()
{
  if (!m_pmq_mutex) return;
  
  m_pmq_mutex->Enter();
  PMQ_rec *p=m_pmq, *startofchain=m_pmq;
  m_pmq=m_pmq_tail=0;
  m_pmq_mutex->Leave();
  
  int cnt=0;
  // process out queue
  while (p)
  {
    // process this message
//    [(id)p->hwnd onSwellMessage:(int)p->msg p1:(WPARAM)p->wParam p2:(LPARAM)p->lParam];
    SendMessage(p->hwnd,p->msg,p->wParam,p->lParam); 

    cnt ++;
    if (!p->next) // add the chain back to empties
    {
      m_pmq_mutex->Enter();
      m_pmq_size-=cnt;
      p->next=m_pmq_empty;
      m_pmq_empty=startofchain;
      m_pmq_mutex->Leave();
      break;
    }
    p=p->next;
  }
}

void SWELL_Internal_PMQ_ClearAllMessages(HWND hwnd)
{
  if (!m_pmq_mutex) return;
  
  m_pmq_mutex->Enter();
  PMQ_rec *p=m_pmq;
  PMQ_rec *lastrec=NULL;
  while (p)
  {
    if (hwnd && p->hwnd != hwnd) { lastrec=p; p=p->next; }
    else
    {
      PMQ_rec *next=p->next; 
      
      p->next=m_pmq_empty; // add p to empty list
      m_pmq_empty=p;
      m_pmq_size--;
      
      
      if (p==m_pmq_tail) m_pmq_tail=lastrec; // update tail
      
      if (lastrec)  p = lastrec->next = next;
      else p = m_pmq = next;
    }
  }
  m_pmq_mutex->Leave();
}

BOOL SWELL_Internal_PostMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (!hwnd||!m_pmq_mutex) return FALSE;
  if (![(id)hwnd respondsToSelector:@selector(swellCanPostMessage)]) return FALSE;

  BOOL ret=FALSE;
  m_pmq_mutex->Enter();

  if (m_pmq_size<MAX_POSTMESSAGE_SIZE && [(id)hwnd swellCanPostMessage])
  {
    PMQ_rec *rec=m_pmq_empty;
    if (rec) m_pmq_empty=rec->next;
    else rec=(PMQ_rec*)malloc(sizeof(PMQ_rec));
    rec->next=0;
    rec->hwnd=hwnd;
    rec->msg=msg;
    rec->wParam=wParam;
    rec->lParam=lParam;

    if (m_pmq_tail) m_pmq_tail->next=rec;
    else 
    {
      PMQ_rec *p=m_pmq;
      while (p && p->next) p=p->next; // shouldnt happen unless m_pmq is NULL As well but why not for safety
      if (p) p->next=rec;
      else m_pmq=rec;
    }
    m_pmq_tail=rec;
    m_pmq_size++;

    ret=TRUE;
  }

  m_pmq_mutex->Leave();

  return ret;
}






int EnumPropsEx(HWND hwnd, PROPENUMPROCEX proc, LPARAM lParam)
{
  if (!hwnd || ![(id)hwnd respondsToSelector:@selector(swellEnumProps:lp:)]) return -1;
  return (int)[(id)hwnd swellEnumProps:proc lp:lParam];
}

HANDLE GetProp(HWND hwnd, const char *name)
{
  if (!hwnd || ![(id)hwnd respondsToSelector:@selector(swellGetProp:wantRemove:)]) return NULL;
  return (HANDLE)[(id)hwnd swellGetProp:name wantRemove:NO];
}

BOOL SetProp(HWND hwnd, const char *name, HANDLE val)
{
  if (!hwnd || ![(id)hwnd respondsToSelector:@selector(swellSetProp:value:)]) return FALSE;
  return (BOOL)!![(id)hwnd swellSetProp:name value:val];
}

HANDLE RemoveProp(HWND hwnd, const char *name)
{
  if (!hwnd || ![(id)hwnd respondsToSelector:@selector(swellGetProp:wantRemove:)]) return NULL;
  return (HANDLE)[(id)hwnd swellGetProp:name wantRemove:YES];
}




#endif
