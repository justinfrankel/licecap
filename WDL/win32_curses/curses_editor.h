#ifndef _WDL_CURSESEDITOR_H_
#define _WDL_CURSESEDITOR_H_

#include "../wdlstring.h"
#include "../ptrlist.h"

class WDL_CursesEditor
{
public:
  WDL_CursesEditor(void *cursesCtx);
  virtual ~WDL_CursesEditor();
  
  bool IsDirty() const { return m_clean_undopos != m_undoStack_pos; }

  virtual int onChar(int c);
  virtual void onRightClick() { }

  int updateFile(); // saves file on disk
  void RunEditor(); // called from timer/main loop when on simulated curses -- if on a real console just call onChar(getch())

  void *m_cursesCtx; // win32CursesCtx *

  int m_color_bottomline, m_color_statustext,  m_color_selection,  m_color_message; // COLOR_PAIR(x)
  int m_top_margin, m_bottom_margin;


protected:
  class refcntString;
  class editUndoRec;

  int init(const char *fn, const char *init_if_empty=0); 
  void draw(int lineidx=-1);
  void draw_message(const char *str);
  void draw_status_state();
  void setCursor(int isVscroll=0);
  LRESULT onMouseMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  static LRESULT _onMouseMessage(void *user_data, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    if (user_data) return ((WDL_CursesEditor*)user_data)->onMouseMessage(hwnd,uMsg,wParam,lParam);
    return 0;
  }
  void runSearch();

  void indentSelect(int amt);
  void removeSelect();
  void getselectregion(int &minx, int &miny, int &maxx, int &maxy);
  void doDrawString(int y, int x, int line_n, const char *p, int ml, bool *c_comment_state, int skipcnt);

  void saveUndoState();
  void preSaveUndoState(); // updates coordinates of edit to last rec
  void loadUndoState(editUndoRec *rec);

  virtual int GetPreviousCommentStartEnd(int *line, int *col); // pass current line/col, updates with interesting point, returns nonzero if start (1 if /*, 2 if //)

  virtual void mvaddnstr_highlight(int y, int x, const char *p, int ml, bool *c_comment_state, int skipcnt);
  virtual void draw_top_line() { }// within m_top_margin
  virtual void draw_bottom_line();
  virtual bool LineCanAffectOtherLines(const char *txt) // if multiline comment etc
  {
    return false;
  }

  int getVisibleLines() const;
  
  WDL_FastString m_filename;
  WDL_PtrList<WDL_FastString> m_text;
  WDL_PtrList<editUndoRec> m_undoStack;
  int m_undoStack_pos;
  int m_clean_undopos;
  
  int m_state; // values >0 used by specific impls, negatives used by builtin funcs

  int m_selecting;
  int m_select_x1,m_select_y1,m_select_x2,m_select_y2;

  int m_offs_x, m_offs_y;
  int m_curs_x, m_curs_y;
  int m_want_x;


  char newfilename_str[256];
  WDL_FastString m_newfn;

  static char s_search_string[256];
  static int s_overwrite;
  static WDL_FastString s_fake_clipboard;


  class refcntString
  {
    ~refcntString() { free(str); }
    char *str;
    int str_len;
    int refcnt;
  public:
    refcntString(const char *val, int val_len) 
    { 
      str_len = val_len;
      str=(char*)malloc(str_len+1);
      if (str) memcpy(str,val,str_len+1);
      refcnt=0; 
    }

    void AddRef() { refcnt++; }
    void Release() { if (!--refcnt) delete this; }

    const char *getStr() const { return str?str:""; }
    int getStrLen() const { return str_len; }
  };
  class editUndoRec
  {
    public:
     editUndoRec() { }
     ~editUndoRec() 
    { 
      int x;
      for (x=0;x<m_htext.GetSize();x++) 
      {
        refcntString *rs=m_htext.Get(x);
        if (rs) rs->Release();
      }
      m_htext.Empty();
    }

    WDL_PtrList<refcntString> m_htext;

    int m_offs_x, m_offs_y;
    int m_curs_x, m_curs_y;
  };
};
#endif