#ifndef _WDL_WIN32_HELPERS_H_
#define _WDL_WIN32_HELPERS_H_

// helpers to make some tedious APIs a little helper

static HMENU InsertSubMenu(HMENU hMenu, int pos, const char *name, int flags=0)
{
  if (!hMenu) return NULL;
  MENUITEMINFO mii = { sizeof(mii), MIIM_STATE|MIIM_TYPE|MIIM_SUBMENU, MFT_STRING, flags, 0, CreatePopupMenu(), NULL, NULL, 0, (char*)name };
  InsertMenuItem(hMenu,pos,TRUE,&mii);
  return mii.hSubMenu;
}

static void InsertMenuString(HMENU hMenu, int pos, const char *name, int idx, int flags=0)
{
  MENUITEMINFO mii = { sizeof(mii), MIIM_STATE|MIIM_TYPE|MIIM_ID, MFT_STRING, flags, idx, NULL, NULL, NULL, 0, (char*)name };
  if (hMenu) InsertMenuItem(hMenu,pos,TRUE,&mii);
}

static void InsertMenuSeparator(HMENU hMenu, int pos)
{
  MENUITEMINFO mii = { sizeof(mii), MIIM_TYPE, MFT_SEPARATOR, };
  if (hMenu) InsertMenuItem(hMenu,pos,TRUE,&mii);
}

#endif
