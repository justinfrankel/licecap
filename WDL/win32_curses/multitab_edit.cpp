#ifndef CURSES_INSTANCE
#define CURSES_INSTANCE ((win32CursesCtx *)m_cursesCtx)
#endif

#include "curses.h"
#ifdef _WIN32
#include <windows.h>
#else
#include "../swell/swell.h"
#endif
#include <stdlib.h>
#include <string.h>
#include "multitab_edit.h"

#include "../win32_utf8.h"
#include "../wdlcstring.h"

#define UI_STATE_SAVE_AS_NEW 11
#define UI_STATE_SAVE_ON_CLOSE 12

#define COLOR_TOPLINE COLOR_PAIR(6)


void MultiTab_Editor::draw_top_line()
{
  int ypos=0;
  if (m_top_margin > 1)
  {
    int xpos=0;
    int x;
    move(ypos++,0);
    const int cnt= GetTabCount();
    int tsz=16;
    // this is duplicated in onMouseMessage
    if (cnt>0) tsz=COLS/cnt;
    if (tsz>128)tsz=128;
    if (tsz<12) tsz=12;

    for (x= 0; x < cnt && xpos < COLS; x ++)
    {
      MultiTab_Editor *ed = GetTab(x);
      if (ed)
      {
        char buf[128 + 8];
        memset(buf,' ',tsz);
        const char *p = WDL_get_filepart(ed->GetFileName());
        const int lp=strlen(p);
        int skip=0;        
        if (x<9) 
        { 
          if (tsz>16)
          {
#ifdef __APPLE__
            memcpy(buf,"<Cmd+",skip=5);
#else
            memcpy(buf,"<Ctrl+",skip=6);
#endif
          }
          buf[skip++]='F'; 
          buf[skip++] = '1'+x; 
          buf[skip++] = '>';
          skip++;
        }
        memcpy(buf+skip,p,min(tsz-1-skip,lp));
        buf[tsz]=0;
        int l = tsz;
        if (l > COLS-xpos) l = COLS-xpos;
        if (ed == this)
        {
          attrset(SYNTAX_HIGHLIGHT2|A_BOLD);
        }
        else
        {
          attrset(A_NORMAL);
        }
        addnstr(buf,l);
        xpos += l;
      }
    }
    if (xpos < COLS) clrtoeol();
  }
  attrset(COLOR_TOPLINE|A_BOLD);
  bkgdset(COLOR_TOPLINE);
  const char *p=GetFileName();
  move(ypos,0);
  if (COLS>4)
  {
    const int pl = (int) strlen(p);
    if (pl > COLS-1 && COLS > 4)
    {
      addstr("...");
      p+=pl - (COLS-1) + 4;
    }
    addstr(p);
  }
  clrtoeol();
}

void MultiTab_Editor::draw(int lineidx)
{
  if (m_top_margin != 0) m_top_margin = GetTabCount()>1 ? 2 : 1;
  WDL_CursesEditor::draw(lineidx);
}

#define CTRL_KEY_DOWN (GetAsyncKeyState(VK_CONTROL)&0x8000)
#define SHIFT_KEY_DOWN (GetAsyncKeyState(VK_SHIFT)&0x8000)
#define ALT_KEY_DOWN (GetAsyncKeyState(VK_MENU)&0x8000)

int MultiTab_Editor::onChar(int c)
{
  if (!m_state && !SHIFT_KEY_DOWN && !ALT_KEY_DOWN && c =='W'-'A'+1)
  {
    if (GetTab(0) == this) return 0; // first in list = do nothing

    if (IsDirty())
    {
      m_state=UI_STATE_SAVE_ON_CLOSE;
      attrset(m_color_message);
      bkgdset(m_color_message);
      mvaddstr(LINES-1,0,"Save file before closing (y/N)? ");
      clrtoeol();
      attrset(0);
      bkgdset(0);
    }
    else
    {
      CloseCurrentTab();

      delete this;
      // context no longer valid!
      return 1;
    }
    return 0;
  }

  if (m_state == UI_STATE_SAVE_ON_CLOSE)
  {
    if (isalnum(c) || isprint(c) || c==27)
    {
      if (c == 27)
      {
        m_state=0;
        draw();
        draw_message("Cancelled close of file.");
        setCursor();
        return 0;
      }
      if (toupper(c) == 'N' || toupper(c) == 'Y')
      {
        if (toupper(c) == 'Y') 
        {
          if(updateFile())
          {
            m_state=0;
            draw();
            draw_message("Error writing file, changes not saved!");
            setCursor();
            return 0;
          }
        }
        CloseCurrentTab();

        delete this;
        // this no longer valid, return 1 to avoid future calls in onChar()

        return 1;
      }
    }
    return 0;
  }
  else if (m_state == UI_STATE_SAVE_AS_NEW)
  {
    if (isalnum(c) || isprint(c) || c==27 || c == '\r' || c=='\n')
    {
      if (toupper(c) == 'N' || c == 27) 
      {
        m_state=0;
        draw();
        draw_message("Cancelled create new file.");
        setCursor();
        return 0;
      }
      m_state=0;

      AddTab(m_newfn.Get());
    }
    return 0;
  }

  if ((c==27 || c==29 || (c >= KEY_F1 && c<=KEY_F10)) && CTRL_KEY_DOWN)
  {
    int idx=c-KEY_F1;
    bool rel=true;
    if (c==27) idx=-1;
    else if (c==29) idx=1;
    else rel=false;
    SwitchTab(idx,rel);

    return 1;
  }

  return WDL_CursesEditor::onChar(c);
}

void MultiTab_Editor::OpenFileInTab(const char *fnp)
{
  // try to find file to open
  WDL_FastString s;        
  FILE *fp=NULL;
  {
    const char *ptr = fnp;
    while (!fp && *ptr)
    {          
      // first try same path as loading effect
      if (m_filename.Get()[0])
      {
        s.Set(m_filename.Get());
        const char *sp=s.Get()+s.GetLength();
        while (sp>=s.Get() && *sp != '\\' && *sp != '/') sp--;
        s.SetLen(sp + 1 - s.Get());
        if (s.GetLength())
        {
          s.Append(ptr);
          fp=fopenUTF8(s.Get(),"rb");
        }
      }

      // scan past any / or \\, and try again
      if (!fp)
      {
        while (*ptr && *ptr != '\\' && *ptr != '/') ptr++;
        if (*ptr) ptr++;
      }
    }
  }

  if (!fp) 
  {
    s.Set("");
    fp = tryToFindOrCreateFile(fnp,&s);
  }

  if (!fp && s.Get()[0])
  {
    m_newfn.Set(s.Get());

    if (COLS > 25)
    {
      int allowed = COLS-25;
      if (s.GetLength()>allowed)
      {
        s.DeleteSub(0,s.GetLength() - allowed + 3);
        s.Insert("...",0);
      }
      s.Insert("Create new file '",0);
      s.Append("' (Y/n)? ");
    }
    else
      s.Set("Create new file (Y/n)? ");

    m_state=UI_STATE_SAVE_AS_NEW;
    attrset(m_color_message);
    bkgdset(m_color_message);
    mvaddstr(LINES-1,0,s.Get());
    clrtoeol();
    attrset(0);
    bkgdset(0);
  }
  else if (fp)
  {
    fclose(fp);
    int x;
    for (x=0;x<GetTabCount();x++)
    {
      MultiTab_Editor *e = GetTab(x);
      if (e && !stricmp(e->GetFileName(),s.Get()))
      {
        SwitchTab(x,false);
        return;
      }
    }
    AddTab(s.Get());
  }
}


LRESULT MultiTab_Editor::onMouseMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_LBUTTONDBLCLK:
      if (CURSES_INSTANCE && CURSES_INSTANCE->m_font_w && CURSES_INSTANCE->m_font_h)
      {
        const int y = ((short)HIWORD(lParam)) / CURSES_INSTANCE->m_font_h - m_top_margin;
        const int x = ((short)LOWORD(lParam)) / CURSES_INSTANCE->m_font_w + m_offs_x;
        WDL_FastString *fs=m_text.Get(y + m_paneoffs_y[m_curpane]);
        if (fs && y >= 0)
        {
          const char *url=fs->Get();
          
          while (NULL != (url = strstr(url,"http://")))
          {
            if (url != fs->Get() && isalnum(url[-1]))
            {
              url+=7;
            }
            else
            {
              const int soffs = (int) (url - fs->Get());
              char tmp[512];
              char *p=tmp;
              while (p < (tmp+sizeof(tmp)-1) &&
                    *url && *url != ' ' && *url != ')' && *url != '\t' && *url != '"' && *url != '\'' )
              {
                *p++ = *url++;
              }
              *p=0;
              if (strlen(tmp) >= 10 && x >= soffs && x<(url-fs->Get()))
              {
                ShellExecute(hwnd,"open",tmp,"","",0);
                break;
              }
            }
          }
        }
      }

    break;
    case WM_LBUTTONDOWN:
      if (CURSES_INSTANCE && CURSES_INSTANCE->m_font_w && CURSES_INSTANCE->m_font_h)
      {
        int x = ((short)LOWORD(lParam)) / CURSES_INSTANCE->m_font_w;
        int y = ((short)HIWORD(lParam)) / CURSES_INSTANCE->m_font_h;
        const int tabcnt=GetTabCount();
        if (y==0 && tabcnt>1)
        {
          int tsz=COLS/tabcnt;
          // this is duplicated in draw_top_line
          if (tsz>128)tsz=128;
          if (tsz<12) tsz=12;
          SwitchTab(x/tsz,false);

          return 1;
        }
      }
    break;
  }
  return WDL_CursesEditor::onMouseMessage(hwnd,uMsg,wParam,lParam);
}



