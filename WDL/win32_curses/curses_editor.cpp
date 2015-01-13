#ifdef _WIN32
#include <windows.h>
#else
#include "../swell/swell.h"
#endif
#include <stdlib.h>
#include <string.h>
#ifndef CURSES_INSTANCE
#define CURSES_INSTANCE ((win32CursesCtx*)m_cursesCtx)
#endif

#include "curses_editor.h"
#include "../win32_utf8.h"
#include "../wdlcstring.h"
#include "curses.h"

#ifndef VALIDATE_TEXT_CHAR
#define VALIDATE_TEXT_CHAR(thischar) ((thischar) >= 0 && (thischar) < 256 && (isspace(thischar) || isgraph(thischar)))
#endif


WDL_FastString WDL_CursesEditor::s_fake_clipboard;
int WDL_CursesEditor::s_overwrite=0;
char WDL_CursesEditor::s_search_string[256];

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x20A
#endif

static void __curses_onresize(win32CursesCtx *ctx)
{
  WDL_CursesEditor *p = (WDL_CursesEditor *)ctx->user_data;
  if (p)
  {
    p->draw();
    p->setCursor();
  }
}
WDL_CursesEditor::WDL_CursesEditor(void *cursesCtx)
{ 
  m_max_undo_states = 500;
  m_indent_size=2;
  m_cursesCtx = cursesCtx;

  m_color_bottomline = COLOR_PAIR(1);
  m_color_statustext = COLOR_PAIR(1);
  m_color_selection = COLOR_PAIR(2);
  m_color_message = COLOR_PAIR(2);

  m_top_margin=0;
  m_bottom_margin=1;

  m_selecting=0;
  m_select_x1=m_select_y1=m_select_x2=m_select_y2=0;
  m_state=0;
  m_offs_x=0;
  m_curs_x=m_curs_y=0;
  m_want_x=-1;
  m_undoStack_pos=-1;
  m_clean_undopos=0;

  m_curpane=0;
  m_pane_div=1.0;
  m_paneoffs_y[0]=m_paneoffs_y[1]=0;

  m_curpane=0;
  m_scrollcap=0;
  m_scrollcap_yoffs=0;

#ifdef WDL_IS_FAKE_CURSES
  if (m_cursesCtx)
  {
    CURSES_INSTANCE->user_data = this;
    CURSES_INSTANCE->onMouseMessage = _onMouseMessage;
    CURSES_INSTANCE->want_scrollbar=1; // 1 or 2 chars wide
    CURSES_INSTANCE->do_update = __curses_onresize;
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

int  WDL_CursesEditor::GetPaneDims(int* paney, int* paneh) // returns ypos of divider
{
  const int pane_divy=(int)(m_pane_div*(double)(LINES-m_top_margin-m_bottom_margin-1));
  if (paney)
  {
    paney[0]=m_top_margin;
    paney[1]=m_top_margin+pane_divy+1;
  }
  if (paneh)
  {
    paneh[0]=pane_divy+(m_pane_div >= 1.0 ? 1 : 0);
    paneh[1]=LINES-pane_divy-m_top_margin-m_bottom_margin-1;
    
  }
  return pane_divy;
}


int WDL_CursesEditor::getVisibleLines() const { return LINES-m_bottom_margin-m_top_margin; }


LRESULT WDL_CursesEditor::onMouseMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static int s_mousedown[2];

  switch (uMsg)
  {
    case WM_CAPTURECHANGED:
    break;

    case WM_MOUSEMOVE:
      if (GetCapture()==hwnd && CURSES_INSTANCE->m_font_w && CURSES_INSTANCE->m_font_h)
      {
        POINT pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
        int cx=pt.x/CURSES_INSTANCE->m_font_w;
        int cy=pt.y/CURSES_INSTANCE->m_font_h;

        int paney[2], paneh[2];
        const int pane_divy=GetPaneDims(paney, paneh);

        if (m_scrollcap)
        {
          int i=m_scrollcap-1;

          if (i == 2) // divider
          {
            int divy=cy-paney[0];
            if (divy != pane_divy)
            {
              if (divy <= 0 || divy >= LINES-m_top_margin-m_bottom_margin-1)
              {
                if (divy <= 0.0) m_paneoffs_y[0]=m_paneoffs_y[1];
                m_curpane=0;
                m_pane_div=1.0;
              }
              else
              {
                m_pane_div=(double)divy/(double)(LINES-m_top_margin-m_bottom_margin-1);
              }
              draw();
              draw_status_state();
            }
          }
          else
          {
            int prevoffs=m_paneoffs_y[i];
            int pxy=paney[i]*CURSES_INSTANCE->m_font_h;
            int pxh=paneh[i]*CURSES_INSTANCE->m_font_h;

            if (pxh > CURSES_INSTANCE->scroll_h[i])
            {
              m_paneoffs_y[i]=(m_text.GetSize()-paneh[i])*(pt.y-m_scrollcap_yoffs-pxy)/(pxh-CURSES_INSTANCE->scroll_h[i]);
              int maxscroll=m_text.GetSize()-paneh[i]+4;
              if (m_paneoffs_y[i] > maxscroll) m_paneoffs_y[i]=maxscroll;
              if (m_paneoffs_y[i] < 0) m_paneoffs_y[i]=0;
            }

            if (m_paneoffs_y[i] != prevoffs)
            {
              draw();
              draw_status_state();
              
              const int col=m_curs_x-m_offs_x;
              int line=m_curs_y+paney[m_curpane]-m_paneoffs_y[m_curpane];
              if (line >= paney[m_curpane] && line < paney[m_curpane]+paneh[m_curpane]) move(line, col);
            }
          }

          return 0;
        }

        if (!m_selecting && (cx != s_mousedown[0] || cy != s_mousedown[1]))
        {
          m_select_x2=m_select_x1=m_curs_x;
          m_select_y2=m_select_y1=m_curs_y;
          m_selecting=1;
        }

        int x=cx+m_offs_x;
        int y=cy+m_paneoffs_y[m_curpane]-paney[m_curpane];
        if (m_selecting && (m_select_x2!=x || m_select_y2 != y))
        {
          if (y < m_paneoffs_y[m_curpane] && m_paneoffs_y[m_curpane] > 0)
          {
            m_paneoffs_y[m_curpane]--;
          }
          else if (y >= m_paneoffs_y[m_curpane]+paneh[m_curpane] && m_paneoffs_y[m_curpane]+paneh[m_curpane] < m_text.GetSize())
          {
            m_paneoffs_y[m_curpane]++;
          }
          if (x < m_offs_x && m_offs_x > 0)
          {
            m_offs_x--;
          }
          else if (x > m_offs_x+COLS)
          {
            int maxlen=0;
            int a;
            for (a=0; a < paneh[m_curpane]; a++)
            {
              WDL_FastString* s=m_text.Get(m_paneoffs_y[m_curpane]+a);
              if (s && s->GetLength() > maxlen) maxlen=s->GetLength();
            }
            if (maxlen > m_offs_x+COLS-8) m_offs_x++;
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

          int y=m_curs_y+paney[m_curpane]-m_paneoffs_y[m_curpane];
          if (y >= paney[m_curpane] && y < paney[m_curpane]+paneh[m_curpane]) setCursor();
        }
      }
      break;
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_LBUTTONDOWN:
    if (CURSES_INSTANCE->m_font_w && CURSES_INSTANCE->m_font_h)
    {
      int mousex=(short)LOWORD(lParam);
      int mousey=(short)HIWORD(lParam);

      int cx=mousex/CURSES_INSTANCE->m_font_w;
      int cy=mousey/CURSES_INSTANCE->m_font_h;
      if (cx > COLS) cx=COLS;
      if (cx < 0) cx=0;
      if (cy > LINES) cy=LINES;
      if (cy < 0) cy=0;

      m_state=0; // any click clears the state
      s_mousedown[0]=cx;
      s_mousedown[1]=cy;

      int paney[2], paneh[2];
      const int pane_divy=GetPaneDims(paney, paneh);

      if (uMsg == WM_LBUTTONDOWN && m_pane_div > 0.0 && m_pane_div < 1.0 && cy == m_top_margin+pane_divy)
      {
        SetCapture(hwnd);
        m_scrollcap=3;
        return 0;
      }

      int pane=-1;
      if (cy >= paney[0] && cy < paney[0]+paneh[0]) pane=0;
      else if (cy >= paney[1] && cy < paney[1]+paneh[1]) pane=1;

      if ((uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONDBLCLK) && pane >= 0 &&
          cx >= COLS-CURSES_INSTANCE->drew_scrollbar[pane])
      {
        const int st=paney[pane]*CURSES_INSTANCE->m_font_h+CURSES_INSTANCE->scroll_y[pane];
        const int sb=st+CURSES_INSTANCE->scroll_h[pane];

        int prevoffs=m_paneoffs_y[pane];

        if (mousey < st)
        {
          m_paneoffs_y[pane] -= paneh[pane];
          if (m_paneoffs_y[pane] < 0) m_paneoffs_y[pane]=0;
        }
        else if (mousey < sb)
        {
          if (uMsg == WM_LBUTTONDOWN)
          {
            SetCapture(hwnd);
            m_scrollcap=pane+1;
            m_scrollcap_yoffs=mousey-st;
          }
        }
        else
        {
          m_paneoffs_y[pane] += paneh[pane];
          int maxscroll=m_text.GetSize()-paneh[pane]+4;
          if (m_paneoffs_y[pane] > maxscroll) m_paneoffs_y[pane]=maxscroll;
        }
        
        if (prevoffs != m_paneoffs_y[pane])
        {
          draw();
          draw_status_state();

          int y=m_curs_y+paney[m_curpane]-m_paneoffs_y[m_curpane];
          if (y >= paney[m_curpane] && y < paney[m_curpane]+paneh[m_curpane]) setCursor();
          return 0;
        }
      }

      if (uMsg == WM_LBUTTONDOWN) m_selecting=0;

      if (pane >= 0) m_curpane=pane;           

      m_curs_x=cx+m_offs_x;
      m_curs_y=cy+m_paneoffs_y[m_curpane]-paney[m_curpane];

      bool end = (m_curs_y > m_text.GetSize()-1);
      if (end) m_curs_y=m_text.GetSize()-1;
      if (m_curs_y < 0) m_curs_y = 0;

      WDL_FastString *s=m_text.Get(m_curs_y); 
      if (m_curs_x < 0) m_curs_x = 0;
      if (s && (end || m_curs_x > s->GetLength())) m_curs_x=s->GetLength();

      if (uMsg == WM_LBUTTONDBLCLK && s && s->GetLength())
      {
        if (m_curs_x < s->GetLength())
        {     
          int x1=m_curs_x;
          int x2=m_curs_x+1;
          const char* p=s->Get();
          while (x1 > 0 && (isalnum(p[x1-1]) || p[x1-1] == '_')) --x1;
          while (x2 < s->GetLength()-1 && (isalnum(p[x2]) || p[x2] == '_')) ++x2;
          if (x2 > x1)
          {
            m_select_x1=x1;
            m_curs_x=m_select_x2=x2;
            m_select_y1=m_select_y2=m_curs_y;
            m_selecting=1;
          }
        }
      }

      draw();
      setCursor();

      if (uMsg == WM_LBUTTONDOWN) 
      {
        SetCapture(hwnd);
      }
      else if (uMsg == WM_RBUTTONDOWN)
      {
        onRightClick(hwnd);
      }
    }
    return 0;

    case WM_LBUTTONUP:
      ReleaseCapture();
      m_scrollcap=0;
      m_scrollcap_yoffs=0;
    return 0;

    case WM_MOUSEWHEEL:
      if (CURSES_INSTANCE->m_font_h)
      {
        POINT p;
        GetCursorPos(&p);
        ScreenToClient(hwnd, &p);
        p.y /= CURSES_INSTANCE->m_font_h;

        int paney[2], paneh[2];
        int pane_divy=GetPaneDims(paney, paneh);
        int pane=-1;
        if (p.y >= paney[0] && p.y < paney[0]+paneh[0]) pane=0;
        else if (p.y >= paney[1] && p.y < paney[1]+paneh[1]) pane=1;
        if (pane < 0) pane=m_curpane;

        m_paneoffs_y[pane] -= ((short)HIWORD(wParam))/20;

        int maxscroll=m_text.GetSize()-paneh[pane]+4;
        if (m_paneoffs_y[pane] > maxscroll) m_paneoffs_y[pane]=maxscroll;
        if (m_paneoffs_y[pane] < 0) m_paneoffs_y[pane]=0;

        draw();

        int y=m_curs_y+paney[m_curpane]-m_paneoffs_y[m_curpane];
        if (y >= paney[m_curpane] && y < paney[m_curpane]+paneh[m_curpane]) setCursor();
        else draw_status_state();
      }
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
      { int x; for(x=0;x<m_indent_size;x++) str->Append(" "); }
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
  int paney[2], paneh[2];
  const int pane_divy=GetPaneDims(paney, paneh);

  attrset(m_color_statustext);
  bkgdset(m_color_statustext);

  int line=LINES-1;
  const char* whichpane="";
  if (m_pane_div > 0.0 && m_pane_div < 1.0)
  {
    whichpane=(!m_curpane ? "Upper pane: " : "Lower pane: ");
    line=m_top_margin+pane_divy;
    move(line, 0);
    clrtoeol();
  }

  char str[512];
  snprintf(str, sizeof(str), "%sLine %d/%d, Col %d [%s]%s",
    whichpane, m_curs_y+1, m_text.GetSize(), m_curs_x, 
    (s_overwrite ? "OVR" : "INS"), (m_clean_undopos == m_undoStack_pos ? "" : "*"));
  int len=strlen(str);
  int x=COLS-len-1;
  mvaddnstr(line, x, str, len);
  clrtoeol();

  attrset(0);
  bkgdset(0);  

  const int col=m_curs_x-m_offs_x;
  line=m_curs_y+paney[m_curpane]-m_paneoffs_y[m_curpane];
  if (line >= paney[m_curpane] && line < paney[m_curpane]+paneh[m_curpane]) move(line, col);
}

void WDL_CursesEditor::setCursor(int isVscroll, double ycenter)
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
  else if (m_curs_x >= m_offs_x + COLS) { m_offs_x=m_curs_x-COLS+1; redraw=1; }

  int paney[2], paneh[2];
  GetPaneDims(paney, paneh);
  int y;
  if (ycenter >= 0.0 && ycenter < 1.0)
  {
    y=(int)((double)paneh[m_curpane]*ycenter);
    m_paneoffs_y[m_curpane]=m_curs_y-y;
    if (m_paneoffs_y[m_curpane] < 0)
    {
      y += m_paneoffs_y[m_curpane];
      m_paneoffs_y[m_curpane]=0;
    }
    redraw=1;
  }
  else
  {
    y=m_curs_y-m_paneoffs_y[m_curpane];
    if (y < 0) 
    {
      m_paneoffs_y[m_curpane] += y;
      y=0;
      redraw=1;
    }
    else if (y >= paneh[m_curpane]) 
    {
      m_paneoffs_y[m_curpane] += y-paneh[m_curpane]+1;
      y=paneh[m_curpane]-1;
      redraw=1;
    }
  }

  if (redraw) draw();

  draw_status_state();

  y += paney[m_curpane];
  move(y, m_curs_x-m_offs_x);
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

  int paney[2], paneh[2];
  const int pane_divy=GetPaneDims(paney, paneh);

  const int col=m_curs_x-m_offs_x;
  int line=m_curs_y+paney[m_curpane]-m_paneoffs_y[m_curpane];
  if (line >= paney[m_curpane] && line < paney[m_curpane]+paneh[m_curpane]) move(line, col);
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
      minx=wdl_min(m_select_x1,m_select_x2);
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
  const int VISIBLE_LINES = getVisibleLines();

  int paney[2], paneh[2];
  const int pane_divy=GetPaneDims(paney, paneh);

#ifdef WDL_IS_FAKE_CURSES
  if (m_cursesCtx)
  {
    CURSES_INSTANCE->offs_y[0]=m_paneoffs_y[0];
    CURSES_INSTANCE->offs_y[1]=m_paneoffs_y[1];
    CURSES_INSTANCE->div_y=pane_divy;
    CURSES_INSTANCE->tot_y=m_text.GetSize();

    CURSES_INSTANCE->scrollbar_topmargin = m_top_margin;
    CURSES_INSTANCE->scrollbar_botmargin = m_bottom_margin;
  }
#endif

  attrset(A_NORMAL);

  if (lineidx >= 0)
  {
    int comment_state = GetCommentStateForLineStart(lineidx);
    WDL_FastString *s=m_text.Get(lineidx);
    if (s)
    {
      int y=lineidx-m_paneoffs_y[0];
      if (y >= 0 && y < paneh[0])
      {
        doDrawString(paney[0]+y, 0, lineidx, s->Get(), COLS, &comment_state, wdl_min(s->GetLength(), m_offs_x));
      } 
      y=lineidx-m_paneoffs_y[1];
      if (y >= 0 && y < paneh[1])
      {
        doDrawString(paney[1]+y, 0, lineidx, s->Get(), COLS, &comment_state, wdl_min(s->GetLength(), m_offs_x));
      }
    }
    return;
  }

  __curses_invalidatefull((win32CursesCtx*)m_cursesCtx,false);

  draw_top_line();

  attrset(A_NORMAL);
  bkgdset(A_NORMAL);

  move(m_top_margin,0);
  clrtoeol();

  int pane, i;
  for (pane=0; pane < 2; ++pane)
  {
    int ln=m_paneoffs_y[pane];
    int y=paney[pane];
    int h=paneh[pane];

    int comment_state=GetCommentStateForLineStart(ln);
 
    for(i=0; i < h; ++i, ++ln, ++y)
    { 
      WDL_FastString *s=m_text.Get(ln);
      if (!s) 
      {
        move(y,0);
        clrtoeol();
      }
      else
      {
        doDrawString(y,0,ln,s->Get(),COLS,&comment_state,wdl_min(m_offs_x,s->GetLength()));
      }
    }
  }

  attrset(m_color_bottomline);
  bkgdset(m_color_bottomline);

  if (m_bottom_margin>0)
  {
    move(LINES-1, 0);
#define BOLD(x) { attrset(m_color_bottomline|A_BOLD); addstr(x); attrset(m_color_bottomline&~A_BOLD); }
    if (m_selecting) 
    {
      mvaddstr(LINES-1,0,"SELECTING  ESC:cancel Ctrl+(");
      BOLD("C"); addstr("opy ");
      BOLD("X"); addstr(":cut ");
      BOLD("V"); addstr(":paste)");
    }
    else 
    {
      mvaddstr(LINES-1, 0, "Ctrl+(");

      if (m_pane_div <= 0.0 || m_pane_div >= 1.0) 
      {
        BOLD("P"); addstr("ane ");
      }
      else
      {
        BOLD("O"); addstr("therpane ");
        addstr("no"); BOLD("P"); addstr("anes ");
      }
      BOLD("F"); addstr("ind ");
      addstr("ma"); BOLD("T"); addstr("ch");
      draw_bottom_line();
      addstr(")");
    }
#undef BOLD
    clrtoeol();
  }

  attrset(0);
  bkgdset(0);

  __curses_invalidatefull((win32CursesCtx*)m_cursesCtx,true);
}

void WDL_CursesEditor::draw_bottom_line()
{
  // implementers add key commands here
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
              s->Insert("                  ",0,wdl_min(amt-a,16));
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
      
            if (x == maxy) ex=wdl_min(maxx,tmp);
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
      if (m_curs_y >= m_text.GetSize())
      {
        m_curs_y = m_text.GetSize();
        m_text.Add(new WDL_FastString);
      }
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

void WDL_CursesEditor::highlight_line(int line)
{ 
  if (line >= 0 && line < m_text.GetSize())
  {
    m_curs_x=0;
    m_curs_y=line;

    WDL_FastString* s=m_text.Get(line);
    if (s && s->GetLength())
    {
      m_select_x1=0;
      m_select_x2=s->GetLength();
      m_select_y1=m_select_y2=m_curs_y;
      m_selecting=1;
      draw();
    }

    setCursor();
  }
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
         const int linelen = tl->GetLength();
         for (; startx <= linelen-srchlen; startx++)
           if (!strnicmp(p+startx,s_search_string,srchlen)) 
           {
             m_select_y1=m_select_y2=m_curs_y=line;
             m_select_x1=m_curs_x=startx;
             m_select_x2=startx+srchlen;
             m_selecting=1;
             found=1;
             break;
           }
       }
            

       startx=0;
     }
     if (!found && (m_curs_y>0 || m_curs_x > 0))
     {
       wrapflag=1;
       numlines = wdl_min(m_curs_y+1,numlines);
       for (line = 0; line < numlines && !found; line ++)
       {
         WDL_FastString *tl = m_text.Get(line);
         const char *p;

         if (tl && (p=tl->Get()))
         {
           const int linelen = tl->GetLength();
           for (; startx <= linelen-srchlen; startx++)
             if (!strnicmp(p+startx,s_search_string,srchlen)) 
             {
               m_select_y1=m_select_y2=m_curs_y=line;
               m_select_x1=m_curs_x=startx;
               m_select_x2=startx+srchlen;
               m_selecting=1;
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
       setCursor();
       char buf[512];
       snprintf(buf,sizeof(buf),"Found %s'%s'  Ctrl+G:next",wrapflag?"(wrapped) ":"",s_search_string);
       draw_message(buf);
       return;
     }
   }

   draw();
   setCursor();
   char buf[512];
   if (s_search_string[0]) snprintf(buf,sizeof(buf),"String '%s' not found",s_search_string);
   else lstrcpyn_safe(buf,"No search string",sizeof(buf));
   draw_message(buf);
}

static int categorizeCharForWordNess(int c)
{
  if (c >= 0 && c < 256)
  {
    if (isspace(c)) return 0;
    if (isalnum(c) || c == '_') return 1;
    if (c == ';') return 2; // I prefer this, since semicolons are somewhat special
  }
  return 3;
}

#define CTRL_KEY_DOWN (GetAsyncKeyState(VK_CONTROL)&0x8000)
#define SHIFT_KEY_DOWN (GetAsyncKeyState(VK_SHIFT)&0x8000)
#define ALT_KEY_DOWN (GetAsyncKeyState(VK_MENU)&0x8000)

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
         setCursor();
         draw_message("Find cancelled.");
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
    if (SHIFT_KEY_DOWN)      
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
    case 'O'-'A'+1:
      if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN)
      {
        if (m_pane_div <= 0.0 || m_pane_div >= 1.0)
        {
          onChar('P'-'A'+1);
        }
        if (m_pane_div > 0.0 && m_pane_div < 1.0) 
        {
          m_curpane=!m_curpane;
          draw();
          draw_status_state();
          int paney[2], paneh[2];
          GetPaneDims(paney, paneh);
          if (m_curs_y-m_paneoffs_y[m_curpane] < 0) m_curs_y=m_paneoffs_y[m_curpane];
          else if (m_curs_y-m_paneoffs_y[m_curpane] >= paneh[m_curpane]) m_curs_y=paneh[m_curpane]+m_paneoffs_y[m_curpane]-1;
          setCursor();
        }
      }
    break;
    case 'P'-'A'+1:
      if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN)
      {
        if (m_pane_div <= 0.0 || m_pane_div >= 1.0) 
        {
          m_pane_div=0.5;
          m_paneoffs_y[1]=m_paneoffs_y[0];
        }
        else 
        {
          m_pane_div=1.0;
          if (m_curpane) m_paneoffs_y[0]=m_paneoffs_y[1];
          m_curpane=0;
        }
        draw();
        draw_status_state();

        int paney[2], paneh[2];
        const int pane_divy=GetPaneDims(paney, paneh);
        setCursor();
      }
    break;
    
    case 407:
    case 'Z'-'A'+1:
      if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN)
      {
        if (m_undoStack_pos > 0)
        {
           m_undoStack_pos--;
           loadUndoState(m_undoStack.Get(m_undoStack_pos));
           draw();
           setCursor();
           char buf[512];
           snprintf(buf,sizeof(buf),"Undid action - %d items in undo buffer",m_undoStack_pos);
           draw_message(buf);
        }
        else 
        {
          draw_message("Can't Undo");
        }   
        break;
      }
    // fall through
    case 'Y'-'A'+1:
      if ((c == 'Z'-'A'+1 || !SHIFT_KEY_DOWN) && !ALT_KEY_DOWN)
      {
        if (m_undoStack_pos < m_undoStack.GetSize()-1)
        {
          m_undoStack_pos++;
          loadUndoState(m_undoStack.Get(m_undoStack_pos));
          draw();
          setCursor();
          char buf[512];
          snprintf(buf,sizeof(buf),"Redid action - %d items in redo buffer",m_undoStack.GetSize()-m_undoStack_pos-1);
          draw_message(buf);
        }
        else 
        {
          draw_message("Can't Redo");  
        }
      }
    break;
    case KEY_IC:
      if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN)
      {
        s_overwrite=!s_overwrite;
        setCursor();
        break;
      }
      // fqll through
    case 'V'-'A'+1:
      if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN)
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
         setCursor();
         draw_message("Pasted");
         saveUndoState();
       }
       else 
       {
         setCursor();
         draw_message("Clipboard empty");
       }
     }
  break;

  case KEY_DC:
    if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN)
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
    if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN && m_selecting)
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
      
            if (x == maxy) ex=wdl_min(maxx,tmp);
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
      setCursor();
      draw_message(status);
    }
  break;
  case 'A'-'A'+1:
    if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN)
    {
      m_selecting=1;
      m_select_x1=0;
      m_select_y1=0;
      m_select_y2=m_text.GetSize()-1;
      m_select_x2=0;
      if (m_text.Get(m_select_y2))
        m_select_x2=m_text.Get(m_select_y2)->GetLength();
      draw();
      setCursor();
    }
  break;
  case 27:
    if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN && m_selecting)
    {
      m_selecting=0;
      draw();
      setCursor();
      break;
    }
  break;
  case KEY_F3:
  case 'G'-'A'+1:
    if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN && s_search_string[0])
    {
      runSearch();
      return 0;
    }
  // fall through
  case 'F'-'A'+1:
    if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN)
    {
      draw_message("");
      attrset(m_color_message);
      bkgdset(m_color_message);
      mvaddstr(LINES-1,0,"Find string (ESC to cancel): ");
      if (m_selecting && m_select_y1==m_select_y2)
      {
        WDL_FastString* s=m_text.Get(m_select_y1);
        if (s)
        {
          const char* p=s->Get();
          int xlo=wdl_min(m_select_x1, m_select_x2);
          int xhi=max(m_select_x1, m_select_x2);
          int i;
          for (i=xlo; i < xhi; ++i)
          {
            if (!isalnum(p[i]) && p[i] != '_') break;
          }
          if (i == xhi && xhi > xlo && xhi-xlo < sizeof(s_search_string))
          {
            lstrcpyn(s_search_string, p+xlo, xhi-xlo+1);
          }
        }
      }
      addstr(s_search_string);
      clrtoeol();
      attrset(0);
      bkgdset(0);
      m_state=-3; // find, initial (m_state=4 when we've typed something)
    }
  break;
  case KEY_DOWN:
    {
      if (CTRL_KEY_DOWN)
      {
        int paney[2], paneh[2];
        GetPaneDims(paney, paneh);
        int maxscroll=m_text.GetSize()-paneh[m_curpane]+4;
        if (m_paneoffs_y[m_curpane] < maxscroll-1)
        {
          m_paneoffs_y[m_curpane]++;
          if (m_curs_y < m_paneoffs_y[m_curpane]) m_curs_y=m_paneoffs_y[m_curpane];
          draw();
        }
      }
      else
      {
        m_curs_y++;
        if (m_curs_y>=m_text.GetSize()) m_curs_y=m_text.GetSize()-1;
        if (m_curs_y < 0) m_curs_y=0;
      }
      if (m_selecting) { setCursor(1); m_select_x2=m_curs_x; m_select_y2=m_curs_y; draw(); }
      setCursor(1);
    }
  break;
  case KEY_UP:
    {
      if (CTRL_KEY_DOWN)
      {
        if (m_paneoffs_y[m_curpane] > 0)
        {
          int paney[2], paneh[2];
          GetPaneDims(paney, paneh);
          m_paneoffs_y[m_curpane]--;
          if (m_curs_y >  m_paneoffs_y[m_curpane]+paneh[m_curpane]-1) m_curs_y = m_paneoffs_y[m_curpane]+paneh[m_curpane]-1;
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
      if (m_curs_y > m_paneoffs_y[m_curpane])
      {
        m_curs_y=m_paneoffs_y[m_curpane];
        if (m_curs_y < 0) m_curs_y=0;
      }
      else 
      {
        int paney[2], paneh[2];
        GetPaneDims(paney, paneh);
        m_curs_y -= paneh[m_curpane];
        if (m_curs_y < 0) m_curs_y=0;
        m_paneoffs_y[m_curpane]=m_curs_y;
      }
      if (m_selecting) { setCursor(1); m_select_x2=m_curs_x; m_select_y2=m_curs_y; }
      draw();
      setCursor(1);
    }
  break; 
  case KEY_NPAGE:
    {
      int paney[2], paneh[2]; 
      GetPaneDims(paney, paneh);
      if (m_curs_y >= m_paneoffs_y[m_curpane]+paneh[m_curpane]-1) m_paneoffs_y[m_curpane]=m_curs_y-1;
      m_curs_y = m_paneoffs_y[m_curpane]+paneh[m_curpane]-1;
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
        if (CTRL_KEY_DOWN)
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
        if (CTRL_KEY_DOWN)
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
      if (CTRL_KEY_DOWN) m_curs_y=0;
      if (m_selecting) { setCursor(); m_select_x2=m_curs_x; m_select_y2=m_curs_y; draw(); }
      setCursor();
    }
  break;
  case KEY_END:
    {
      if (m_text.Get(m_curs_y)) m_curs_x=m_text.Get(m_curs_y)->GetLength();
      if (CTRL_KEY_DOWN) m_curs_y=m_text.GetSize();
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
      if (!tl) 
      {
        m_curs_y--;
        if (fl) m_curs_x=fl->GetLength();
        draw();
        saveUndoState();
        setCursor();
      }
      else if (fl)
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
    if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN)
    {
      draw();
      setCursor();
    }
  break;
  case 13: //KEY_ENTER:
    //insert newline
    preSaveUndoState();

    if (m_selecting) { removeSelect(); draw(); setCursor(); }
    if (m_curs_y >= m_text.GetSize())
    {
      m_curs_y=m_text.GetSize();
      m_text.Add(new WDL_FastString);
    }
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
      indentSelect(isRev?-m_indent_size:m_indent_size);
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
        char str[64];
        int slen ;
        if (c == '\t') 
        {
          slen = wdl_min(m_indent_size,64);
          if (slen<1) slen=1;
          int x; 
          for(x=0;x<slen;x++) str[x]=' ';
        }
        else
        {
          str[0]=c;
          slen = 1;
        }


        bool hadCom = LineCanAffectOtherLines(ss->Get(),-1,-1);
        if (s_overwrite)
        {
          if (!hadCom) hadCom = LineCanAffectOtherLines(ss->Get(),m_curs_x,slen);
          ss->DeleteSub(m_curs_x,slen);
        }
        ss->Insert(str,m_curs_x,slen);
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
    rec->m_curs_x=m_curs_x;
    rec->m_curs_y=m_curs_y;
    rec->m_curpane=m_curpane;
    rec->m_pane_div=m_pane_div;
    rec->m_paneoffs_y[0]=m_paneoffs_y[0];
    rec->m_paneoffs_y[1]=m_paneoffs_y[1];
  }
}
void WDL_CursesEditor::saveUndoState() 
{
  editUndoRec *rec=new editUndoRec;
  rec->m_offs_x=m_offs_x;
  rec->m_curs_x=m_curs_x;
  rec->m_curs_y=m_curs_y;
  rec->m_curpane=m_curpane;
  rec->m_pane_div=m_pane_div;
  rec->m_paneoffs_y[0]=m_paneoffs_y[0];
  rec->m_paneoffs_y[1]=m_paneoffs_y[1];

  editUndoRec *lrec[5];
  lrec[0] = m_undoStack.Get(m_undoStack_pos);
  lrec[1] = m_undoStack.Get(m_undoStack_pos+1);
  lrec[2] = m_undoStack.Get(m_undoStack_pos-1);
  lrec[3] = m_undoStack.Get(m_undoStack_pos-2);
  lrec[4] = m_undoStack.Get(m_undoStack_pos-3);

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
      for(w=0;w<5 && !ns;w++)
      {
        if (lrec[w])
        {
          int y;
          for (y=0; y<15; y ++)
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
  
  if (m_undoStack.GetSize() > m_max_undo_states) 
  {
    for (x = 1; x < m_undoStack.GetSize()/2; x += 2)
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
  m_curs_x=rec->m_curs_x;
  m_curs_y=rec->m_curs_y;

  m_curpane=rec->m_curpane;
  m_pane_div=rec->m_pane_div;
  m_paneoffs_y[0]=rec->m_paneoffs_y[0];
  m_paneoffs_y[1]=rec->m_paneoffs_y[1];
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
