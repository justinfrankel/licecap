#ifndef _SWELL_MENUGEN_H_
#define _SWELL_MENUGEN_H_


/**
  SWELL - (Simple Windows Emulation Layer Library)
  Copyright (C) 2006 and later, Cockos Incorporated.
  Dynamic menu generation

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


  Usage:

  #include "swell-menugen.h"
  #include "resource.h"
 
  ...


  HMENU hMenu;

  SWELL_MENUGEN_BEGIN(hMenu)

     // paste in your .rc here


    MENUITEM "Always on top",               ID_ALWAYS_ON_TOP
    MENUITEM "Fullscreen",                  ID_FULLSCREEN

    POPUP "&Help"
    BEGIN
        POPUP "Documentation"
        BEGIN
            MENUITEM "No documentation files found", ID_NONE, GRAYED
        END
        MENUITEM "&Keyboard Shortcuts...",      ID_KB_HELP
        MENUITEM SEPARATOR
        MENUITEM "&About...",                   ID_HELP_ABOUT
    END


  SWELL_MENUGEN_END()


  TrackPopupMenu(hMenu , ...) // or whatever you want with this

  DestroyMenu(hMenu);

  */


#include "swell.h"
#include "../ptrlist.h"

// use these, without semicolons after. menu should be uninitialized before, and will be the menu after
#define SWELL_MENUGEN_BEGIN(menu) { WDL_PtrList<void> mstack;   mstack.Add((menu)=CreatePopupMenu()
#define SWELL_MENUGEN_END() ); }


static void __filtnametobuf(char *out, const char *in, int outsz)
{
  while (*in && outsz>1)
  {
    if (*in == '&')
    {
      in++;
    }
    *out++=*in++;
    outsz--;
  }
  *out=0;
}

static void __AddPopup(WDL_PtrList<void> *stack, const char *name)
{
  HMENU subMenu=CreatePopupMenu();
  
  char buf[1024];
  __filtnametobuf(buf,name,sizeof(buf));
  
  MENUITEMINFO mi={sizeof(mi),MIIM_SUBMENU|MIIM_STATE|MIIM_TYPE,MFT_STRING,
    0,0,NULL,NULL,NULL,0,(char *)buf};
  mi.hSubMenu=subMenu;
  HMENU hMenu=stack->Get(stack->GetSize()-1);
  InsertMenuItem(hMenu,GetMenuItemCount(hMenu),TRUE,&mi);
  stack->Add(subMenu);
}

static void __AddMenuItem(HMENU hMenu, const char *name, int idx, int flags=0)
{
  char buf[1024];
  if (name) __filtnametobuf(buf,name,sizeof(buf));
  MENUITEMINFO mi={sizeof(mi),MIIM_ID|MIIM_STATE|MIIM_TYPE,MFT_STRING,
    (flags)?MFS_GRAYED:0,idx,NULL,NULL,NULL,0,(char *)buf};
  if (!name)
  {
    mi.fType = MFT_SEPARATOR;
    mi.fMask&=~(MIIM_STATE|MIIM_ID);
  }
  InsertMenuItem(hMenu,GetMenuItemCount(hMenu),TRUE,&mi);
}

#define GRAYED 1
#define INACTIVE 2
#define POPUP ); __AddPopup(&mstack, 
#define MENUITEM ); __AddMenuItem(mstack.Get(mstack.GetSize()-1), 
#define SEPARATOR NULL, -1
#define BEGIN
#define END ); mstack.Delete(mstack.GetSize()-1
                             
#endif//_SWELL_MENUGEN_H_
                             