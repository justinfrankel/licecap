#ifndef CURSES_INSTANCE
#define CURSES_INSTANCE ((win32CursesCtx*)m_cursesCtx)
#endif

#include "curses_editor.h"
#include "../win32_utf8.h"
#include "../wdlcstring.h"
#include "curses.h"

#define VALIDATE_TEXT_CHAR(thischar) ((isspace(thischar) || isgraph(thischar)) && (thischar) < 256)
#define TAB_STR "  "

WDL_FastString WDL_CursesEditor::s_fake_clipboard;
int WDL_CursesEditor::s_overwrite=0;
char WDL_CursesEditor::s_search_string[256];

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x20A
#endif


WDL_CursesEditor::WDL_CursesEditor(void *cursesCtx)
{ 
  m_cursesCtx = cursesCtx;

  m_color_bottomline = COLOR_PAIR(1);
  m_color_statustext = COLOR_PAIR(1);
  m_color_selection = COLOR_PAIR(2);
  m_color_message = COLOR_PAIR(2);

  m_top_margin=0;
  m_bottom_margin=2;

  m_selecting=0;
  m_select_x1=m_select_y1=m_select_x2=m_select_y2=0;
  m_state=0;
  m_offs_x=m_offs_y=0;
  m_curs_x=m_curs_y=0;
  m_want_x=-1;
  m_undoStack_pos=-1;
  m_clean_undopos=0;


#ifdef WDL_IS_FAKE_CURSES
  if (m_cursesCtx)
  {
    CURSES_INSTANCE->user_data = this;
    CURSES_INSTANCE->onMouseMessage = _onMouseMessage;
  }
#endif

  initscr();
  cbreak();
  noecho();
  nonl();
  intrflush(stdscr,FALSE);
  keypad(stdscr,TRUE);
  nodelay(stdscr,TRUE);
  raw(); // disable ctrl+C etc. no way to kill if allow quit isn't defined, yay.
  start_color();
  init_pair(1, COLOR_WHITE, COLOR_BLUE); // normal status lines
  init_pair(2, COLOR_BLACK, COLOR_CYAN); // value

  erase();
  refresh();
}

int WDL_CursesEditor::getVisibleLines() const { return LINES-m_bottom_margin-m_top_margin; }


LRESULT WDL_CursesEditor::onMouseMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_CAPTURECHANGED:
    break;

    case WM_MOUSEMOVE:
      if (GetCapture()==hwnd && CURSES_INSTANCE->m_font_w && CURSES_INSTANCE->m_font_h)
      {
        int x = ((short)LOWORD(lParam)) / CURSES_INSTANCE->m_font_w + m_offs_x;
        int y = ((short)HIWORD(lParam)) / CURSES_INSTANCE->m_font_h + m_offs_y - m_top_margin;
        if (!m_selecting && (x != m_curs_x || y != m_curs_y)) 
        {
          m_select_x2=m_select_x1=m_curs_x;
          m_select_y2=m_select_y1=m_curs_y;
          m_selecting=1;
        }

        if (m_selecting && (m_select_x2!=x || m_select_y2 != y))
        {
          if (y < m_offs_y && m_offs_y > 0)
          {
            m_offs_y--;           
          }
          else if (y > m_offs_y + getVisibleLines() && m_offs_y + getVisibleLines() < m_text.GetSize())
          {
            m_offs_y++;
          }
          if (x < m_offs_x && m_offs_x > 0)
          {
            m_offs_x--;
          }
          else if (x > m_offs_x+COLS)
          {
            int sz=0;
            int a;
            const int vl=getVisibleLines();
            for (a=0;a<vl;a++)
              if (m_text.Get(m_offs_y + a) && m_text.Get(m_offs_y + a)->GetLength() > sz) sz=m_text.Get(m_offs_y + a)->GetLength();
            if (sz > m_offs_x + COLS - 8) m_offs_x++;
          }
          m_select_y2=y;
          m_select_x2=x;
          if (m_select_y2<0) m_select_y2=0;
          else if (m_select_y2>=m_text.GetSize()) 
          {
            m_select_y2=m_text.GetSize()-1;
            WDL_FastString *s=m_text.Get(m_select_y2);
            if (s) m_select_x2 = s->GetLength();
          }
          if (m_select_x2<0)m_select_x2=0;
          WDL_FastString *s=m_text.Get(m_select_y2);
          if (s && m_select_x2>s->GetLength()) m_select_x2 = s->GetLength();
          draw();
          if (m_curs_y >= m_offs_y && m_curs_y < m_offs_y + getVisibleLines()) setCursor();
        }
      }
    break;
    case WM_RBUTTONDOWN:
    case WM_LBUTTONDOWN:
      if (CURSES_INSTANCE->m_font_w && CURSES_INSTANCE->m_font_h)
      {
        if (uMsg == WM_LBUTTONDOWN) m_selecting=0;
        m_curs_x = m_offs_x + LOWORD(lParam) / CURSES_INSTANCE->m_font_w;
        int a = HIWORD(lParam)/CURSES_INSTANCE->m_font_h - m_top_margin;
        if (a>=getVisibleLines()) a=getVisibleLines()-1;
        if (a<0)a=0;
        m_curs_y = m_offs_y + a;
        if (m_curs_y > m_text.GetSize()-1) 
        {
          m_curs_y = m_text.GetSize()-1;
          WDL_FastString *s=m_text.Get(m_curs_y);
          if (s) m_curs_x=s->GetLength();
        }
        if (m_curs_x < 0) m_curs_x = 0;
        WDL_FastString *s=m_text.Get(m_curs_y);
        if (s && m_curs_x > s->GetLength()) m_curs_x=s->GetLength();
        if (m_curs_y < 0) m_curs_y = 0;

        draw();
        setCursor();

        if (uMsg == WM_LBUTTONDOWN) 
        {
          SetCapture(hwnd);
        }
        else
        {
          onRightClick();
        }
      }
    return 0;
    case WM_LBUTTONUP:
      ReleaseCapture();
    return 0;

    case WM_MOUSEWHEEL:
      m_offs_y -= ((short)HIWORD(wParam))/20;
      if(m_offs_y > m_text.GetSize() - getVisibleLines()) m_offs_y=m_text.GetSize() - getVisibleLines();
      if (m_offs_y < 0) m_offs_y=0;

      draw();
      if (m_curs_y >= m_offs_y && m_curs_y < m_offs_y + getVisibleLines()) setCursor();
      else draw_status_state();

    break;
  }
  return 0;
}


WDL_CursesEditor::~WDL_CursesEditor()
{
  endwin();
  m_text.Empty(true);
  m_undoStack.Empty(true);
}

int WDL_CursesEditor::init(const char *fn, const char *init_if_empty)
{
  m_filename.Set(fn);
  FILE *fh=fopenUTF8(fn,"rt");

  if (!fh) 
  {
    if (init_if_empty)
    {
      fh=fopenUTF8(fn,"w+t");
      if (fh)
      {
        fwrite(init_if_empty,1,strlen(init_if_empty),fh);

        fseek(fh,0,SEEK_SET);
      }
    }
    if (!fh)
    {
      saveUndoState();
      m_clean_undopos=m_undoStack_pos;
      return 1;
    }
  }
  while(!feof(fh))
  {
    char line[4096];
    line[0]=0;
    fgets(line,sizeof(line),fh);
    if (!line[0]) break;

    int l=strlen(line);
    while(l>0 && (line[l-1]=='\r' || line[l-1]=='\n'))
    {
      line[l-1]=0;
      l--;
    }
    WDL_FastString *str=new WDL_FastString;
    char *p=line,*np;
    while ((np=strstr(p,"\t"))) // this should be optional, perhaps
    {
      *np=0;
      str->Append(p);
      str->Append(TAB_STR);
      p=np+1;
    }
    if (p) str->Append(p);
    m_text.Add(str);
  }
  fclose(fh);
  saveUndoState();
  m_clean_undopos=m_undoStack_pos;

  return 0;
}

void WDL_CursesEditor::draw_status_state()
{
  // always show this? if (m_bottom_margin>0)
  {
    char statusstr[512];
    snprintf(statusstr,sizeof(statusstr),"Line %d/%d, Col %d [%s%s]%s",m_curs_y+1,m_text.GetSize(),m_curs_x,s_overwrite?"OVR":"INS","",m_clean_undopos == m_undoStack_pos ? "" :"M");


    attrset(m_color_statustext);
    bkgdset(m_color_statustext);
 
    mvaddstr(LINES-1,COLS-28,statusstr);
    clrtoeol();

    attrset(0);
    bkgdset(0);
  }
}

void WDL_CursesEditor::setCursor(int isVscroll)
{
  int maxx=m_text.Get(m_curs_y) ? m_text.Get(m_curs_y)->GetLength() : 0;

  if (isVscroll)
  {
    if (m_want_x >= 0) m_curs_x=m_want_x;
  }
  else m_want_x=-1;

  if(m_curs_x>maxx)
  {
    if (isVscroll) m_want_x=m_curs_x;
    m_curs_x=maxx;
  }
  int redraw=0;

  if (m_curs_x < m_offs_x) { redraw=1; m_offs_x=m_curs_x; }
  else if (m_curs_x >= m_offs_x + COLS) { m_offs_x = m_curs_x - COLS + 1; redraw=1; }

  //screen boundary checking

  if(m_curs_y>=(m_offs_y+getVisibleLines()))
  {
    m_offs_y = m_curs_y - getVisibleLines()+1;
    redraw=1;
  }

  if(m_curs_y<m_offs_y)
  {
    m_offs_y=m_curs_y;
    redraw=1;
  }

  if(redraw) draw();

  draw_status_state();
  
  
  //put cursor on screen
  move(m_top_margin+m_curs_y-m_offs_y,m_curs_x-m_offs_x);
}

void WDL_CursesEditor::draw_message(const char *str)
{
  int l=strlen(str);
  if (l > COLS-2) l=COLS-2;
  if (str[0]) 
  {
    attrset(m_color_message);
    bkgdset(m_color_message);
  }
  mvaddnstr(LINES-(m_bottom_margin>1?2:1),0,str,l);
  clrtoeol();
  if (str[0])
  {
    attrset(0);
    bkgdset(0);
  }
}


void WDL_CursesEditor::mvaddnstr_highlight(int y, int x, const char *p, int ml, int *c_comment_state, int skipcnt)
{
  move(y,x);
  attrset(A_NORMAL);
  while (ml > 0 && *p)
  {
    if (--skipcnt < 0) addch(*p);
    p++;
    ml--;
  }
  if (ml > 0) clrtoeol();
}

void WDL_CursesEditor::getselectregion(int &minx, int &miny, int &maxx, int &maxy) // gets select region
{
    if (m_select_y2 < m_select_y1)
    {
      miny=m_select_y2; maxy=m_select_y1;
      minx=m_select_x2; maxx=m_select_x1;
    }
    else if (m_select_y1 < m_select_y2)
    {
      miny=m_select_y1; maxy=m_select_y2;
      minx=m_select_x1; maxx=m_select_x2;
    }
    else
    {
      miny=maxy=m_select_y1;
      minx=min(m_select_x1,m_select_x2);
      maxx=max(m_select_x1,m_select_x2);
    }
}

void WDL_CursesEditor::doDrawString(int y, int x, int line_n, const char *p, int ml, int *c_comment_state, int skipcnt)
{
  if (skipcnt < 0) skipcnt=0;
  mvaddnstr_highlight(y,x,p,ml + skipcnt,c_comment_state, skipcnt);

  if (m_selecting)
  {
    int miny,maxy,minx,maxx;
    getselectregion(minx,miny,maxx,maxy);
   
    if (line_n >= miny && line_n <= maxy && (miny != maxy || minx < maxx))
    {
      minx-=skipcnt;
      maxx-=skipcnt;

      if (line_n > miny) minx=0;
      if (line_n < maxy) maxx=ml;

      if (minx<0)minx=0;
      if (minx > ml) minx=ml;
      if (maxx > ml) maxx=ml;

      if (maxx > minx)
      {
        int a = skipcnt + minx;
        while (a-- > 0 && *p) p++;

        a=strlen(p);
        if (a > maxx-minx) a= maxx-minx;

        attrset(m_color_selection);
        mvaddnstr(y,x+minx, p, a);
        attrset(A_NORMAL);
      }
      else if (maxx==minx && !*p && ml>0)
      {
        attrset(m_color_selection);
        mvaddstr(y,x+minx," ");
        attrset(A_NORMAL);
      }
    }
  }
}

int WDL_CursesEditor::GetCommentStateForLineStart(int line) // pass current line/col, updates with previous interesting point, returns true if start of comment, or false if end of previous comment
{
  return 0;
}

void WDL_CursesEditor::draw(int lineidx)
{
  attrset(A_NORMAL);

  if (lineidx >= 0)
  {
    int comment_state = GetCommentStateForLineStart(lineidx);
    WDL_FastString *s=m_text.Get(lineidx);
    if (s && lineidx >= m_offs_y && lineidx < m_offs_y+getVisibleLines())
    {
      doDrawString(lineidx-m_offs_y+m_top_margin,0,lineidx,s->Get(),COLS,&comment_state, min(s->GetLength(),m_offs_x));
    }
    return;
  }

  draw_top_line();

  attrset(A_NORMAL);
  bkgdset(A_NORMAL);

  move(m_top_margin,0);
  clrtoeol();

  int comment_state = GetCommentStateForLineStart(m_offs_y);
  const int VISIBLE_LINES = getVisibleLines();
  for(int i=0;i<VISIBLE_LINES;i++)
  { 
    int ln=i+m_offs_y;

    WDL_FastString *s=m_text.Get(ln);
    if(!s) 
    {
      move(i+m_top_margin,0);
      clrtoeol();
      continue;
    }

    doDrawString(i+m_top_margin,0,ln,s->Get(),COLS,&comment_state,min(m_offs_x,s->GetLength()));
  }
  
//  move(LINES-2,0);
//  clrtoeol();
  if (m_bottom_margin>0)
  {
    attrset(m_color_bottomline);
    bkgdset(m_color_bottomline);
    if (m_selecting) 
    {
      mvaddstr(LINES-1,0,"SELECTING - ESC-cancel, " "Ctrl+C,X,V, etc");
    }
    else 
    {
      draw_bottom_line();
    }
    clrtoeol();
    attrset(0);
    bkgdset(0);
  }
}

void WDL_CursesEditor::draw_bottom_line()
{
  if (m_bottom_margin>0)
  {
    mvaddstr(LINES-1,0,"Ctrl+(");

  #define DO(x,y) { attrset(m_color_bottomline|A_BOLD); addstr(x); attrset(m_color_bottomline&~A_BOLD); addstr(y);}
      DO("F","ind ");
      DO("","ma");
      DO("T","");
      DO("","ch");
  #undef DO
      addstr(")");
  }
}

int WDL_CursesEditor::updateFile()
{
  FILE *fp=fopenUTF8(m_filename.Get(),"wt");
  if (!fp) return 1;
  int x;
  for (x = 0; x < m_text.GetSize(); x ++)
  {
    if (m_text.Get(x)) fprintf(fp,"%s\n",m_text.Get(x)->Get());
  }
  fclose(fp);
  sync();
  m_clean_undopos = m_undoStack_pos;
  return 0;
}

void WDL_CursesEditor::indentSelect(int amt)
{
  if (m_selecting)  // remove selected text
  {
    int miny,maxy,minx,maxx;
    int x;
    getselectregion(minx,miny,maxx,maxy);
    if (maxy >= miny)
    {
      for (x = miny; x <= maxy; x ++)
      {
        WDL_FastString *s=m_text.Get(x);
        if (s)
        {
          if (amt>0)
          {
            int a;
            for(a=0;a<amt;a+=16)
              s->Insert("                  ",0,min(amt-a,16));
          }
          else if (amt<0)
          {
            int a=0;
            while (a<-amt && s->Get()[a]== ' ') a++;
            s->DeleteSub(0,a);
          }
        }
      }
    }
  }
}

void WDL_CursesEditor::removeSelect()
{
  if (m_selecting)  // remove selected text
  {
    int miny,maxy,minx,maxx;
    int x;
    getselectregion(minx,miny,maxx,maxy);
    m_curs_x = minx;
    m_curs_y = miny;
    if (m_curs_y < 0) m_curs_y=0;
          
      if (minx != maxx|| miny != maxy) 
      {
        int fht=0,lht=0;
        for (x = miny; x <= maxy; x ++)
        {
          WDL_FastString *s=m_text.Get(x);
          if (s)
          {
            int sx,ex;
            if (x == miny) sx=max(minx,0);
            else sx=0;

            int tmp=s->GetLength();
            if (sx > tmp) sx=tmp;
      
            if (x == maxy) ex=min(maxx,tmp);
            else ex=tmp;
      
            if (sx == 0 && ex == tmp) // remove entire line
            {
              m_text.Delete(x,true);
              if (x==miny) miny--;
              x--;
              maxy--;
            }
            else { if (x==miny) fht=1; if (x == maxy) lht=1; s->DeleteSub(sx,ex-sx); }
          }
        }
        if (fht && lht && miny+1 == maxy)
        {
          m_text.Get(miny)->Append(m_text.Get(maxy)->Get());
          m_text.Delete(maxy,true);
        }

      }
      m_selecting=0;
    }

}

static WDL_FastString *newIndentedFastString(const char *tstr, int indent_to_pos)
{
  WDL_FastString *s=new WDL_FastString;
  if (indent_to_pos>=0) 
  {
    while (*tstr == '\t' || *tstr == ' ') tstr++;
    int x;
    for (x=0;x<indent_to_pos;x++) s->Append(" ");
  }
  s->Append(tstr);
  return s;

}

void WDL_CursesEditor::runSearch()
{
   if (s_search_string[0]) 
   {
     int wrapflag=0,found=0;
     int line;
     int numlines = m_text.GetSize();
     int startx=m_curs_x+1;
     const int srchlen=strlen(s_search_string);
     for (line = m_curs_y; line < numlines && !found; line ++)
     {
       WDL_FastString *tl = m_text.Get(line);
       const char *p;

       if (tl && (p=tl->Get()))
       {
         int linelen = tl->GetLength();
         for (; startx < linelen-srchlen; startx++)
           if (!strnicmp(p+startx,s_search_string,srchlen)) 
           {
             m_curs_y=line;
             m_curs_x=startx;
             found=1;
             break;
           }
       }
            

       startx=0;
     }
     if (!found && (m_curs_y>0 || m_curs_x > 0))
     {
       wrapflag=1;
       numlines = min(m_curs_y+1,numlines);
       for (line = 0; line < numlines && !found; line ++)
       {
         WDL_FastString *tl = m_text.Get(line);
         const char *p;

         if (tl && (p=tl->Get()))
         {
           int linelen = tl->GetLength();
           for (; startx < linelen-srchlen; startx++)
             if (!strnicmp(p+startx,s_search_string,srchlen)) 
             {
               m_curs_y=line;
               m_curs_x=startx;
               found=1;
               break;
             }
         }           
         startx=0;
       }
     }
     if (found)
     {
       draw();
       char buf[512];
       snprintf(buf,sizeof(buf),"Found %s'%s'",wrapflag?"(wrapped) ":"",s_search_string);
       draw_message(buf);
       setCursor();
       return;
     }
   }

   draw();
   char buf[512];
   if (s_search_string[0]) snprintf(buf,sizeof(buf),"String '%s' not found",s_search_string);
   else lstrcpyn_safe(buf,"No search string",sizeof(buf));
   draw_message(buf);
   setCursor();
}

static int categorizeCharForWordNess(int c)
{
  if (isspace(c)) return 0;
  if (isalnum(c) || c == '_') return 1;
  if (c==';') return 2; // I prefer this, since semicolons are somewhat special

  return 3;
}

int WDL_CursesEditor::onChar(int c)
{
  if (m_state == -3 || m_state == -4)
  {
    switch (c)
    {
       case '\r': case '\n':
         m_state=0;
         runSearch();
       break;
       case 27: 
         m_state=0; 
         draw();
         draw_message("Find cancelled.");
         setCursor();
       break;
       case KEY_BACKSPACE: if (s_search_string[0]) s_search_string[strlen(s_search_string)-1]=0; m_state=-4; break;
       default: 
         if (VALIDATE_TEXT_CHAR(c)) 
         { 
           int l=m_state == -3 ? 0 : strlen(s_search_string); 
           m_state = -4;
           if (l < (int)sizeof(s_search_string)-1) { s_search_string[l]=c; s_search_string[l+1]=0; } 
         } 
        break;
     }
     if (m_state)
     {
       attrset(m_color_message);
       bkgdset(m_color_message);
       mvaddstr(LINES-1,29,s_search_string);
       clrtoeol(); 
       attrset(0);
       bkgdset(0);
     }
     return 0;
  }
  if (c==KEY_DOWN || c==KEY_UP || c==KEY_PPAGE||c==KEY_NPAGE || c==KEY_RIGHT||c==KEY_LEFT||c==KEY_HOME||c==KEY_END)
  {
    if (GetAsyncKeyState(VK_SHIFT)&0x8000)      
    {
      if (!m_selecting)
      {
        m_select_x2=m_select_x1=m_curs_x; m_select_y2=m_select_y1=m_curs_y;
        m_selecting=1;
      }
    }
    else if (m_selecting) { m_selecting=0; draw(); }
  }

  switch(c)
  {
    case 407:
    case 'Z'-'A'+1:
      if (!(GetAsyncKeyState(VK_SHIFT)&0x8000))
      {
        if (m_undoStack_pos > 0)
        {
           m_undoStack_pos--;
           loadUndoState(m_undoStack.Get(m_undoStack_pos));
           draw();
           char buf[512];
           snprintf(buf,sizeof(buf),"Undid action -- %d items in undo buffer",m_undoStack_pos);
           draw_message(buf);
           setCursor();
        }
        else draw_message("Can't Undo");

        break;
      }
      // fall through
    case 'Y'-'A'+1:
      if (m_undoStack_pos < m_undoStack.GetSize()-1)
      {
        m_undoStack_pos++;
        loadUndoState(m_undoStack.Get(m_undoStack_pos));
        draw();
        char buf[512];
        snprintf(buf,sizeof(buf),"Redid action -- %d items in redo buffer",m_undoStack.GetSize()-m_undoStack_pos-1);
        draw_message(buf);
        setCursor();
      }
      else draw_message("Can't Redo");  
    break;
    case KEY_IC:
      if (!(GetAsyncKeyState(VK_SHIFT)&0x8000))
      {
        s_overwrite=!s_overwrite;
        setCursor();
        break;
      }
      // fqll through
    case 'V'-'A'+1:
      {
        // generate a m_clipboard using win32 clipboard data
        WDL_PtrList<const char> lines;
        WDL_String buf;
#ifdef WDL_IS_FAKE_CURSES
        if (CURSES_INSTANCE)
        {
          OpenClipboard(CURSES_INSTANCE->m_hwnd);
          HANDLE h=GetClipboardData(CF_TEXT);
          if (h)
          {
            char *t=(char *)GlobalLock(h);
            int s=GlobalSize(h);
            buf.Set(t,s);
            GlobalUnlock(t);        
          }
          CloseClipboard();
        }
        else
#endif
        {
          buf.Set(s_fake_clipboard.Get());
        }

        if (buf.Get() && buf.Get()[0])
        {
          char *src=buf.Get();
          while (*src)
          {
            char *seek=src;
            while (*seek && *seek != '\r' && *seek != '\n') seek++;
            char hadclr=*seek;
            if (*seek) *seek++=0;
            lines.Add(src);

            if (hadclr == '\r' && *seek == '\n') seek++;

            if (hadclr && !*seek)
            {
              lines.Add("");
            }
            src=seek;
          }
        }
        if (lines.GetSize())
        {
          removeSelect();
          // insert lines at m_curs_y,m_curs_x
          if (m_curs_y >= m_text.GetSize()) m_curs_y=m_text.GetSize()-1;
          if (m_curs_y < 0) m_curs_y=0;

          preSaveUndoState();
          WDL_FastString poststr;
          int x;
          int indent_to_pos = -1;
          for (x = 0; x < lines.GetSize(); x ++)
          {
            WDL_FastString *str=m_text.Get(m_curs_y);
            const char *tstr=lines.Get(x);
            if (!tstr) tstr="";
            if (!x)
            {
              if (str)
              {
                if (m_curs_x < 0) m_curs_x=0;
                int tmp=str->GetLength();
                if (m_curs_x > tmp) m_curs_x=tmp;
  
                poststr.Set(str->Get()+m_curs_x);
                str->SetLen(m_curs_x);

                const char *p = str->Get();
                while (*p == ' ' || *p == '\t') p++;
                if (!*p && p > str->Get())
                {
                  if (lines.GetSize()>1)
                  {
                    while (*tstr == ' ' || *tstr == '\t') tstr++;
                  }
                  indent_to_pos = m_curs_x;
                }

                str->Append(tstr);
              }
              else
              {
                m_text.Insert(m_curs_y,(str=new WDL_FastString(tstr)));
              }
              if (lines.GetSize() > 1)
              {
                m_curs_y++;
              }
              else
              {
                m_curs_x = str->GetLength();
                str->Append(poststr.Get());
              }
           }
           else if (x == lines.GetSize()-1)
           {
             WDL_FastString *s=newIndentedFastString(tstr,indent_to_pos);
             m_curs_x = s->GetLength();
             s->Append(poststr.Get());
             m_text.Insert(m_curs_y,s);
           }
           else
           {
             m_text.Insert(m_curs_y,newIndentedFastString(tstr,indent_to_pos));
             m_curs_y++;
           }
         }
         draw();
         draw_message("Pasted");
         saveUndoState();
         setCursor();
       }
       else 
       {
         draw_message("Clipboard empty");
         setCursor();
       }
     }
  break;

  case KEY_DC:
    if (!(GetAsyncKeyState(VK_SHIFT)&0x8000))
    {
      WDL_FastString *s;
      if (m_selecting)
      {
        preSaveUndoState();
        removeSelect();
        draw();
        saveUndoState();
        setCursor();
      }
      else if ((s=m_text.Get(m_curs_y)))
      {
        if (m_curs_x < s->GetLength())
        {
          preSaveUndoState();

          bool hadCom = LineCanAffectOtherLines(s->Get(),m_curs_x,1); 
          s->DeleteSub(m_curs_x,1);
          if (!hadCom) hadCom = LineCanAffectOtherLines(s->Get(),-1,-1);
          draw(hadCom ? -1 : m_curs_y);
          saveUndoState();
          setCursor();
        }
        else // append next line to us
        {
          if (m_curs_y < m_text.GetSize()-1)
          {
            preSaveUndoState();

            WDL_FastString *nl=m_text.Get(m_curs_y+1);
            if (nl)
            {
              s->Append(nl->Get());
            }
            m_text.Delete(m_curs_y+1,true);

            draw();
            saveUndoState();
            setCursor();
          }
        }
      }
      break;
    }
  case 'C'-'A'+1:
  case 'X'-'A'+1:
    if (m_selecting)
    {
      if (c!= 'C'-'A'+1) m_selecting=0;
      int miny,maxy,minx,maxx;
      int x;
      getselectregion(minx,miny,maxx,maxy);
      const char *status="";
      char statusbuf[512];

      if (minx != maxx|| miny != maxy) 
      {
        int bytescopied=0;
        s_fake_clipboard.Set("");

        int lht=0,fht=0;
        if (c != 'C'-'A'+1) preSaveUndoState();

        for (x = miny; x <= maxy; x ++)
        {
          WDL_FastString *s=m_text.Get(x);
          if (s) 
          {
            const char *str=s->Get();
            int sx,ex;
            if (x == miny) sx=max(minx,0);
            else sx=0;
            int tmp=s->GetLength();
            if (sx > tmp) sx=tmp;
      
            if (x == maxy) ex=min(maxx,tmp);
            else ex=tmp;
      
            bytescopied += ex-sx + (x!=maxy);
            if (s_fake_clipboard.Get() && s_fake_clipboard.Get()[0]) s_fake_clipboard.Append("\r\n");
            s_fake_clipboard.Append(ex-sx?str+sx:"",ex-sx);

            if (c != 'C'-'A'+1)
            {
              if (sx == 0 && ex == tmp) // remove entire line
              {
                m_text.Delete(x,true);
                if (x==miny) miny--;
                x--;
                maxy--;
              }
              else { if (x==miny) fht=1; if (x == maxy) lht=1; s->DeleteSub(sx,ex-sx); }
            }
          }
        }
        if (fht && lht && miny+1 == maxy)
        {
          m_text.Get(miny)->Append(m_text.Get(maxy)->Get());
          m_text.Delete(maxy,true);
        }
        if (c != 'C'-'A'+1)
        {
          m_curs_y=miny;
          if (m_curs_y < 0) m_curs_y=0;
          m_curs_x=minx;
          saveUndoState();
          snprintf(statusbuf,sizeof(statusbuf),"Cut %d bytes",bytescopied);
        }
        else
          snprintf(statusbuf,sizeof(statusbuf),"Copied %d bytes",bytescopied);

#ifdef WDL_IS_FAKE_CURSES
        if (CURSES_INSTANCE)
        {
          int l=s_fake_clipboard.GetLength()+1;
          HANDLE h=GlobalAlloc(GMEM_MOVEABLE,l);
          void *t=GlobalLock(h);
          memcpy(t,s_fake_clipboard.Get(),l);
          GlobalUnlock(h);
          OpenClipboard(CURSES_INSTANCE->m_hwnd);
          EmptyClipboard();
          SetClipboardData(CF_TEXT,h);
          CloseClipboard();
        }
#endif

        status=statusbuf;
      }
      else status="No selection";

      draw();
      draw_message(status);
      setCursor();
    }
  break;
  case 'A'-'A'+1:
    m_selecting=1;
    m_select_x1=0;
    m_select_y1=0;
    m_select_y2=m_text.GetSize()-1;
    m_select_x2=0;
    if (m_text.Get(m_select_y2))
      m_select_x2=m_text.Get(m_select_y2)->GetLength();
    draw();
    setCursor();
  break;
  case 27:
    if (m_selecting)
    {
      m_selecting=0;
      draw();
      setCursor();
      break;
    }
  return 0;
  case KEY_F3:
  case 'G'-'A'+1:
    if (s_search_string[0])
    {
      runSearch();
      return 0;
    }
  case 'F'-'A'+1:
    draw_message("");
    attrset(m_color_message);
    bkgdset(m_color_message);
    mvaddstr(LINES-1,0,"Find string (ESC to cancel): ");
    addstr(s_search_string);
    clrtoeol();
    attrset(0);
    bkgdset(0);
    m_state=-3; // find, initial (m_state=4 when we've typed something)
  return 0;
  case KEY_DOWN:
    {
      if (GetAsyncKeyState(VK_CONTROL)&0x8000)      
      {
        if (m_offs_y < m_text.GetSize() - 4) 
        {
          m_offs_y++;
          if (m_curs_y < m_offs_y) m_curs_y = m_offs_y;
          draw();
        }
      }
      else
      {
        m_curs_y++;
        if(m_curs_y>=m_text.GetSize()) m_curs_y=m_text.GetSize()-1;
        if(m_curs_y < 0) m_curs_y=0;
      }
      if (m_selecting) { setCursor(1); m_select_x2=m_curs_x; m_select_y2=m_curs_y; draw(); }
      setCursor(1);
    }
  break;
  case KEY_UP:
    {
      if (GetAsyncKeyState(VK_CONTROL)&0x8000)      
      {
        if (m_offs_y>0) 
        {
          m_offs_y--;
          if (m_curs_y>m_offs_y + getVisibleLines() - 1) m_curs_y = m_offs_y + getVisibleLines() - 1;
          if (m_curs_y < 0) m_curs_y=0;
          draw();
        }
      }
      else
      {
        if(m_curs_y>0) m_curs_y--;
      }
      if (m_selecting) { setCursor(1); m_select_x2=m_curs_x; m_select_y2=m_curs_y; draw(); }
      setCursor(1);
    }
  break;
  case KEY_PPAGE:
    {
      if (m_curs_y > m_offs_y) 
      {
        m_curs_y=m_offs_y;
        if (m_curs_y < 0) m_curs_y=0;
      }
      else 
      {
        m_curs_y -= getVisibleLines();
        if (m_curs_y < 0) m_curs_y=0;
        m_offs_y=m_curs_y;
      }
      if (m_selecting) { setCursor(1); m_select_x2=m_curs_x; m_select_y2=m_curs_y; }
      draw();
      setCursor(1);
    }
  break;
  case KEY_NPAGE:
    {
      if (m_curs_y >= m_offs_y+getVisibleLines() - 1) m_offs_y=m_curs_y+1;
      m_curs_y = m_offs_y+getVisibleLines() - 1;
      if (m_curs_y >= m_text.GetSize()) m_curs_y=m_text.GetSize()-1;
      if (m_curs_y < 0) m_curs_y=0;
      if (m_selecting) { setCursor(1); m_select_x2=m_curs_x; m_select_y2=m_curs_y; }
      draw();
      setCursor(1);
    }
  break;
  case KEY_RIGHT:
    {
      if (1) // wrap across lines
      {
        WDL_FastString *s = m_text.Get(m_curs_y);
        if (s && m_curs_x >= s->GetLength() && m_curs_y < m_text.GetSize()) { m_curs_y++; m_curs_x = -1; }
      }

      if(m_curs_x<0) 
      {
        m_curs_x=0;
      }
      else
      {
        if (GetAsyncKeyState(VK_CONTROL)&0x8000)      
        {
          WDL_FastString *s = m_text.Get(m_curs_y);
          if (!s||m_curs_x >= s->GetLength()) break;
          int lastType = categorizeCharForWordNess(s->Get()[m_curs_x++]);
          while (m_curs_x < s->GetLength())
          {
            int thisType = categorizeCharForWordNess(s->Get()[m_curs_x]);
            if (thisType != lastType && thisType != 0) break;
            lastType=thisType;
            m_curs_x++;
          }
        }
        else 
        {
          m_curs_x++;
        }
      }
      if (m_selecting) { setCursor(); m_select_x2=m_curs_x; m_select_y2=m_curs_y; draw(); }
      setCursor();
    }
  break;
  case KEY_LEFT:
    {
      bool doMove=true;
      if (1) // wrap across lines
      {
        WDL_FastString *s = m_text.Get(m_curs_y);
        if (s && m_curs_y>0 && m_curs_x == 0) 
        { 
          s = m_text.Get(--m_curs_y);
          if (s) 
          {
            m_curs_x = s->GetLength(); 
            doMove=false;
          }
        }
      }

      if(m_curs_x>0 && doMove) 
      {
        if (GetAsyncKeyState(VK_CONTROL)&0x8000)      
        {
          WDL_FastString *s = m_text.Get(m_curs_y);
          if (!s) break;
          if (m_curs_x > s->GetLength()) m_curs_x = s->GetLength();
          m_curs_x--;

          int lastType = categorizeCharForWordNess(s->Get()[m_curs_x--]);
          while (m_curs_x >= 0)
          {
            int thisType = categorizeCharForWordNess(s->Get()[m_curs_x]);
            if (thisType != lastType && lastType != 0) break;
            lastType=thisType;
            m_curs_x--;
          }
          m_curs_x++;
        }
        else 
        {
          m_curs_x--;
        }
      }
      if (m_selecting) { setCursor(); m_select_x2=m_curs_x; m_select_y2=m_curs_y; draw(); }
      setCursor();
    }
  break;
  case KEY_HOME:
    {
      m_curs_x=0;
      if (m_selecting) { setCursor(); m_select_x2=m_curs_x; m_select_y2=m_curs_y; draw(); }
      setCursor();
    }
  break;
  case KEY_END:
    {
      if (m_text.Get(m_curs_y)) m_curs_x=m_text.Get(m_curs_y)->GetLength();
      if (m_selecting) { setCursor(); m_select_x2=m_curs_x; m_select_y2=m_curs_y; draw(); }
      setCursor();
    }
  break;
  case KEY_BACKSPACE: // backspace, baby
    if (m_selecting)
    {
      preSaveUndoState();
      removeSelect();
      draw();
      saveUndoState();
      setCursor();
    }
    else if (m_curs_x > 0)
    {
      WDL_FastString *tl=m_text.Get(m_curs_y);
      if (tl)
      {
        preSaveUndoState();

        bool hadCom = LineCanAffectOtherLines(tl->Get(), m_curs_x-1,1);
        tl->DeleteSub(--m_curs_x,1);
        if (!hadCom) hadCom = LineCanAffectOtherLines(tl->Get(),-1,-1);
        draw(hadCom?-1:m_curs_y);
        saveUndoState();
        setCursor();
      }
    }
    else // append current line to previous line
    {
      WDL_FastString *fl=m_text.Get(m_curs_y-1), *tl=m_text.Get(m_curs_y);
      if (fl && tl)
      {
        preSaveUndoState();
        m_curs_x=fl->GetLength();
        fl->Append(tl->Get());

        m_text.Delete(m_curs_y--,true);
        draw();
        saveUndoState();
        setCursor();
      }
    }
  break;
  case 'L'-'A'+1:
    draw();
    setCursor();
  break;
  case 13: //KEY_ENTER:
    //insert newline
    preSaveUndoState();

    if (m_selecting) { removeSelect(); draw(); setCursor(); }
    if (s_overwrite)
    {
      WDL_FastString *s = m_text.Get(m_curs_y);
      int plen=0;
      const char *pb=NULL;
      if (s)
      {
        pb = s->Get();
        while (plen < m_curs_x && (pb[plen]== ' ' || pb[plen] == '\t')) plen++;
      }
      if (++m_curs_y >= m_text.GetSize())
      {
        m_curs_y = m_text.GetSize();
        WDL_FastString *ns=new WDL_FastString;
        if (plen>0) ns->Set(pb,plen);
        m_text.Insert(m_curs_y,ns);
      }
      s = m_text.Get(m_curs_y);
      if (s && plen > s->GetLength()) plen=s->GetLength();
      m_curs_x=plen;
    }
    else 
    {
      WDL_FastString *s = m_text.Get(m_curs_y);
      if (s)
      {
        if (m_curs_x > s->GetLength()) m_curs_x = s->GetLength();
        WDL_FastString *nl = new WDL_FastString();
        int plen=0;
        const char *pb = s->Get();
        while (plen < m_curs_x && (pb[plen]== ' ' || pb[plen] == '\t')) plen++;

        if (plen>0) nl->Set(pb,plen);

        nl->Append(pb+m_curs_x);
        m_text.Insert(++m_curs_y,nl);
        s->SetLen(m_curs_x);
        m_curs_x=plen;
      }
    }
    m_offs_x=0;

    draw();
    saveUndoState();
    setCursor();
  break;
  case '\t':
    if (m_selecting)
    {
      preSaveUndoState();

      bool isRev = !!(GetAsyncKeyState(VK_SHIFT)&0x8000);
      indentSelect(isRev?-2:2);
      // indent selection:
      draw();
      setCursor();
      saveUndoState();
      break;
    }
  default:
    //insert char
    if(VALIDATE_TEXT_CHAR(c))
    { 
      preSaveUndoState();

      if (m_selecting) { removeSelect(); draw(); setCursor(); }
      if (!m_text.Get(m_curs_y)) m_text.Insert(m_curs_y,new WDL_FastString);

      WDL_FastString *ss;
      if ((ss=m_text.Get(m_curs_y)))
      {
        char str[8]={c,};
        int slen = 1;
        if (c == '\t') { strcpy(str,TAB_STR); slen=strlen(TAB_STR); }

        bool hadCom = LineCanAffectOtherLines(ss->Get(),-1,-1);
        if (s_overwrite)
        {
          if (!hadCom) hadCom = LineCanAffectOtherLines(ss->Get(),m_curs_x,slen);
          ss->DeleteSub(m_curs_x,slen);
        }
        ss->Insert(str,m_curs_x);
        if (!hadCom) hadCom = LineCanAffectOtherLines(ss->Get(),m_curs_x,slen);

        m_curs_x += slen;
        draw(hadCom ? -1 : m_curs_y);
      }
      saveUndoState();
      setCursor();
    }
    break;
  }
  return 0;
}

void WDL_CursesEditor::preSaveUndoState() 
{
  editUndoRec *rec= m_undoStack.Get(m_undoStack_pos);
  if (rec)
  {
    rec->m_offs_x=m_offs_x;
    rec->m_offs_y=m_offs_y;
    rec->m_curs_x=m_curs_x;
    rec->m_curs_y=m_curs_y;
  }
}
void WDL_CursesEditor::saveUndoState() 
{
  editUndoRec *rec=new editUndoRec;
  rec->m_offs_x=m_offs_x;
  rec->m_offs_y=m_offs_y;
  rec->m_curs_x=m_curs_x;
  rec->m_curs_y=m_curs_y;

  editUndoRec *lrec[3];
  lrec[0] = m_undoStack.Get(m_undoStack_pos);
  lrec[1] = m_undoStack.Get(m_undoStack_pos+1);
  lrec[2] = m_undoStack.Get(m_undoStack_pos-1);

  int x;
  for (x = 0; x < m_text.GetSize(); x ++)
  {
    refcntString *ns = NULL;

    const WDL_FastString *s=m_text.Get(x);
    const int s_len=s?s->GetLength():0;
    
    if (s_len>0)
    {
      const char *cs=s?s->Get():"";
      int w;
      for(w=0;w<3 && !ns;w++)
      {
        if (lrec[w])
        {
          int y;
          for (y=0; y<7; y ++)
          {
            refcntString *a = lrec[w]->m_htext.Get(x + ((y&1) ? -(y+1)/2 : y/2));
            if (a && a->getStrLen() == s_len && !strcmp(a->getStr(),cs))
            {
              ns = a;
              break;
            }
          }
        }
      }

      if (!ns) ns = new refcntString(cs,s_len);

      ns->AddRef();
    }
    else
    {
      // ns is NULL, blank line
    }

    rec->m_htext.Add(ns);
  }

  for (x = m_undoStack.GetSize()-1; x > m_undoStack_pos; x --)
  {
    m_undoStack.Delete(x,true);
  }
  if (m_clean_undopos > m_undoStack_pos) 
    m_clean_undopos=-1;
  
  if (m_undoStack.GetSize() > 200) // only keep 100-200 copies around
  {
    for (x = 1; x < m_undoStack.GetSize(); x += 2)
    {
      if (x != m_clean_undopos)// don't delete our preferred special (last saved) one
      {
        m_undoStack.Delete(x,true);
        if (m_clean_undopos > x) m_clean_undopos--;
        x--;
      }
    }

    m_undoStack_pos = m_undoStack.GetSize()-1;
  }
  m_undoStack_pos++;
  m_undoStack.Add(rec);
}

void WDL_CursesEditor::loadUndoState(editUndoRec *rec)
{
  int x;
  m_text.Empty(true);
  for (x = 0; x < rec->m_htext.GetSize(); x ++)
  {
    refcntString *s=rec->m_htext.Get(x);
    m_text.Add(new WDL_FastString(s?s->getStr():""));
  }

  m_offs_x=rec->m_offs_x;
  m_offs_y=rec->m_offs_y;
  m_curs_x=rec->m_curs_x;
  m_curs_y=rec->m_curs_y;
}


void WDL_CursesEditor::RunEditor()
{
  int x;
  for(x=0;x<16;x++)
  {
    if (!CURSES_INSTANCE) break;

    int thischar = getch();
    if (thischar==ERR) break;

    if (onChar(thischar)) break;
  }
}
