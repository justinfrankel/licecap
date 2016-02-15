#ifndef _WDL_CURSESEDITOR_H_
#define _WDL_CURSESEDITOR_H_

#include "../wdlstring.h"
#include "../ptrlist.h"
#include <time.h>

class WDL_CursesEditor
{
public:
  WDL_CursesEditor(void *cursesCtx);
  virtual ~WDL_CursesEditor();
  
  bool IsDirty() const { return m_clean_undopos != m_undoStack_pos; }
  void SetDirty() { m_clean_undopos = -1; }

  virtual int onChar(int c);
  virtual void onRightClick(HWND hwnd) { } 

  virtual int updateFile(); // saves file on disk
  void RunEditor(); // called from timer/main loop when on simulated curses -- if on a real console just call onChar(getch())

  void *m_cursesCtx; // win32CursesCtx *

  int m_color_bottomline, m_color_statustext,  m_color_selection,  m_color_message; // COLOR_PAIR(x)
  int m_top_margin, m_bottom_margin;

  const char *GetFileName() { return m_filename.Get(); }
  time_t GetLastModTime() const { return m_filelastmod; } // returns file mod time of last save or load, or 0 if unknown
  void SetLastModTime(time_t v) { m_filelastmod=v; } // in case caller wants to manually set this value

  virtual void setCursor(int isVscroll=0, double ycenter=-1.0);

  int m_indent_size;
  int m_max_undo_states;

  virtual int init(const char *fn, const char *init_if_empty=0); 
  virtual int reload_file(bool clearUndo=false);
  virtual void draw(int lineidx=-1);

  virtual void highlight_line(int line);

protected:
  class refcntString;
  class editUndoRec;

  void loadLines(FILE* fh);
  void getLinesFromClipboard(WDL_FastString &buf, WDL_PtrList<const char> &lines);

  void draw_message(const char *str);
  void draw_status_state();

  virtual LRESULT onMouseMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  static LRESULT _onMouseMessage(void *user_data, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    if (user_data) return ((WDL_CursesEditor*)user_data)->onMouseMessage(hwnd,uMsg,wParam,lParam);
    return 0;
  }
  
  void runSearch();

  void indentSelect(int amt);
  void removeSelect();
  void getselectregion(int &minx, int &miny, int &maxx, int &maxy);
  void doDrawString(int y, int line_n, const char *p, int *c_comment_state);

  void saveUndoState(); // updates rec[0]/rec[1], rec[0] is view after edit (rec[1] will be view after following edit)
  void preSaveUndoState(); // updates coordinates of edit to last rec[1]
  void loadUndoState(editUndoRec *rec, int idx); // idx=0 on redo, 1=on undo

  void updateLastModTime();

  virtual int GetCommentStateForLineStart(int line); // pass current line, returns flags (which will be passed as c_comment_state)

  virtual void draw_line_highlight(int y, const char *p, int *c_comment_state);
  virtual void draw_top_line() { }// within m_top_margin
  virtual void draw_bottom_line();
  virtual bool LineCanAffectOtherLines(const char *txt, int spos, int slen) // if multiline comment etc
  {
    return false;
  }

  int getVisibleLines() const;
  
  WDL_FastString m_filename;
  time_t m_filelastmod; // last written-to or read-from modification time, or 0 if unknown
  int m_newline_mode; // detected from input. 0 = \n, 1=\r\n

  // auto-detected on input, set to >0 (usually m_indent_size) if all leading whitespace was tabs (some fuzzy logic too)
  // this is a workaround until we retrofit real tab (and UTF-8) support
  int m_write_leading_tabs; 

  WDL_PtrList<WDL_FastString> m_text;
  WDL_PtrList<editUndoRec> m_undoStack;
  int m_undoStack_pos;
  int m_clean_undopos;
  
  int m_state; // values >0 used by specific impls, negatives used by builtin funcs

  int m_selecting;
  int m_select_x1,m_select_y1,m_select_x2,m_select_y2;

  int m_offs_x;
  int m_curs_x, m_curs_y;
  int m_want_x; // to restore active column when vscrolling

  int m_scrollcap; // 1=top, 2=bottom, 3=divider
  int m_scrollcap_yoffs;
  
  int m_curpane;
  double m_pane_div;
  int m_paneoffs_y[2]; 

  int GetPaneDims(int* paney, int* paneh);

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

    int m_offs_x[2];
    int m_curs_x[2], m_curs_y[2];

    int m_curpane[2];
    double m_pane_div[2];
    int m_paneoffs_y[2][2];
  };
};
#endif
