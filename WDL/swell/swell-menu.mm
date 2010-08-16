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
  

    This file provides basic windows menu API to interface an NSMenu

  */


#include "swell.h"


bool SetMenuItemText(HMENU hMenu, int idx, int flag, const char *text)
{
  NSMenu *menu=(NSMenu *)hMenu;
  
  NSMenuItem *item;
  if (flag & MF_BYPOSITION) item=[menu itemAtIndex:idx];
  else item =[menu itemWithTag:idx];
  if (!item) 
  {
    if (!(flag & MF_BYPOSITION))
    {
      int x;
      int n=[menu numberOfItems];
      for (x = 0; x < n; x ++)
      {
        item=[menu itemAtIndex:x];
        if (item && [item hasSubmenu])
        {
          NSMenu *m=[item submenu];
          if (m && SetMenuItemText(m,idx,flag,text)) return true;
        }
      }
    }
    return false;
  }
  NSString *label=(NSString *)CFStringCreateWithCString(NULL,text,kCFStringEncodingUTF8); 
  
  [item setTitle:label];
  
  [label release];
  return true;
}

bool EnableMenuItem(HMENU hMenu, int idx, int en)
{
  NSMenu *menu=(NSMenu *)hMenu;
  
  NSMenuItem *item;
  if (en & MF_BYPOSITION) item=[menu itemAtIndex:idx];
  else item =[menu itemWithTag:idx];
  if (!item) 
  {
    if (!(en & MF_BYPOSITION))
    {
      int x;
      int n=[menu numberOfItems];
      for (x = 0; x < n; x ++)
      {
        item=[menu itemAtIndex:x];
        if (item && [item hasSubmenu])
        {
          NSMenu *m=[item submenu];
          if (m && EnableMenuItem(m,idx,en)) return true;
        }
      }
    }
    return false;
  }
  [item setEnabled:((en&MF_GRAYED)?NO:YES)];
  return true;
}

bool CheckMenuItem(HMENU hMenu, int idx, int chk)
{
  NSMenu *menu=(NSMenu *)hMenu;
  
  NSMenuItem *item;
  if (chk & MF_BYPOSITION) item=[menu itemAtIndex:idx];
  else item =[menu itemWithTag:idx];
  if (!item) 
  {
    if (!(chk & MF_BYPOSITION))
    {
      int x;
      int n=[menu numberOfItems];
      for (x = 0; x < n; x ++)
      {
        item=[menu itemAtIndex:x];
        if (item && [item hasSubmenu])
        {
          NSMenu *m=[item submenu];
          if (m && CheckMenuItem(m,idx,chk)) return true;
        }
      }
    }
    return false;  
  }
  [item setState:((chk&MF_CHECKED)?NSOnState:NSOffState)];
  
  return true;
}

HMENU GetSubMenu(HMENU hMenu, int pos)
{
  NSMenu *menu=(NSMenu *)hMenu;
  
  NSMenuItem *item=[menu itemAtIndex:pos]; 
  if (item && [item hasSubmenu]) return [item submenu];
  return 0;
}

int GetMenuItemCount(HMENU hMenu)
{
  NSMenu *menu=(NSMenu *)hMenu;
  return [menu numberOfItems];
}

int GetMenuItemID(HMENU hMenu, int pos)
{
  NSMenu *menu=(NSMenu *)hMenu;
  
  NSMenuItem *item=[menu itemAtIndex:pos]; 
  if (item) 
  {
    if ([item hasSubmenu]) return -1;
    return [item tag];
  }
  return 0;
}

bool SetMenuItemModifier(HMENU hMenu, int idx, int flag, void *ModNSS, unsigned int mask)
{
  NSMenu *menu=(NSMenu *)hMenu;
  
  NSMenuItem *item;
  if (flag & MF_BYPOSITION) item=[menu itemAtIndex:idx];
  else item =[menu itemWithTag:idx];
  if (!item) 
  {
    if (!(flag & MF_BYPOSITION))
    {
      int x;
      int n=[menu numberOfItems];
      for (x = 0; x < n; x ++)
      {
        item=[menu itemAtIndex:x];
        if (item && [item hasSubmenu])
        {
          NSMenu *m=[item submenu];
          if (m && SetMenuItemModifier(m,idx,flag,ModNSS,mask)) return true;
        }
      }
    }
    return false;
  }
  if (ModNSS) [item setKeyEquivalent:(NSString *)ModNSS];
  [item setKeyEquivalentModifierMask:mask];
  return true;
}

HMENU CreatePopupMenu()
{
  NSMenu *m=[[NSMenu alloc] init];
  [m setAutoenablesItems:NO];

  return (HMENU)m;
}

void DestroyMenu(HMENU hMenu)
{
  if (hMenu)
  {
    NSMenu *m=(NSMenu *)hMenu;
    [m release];
  }
}

@interface menuItemDataHold : NSObject
{
  DWORD m_data;
}
-(id) initWithVal:(DWORD)val;
-(DWORD) getval;
@end
@implementation menuItemDataHold
-(id) initWithVal:(DWORD)val
{
  if ((self = [super init]))
  {
    m_data=val;
  }
  return self;
}
-(DWORD) getval
{
  return m_data;
}
@end

int AddMenuItem(HMENU hMenu, int pos, const char *name, int tagid)
{
  if (!hMenu) return -1;
  NSMenu *m=(NSMenu *)hMenu;
  NSString *label=(NSString *)CFStringCreateWithCString(NULL,name,kCFStringEncodingUTF8); 
  NSMenuItem *item=[m insertItemWithTitle:label action:NULL keyEquivalent:@"" atIndex:pos];
  [label release];
  [item setTag:tagid];
  [item setEnabled:YES];
  return 0;
}

void DeleteMenu(HMENU hMenu, int idx, int flag)
{
  if (!hMenu) return;
  NSMenu *m=(NSMenu *)hMenu;
  NSMenuItem *item;
  
  if (flag&MF_BYPOSITION)
  {
    item=[m itemAtIndex:idx];
  }
  else
  {
    item=[m itemWithTag:idx];
  }
  if (!item) return;
  [m removeItem:item];
}


BOOL SetMenuItemInfo(HMENU hMenu, int pos, BOOL byPos, MENUITEMINFO *mi)
{
  if (!hMenu) return 0;
  NSMenu *m=(NSMenu *)hMenu;
  NSMenuItem *item;
  if (byPos) item=[m itemAtIndex:pos];
  else item=[m itemWithTag:pos];

  if (!item) 
  {
    if (!byPos)
    {
      int x;
      int n=[m numberOfItems];
      for (x = 0; x < n; x ++)
      {
        item=[m itemAtIndex:x];
        if (item && [item hasSubmenu])
        {
          NSMenu *m1=[item submenu];
          if (m1 && SetMenuItemInfo(m1,pos,byPos,mi)) return true;
        }
      }      
    }
    return 0;
  }
  if (mi->fMask & MIIM_TYPE)
  {
    if (mi->fType == MFT_STRING && mi->dwTypeData)
    {
      NSString *label=(NSString *)CFStringCreateWithCString(NULL, mi->dwTypeData,kCFStringEncodingUTF8); 
      
      [item setTitle:label];
      
      [label release];      
    }
  }
  if (mi->fMask & MIIM_STATE)
  {
    [item setState:((mi->fState&MFS_CHECKED)?NSOnState:NSOffState)];
    [item setEnabled:((mi->fState&MFS_GRAYED)?NO:YES)];
  }
  if (mi->fMask & MIIM_ID)
  {
    [item setTag:mi->wID];
  }
  
  return true;
}

BOOL GetMenuItemInfo(HMENU hMenu, int pos, BOOL byPos, MENUITEMINFO *mi)
{
  if (!hMenu) return 0;
  NSMenu *m=(NSMenu *)hMenu;
  NSMenuItem *item;
  if (byPos)
  {
    item=[m itemAtIndex:pos];
  }
  else item=[m itemWithTag:pos];
  
  if (!item) 
  {
    if (!byPos)
    {
      int x;
      int n=[m numberOfItems];
      for (x = 0; x < n; x ++)
      {
        item=[m itemAtIndex:x];
        if (item && [item hasSubmenu])
        {
          NSMenu *m1=[item submenu];
          if (m1 && GetMenuItemInfo(m1,pos,byPos,mi)) return true;
        }
      }      
    }
    return 0;
  }
  
  if (mi->fMask & MIIM_TYPE)
  {
    if ([item isSeparatorItem]) mi->fType = MFT_SEPARATOR;
    else
    {
      mi->fType = MFT_STRING;
      if (mi->dwTypeData && mi->cch)
      {
        mi->dwTypeData[0]=0;
        [[item title] getCString:(char *)mi->dwTypeData maxLength:( mi->cch-1)];
        mi->dwTypeData[ mi->cch-1]=0;
      }
    }
  }
  
  if (mi->fMask & MIIM_DATA)
  {
    menuItemDataHold *h=[item representedObject];
    mi->dwItemData = (DWORD) (h && [h isKindOfClass:[menuItemDataHold class]]? [h getval] : 0);
  }
  if (mi->fMask & MIIM_STATE)
  {
    mi->fState=0;
    if ([item state]) mi->fState|=MFS_CHECKED;
    if (![item isEnabled]) mi->fState|=MFS_GRAYED;
  }
  return 1;
  
}

void InsertMenuItem(HMENU hMenu, int pos, BOOL byPos, MENUITEMINFO *mi)
{
  if (!hMenu) return;
  NSMenu *m=(NSMenu *)hMenu;
  NSMenuItem *item;
  int ni=[m numberOfItems];
  if (pos < 0 || pos > ni) pos=ni;
  
  if (mi->fType == MFT_STRING)
  {
    NSString *label=(NSString *)CFStringCreateWithCString(NULL,mi->dwTypeData?mi->dwTypeData:"(null)",kCFStringEncodingUTF8); 
    item=[m insertItemWithTitle:label action:NULL keyEquivalent:@"" atIndex:pos];
    [label release];
  }
  else if (mi->fType == MFT_BITMAP)
  {
    item=[m insertItemWithTitle:@"(no image)" action:NULL keyEquivalent:@"" atIndex:pos];
    if (mi->dwTypeData)
    {
      NSImage *i=(NSImage *)GetNSImageFromHICON(mi->dwTypeData);
      if (i)
      {
        [item setImage:i];
        [item setTitle:@""];
      }
    }
  }
  else
  {
    item = [NSMenuItem separatorItem];
    [m insertItem:item atIndex:pos];
  }
  if ((mi->fMask & MIIM_SUBMENU) && mi->hSubMenu)
  {
    [m setSubmenu:(NSMenu *)mi->hSubMenu forItem:item];
    [((NSMenu *)mi->hSubMenu) release]; // let the parent menu free it
  }
  [item setEnabled:YES];
  if (mi->fMask & MIIM_STATE)
  {
    if (mi->fState&MFS_GRAYED)
    {
      [item setEnabled:NO];
    }
    if (mi->fState&MFS_CHECKED)
    {
      [item setState:NSOnState];
    }
  }
  if (mi->fMask & MIIM_DATA)
  {
    menuItemDataHold *h=[[menuItemDataHold alloc] initWithVal:mi->dwItemData];
    [item setRepresentedObject:h];
    [h release];
  }
  else
    [item setRepresentedObject:nil];
 
  if (mi->fMask & MIIM_ID)
    [item setTag:mi->wID];
  
  NSMenuItem *fi=[m itemAtIndex:0];
  if (fi && fi != item)
  {
    if ([fi action]) [item setAction:[fi action]]; 
    if ([fi target]) [item setTarget:[fi target]]; 
  }
}


@interface PopupRecv : NSObject
{
  int m_act;
}
@end

@implementation PopupRecv
-(id) init
{
  if ((self = [super init]))
  {
    m_act=0;
  }
  return self;
}

-(void) onCommand:(id)sender
{
  int tag=[sender tag];
  if (tag)
    m_act=tag;
}

-(int) isCommand
{
  return m_act;
}

@end

static void PreprocessMenu(id recv, NSMenu *m)
{
  int x,n=[m numberOfItems];
  for (x = 0; x < n; x++)
  {
    NSMenuItem *item=[m itemAtIndex:x];
    if (item)
    {
      if ([item hasSubmenu])
      {
        NSMenu *mm=[item submenu];
        if (mm) PreprocessMenu(recv,mm);
      }
      else
      {
        [item setTarget:recv];
        [item setAction:@selector(onCommand:)];
      }
    }
  }
}

int SWELL_TrackPopupMenu(HMENU hMenu, int xpos, int ypos, HWND hwnd)
{
  if (hMenu)
  {
    NSMenu *m=(NSMenu *)hMenu;
    NSView *v=(NSView *)hwnd;
    if (!v) v=[[NSApp mainWindow] contentView];
    if (!v) return 0;
    
    PopupRecv *recv = [[PopupRecv alloc] init];
    
    PreprocessMenu(recv,m);
    
    
    [NSMenu popUpContextMenu:m withEvent:[NSApp currentEvent] forView:v];

    int ret=[recv isCommand];
    
    [PopupRecv release];
    return ret;
  }
  return 0;
}
