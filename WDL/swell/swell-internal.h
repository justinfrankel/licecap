#ifndef _SWELL_INTERNAL_H_
#define _SWELL_INTERNAL_H_



#ifdef SWELL_TARGET_GTK
#include <gtk/gtk.h>
#endif


#ifdef SWELL_TARGET_OSX

#include "../ptrlist.h"

#define __SWELL_PREFIX_CLASSNAME(cname) SWELL_APP_PREFIX##cname

// this defines interfaces to internal swell classes
#define SWELL_hwndChild __SWELL_PREFIX_CLASSNAME(_hwnd) 
#define SWELL_hwndCarbonHost __SWELL_PREFIX_CLASSNAME(_hwndcarbonhost)

#define SWELL_ModelessWindow __SWELL_PREFIX_CLASSNAME(_modelesswindow)
#define SWELL_ModalDialog __SWELL_PREFIX_CLASSNAME(_dialogbox)

#define SWELL_ListView __SWELL_PREFIX_CLASSNAME(_listview)
#define SWELL_TreeView __SWELL_PREFIX_CLASSNAME(_treeview)
#define SWELL_TabView __SWELL_PREFIX_CLASSNAME(_tabview)
#define SWELL_ProgressView  __SWELL_PREFIX_CLASSNAME(_progind)
#define SWELL_TextView  __SWELL_PREFIX_CLASSNAME(_textview)
#define SWELL_BoxView __SWELL_PREFIX_CLASSNAME(_box)
#define SWELL_Button __SWELL_PREFIX_CLASSNAME(_button)
#define SWELL_PopUpButton __SWELL_PREFIX_CLASSNAME(_pub)
#define SWELL_ComboBox __SWELL_PREFIX_CLASSNAME(_cbox)

#define SWELL_StatusCell __SWELL_PREFIX_CLASSNAME(_statuscell)
#define SWELL_ODListViewCell __SWELL_PREFIX_CLASSNAME(_ODlistviewcell)
#define SWELL_ODButtonCell __SWELL_PREFIX_CLASSNAME(_ODbuttoncell)

#define SWELL_FocusRectWnd __SWELL_PREFIX_CLASSNAME(_drawfocusrectwnd)

#define SWELL_DataHold __SWELL_PREFIX_CLASSNAME(_sdh)
#define SWELL_ThreadTmp __SWELL_PREFIX_CLASSNAME(_thread)
#define SWELL_PopupMenuRecv __SWELL_PREFIX_CLASSNAME(_trackpopupmenurecv)


#ifdef __OBJC__

@interface SWELL_DataHold : NSObject
{
  void *m_data;
}
-(id) initWithVal:(void *)val;
-(void *) getValue;
@end

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


class SWELL_ListView_Row
{
public:
  SWELL_ListView_Row()
  {
    m_imageidx=0;
    m_param=0;
  }
  ~SWELL_ListView_Row()
  {
    m_vals.Empty(true,free);
  }
  WDL_PtrList<char> m_vals;
  LPARAM m_param;
  int m_imageidx;
};


class SWELL_TreeView_Item
{
public:
  SWELL_TreeView_Item()
  {
    m_param=0;
    m_value=0;
    m_haschildren=false;
    m_dh = [[SWELL_DataHold alloc] initWithVal:this];
  }
  ~SWELL_TreeView_Item()
  {
    free(m_value);
    m_children.Empty(true);
    [m_dh release];
  }
  
  bool FindItem(HTREEITEM it, SWELL_TreeView_Item **parOut, int *idxOut)
  {
    int a=m_children.Find((SWELL_TreeView_Item*)it);
    if (a>=0)
    {
      *parOut=this;
      *idxOut=a;
      return true;
    }
    int x;
    for (x = 0; x < m_children.GetSize(); x ++)
    {
      if (m_children.Get(x)->FindItem(it,parOut,idxOut)) return true;
    }
    return false;
  }
  
  SWELL_DataHold *m_dh;
  
  bool m_haschildren;
  char *m_value;
  WDL_PtrList<SWELL_TreeView_Item> m_children; // only used in tree mode
  LPARAM m_param;
};





@interface SWELL_TabView : NSTabView
{
  int m_tag;
  id m_dest;
}
@end

@interface SWELL_ProgressView : NSProgressIndicator
{
  int m_tag;
}
@end

@interface SWELL_StatusCell : NSCell
{
  NSImage *status;
}
@end

@interface SWELL_TreeView : NSOutlineView
{
  @public
  bool m_fakerightmouse;
  LONG style;
  WDL_PtrList<SWELL_TreeView_Item> *m_items;
}
@end

@interface SWELL_ListView : NSTableView
{
  int m_leftmousemovecnt;
  bool m_fakerightmouse;
  @public
  LONG style;
  int ownermode_cnt;
  int m_start_item;
  int m_start_item_clickmode;
  int m_lbMode;
  WDL_PtrList<SWELL_ListView_Row> *m_items;
  WDL_PtrList<NSTableColumn> *m_cols;
  WDL_PtrList<char> *m_status_imagelist;
}
-(LONG)getSwellStyle;
-(void)setSwellStyle:(LONG)st;
-(int)getSwellNotificationMode;
-(void)setSwellNotificationMode:(int)lbMode;
@end

@interface SWELL_ODButtonCell : NSButtonCell
{
}
@end

@interface SWELL_ODListViewCell : NSCell
{
  SWELL_ListView *m_ownctl;
  int m_lastidx;
}
-(void)setOwnerControl:(SWELL_ListView *)t;
-(void)setItemIdx:(int)idx;
@end

@interface SWELL_Button : NSButton
{
  void *m_swellGDIimage;
  LONG m_userdata;
  int m_radioflags;
}
-(int)swellGetRadioFlags;
-(void)swellSetRadioFlags:(int)f;
-(LONG)getSwellUserData;
-(void)setSwellUserData:(LONG)val;
-(void)setSwellGDIImage:(void *)par;
-(void *)getSwellGDIImage;
@end

@interface SWELL_TextView : NSTextView
{
  int m_tag;
}
-(int) tag;
-(void) setTag:(int)tag;
@end

@interface SWELL_BoxView : NSBox
{
  int m_tag;
}
-(int) tag;
-(void) setTag:(int)tag;
@end

@interface SWELL_FocusRectWnd : NSView
{
}
@end

@interface SWELL_ThreadTmp : NSObject
{
}
-(void)bla;
@end


@interface SWELL_hwndChild : NSView
{
  BOOL m_enabled;
  DLGPROC m_dlgproc;
  WNDPROC m_wndproc;
  LONG m_userdata;
  LONG m_extradata[32];
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
  @public
  unsigned int m_create_windowflags;
}
- (id)initChild:(SWELL_DialogResourceIndex *)resstate Parent:(NSView *)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par;
-(LONG)sendMouseMessage:(int)msg event:(NSEvent*)theEvent;
- (LRESULT)onSwellMessage:(UINT)msg p1:(WPARAM)wParam p2:(LPARAM)lParam;
-(HANDLE)swellExtendedDragOp:(id <NSDraggingInfo>)sender retGlob:(BOOL)retG;
- (const char *)onSwellGetText;
-(void)onSwellSetText:(const char *)buf;
-(LONG)swellGetExtendedStyle;
-(void)swellSetExtendedStyle:(LONG)st;
-(HMENU)swellGetMenu;
-(void)swellSetMenu:(HMENU)menu;
-(LONG)getSwellUserData;
-(void)setSwellUserData:(LONG)val;

-(LPARAM)getSwellExtraData:(int)idx;
-(void)setSwellExtraData:(int)idx value:(LPARAM)val;
-(void)setSwellWindowProc:(WNDPROC)val;
-(WNDPROC)getSwellWindowProc;
-(void)setSwellDialogProc:(DLGPROC)val;
-(DLGPROC)getSwellDialogProc;

-(void) getSwellPaintInfo:(PAINTSTRUCT *)ps;
- (int)swellCapChangeNotify;
-(unsigned int)swellCreateWindowFlags;

-(bool)swellCanPostMessage;
-(int)swellEnumProps:(PROPENUMPROCEX)proc lp:(LPARAM)lParam;
-(void *)swellGetProp:(const char *)name wantRemove:(BOOL)rem;
-(int)swellSetProp:(const char *)name value:(void *)val ;
@end

@interface SWELL_ModelessWindow : NSWindow
{
  BOOL m_enabled;
  id m_owner;
  OwnedWindowListRec *m_ownedwnds;
}
- (id)initModeless:(SWELL_DialogResourceIndex *)resstate Parent:(HWND)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par;
- (id)initModelessForChild:(HWND)child owner:(HWND)owner styleMask:(unsigned int)smask;
- (void)swellDestroyAllOwnedWindows;
- (void)swellRemoveOwnedWindow:(NSWindow *)wnd;
- (void)swellSetOwner:(id)owner;
- (id)swellGetOwner;
- (void **)swellGetOwnerWindowHead;
-(void)swellDoDestroyStuff;
@end

@interface SWELL_ModalDialog : NSPanel
{
  BOOL m_enabled;
  id m_owner;
  OwnedWindowListRec *m_ownedwnds;
  
  int m_rv;
}
- (id)initDialogBox:(SWELL_DialogResourceIndex *)resstate Parent:(HWND)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par;
- (void)swellDestroyAllOwnedWindows;
- (void)swellRemoveOwnedWindow:(NSWindow *)wnd;
- (void)swellSetOwner:(id)owner;
- (id)swellGetOwner;
- (void **)swellGetOwnerWindowHead;
-(void)swellDoDestroyStuff;

-(void)swellSetModalRetVal:(int)r;
-(int)swellGetModalRetVal;
@end


@interface SWELL_hwndCarbonHost : SWELL_hwndChild
{
  @public
  NSWindow *m_cwnd;
  WindowRef m_wndref;
  bool m_needattach;
}
-(BOOL)swellIsCarbonHostingView;
-(void)swellDoRepos;
@end


@interface SWELL_PopupMenuRecv : NSObject
{
  int m_act;
  HWND cbwnd;
}
-(id) initWithWnd:(HWND)wnd;
-(void) onSwellCommand:(id)sender;
-(int) isCommand;
- (void)menuNeedsUpdate:(NSMenu *)menu;

@end

@interface SWELL_PopUpButton : NSPopUpButton
{
  LONG m_style;
}
-(void)setSwellStyle:(LONG)style;
-(LONG)getSwellStyle;
@end

@interface SWELL_ComboBox : NSComboBox
{
@public
  LONG m_style;
  WDL_PtrList<char> *m_ids;
}
-(id)init;
-(void)dealloc;
-(void)setSwellStyle:(LONG)style;
-(LONG)getSwellStyle;
@end



// GDI internals

#define TYPE_PEN 1
#define TYPE_BRUSH 2
#define TYPE_FONT 3
#define TYPE_BITMAP 4

typedef struct 
{
  int type;
  CGColorRef color;
  int wid;
  NSImage *bitmapptr;  
  
  // only used if HFONT and using NSString to draw text
  NSMutableDictionary *fontdict;
  NSMutableParagraphStyle *fontparagraphinfo; // dont release this, it's owned by fontdict
  
  // only used if HFONT and using ATSU to draw text (faster)
  ATSUStyle font_style;
  float font_rotation;
} GDP_OBJECT;

typedef struct {
  CGContextRef ctx; 
  void *ownedData;
  GDP_OBJECT *curpen;
  GDP_OBJECT *curbrush;
  GDP_OBJECT *curfont;
  
  NSColor *curtextcol;
  CGColorRef curcgtextcol;
  int cur_text_color_int; // text color as int
  
  int curbkcol;
  int curbkmode;
  CGImageRef bitmapimagecache;
  bool bitmapimagecache_dirty;
  float lastpos_x,lastpos_y;
  
} GDP_CTX;

#endif // __OBJC__

#endif // SWELL_TARGET_OSX

#endif
