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
  

    This file provides basic APIs for browsing for files, directories, and messageboxes.

    These APIs don't all match the Windows equivelents, but are close enough to make it not too much trouble.

  */


#ifndef SWELL_PROVIDED_BY_APP

#include "swell.h"
#include "swell-internal.h"
#include "swell-dlggen.h"

#include "../wdlcstring.h"
#include "../assocarray.h"
#include <dirent.h>
#include <time.h>

static const char *BFSF_Templ_dlgid;
static DLGPROC BFSF_Templ_dlgproc;
static struct SWELL_DialogResourceIndex *BFSF_Templ_reshead;
void BrowseFile_SetTemplate(const char *dlgid, DLGPROC dlgProc, struct SWELL_DialogResourceIndex *reshead)
{
  BFSF_Templ_reshead=reshead;
  BFSF_Templ_dlgid=dlgid;
  BFSF_Templ_dlgproc=dlgProc;
}

class BrowseFile_State
{
public:
  enum modeEnum { SAVE=0,OPEN, OPENMULTI, OPENDIR };

  BrowseFile_State(const char *_cap, const char *_idir, const char *_ifile, const char *_el, modeEnum _mode, char *_fnout, int _fnout_sz) :
    caption(_cap), initialdir(_idir), initialfile(_ifile), extlist(_el), mode(_mode), fnout(_fnout), fnout_sz(_fnout_sz), viewlist(true)
  {
  }
  ~BrowseFile_State()
  {
  }

  const char *caption;
  const char *initialdir;
  const char *initialfile;
  const char *extlist;

  modeEnum mode;
  char *fnout; // if NULL this will be malloced by the window
  int fnout_sz;

  struct rec {
    WDL_INT64 size;
    time_t date;
  };

  WDL_StringKeyedArray2<rec> viewlist; // first byte of string will define type -- 1 = directory, 2= file


  void scan_path(const char *path, const char *filterlist, bool dir_only)
  {
    viewlist.DeleteAll();
    DIR *dir = opendir(path);
    if (!dir) return;
    char tmp[2048];
    struct dirent *ent;
    while (NULL != (ent = readdir(dir)))
    {
      if (ent->d_name[0] == '.') continue;
      bool is_dir = (ent->d_type == DT_DIR);

      if (ent->d_type == DT_LNK)
      {
        snprintf(tmp,sizeof(tmp),"%s/%s",path,ent->d_name);
        char *rp = realpath(tmp,NULL);
        if (rp)
        {
          DIR *d = opendir(rp);
          if (d) { is_dir = true; closedir(d); }
          free(rp);
        }
      }
      if (!dir_only || is_dir)
      {
        if (filterlist && *filterlist && !is_dir)
        {
          const char *f = filterlist;
          while (*f)
          {
            const char *nf = f;
            while (*nf && *nf != ';') nf++;
            if (*f != '*')
            {
              const char *nw = f;
              while (nw < nf && *nw != '*') nw++;

              if ((nw!=nf || strlen(ent->d_name) == nw-f) && !strncasecmp(ent->d_name,f,nw-f)) 
              {
                // matched leading text
                if (nw == nf) break;
                f = nw;
              }
            }

            if (*f == '*')
            {
              f++;
              if (!*f || *f == ';' || (*f == '.' && f[1] == '*')) break;
              size_t l = strlen(ent->d_name);
              if (l > nf-f && !strncasecmp(ent->d_name + l - (nf-f), f,nf-f)) break;
            }
            f = nf;
            while (*f == ';') f++;
          }
          if (!*f) continue; // did not match
        }
        snprintf(tmp,sizeof(tmp),"%s/%s",path,ent->d_name);
        struct stat64 st={0,};
        stat64(tmp,&st);
      
        rec r = { st.st_size, st.st_mtime } ;
        tmp[0] = is_dir?1:2;
        lstrcpyn_safe(tmp+1,ent->d_name,sizeof(tmp)-1);
        viewlist.AddUnsorted(tmp,r);
      }
    }
    viewlist.Resort();

    closedir(dir);
  }
};

static LRESULT WINAPI swellFileSelectProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  const int maxPathLen = 2048;
  const char *multiple_files = "(multiple files)";
  switch (uMsg)
  {
    case WM_CREATE:
      if (lParam)  // swell-specific
      {
        SetWindowLong(hwnd,GWL_WNDPROC,(LPARAM)SwellDialogDefaultWindowProc);
        SetWindowLong(hwnd,DWL_DLGPROC,(LPARAM)swellFileSelectProc);
        SetWindowLongPtr(hwnd,GWLP_USERDATA,lParam);
        BrowseFile_State *parms = (BrowseFile_State *)lParam;
        if (parms->caption) SetWindowText(hwnd,parms->caption);

        SWELL_MakeSetCurParms(1,1,0,0,hwnd,false,false);

        SWELL_MakeButton(0,
              parms->mode == BrowseFile_State::OPENDIR ? "Choose directory" :
              parms->mode == BrowseFile_State::SAVE ? "Save" : "Open",
              IDOK,0,0,0,0, 0);

        SWELL_MakeButton(0, "Cancel", IDCANCEL,0,0,0,0, 0);
        HWND edit = SWELL_MakeEditField(0x100, 0,0,0,0,  0);
        HWND dir = SWELL_MakeCombo(0x103, 0,0,0,0, CBS_DROPDOWNLIST);

        HWND list = SWELL_MakeControl("",0x104,"SysListView32",LVS_REPORT|LVS_SHOWSELALWAYS|
              (parms->mode == BrowseFile_State::OPENMULTI ? 0 : LVS_SINGLESEL)|
              LVS_OWNERDATA|WS_BORDER|WS_TABSTOP,0,0,0,0,0);
        if (list)
        {
          LVCOLUMN c={LVCF_TEXT|LVCF_WIDTH, 0, 280, (char*)"Filename" };
          ListView_InsertColumn(list,0,&c);
          c.cx = 120;
          c.pszText = (char*) "Size";
          ListView_InsertColumn(list,1,&c);
          c.cx = 140;
          c.pszText = (char*) "Date";
          ListView_InsertColumn(list,2,&c);
        }
        HWND extlist = (parms->extlist && *parms->extlist) ? SWELL_MakeCombo(0x105, 0,0,0,0, CBS_DROPDOWNLIST) : NULL;
        if (extlist)
        {
          const char *p = parms->extlist;
          while (*p)
          {
            const char *rd=p;
            p += strlen(p)+1;
            if (!*p) break;
            int a = SendMessage(extlist,CB_ADDSTRING,0,(LPARAM)rd);
            SendMessage(extlist,CB_SETITEMDATA,a,(LPARAM)p);
            p += strlen(p)+1;
          }
          SendMessage(extlist,CB_SETCURSEL,0,0);
        }

        SWELL_MakeLabel(-1,parms->mode == BrowseFile_State::OPENDIR ? "Directory: " : "File:",0x101, 0,0,0,0, 0); 
        
        if (BFSF_Templ_dlgid && BFSF_Templ_dlgproc)
        {
          HWND dlg = SWELL_CreateDialog(BFSF_Templ_reshead, BFSF_Templ_dlgid, hwnd, BFSF_Templ_dlgproc, 0);
          if (dlg) SetWindowLong(dlg,GWL_ID,0x102);
          BFSF_Templ_dlgproc=0;
          BFSF_Templ_dlgid=0;
        }

        SWELL_MakeSetCurParms(1,1,0,0,NULL,false,false);

        if (edit && dir)
        {
          char buf[maxPathLen];
          const char *filepart = "";
          if (parms->initialfile && *parms->initialfile && *parms->initialfile != '.') 
          { 
            lstrcpyn_safe(buf,parms->initialfile,sizeof(buf));
            char *p = (char *)WDL_get_filepart(buf);
            if (p > buf) { p[-1]=0; filepart = p; }
          }
          else if (parms->initialdir && *parms->initialdir) 
          {
            lstrcpyn_safe(buf,parms->initialdir,sizeof(buf));
          }
          else getcwd(buf,sizeof(buf));

          SetWindowText(edit,filepart);
          SendMessage(hwnd, WM_USER+100, 0x103, (LPARAM)buf);
        }

        SetWindowPos(hwnd,NULL,0,0,600, 400, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
        SendMessage(hwnd,WM_USER+100,1,0); // refresh list
      }
    break;
    case WM_USER+100:
      switch (wParam)
      {
        case 0x103: // update directory combo box -- destroys buffer pointed to by lParam
          if (lParam)
          {
            char *path = (char*)lParam;
            HWND combo=GetDlgItem(hwnd,0x103);
            SendMessage(combo,CB_RESETCONTENT,0,0);
            WDL_remove_trailing_dirchars(path);
            while (path[0]) 
            {
              SendMessage(combo,CB_ADDSTRING,0,(LPARAM)path);
              WDL_remove_filepart(path);
              WDL_remove_trailing_dirchars(path);
            }
            SendMessage(combo,CB_ADDSTRING,0,(LPARAM)"/");
            SendMessage(combo,CB_SETCURSEL,0,0);
          } 
        break;
        case 1:
        {
          BrowseFile_State *parms = (BrowseFile_State *)GetWindowLongPtr(hwnd,GWLP_USERDATA);
          if (parms)
          {
            char buf[maxPathLen];
            const char *filt = NULL;
            buf[0]=0;
            int a = (int) SendDlgItemMessage(hwnd,0x105,CB_GETCURSEL,0,0);
            if (a>=0) filt = (const char *)SendDlgItemMessage(hwnd,0x105,CB_GETITEMDATA,a,0);

            a = (int) SendDlgItemMessage(hwnd,0x103,CB_GETCURSEL,0,0);
            if (a>=0) SendDlgItemMessage(hwnd,0x103,CB_GETLBTEXT,a,(LPARAM)buf);

            if (buf[0]) parms->scan_path(buf, filt, parms->mode == BrowseFile_State::OPENDIR);
            else parms->viewlist.DeleteAll();
            HWND list = GetDlgItem(hwnd,0x104);
            ListView_SetItemCount(list, 0); // clear selection
            ListView_SetItemCount(list, parms->viewlist.GetSize());
            ListView_RedrawItems(list,0, parms->viewlist.GetSize());
          }
        }
        break;
      }
    break;
    case WM_GETMINMAXINFO:
      {
        LPMINMAXINFO p=(LPMINMAXINFO)lParam;
        p->ptMinTrackSize.x = 300;
        p->ptMinTrackSize.y = 300;
      }
    break;
    case WM_SIZE:
      {
        BrowseFile_State *parms = (BrowseFile_State *)GetWindowLongPtr(hwnd,GWLP_USERDATA);
        // reposition controls
        RECT r;
        GetClientRect(hwnd,&r);
        const int buth = 24, cancelbutw = 50, okbutw = parms->mode == BrowseFile_State::OPENDIR ? 120 : 50;
        const int xborder = 4, yborder=8;
        const int fnh = 20, fnlblw = parms->mode == BrowseFile_State::OPENDIR ? 70 : 50;

        int ypos = r.bottom - 4 - buth;
        int xpos = r.right;
        SetWindowPos(GetDlgItem(hwnd,IDCANCEL), NULL, xpos -= cancelbutw + xborder, ypos, cancelbutw,buth, SWP_NOZORDER|SWP_NOACTIVATE);
        SetWindowPos(GetDlgItem(hwnd,IDOK), NULL, xpos -= okbutw + xborder, ypos, okbutw,buth, SWP_NOZORDER|SWP_NOACTIVATE);

        HWND emb = GetDlgItem(hwnd,0x102);
        if (emb)
        {
          RECT sr;
          GetClientRect(emb,&sr);
          if (ypos > r.bottom-4-sr.bottom) ypos = r.bottom-4-sr.bottom;
          SetWindowPos(emb,NULL, xborder,ypos, xpos - xborder*2, sr.bottom, SWP_NOZORDER|SWP_NOACTIVATE);
          ShowWindow(emb,SW_SHOWNA);
        }

        HWND filt = GetDlgItem(hwnd,0x105);
        if (filt)
        {
          SetWindowPos(filt, NULL, xborder*2 + fnlblw, ypos -= fnh + yborder, r.right-fnlblw-xborder*3, fnh, SWP_NOZORDER|SWP_NOACTIVATE);
        }

        SetWindowPos(GetDlgItem(hwnd,0x100), NULL, xborder*2 + fnlblw, ypos -= fnh + yborder, r.right-fnlblw-xborder*3, fnh, SWP_NOZORDER|SWP_NOACTIVATE);
        SetWindowPos(GetDlgItem(hwnd,0x101), NULL, xborder, ypos, fnlblw, fnh, SWP_NOZORDER|SWP_NOACTIVATE);
        SetWindowPos(GetDlgItem(hwnd,0x103), NULL, xborder, 0, r.right-xborder*2, fnh, SWP_NOZORDER|SWP_NOACTIVATE);
  
        SetWindowPos(GetDlgItem(hwnd,0x104), NULL, xborder, fnh+yborder, r.right-xborder*2, ypos - (fnh+yborder) - yborder, SWP_NOZORDER|SWP_NOACTIVATE);
      }
    break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case 0x105:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            SendMessage(hwnd,WM_USER+100,1,0); // refresh list
          }
        return 0;
        case 0x103:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            SendMessage(hwnd,WM_USER+100,1,0); // refresh list
          }
        return 0;
        case IDCANCEL: EndDialog(hwnd,0); return 0;
        case IDOK: 
          {
            char buf[maxPathLen], msg[2048];
            buf[0]=0;

            int a = (int) SendDlgItemMessage(hwnd,0x103,CB_GETCURSEL,0,0);
            if (a>=0)
            {
              SendDlgItemMessage(hwnd,0x103,CB_GETLBTEXT,a,(LPARAM)buf);
              size_t buflen = strlen(buf);
              if (!buflen) strcpy(buf,"/");
              else
              {
                if (buflen > sizeof(buf)-2) buflen = sizeof(buf)-2;
                if (buf[buflen-1]!='/') { buf[buflen++] = '/'; buf[buflen]=0; }
              }
            }
            GetDlgItemText(hwnd,0x100,msg,sizeof(msg));

            BrowseFile_State *parms = (BrowseFile_State *)GetWindowLongPtr(hwnd,GWLP_USERDATA);
            int cnt;
            if (parms->mode == BrowseFile_State::OPENMULTI && (cnt=ListView_GetSelectedCount(GetDlgItem(hwnd,0x104)))>1 && (!*msg || !strcmp(msg,multiple_files)))
            {
              HWND list = GetDlgItem(hwnd,0x104);
              WDL_TypedBuf<char> fs;
              fs.Set(buf,strlen(buf)+1);
              int a = ListView_GetNextItem(list,-1,LVNI_SELECTED);
              while (a != -1 && fs.GetSize() < 4096*4096 && cnt--)
              {
                const char *fn = NULL;
                struct BrowseFile_State::rec *rec = parms->viewlist.EnumeratePtr(a,&fn);
                if (!rec) break;

                if (*fn) fn++; // skip type ident
                fs.Add(fn,strlen(fn)+1);
                a = ListView_GetNextItem(list,a,LVNI_SELECTED);
              }
              fs.Add("",1);

              parms->fnout = (char*)malloc(fs.GetSize());
              if (parms->fnout) memcpy(parms->fnout,fs.Get(),fs.GetSize());

              EndDialog(hwnd,1);
              return 0;
            }
            else 
            {
              if (msg[0] == '.' && (msg[1] == '.' || msg[1] == 0))
              {
                if (msg[1] == '.') SendDlgItemMessage(hwnd,0x103,CB_SETCURSEL,a+1,0);
                SetDlgItemText(hwnd,0x100,"");
                SendMessage(hwnd,WM_USER+100,1,0); // refresh list
                return 0;
              }
              else if (msg[0] == '/') lstrcpyn_safe(buf,msg,sizeof(buf));
              else lstrcatn(buf,msg,sizeof(buf));
            }

            switch (parms->mode)
            {
              case BrowseFile_State::OPENDIR:
                 if (!buf[0]) return 0;
                 else if (msg[0])
                 {
                   // navigate to directory if filepart set
                   DIR *dir = opendir(buf);
                   if (!dir) 
                   {
                     snprintf(msg,sizeof(msg),"Error opening directory:\r\n\r\n%s\r\n\r\nCreate?",buf);
                     if (MessageBox(hwnd,msg,"Create directory?",MB_OKCANCEL)==IDCANCEL) return 0;
                     CreateDirectory(buf,NULL);
                     dir=opendir(buf);
                   }
                   if (!dir) { MessageBox(hwnd,"Error creating directory","Error",MB_OK); return 0; }
                   closedir(dir);
                   SendMessage(hwnd, WM_USER+100, 0x103, (LPARAM)buf);
                   SetDlgItemText(hwnd,0x100,"");
                   SendMessage(hwnd,WM_USER+100,1,0); // refresh list

                   return 0;
                 }
                 else
                 {
                   DIR *dir = opendir(buf);
                   if (!dir) return 0;
                   closedir(dir);
                 }
              break;
              case BrowseFile_State::SAVE:
                 if (!buf[0]) return 0;
                 else  
                 {
                   struct stat64 st={0,};
                   DIR *dir = opendir(buf);
                   if (dir)
                   {
                     closedir(dir);
                     SendMessage(hwnd, WM_USER+100, 0x103, (LPARAM)buf);
                     SetDlgItemText(hwnd,0x100,"");
                     SendMessage(hwnd,WM_USER+100,1,0); // refresh list
                     return 0;
                   }
                   if (!stat64(buf,&st))
                   {
                     snprintf(msg,sizeof(msg),"File exists:\r\n\r\n%s\r\n\r\nOverwrite?",buf);
                     if (MessageBox(hwnd,msg,"Overwrite file?",MB_OKCANCEL)==IDCANCEL) return 0;
                   }
                 }
              break;
              default:
                 if (!buf[0]) return 0;
                 else  
                 {
                   struct stat64 st={0,};
                   DIR *dir = opendir(buf);
                   if (dir)
                   {
                     closedir(dir);
                     SendMessage(hwnd, WM_USER+100, 0x103, (LPARAM)buf);
                     SetDlgItemText(hwnd,0x100,"");
                     SendMessage(hwnd,WM_USER+100,1,0); // refresh list
                     return 0;
                   }
                   if (stat64(buf,&st))
                   {
                     snprintf(msg,sizeof(msg),"File does not exist:\r\n\r\n%s",buf);
                     MessageBox(hwnd,msg,"File not found",MB_OK);
                     return 0;
                   }
                 }
              break;
            }
            if (parms->fnout) 
            {
              lstrcpyn_safe(parms->fnout,buf,parms->fnout_sz);
            }
            else
            {
              size_t l = strlen(buf);
              parms->fnout = (char*)calloc(l+2,1);
              memcpy(parms->fnout,buf,l);
            }
          }
          EndDialog(hwnd,1);
        return 0;
      }
    break;
    case WM_NOTIFY:
      {
        LPNMHDR l=(LPNMHDR)lParam;
        if (l->code == LVN_GETDISPINFO)
        {
          BrowseFile_State *parms = (BrowseFile_State *)GetWindowLongPtr(hwnd,GWLP_USERDATA);
          NMLVDISPINFO *lpdi = (NMLVDISPINFO*) lParam;
          const int idx=lpdi->item.iItem;
          if (l->idFrom == 0x104 && parms && idx >=0 && idx < parms->viewlist.GetSize())
          {
            const char *fn = NULL;
            struct BrowseFile_State::rec *rec = parms->viewlist.EnumeratePtr(idx,&fn);
            if (rec && fn)
            {
              if (lpdi->item.mask&LVIF_TEXT) 
              {
                switch (lpdi->item.iSubItem)
                {
                  case 0:
                    if (fn[0]) lstrcpyn_safe(lpdi->item.pszText,fn+1,lpdi->item.cchTextMax);
                  break;
                  case 1:
                    if (fn[0] == 1) 
                    {
                      lstrcpyn_safe(lpdi->item.pszText,"<DIR>",lpdi->item.cchTextMax);
                    }
                    else
                    {
                      static const char *tab[]={ "bytes","KB","MB","GB" };
                      int lf=0;
                      WDL_INT64 s=rec->size;
                      if (s<1024)
                      {
                        snprintf(lpdi->item.pszText,lpdi->item.cchTextMax,"%d %s  ",(int)s,tab[0]);
                        break;
                      }
                      
                      int w = 1;
                      do {  w++; lf = (int)(s&1023); s/=1024; } while (s >= 1024 && w<4);
                      snprintf(lpdi->item.pszText,lpdi->item.cchTextMax,"%d.%d %s  ",(int)s,(int)((lf*10.0)/1024.0+0.5),tab[w-1]);
                    }
                  break;
                  case 2:
                    if (rec->date > 0 && rec->date < WDL_INT64_CONST(0x793406fff))
                    {
                      struct tm *a=localtime(&rec->date);
                      if (a)
                      {
                        char str[512];
                        strftime(str,sizeof(str),"%c",a);
                        lstrcpyn(lpdi->item.pszText, str,lpdi->item.cchTextMax);
                      }
                    }
                  break;
                }
              }
            }
          }
        }
        else if (l->code == LVN_ODFINDITEM)
        {
        }
        else if (l->code == LVN_ITEMCHANGED)
        {
          const int selidx = ListView_GetNextItem(l->hwndFrom, -1, LVNI_SELECTED);
          if (selidx>=0)
          {
            BrowseFile_State *parms = (BrowseFile_State *)GetWindowLongPtr(hwnd,GWLP_USERDATA);
            if (parms && parms->mode == BrowseFile_State::OPENMULTI && ListView_GetSelectedCount(l->hwndFrom)>1)
            {
              SetDlgItemText(hwnd,0x100,multiple_files);
            }
            else
            {
              const char *fn = NULL;
              struct BrowseFile_State::rec *rec = parms ? parms->viewlist.EnumeratePtr(selidx,&fn) : NULL;
              if (rec)
              {
                if (fn && fn[0]) SetDlgItemText(hwnd,0x100,fn+1);
              }
            }
          }
        }
        else if (l->code == NM_DBLCLK)
        {
          SendMessage(hwnd,WM_COMMAND,IDOK,0);
        }

      }
    break;
  }
  return 0;
}

// return true
bool BrowseForSaveFile(const char *text, const char *initialdir, const char *initialfile, const char *extlist,
                       char *fn, int fnsize)
{
  BrowseFile_State state( text, initialdir, initialfile, extlist, BrowseFile_State::SAVE, fn, fnsize );
  if (!DialogBoxParam(NULL,NULL,NULL,swellFileSelectProc,(LPARAM)&state)) return false;
  if (fn && fnsize > 0 && extlist && *extlist && WDL_get_fileext(fn)[0] != '.')
  {
    const char *erd = extlist+strlen(extlist)+1;
    if (*erd == '*' && erd[1] == '.') // add default extension
    {
      const char *a = (erd+=1);
      while (*erd && *erd != ';') erd++;
      if (erd > a+1) snprintf_append(fn,fnsize,"%.*s",(int)(erd-a),a);
    }
  }

  return true;
}

bool BrowseForDirectory(const char *text, const char *initialdir, char *fn, int fnsize)
{
  BrowseFile_State state( text, initialdir, initialdir, NULL, BrowseFile_State::OPENDIR, fn, fnsize );
  return !!DialogBoxParam(NULL,NULL,NULL,swellFileSelectProc,(LPARAM)&state);
}


char *BrowseForFiles(const char *text, const char *initialdir, 
                     const char *initialfile, bool allowmul, const char *extlist)
{
  BrowseFile_State state( text, initialdir, initialfile, extlist, 
           allowmul ? BrowseFile_State::OPENMULTI : BrowseFile_State::OPEN, NULL, 0 );
  return DialogBoxParam(NULL,NULL,NULL,swellFileSelectProc,(LPARAM)&state) ? state.fnout : NULL;
}


static LRESULT WINAPI swellMessageBoxProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  const int button_spacing = 8;
  switch (uMsg)
  {
    case WM_CREATE:
      if (lParam)  // swell-specific
      {
        SetWindowLong(hwnd,GWL_WNDPROC,(LPARAM)SwellDialogDefaultWindowProc);
        SetWindowLong(hwnd,DWL_DLGPROC,(LPARAM)swellMessageBoxProc);
        void **parms = (void **)lParam;
        if (parms[1]) SetWindowText(hwnd,(const char*)parms[1]);


        int nbuttons=1;
        const char *buttons[3] = { "OK", "", "" };
        int button_ids[3] = {IDOK,0,0};
        int button_sizes[3];

        int mode =  ((int)(INT_PTR)parms[2]);
        if (mode == MB_RETRYCANCEL) { buttons[0]="Retry"; button_ids[0]=IDRETRY;  }
        if (mode == MB_YESNO || mode == MB_YESNOCANCEL) { buttons[0]="Yes"; button_ids[0] = IDYES;  buttons[nbuttons] = "No"; button_ids[nbuttons] = IDNO; nbuttons++; }
        if (mode == MB_OKCANCEL || mode == MB_YESNOCANCEL || mode == MB_RETRYCANCEL) { buttons[nbuttons] = "Cancel"; button_ids[nbuttons] = IDCANCEL; nbuttons++; }

        SWELL_MakeSetCurParms(1,1,0,0,hwnd,false,false);
        RECT labsize = {0,0,300,20};
        HWND lab = SWELL_MakeLabel(-1,parms[0] ? (const char *)parms[0] : "", 0x100, 0,0,10,10,SS_CENTER); //we'll resize this manually
        HDC dc=GetDC(lab); 
        if (lab && parms[0])
        {
          DrawText(dc,(const char *)parms[0],-1,&labsize,DT_CALCRECT|DT_NOPREFIX);// if dc isnt valid yet, try anyway
        }
        labsize.top += 10;
        labsize.bottom += 18;

        int x;
        int button_height=0, button_total_w=0;;
        for (x = 0; x < nbuttons; x ++)
        {
          RECT r={0,0,35,12};
          DrawText(dc,buttons[x],-1,&r,DT_CALCRECT|DT_NOPREFIX|DT_SINGLELINE);
          button_sizes[x] = r.right-r.left + 8;
          button_total_w += button_sizes[x] + (x ? button_spacing : 0);
          if (r.bottom-r.top+10 > button_height) button_height = r.bottom-r.top+10;
        }

        if (labsize.right < button_total_w+16) labsize.right = button_total_w+16;

        int xpos = labsize.right/2 - button_total_w/2;
        for (x = 0; x < nbuttons; x ++)
        {
          SWELL_MakeButton(0,buttons[x],button_ids[x],xpos,labsize.bottom,button_sizes[x],button_height,0);
          xpos += button_sizes[x] + button_spacing;
        }

        if (dc) ReleaseDC(lab,dc);
        SWELL_MakeSetCurParms(1,1,0,0,NULL,false,false);
        SetWindowPos(hwnd,NULL,0,0,labsize.right + 16,labsize.bottom + button_height + 8,SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
        if (lab) SetWindowPos(lab,NULL,8,0,labsize.right,labsize.bottom,SWP_NOACTIVATE|SWP_NOZORDER);
      }
    break;
    case WM_SIZE:
      {
        RECT r;
        GetClientRect(hwnd,&r);
        HWND h = GetWindow(hwnd,GW_CHILD);
        int n = 100;
        int w[8];
        HWND tab[8],lbl=NULL;
        int tabsz=0, bxwid=0, button_height=0;
        while (h && n--) {
          int idx = GetWindowLong(h,GWL_ID);
          if (idx == IDCANCEL || idx == IDOK || idx == IDNO || idx == IDYES) 
          { 
            RECT tr;
            GetClientRect(h,&tr);
            tab[tabsz] = h;
            w[tabsz++] = tr.right - tr.left;
            button_height = tr.bottom-tr.top;
            bxwid += tr.right-tr.left;
          } else if (idx==0x100) lbl=h;
          h = GetWindow(h,GW_HWNDNEXT);
        }
        if (lbl) SetWindowPos(h,NULL,8,0,r.right,r.bottom - 8 - button_height,  SWP_NOZORDER|SWP_NOACTIVATE);
        int xo = r.right/2 - (bxwid + (tabsz-1)*button_spacing)/2,x;
        for (x=tabsz-1; x >=0; x--)
        {
          SetWindowPos(tab[x],NULL,xo,r.bottom - button_height - 8, 0,0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
          xo += w[x] + button_spacing;
        }
      }
    break;
    case WM_COMMAND:
      if (LOWORD(wParam) && HIWORD(wParam) == BN_CLICKED ) EndDialog(hwnd,LOWORD(wParam));
    break;
    case WM_CLOSE:
      if (GetDlgItem(hwnd,IDCANCEL)) EndDialog(hwnd,IDCANCEL);
      else if (GetDlgItem(hwnd,IDNO)) EndDialog(hwnd,IDNO);
      else if (GetDlgItem(hwnd,IDYES)) EndDialog(hwnd,IDYES);
      else EndDialog(hwnd,IDOK);
    break;
  }
  return 0;
}

int MessageBox(HWND hwndParent, const char *text, const char *caption, int type)
{
  printf("MessageBox: %s %s\n",text,caption);
  const void *parms[4]= {text,caption,(void*)(INT_PTR)type} ;
  return DialogBoxParam(NULL,NULL,hwndParent,swellMessageBoxProc,(LPARAM)parms);

#if 0
  int ret=0;
  
  if (type == MB_OK)
  {
    // todo
    ret=IDOK;
  }	
  else if (type == MB_OKCANCEL)
  {
    ret = 1; // todo
    if (ret) ret=IDOK;
    else ret=IDCANCEL;
  }
  else if (type == MB_YESNO)
  {
    ret = 1 ; // todo
    if (ret) ret=IDYES;
    else ret=IDNO;
  }
  else if (type == MB_RETRYCANCEL)
  {
    ret = 1; // todo

    if (ret) ret=IDRETRY;
    else ret=IDCANCEL;
  }
  else if (type == MB_YESNOCANCEL)
  {
    ret = 1; // todo

    if (ret == 1) ret=IDYES;
    else if (ret==-1) ret=IDNO;
    else ret=IDCANCEL;
  }
  
  return ret; 
#endif
}

#ifdef SWELL_LICE_GDI
struct ChooseColor_State {
  int ncustom;
  int *custom;

  int h,s,v;

  LICE_IBitmap *bm;
};

// we need to make a more accurate LICE_HSV2RGB pair, this one is lossy, doh
static LRESULT WINAPI swellColorSelectProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static int s_reent,s_vmode;
  enum { wndw=400,
         custsz = 20,
         buth = 24, border = 4, butw = 50, edh=20, edlw = 16, edew = 40, vsize=40,
         psize = border+edlw + edew, yt = border + psize + border + (edh + border)*6 };
  const int customperrow = (wndw-border)/(custsz+border);
  switch (uMsg)
  {
    case WM_CREATE:
      if (lParam)  // swell-specific
      {
        SetWindowLong(hwnd,GWL_WNDPROC,(LPARAM)SwellDialogDefaultWindowProc);
        SetWindowLong(hwnd,DWL_DLGPROC,(LPARAM)swellColorSelectProc);
        SetWindowLongPtr(hwnd,GWLP_USERDATA,lParam);

        SetWindowText(hwnd,"Choose Color");

        SWELL_MakeSetCurParms(1,1,0,0,hwnd,false,false);

        SWELL_MakeButton(0, "OK", IDOK,0,0,0,0, 0);
        SWELL_MakeButton(0, "Cancel", IDCANCEL,0,0,0,0, 0);

        static const char * const lbl[] = { "R","G","B","H","S","V"};
        for (int x=0;x<6;x++)
        {
          SWELL_MakeLabel(0,lbl[x], 0x100+x, 0,0,0,0, 0); 
          SWELL_MakeEditField(0x200+x, 0,0,0,0,  0);
        }

        ChooseColor_State *cs = (ChooseColor_State*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
        SWELL_MakeSetCurParms(1,1,0,0,NULL,false,false);
        int nrows = ((cs?cs->ncustom : 0 ) + customperrow-1)/wdl_max(customperrow,1);
        SetWindowPos(hwnd,NULL,0,0, wndw, 
            yt + buth + border + nrows * (custsz+border), 
            SWP_NOZORDER|SWP_NOMOVE);
        SendMessage(hwnd,WM_USER+100,0,3);
      }
    break;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
      {
        ChooseColor_State *cs = (ChooseColor_State*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
        if (!cs) break;
        RECT r;
        GetClientRect(hwnd,&r);
        const int xt = r.right - edew - edlw - border*3;

        const int y = GET_Y_LPARAM(lParam);
        const int x = GET_X_LPARAM(lParam);
        if (x < xt && y < yt)
        {
          s_vmode = x >= xt-vsize;
          SetCapture(hwnd);
          // fall through
        }
        else 
        {
          if (cs->custom && cs->ncustom && y >= yt && y < r.bottom - buth - border)
          {
            int row = (y-yt) / (custsz+border), rowoffs = (y-yt) % (custsz+border);
            if (rowoffs < custsz)
            {
              int col = (x-border) / (custsz+border), coloffs = (x-border) % (custsz+border);
              if (coloffs < custsz)
              {
                col += customperrow*row;
                if (col >= 0 && col < cs->ncustom)
                {
                  if (uMsg == WM_LBUTTONDOWN)
                  {
                    LICE_RGB2HSV(GetRValue(cs->custom[col]),GetGValue(cs->custom[col]),GetBValue(cs->custom[col]),&cs->h,&cs->s,&cs->v);
                    SendMessage(hwnd,WM_USER+100,0,3);
                  }
                  else
                  {
                    int r,g,b;
                    LICE_HSV2RGB(cs->h,cs->s,cs->v,&r,&g,&b);
                    cs->custom[col] = RGB(r,g,b);
                    InvalidateRect(hwnd,NULL,FALSE);
                  }
                }
              }
            }
          }
          break;
        }
        // fall through
      }
    case WM_MOUSEMOVE:
      if (GetCapture()==hwnd)
      {
        RECT r;
        GetClientRect(hwnd,&r);
        const int xt = r.right - edew - edlw - border*3;
        ChooseColor_State *cs = (ChooseColor_State*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
        if (!cs) break;
        int var = 255 - (GET_Y_LPARAM(lParam) - border)*256 / (yt-border*2);
        if (var<0)var=0;
        else if (var>255)var=255;
        if (s_vmode)
        {
          if (var != cs->v)
          {
            cs->v=var;
            SendMessage(hwnd,WM_USER+100,0,3);
          }
        }
        else
        {
          int hue = (GET_X_LPARAM(lParam) - border)*384 / (xt-border - vsize);
          if (hue<0) hue=0;
          else if (hue>383) hue=383;
          if (cs->h != hue || cs->s != var)
          {
            cs->h=hue;
            cs->s=var;
            SendMessage(hwnd,WM_USER+100,0,3);
          }
        }
      }
    break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
      ReleaseCapture();
    break;
    case WM_PAINT:
      {
        PAINTSTRUCT ps;
        ChooseColor_State *cs = (ChooseColor_State*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
        if (cs && BeginPaint(hwnd,&ps))
        {
          RECT r;
          GetClientRect(hwnd,&r);
          const int xt = r.right - edew - edlw - border*3;
          if (cs->custom && cs->ncustom)
          {
            int ypos = yt;
            int xpos = border;
            for (int x = 0; x < cs->ncustom; x ++)
            {
              HBRUSH br = CreateSolidBrush(cs->custom[x]);
              RECT tr={xpos,ypos,xpos+custsz, ypos+custsz };
              FillRect(ps.hdc,&tr,br);
              DeleteObject(br);

              xpos += border+custsz;
              if (xpos+custsz >= r.right)
              {
                xpos=border;
                ypos += border + custsz;
              }
            }
          }

          {
            int rr,g,b;
            LICE_HSV2RGB(cs->h,cs->s,cs->v,&rr,&g,&b);
            HBRUSH br = CreateSolidBrush(RGB(rr,g,b));
            RECT tr={r.right - border - psize, border, r.right-border, border+psize};
            FillRect(ps.hdc,&tr,br);
            DeleteObject(br);
          }

          if (!cs->bm) cs->bm = new LICE_SysBitmap(xt-border,yt-border);
          else cs->bm->resize(xt-border,yt-border);

          int x1 = xt - border - vsize;
          int var = cs->v;

          const int ysz = yt-border*2;
          const int vary = ysz - 1 - (ysz * cs->v)/256;

          for (int y = 0; y < ysz; y ++)
          {
            LICE_pixel *wr = cs->bm->getBits() + cs->bm->getRowSpan() * y;
            const int sat = 255 - y*256/ysz;
            double xx=0.0, dx=384.0/x1;
            int x;
            for (x = 0; x < x1; x++)
            {
              *wr++ = LICE_HSV2Pix((int)(xx+0.5),sat,var,255);
              xx+=dx;
            }
            LICE_pixel p = LICE_HSV2Pix(cs->h,cs->s,sat ^ (y==vary ? 128 : 0),255);
            for (;x < xt-border;x++) *wr++ = p;
          }
          LICE_pixel p = LICE_HSV2Pix(cs->h,cs->s,(128+cs->v)&255,255);
          const int saty = ysz - 1 - (ysz * cs->s)/256;
          const int huex = (x1*cs->h)/384;
          LICE_Line(cs->bm,huex,saty-4,huex,saty+4,p,.75f,LICE_BLIT_MODE_COPY,false);
          LICE_Line(cs->bm,huex-4,saty,huex+4,saty,p,.75f,LICE_BLIT_MODE_COPY,false);

          BitBlt(ps.hdc,border,border,xt-border,ysz,cs->bm->getDC(),0,0,SRCCOPY);

          EndPaint(hwnd,&ps);
        }
      }

    break;
    case WM_GETMINMAXINFO:
      {
        LPMINMAXINFO p=(LPMINMAXINFO)lParam;
        p->ptMinTrackSize.x = 300;
        p->ptMinTrackSize.y = 300;
      }
    break;
    case WM_USER+100:
      {
        ChooseColor_State *cs = (ChooseColor_State*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
        if (cs)
        {
          int t[6];
          t[3] = cs->h;
          t[4] = cs->s;
          t[5] = cs->v;
          LICE_HSV2RGB(t[3],t[4],t[5],t,t+1,t+2);
          s_reent++;
          for (int x=0;x<6;x++) if (lParam & ((x<3)?1:2)) SetDlgItemInt(hwnd,0x200+x,x==3 ? t[x]*360/384 : t[x],FALSE);
          s_reent--;
          InvalidateRect(hwnd,NULL,FALSE);
        }
      }
    break;
    case WM_SIZE:
      {
        RECT r;
        GetClientRect(hwnd,&r);
        int tx = r.right - edew-edlw-border*2, ty = border*2 + psize;
        for (int x=0;x<6;x++)
        {
          SetWindowPos(GetDlgItem(hwnd,0x100+x),NULL,tx, ty, edlw, edh, SWP_NOZORDER);
          SetWindowPos(GetDlgItem(hwnd,0x200+x),NULL,tx+edlw+border, ty, edew, edh, SWP_NOZORDER);
          ty += border+edh;
        }

        r.right -= border + butw;
        r.bottom -= border + buth;
        SetWindowPos(GetDlgItem(hwnd,IDCANCEL), NULL, r.right, r.bottom, butw, buth, SWP_NOZORDER);
        r.right -= border*2 + butw;
        SetWindowPos(GetDlgItem(hwnd,IDOK), NULL, r.right, r.bottom, butw, buth, SWP_NOZORDER);

      }
    break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDCANCEL:
          EndDialog(hwnd,0);
        break;
        case IDOK:
          EndDialog(hwnd,1);
        break;
        case 0x200:
        case 0x201:
        case 0x202:
        case 0x203:
        case 0x204:
        case 0x205:
          if (!s_reent)
          {
            const bool ishsv = LOWORD(wParam) >= 0x203;
            int offs = ishsv ? 0x203 : 0x200;
            BOOL t = FALSE;
            int h = GetDlgItemInt(hwnd,offs++,&t,FALSE);
            if (!t) break;
            int s = GetDlgItemInt(hwnd,offs++,&t,FALSE);
            if (!t) break;
            int v = GetDlgItemInt(hwnd,offs++,&t,FALSE);
            if (!t) break;

            ChooseColor_State *cs = (ChooseColor_State*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
            if (!ishsv) 
            {
              if (h<0) h=0; else if (h>255) h=255;
              if (s<0) s=0; else if (s>255) s=255;
              if (v<0) v=0; else if (v>255) v=255;
              LICE_RGB2HSV(h,s,v,&h,&s,&v);
            }
            else
            {
              h = h * 384 / 360;
              if (h<0) h=0; else if (h>384) h=384;
              if (s<0) s=0; else if (s>255) s=255;
              if (v<0) v=0; else if (v>255) v=255;
            }

            if (cs)
            {
              cs->h = h;
              cs->s = s;
              cs->v = v;
            }
            SendMessage(hwnd,WM_USER+100,0,ishsv?1:2);
          }
        break;
      }
    break;

  }
  return 0;
}
#endif //SWELL_LICE_GDI

bool SWELL_ChooseColor(HWND h, int *val, int ncustom, int *custom)
{
#ifdef SWELL_LICE_GDI
  ChooseColor_State state = { ncustom, custom };
  int c = val ? *val : 0;
  LICE_RGB2HSV(GetRValue(c),GetGValue(c),GetBValue(c),&state.h,&state.s,&state.v);
  bool rv = DialogBoxParam(NULL,NULL,NULL,swellColorSelectProc,(LPARAM)&state)!=0;
  delete state.bm;
  if (rv && val) 
  {
    int r,g,b;
    LICE_HSV2RGB(state.h,state.s,state.v,&r,&g,&b);
    *val = RGB(r,g,b);
  }
  return rv;
#else
  return false;
#endif
}

bool SWELL_ChooseFont(HWND h, LOGFONT *lf)
{
  return false;
}

#endif
