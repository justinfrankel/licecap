#ifndef _SWELL_INTERNAL_H_
#define _SWELL_INTERNAL_H_

#include "../ptrlist.h"

class SWELL_ListView_Row
{
public:
  SWELL_ListView_Row() : m_param(0), m_imageidx(0), m_tmp(0) { }
  ~SWELL_ListView_Row() { m_vals.Empty(true,free); }
  WDL_PtrList<char> m_vals;

  LPARAM m_param;
  int m_imageidx;
  int m_tmp; // Cocoa uses this temporarily, generic uses it as a mask (1= selected)
};

struct HTREEITEM__;

#ifdef SWELL_TARGET_OSX

#if 0
  // at some point we should enable this and use it in most SWELL APIs that call Cocoa code...
  #define SWELL_BEGIN_TRY @try { 
  #define SWELL_END_TRY(x) } @catch (NSException *ex) { NSLog(@"SWELL exception in %s:%d :: %@:%@\n",__FILE__,__LINE__,[ex name], [ex reason]); x }
#else
  #define SWELL_BEGIN_TRY
  #define SWELL_END_TRY(x)
#endif

#define __SWELL_PREFIX_CLASSNAME3(a,b) a##b
#define __SWELL_PREFIX_CLASSNAME2(a,b) __SWELL_PREFIX_CLASSNAME3(a,b)
#define __SWELL_PREFIX_CLASSNAME(cname) __SWELL_PREFIX_CLASSNAME2(SWELL_APP_PREFIX,cname)

// this defines interfaces to internal swell classes
#define SWELL_hwndChild __SWELL_PREFIX_CLASSNAME(_hwnd) 
#define SWELL_hwndCarbonHost __SWELL_PREFIX_CLASSNAME(_hwndcarbonhost)

#define SWELL_ModelessWindow __SWELL_PREFIX_CLASSNAME(_modelesswindow)
#define SWELL_ModalDialog __SWELL_PREFIX_CLASSNAME(_dialogbox)

#define SWELL_TextField __SWELL_PREFIX_CLASSNAME(_textfield)
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
#define SWELL_ListViewCell __SWELL_PREFIX_CLASSNAME(_listviewcell)
#define SWELL_ODListViewCell __SWELL_PREFIX_CLASSNAME(_ODlistviewcell)
#define SWELL_ODButtonCell __SWELL_PREFIX_CLASSNAME(_ODbuttoncell)
#define SWELL_ImageButtonCell __SWELL_PREFIX_CLASSNAME(_imgbuttoncell)

#define SWELL_FocusRectWnd __SWELL_PREFIX_CLASSNAME(_drawfocusrectwnd)

#define SWELL_DataHold __SWELL_PREFIX_CLASSNAME(_sdh)
#define SWELL_ThreadTmp __SWELL_PREFIX_CLASSNAME(_thread)
#define SWELL_PopupMenuRecv __SWELL_PREFIX_CLASSNAME(_trackpopupmenurecv)

#define SWELL_TimerFuncTarget __SWELL_PREFIX_CLASSNAME(_tft)


#define SWELL_Menu __SWELL_PREFIX_CLASSNAME(_menu)

#ifdef __OBJC__


#if MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_4
typedef int NSInteger;
typedef unsigned int NSUInteger;
#endif

@interface SWELL_Menu : NSMenu
{
}
-(void)dealloc;
- (id)copyWithZone:(NSZone *)zone;
@end

@interface SWELL_DataHold : NSObject
{
  void *m_data;
}
-(id) initWithVal:(void *)val;
-(void *) getValue;
@end

@interface SWELL_TimerFuncTarget : NSObject
{
  TIMERPROC m_cb;
  HWND m_hwnd;
  UINT_PTR m_timerid;
}
-(id) initWithId:(UINT_PTR)tid hwnd:(HWND)h callback:(TIMERPROC)cb;
-(void)SWELL_Timer:(id)sender;
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



@interface SWELL_TextField : NSTextField
- (void)setNeedsDisplay:(BOOL)flag;
- (void)setNeedsDisplayInRect:(NSRect)rect;
@end

@interface SWELL_TabView : NSTabView
{
  NSInteger m_tag;
  id m_dest;
}
@end

@interface SWELL_ProgressView : NSProgressIndicator
{
  NSInteger m_tag;
}
@end

@interface SWELL_ListViewCell : NSTextFieldCell
{
}
@end

@interface SWELL_StatusCell : NSTextFieldCell
{
  NSImage *status;
}
@end

@interface SWELL_TreeView : NSOutlineView
{
  @public
  bool m_fakerightmouse;
  LONG style;
  WDL_PtrList<HTREEITEM__> *m_items;
  NSColor *m_fgColor;
  NSMutableArray *m_selColors;
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
  int m_start_subitem;
  int m_start_item_clickmode;

  int m_lbMode;
  WDL_PtrList<SWELL_ListView_Row> *m_items;
  WDL_PtrList<NSTableColumn> *m_cols;
  WDL_PtrList<HGDIOBJ__> *m_status_imagelist;
  int m_status_imagelist_type;
  int m_fastClickMask;	
  NSColor *m_fgColor;
  NSMutableArray *m_selColors;

  // these are for the new yosemite mouse handling code
  int m_last_plainly_clicked_item, m_last_shift_clicked_item;

}
-(LONG)getSwellStyle;
-(void)setSwellStyle:(LONG)st;
-(int)getSwellNotificationMode;
-(void)setSwellNotificationMode:(int)lbMode;
-(NSInteger)columnAtPoint:(NSPoint)pt;
-(int)getColumnPos:(int)idx; // get current position of column that was originally at idx
-(int)getColumnIdx:(int)pos; // get original index of column that is currently at position
@end

@interface SWELL_ImageButtonCell : NSButtonCell
{
}
- (NSRect)drawTitle:(NSAttributedString *)title withFrame:(NSRect)frame inView:(NSView *)controlView;
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
  LONG_PTR m_userdata;
  int m_radioflags;
}
-(int)swellGetRadioFlags;
-(void)swellSetRadioFlags:(int)f;
-(LONG_PTR)getSwellUserData;
-(void)setSwellUserData:(LONG_PTR)val;
-(void)setSwellGDIImage:(void *)par;
-(void *)getSwellGDIImage;
@end

@interface SWELL_TextView : NSTextView
{
  NSInteger m_tag;
}
-(NSInteger) tag;
-(void) setTag:(NSInteger)tag;
@end

@interface SWELL_BoxView : NSBox
{
  NSInteger m_tag;
}
-(NSInteger) tag;
-(void) setTag:(NSInteger)tag;
@end

@interface SWELL_FocusRectWnd : NSView
{
}
@end

@interface SWELL_ThreadTmp : NSObject
{
@public
  void *a, *b;
}
-(void)bla:(id)obj;
@end



@interface SWELL_hwndChild : NSView // <NSDraggingSource>
{
@public
  int m_enabled; // -1 if preventing focus
  DLGPROC m_dlgproc;
  WNDPROC m_wndproc;
  LONG_PTR m_userdata;
  LONG_PTR m_extradata[32];
  NSInteger m_tag;
  int m_isfakerightmouse;
  char m_hashaddestroy; // 2 = WM_DESTROY has finished completely
  HMENU m_menu;
  BOOL m_flip;
  bool m_supports_ddrop;
  bool m_paintctx_used;
  HDC m_paintctx_hdc;
  WindowPropRec *m_props;
  NSRect m_paintctx_rect;
  BOOL m_isopaque;
  char m_titlestr[1024];
  unsigned int m_create_windowflags;
  NSOpenGLContext *m_glctx;
  char m_isdirty; // &1=self needs redraw, &2=children may need redraw
  char m_allow_nomiddleman;
  id m_lastTopLevelOwner; // save a copy of the owner, if any
  id m_access_cacheptrs[6];
}
- (id)initChild:(SWELL_DialogResourceIndex *)resstate Parent:(NSView *)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par;
- (LRESULT)onSwellMessage:(UINT)msg p1:(WPARAM)wParam p2:(LPARAM)lParam;
-(HANDLE)swellExtendedDragOp:(id <NSDraggingInfo>)sender retGlob:(BOOL)retG;
- (const char *)onSwellGetText;
-(void)onSwellSetText:(const char *)buf;
-(LONG)swellGetExtendedStyle;
-(void)swellSetExtendedStyle:(LONG)st;
-(HMENU)swellGetMenu;
-(BOOL)swellHasBeenDestroyed;
-(void)swellSetMenu:(HMENU)menu;
-(LONG_PTR)getSwellUserData;
-(void)setSwellUserData:(LONG_PTR)val;
-(void)setOpaque:(bool)isOpaque;
-(LPARAM)getSwellExtraData:(int)idx;
-(void)setSwellExtraData:(int)idx value:(LPARAM)val;
-(void)setSwellWindowProc:(WNDPROC)val;
-(WNDPROC)getSwellWindowProc;
-(void)setSwellDialogProc:(DLGPROC)val;
-(DLGPROC)getSwellDialogProc;

- (NSArray*) namesOfPromisedFilesDroppedAtDestination:(NSURL*)droplocation;

-(void) getSwellPaintInfo:(PAINTSTRUCT *)ps;
- (int)swellCapChangeNotify;
-(unsigned int)swellCreateWindowFlags;

-(bool)swellCanPostMessage;
-(int)swellEnumProps:(PROPENUMPROCEX)proc lp:(LPARAM)lParam;
-(void *)swellGetProp:(const char *)name wantRemove:(BOOL)rem;
-(int)swellSetProp:(const char *)name value:(void *)val ;


// NSAccessibility

// attribute methods
//- (NSArray *)accessibilityAttributeNames;
- (id)accessibilityAttributeValue:(NSString *)attribute;
//- (BOOL)accessibilityIsAttributeSettable:(NSString *)attribute;
//- (void)accessibilitySetValue:(id)value forAttribute:(NSString *)attribute;

// parameterized attribute methods
//- (NSArray *)accessibilityParameterizedAttributeNames;
//- (id)accessibilityAttributeValue:(NSString *)attribute forParameter:(id)parameter;

// action methods
//- (NSArray *)accessibilityActionNames;
//- (NSString *)accessibilityActionDescription:(NSString *)action;
//- (void)accessibilityPerformAction:(NSString *)action;

// Return YES if the UIElement doesn't show up to the outside world - i.e. its parent should return the UIElement's children as its own - cutting the UIElement out. E.g. NSControls are ignored when they are single-celled.
- (BOOL)accessibilityIsIgnored;

// Returns the deepest descendant of the UIElement hierarchy that contains the point. You can assume the point has already been determined to lie within the receiver. Override this method to do deeper hit testing within a UIElement - e.g. a NSMatrix would test its cells. The point is bottom-left relative screen coordinates.
- (id)accessibilityHitTest:(NSPoint)point;

// Returns the UI Element that has the focus. You can assume that the search for the focus has already been narrowed down to the reciever. Override this method to do a deeper search with a UIElement - e.g. a NSMatrix would determine if one of its cells has the focus.
- (id)accessibilityFocusedUIElement;




@end

@interface SWELL_ModelessWindow : NSWindow
{
@public
  NSSize lastFrameSize;
  id m_owner;
  OwnedWindowListRec *m_ownedwnds;
  BOOL m_enabled;
  int m_wantraiseamt;
  bool  m_wantInitialKeyWindowOnShow;
}
- (id)initModeless:(SWELL_DialogResourceIndex *)resstate Parent:(HWND)parent dlgProc:(DLGPROC)dlgproc Param:(LPARAM)par outputHwnd:(HWND *)hwndOut forceStyles:(unsigned int)smask;
- (id)initModelessForChild:(HWND)child owner:(HWND)owner styleMask:(unsigned int)smask;
- (void)swellDestroyAllOwnedWindows;
- (void)swellRemoveOwnedWindow:(NSWindow *)wnd;
- (void)swellSetOwner:(id)owner;
- (id)swellGetOwner;
- (void **)swellGetOwnerWindowHead;
-(void)swellDoDestroyStuff;
-(void)swellResetOwnedWindowLevels;
@end

@interface SWELL_ModalDialog : NSPanel
{
  NSSize lastFrameSize;
  id m_owner;
  OwnedWindowListRec *m_ownedwnds;
  
  int m_rv;
  bool m_hasrv;
  BOOL m_enabled;
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
-(bool)swellHasModalRetVal;
@end


@interface SWELL_hwndCarbonHost : SWELL_hwndChild
#ifdef MAC_OS_X_VERSION_10_7
<NSWindowDelegate>
#endif
{
@public
  NSWindow *m_cwnd;

  bool m_whileresizing;
  void* m_wndhandler;   // won't compile if declared EventHandlerRef, wtf
  void* m_ctlhandler;   // not sure if these need to be separate but cant hurt  
  bool m_wantallkeys;
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

#ifndef __AVAILABILITYMACROS__
#error  __AVAILABILITYMACROS__ not defined, include AvailabilityMacros.h!
#endif

// 10.4 doesn't support CoreText, so allow ATSUI if targetting 10.4 SDK
#ifndef MAC_OS_X_VERSION_10_5
  // 10.4 SDK
  #define SWELL_NO_CORETEXT
  #define SWELL_ATSUI_TEXT_SUPPORT
#else

#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5
#ifndef __LP64__
#define SWELL_ATSUI_TEXT_SUPPORT
#endif
#endif

#endif

struct HGDIOBJ__
{
  int type;

  int additional_refcnt; // refcnt of 0 means one owner (if >0, additional owners)
  
  // used by pen/brush
  CGColorRef color;
  int wid;
  NSImage *bitmapptr;  
  
  NSMutableDictionary *__old_fontdict; // unused, for ABI compat
  //
  // if ATSUI used, meaning IsCoreTextSupported() returned false
  ATSUStyle atsui_font_style;

  float font_rotation;

  bool _infreelist;
  struct HGDIOBJ__ *_next;
 
  // if using CoreText to draw text
  void *ct_FontRef;
};

struct HDC__ {
  CGContextRef ctx; 
  void *ownedData; // always use via SWELL_GetContextFrameBuffer() (which performs necessary alignment)
  HGDIOBJ__ *curpen;
  HGDIOBJ__ *curbrush;
  HGDIOBJ__ *curfont;
  
  NSColor *__old_nstextcol; // provided for ABI compat, but unused
  int cur_text_color_int; // text color as int
  
  int curbkcol;
  int curbkmode;
  float lastpos_x,lastpos_y;
  
  void *GLgfxctx; // optionally set
  bool _infreelist;
  struct HDC__ *_next;

  CGColorRef curtextcol; // text color as CGColor
};





// some extras so we can call functions available only on some OSX versions without warnings, and with the correct types
#define SWELL_DelegateExtensions __SWELL_PREFIX_CLASSNAME(_delext)
#define SWELL_ViewExtensions __SWELL_PREFIX_CLASSNAME(_viewext)
#define SWELL_AppExtensions __SWELL_PREFIX_CLASSNAME(_appext)
#define SWELL_WindowExtensions __SWELL_PREFIX_CLASSNAME(_wndext)
#define SWELL_TableColumnExtensions __SWELL_PREFIX_CLASSNAME(_tcolext)

@interface SWELL_WindowExtensions : NSWindow
-(void)setCollectionBehavior:(NSUInteger)a;
@end
@interface SWELL_ViewExtensions : NSView
-(void)_recursiveDisplayRectIfNeededIgnoringOpacity:(NSRect)rect isVisibleRect:(BOOL)vr rectIsVisibleRectForView:(NSView*)v topView:(NSView *)v2;
@end

@interface SWELL_DelegateExtensions : NSObject
-(bool)swellPostMessage:(HWND)dest msg:(int)message wp:(WPARAM)wParam lp:(LPARAM)lParam;
-(void)swellPostMessageClearQ:(HWND)dest;
-(void)swellPostMessageTick:(id)sender;
@end

@interface SWELL_AppExtensions : NSApplication
-(NSUInteger)presentationOptions;
-(void)setPresentationOptions:(NSUInteger)o;
@end
@interface SWELL_TableColumnExtensions : NSTableColumn
-(BOOL)isHidden;
-(void)setHidden:(BOOL)h;
@end




#else
  // compat when compiling targetting OSX but not in objectiveC mode
  struct SWELL_DataHold;
#endif // !__OBJC__

// 10.4 sdk just uses "float"
#if MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_4
  #ifdef __LP64__
    typedef double CGFloat;
  #else
    typedef float CGFloat;
#endif

#endif


#elif defined(SWELL_TARGET_GDK)

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>


#else
// generic 

#endif // end generic

struct HTREEITEM__
{
  HTREEITEM__();
  ~HTREEITEM__();
  bool FindItem(HTREEITEM it, HTREEITEM__ **parOut, int *idxOut);
  
#ifdef SWELL_TARGET_OSX
  SWELL_DataHold *m_dh;
#else
  int m_state; // TVIS_EXPANDED, for ex
#endif
  
  bool m_haschildren;
  char *m_value;
  WDL_PtrList<HTREEITEM__> m_children; // only used in tree mode
  LPARAM m_param;
};



#ifndef SWELL_TARGET_OSX 

#include "../wdlstring.h"

#ifdef SWELL_LICE_GDI
#include "../lice/lice.h"
#endif
#include "../assocarray.h"

#define SWELL_INTERNAL_MENUBAR_SIZE 12

LRESULT SwellDialogDefaultWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct HWND__
{
  HWND__(HWND par, int wID=0, RECT *wndr=NULL, const char *label=NULL, bool visible=false, WNDPROC wndproc=NULL, DLGPROC dlgproc=NULL, HWND ownerWindow=NULL);
  ~HWND__(); // DO NOT USE!!! We would make this private but it breaks PtrList using it on gcc. 

  // using this API prevents the HWND from being valid -- it'll still get its resources destroyed via DestroyWindow() though.
  // DestroyWindow() does cleanup, then the final Release().
  void Retain() { m_refcnt++; }
  void Release() { if (!--m_refcnt) delete this; }
 


 
  const char *m_classname;
  

#ifdef SWELL_TARGET_GDK
  GdkWindow *m_oswindow;
#endif
  WDL_FastString m_title;

  HWND__ *m_children, *m_parent, *m_next, *m_prev;
  HWND__ *m_owner, *m_owned_list, *m_owned_next, *m_owned_prev;
  RECT m_position;
  UINT m_id;
  int m_style, m_exstyle;
  INT_PTR m_userdata;
  WNDPROC m_wndproc;
  DLGPROC m_dlgproc;
  INT_PTR m_extra[64];
  INT_PTR m_private_data; // used by internal controls

  bool m_visible;
  char m_hashaddestroy; // 1 in destroy, 2 full destroy
  bool m_enabled;
  bool m_wantfocus;

  int m_refcnt; 

  HMENU m_menu;

  WDL_StringKeyedArray<void *> m_props;

#ifdef SWELL_LICE_GDI
  void *m_paintctx; // temporarily set for calls to WM_PAINT

// todo:
  bool m_child_invalidated; // if a child is invalidated
  bool m_invalidated; // set to true on direct invalidate. todo RECT instead?

  LICE_IBitmap *m_backingstore; // if NULL, unused (probably only should use on top level windows, but support caching?)
#endif
};

struct HMENU__
{
  HMENU__() { sel_vis = -1; }
  ~HMENU__() { items.Empty(true,freeMenuItem); }

  WDL_PtrList<MENUITEMINFO> items;
  int sel_vis; // for mouse/keyboard nav

  HMENU__ *Duplicate();
  static void freeMenuItem(void *p);

};


struct HGDIOBJ__
{
  int type;
  int additional_refcnt; // refcnt of 0 means one owner (if >0, additional owners)

  int color;
  int wid;

  float alpha;

  struct HGDIOBJ__ *_next;
  bool _infreelist;
  void *typedata; // font: FT_Face, bitmap: LICE_IBitmap
};


struct HDC__ {
#ifdef SWELL_LICE_GDI
  LICE_IBitmap *surface; // owned by context. can be (and usually is, if clipping is desired) LICE_SubBitmap
  POINT surface_offs; // offset drawing into surface by this amount

  RECT dirty_rect; // in surface coordinates, used for GetWindowDC()/GetDC()/etc
  bool dirty_rect_valid;
#else
  void *ownedData; // for mem contexts, support a null rendering 
#endif

  HGDIOBJ__ *curpen;
  HGDIOBJ__ *curbrush;
  HGDIOBJ__ *curfont;
  
  int cur_text_color_int; // text color as int
  
  int curbkcol;
  int curbkmode;
  float lastpos_x,lastpos_y;
  
  struct HDC__ *_next;
  bool _infreelist;
};

#endif // !OSX

HDC SWELL_CreateGfxContext(void *);

// GDP internals
#define TYPE_PEN 1
#define TYPE_BRUSH 2
#define TYPE_FONT 3
#define TYPE_BITMAP 4

typedef struct
{
  void *instptr; 
  void *bundleinstptr;
  int refcnt;

  int (*SWELL_dllMain)(HINSTANCE, DWORD,LPVOID); //last parm=SWELLAPI_GetFunc
  BOOL (*dllMain)(HINSTANCE, DWORD, LPVOID);
  void *lastSymbolRequested;
} SWELL_HINSTANCE;


enum
{
  INTERNAL_OBJECT_START= 0x1000001,
  INTERNAL_OBJECT_THREAD,
  INTERNAL_OBJECT_EVENT,
  INTERNAL_OBJECT_FILE,
  INTERNAL_OBJECT_EXTERNALSOCKET, // socket not owned by us
  INTERNAL_OBJECT_SOCKETEVENT,
  INTERNAL_OBJECT_NSTASK, 
  INTERNAL_OBJECT_END
};

typedef struct
{
   int type; // INTERNAL_OBJECT_*
   int count; // reference count
} SWELL_InternalObjectHeader;

typedef struct
{
  SWELL_InternalObjectHeader hdr;
  DWORD (*threadProc)(LPVOID);
  void *threadParm;
  pthread_t pt;
  DWORD retv;
  bool done;
} SWELL_InternalObjectHeader_Thread;

typedef struct
{
  SWELL_InternalObjectHeader hdr;
  
  pthread_mutex_t mutex;
  pthread_cond_t cond;

  bool isSignal;
  bool isManualReset;
  
} SWELL_InternalObjectHeader_Event;


// used for both INTERNAL_OBJECT_EXTERNALSOCKET and INTERNAL_OBJECT_SOCKETEVENT. 
// if EXTERNALSOCKET, socket[1] ignored and autoReset ignored.
typedef struct
{
  SWELL_InternalObjectHeader hdr;
  int socket[2]; 
  bool autoReset;
} SWELL_InternalObjectHeader_SocketEvent;
 
typedef struct
{
  SWELL_InternalObjectHeader hdr;
  
  FILE *fp;
} SWELL_InternalObjectHeader_File;

typedef struct
{
  SWELL_InternalObjectHeader hdr;
  void *task; 
} SWELL_InternalObjectHeader_NSTask;


bool IsRightClickEmulateEnabled();

#ifdef SWELL_INTERNAL_HTREEITEM_IMPL

HTREEITEM__::HTREEITEM__()
{
  m_param=0;
  m_value=0;
  m_haschildren=false;
#ifdef SWELL_TARGET_OSX
  m_dh = [[SWELL_DataHold alloc] initWithVal:this];
#else
  m_state=0;
#endif
}
HTREEITEM__::~HTREEITEM__()
{
  free(m_value);
  m_children.Empty(true);
#ifdef SWELL_TARGET_OSX
  [m_dh release];
#endif
}


bool HTREEITEM__::FindItem(HTREEITEM it, HTREEITEM__ **parOut, int *idxOut)
{
  int a=m_children.Find((HTREEITEM__*)it);
  if (a>=0)
  {
    if (parOut) *parOut=this;
    if (idxOut) *idxOut=a;
    return true;
  }
  int x;
  const int n=m_children.GetSize();
  for (x = 0; x < n; x ++)
  {
    if (m_children.Get(x)->FindItem(it,parOut,idxOut)) return true;
  }
  return false;
}

#endif

#endif
