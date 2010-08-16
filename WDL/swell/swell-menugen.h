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


#ifdef BEGIN
#undef BEGIN
#endif

#ifdef END
#undef END
#endif


typedef struct SWELL_MenuResourceIndex
{
  int resid;
  void (*createFunc)(HMENU hMenu);
  struct SWELL_MenuResourceIndex *_next;
} SWELL_MenuResourceIndex;
extern SWELL_MenuResourceIndex *SWELL_curmodule_menuresource_head;


#define SWELL_DEFINE_MENU_RESOURCE_BEGIN(recid) \
class NewCustomMenuResource_##recid { \
public: \
  SWELL_MenuResourceIndex m_rec; \
  NewCustomMenuResource_##recid() {  m_rec.resid=recid; m_rec.createFunc=cf; m_rec._next=SWELL_curmodule_menuresource_head; SWELL_curmodule_menuresource_head=&m_rec; } \
  static void cf(HMENU hMenu) { WDL_PtrList<char> mstack; mstack.Add((char *)(hMenu)
        
#define SWELL_DEFINE_MENU_RESOURCE_END(recid) ); } }; static NewCustomMenuResource_##recid NewCustomMenuResourceInst_##recid;     




// internal menu gen stuff


// use these, without semicolons after. menu should be uninitialized before, and will be the menu after
#define SWELL_MENUGEN_BEGIN(menu) { WDL_PtrList<char> mstack;   mstack.Add((char*)(menu)=CreatePopupMenu()
#define SWELL_MENUGEN_END() ); }

static void __SWELL_Menu_AddMenuItem(HMENU hMenu, const char *name, int idx, int flags=0) { return SWELL_Menu_AddMenuItem(hMenu,name,idx,flags); }

// we inline this here so that if WDL_PtrList is differenet across modules we dont have any issues
static void __SWELL_MenuGen_AddPopup(WDL_PtrList<char> *stack, const char *name)
{  
  MENUITEMINFO mi={sizeof(mi),MIIM_SUBMENU|MIIM_STATE|MIIM_TYPE,MFT_STRING,0,0,CreatePopupMenuEx(name),NULL,NULL,0,(char *)name};
  HMENU hMenu=stack->Get(stack->GetSize()-1); stack->Add((char *)mi.hSubMenu);
  InsertMenuItem(hMenu,GetMenuItemCount(hMenu),TRUE,&mi);
}

#define GRAYED 1
#define INACTIVE 2
#define POPUP ); __SWELL_MenuGen_AddPopup(&mstack, 
#define MENUITEM ); __SWELL_Menu_AddMenuItem(mstack.Get(mstack.GetSize()-1), 
#define SEPARATOR NULL, -1
#define BEGIN
#define END ); mstack.Delete(mstack.GetSize()-1
                             
#endif//_SWELL_MENUGEN_H_
                             
