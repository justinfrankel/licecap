#ifndef __MULTITAB_EDIT_H_
#define __MULTITAB_EDIT_H_

#define STATE_BEFORE_CODE -1


#define SYNTAX_HIGHLIGHT1 COLOR_PAIR(3)
#define SYNTAX_HIGHLIGHT2 COLOR_PAIR(4)
#define SYNTAX_COMMENT  COLOR_PAIR(5)
#define SYNTAX_ERROR COLOR_PAIR(6)
#define SYNTAX_FUNC COLOR_PAIR(7)

#ifdef WDL_IS_FAKE_CURSES
#define SYNTAX_REGVAR COLOR_PAIR(8)
#define SYNTAX_KEYWORD COLOR_PAIR(9)
#define SYNTAX_STRING COLOR_PAIR(10)
#define SYNTAX_STRINGVAR COLOR_PAIR(11)
#else
#define SYNTAX_REGVAR COLOR_PAIR(4)
#define SYNTAX_KEYWORD COLOR_PAIR(4)
#define SYNTAX_STRING COLOR_PAIR(3)
#define SYNTAX_STRINGVAR COLOR_PAIR(4)
#endif

#include "curses_editor.h"

// add multiple tab support to WDL_CursesEditor

class MultiTab_Editor : public WDL_CursesEditor
{
public:
  MultiTab_Editor(void *cursesCtx) : WDL_CursesEditor(cursesCtx) {  m_top_margin=1;  }
  virtual ~MultiTab_Editor() { }

  virtual void draw_top_line();
  virtual int onChar(int c);
  virtual LRESULT onMouseMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  virtual int GetTabCount() { return 1; }
  virtual MultiTab_Editor *GetTab(int idx) { if (idx==0) return this; return NULL; }
  virtual bool AddTab(const char *fn) { return false; }
  virtual void SwitchTab(int idx, bool rel) { }
  virtual void CloseCurrentTab() { }

  virtual FILE *tryToFindOrCreateFile(const char *fnp, WDL_FastString *s) { return NULL; } 

  void OpenFileInTab(const char *fnp);

  char newfilename_str[256];
  WDL_FastString m_newfn;

};

// add EEL syntax highlighting and paren matching, hooks for watch/etc
class EEL_Editor : public MultiTab_Editor
{
public:
  EEL_Editor(void *cursesCtx, const char *filename);
  virtual ~EEL_Editor();

  virtual void mvaddnstr_highlight(int y, int x, const char *p, int ml, int *c_comment_state, int skipcnt);
  virtual int GetCommentStateForLineStart(int line); 
  virtual bool LineCanAffectOtherLines(const char *txt, int spos, int slen); // if multiline comment etc
  virtual void doWatchInfo(int c);
  virtual void doParenMatching();

  virtual int onChar(int c);
  virtual void onRightClick();
  virtual void draw_bottom_line();

  virtual void draw(int lineidx=-1);

  virtual LRESULT onMouseMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  virtual int overrideSyntaxDrawingForLine(int *ml, int *skipcnt, const char **p, int *c_comment_state, int *last_attr) { return 0; }
  virtual int namedTokenHighlight(const char *tokStart, int len, int state);

  virtual void Watch_Code(const char *code, char *sstr, size_t sstr_size);
  virtual bool Watch_OnContextHelp(const char *lp, const char *rp, int c, char *sstr, size_t sstr_size);

  virtual int is_code_start_line(const char *p) { return 0; } // pass NULL to see if code-start-lines are even used

  void draw_string_internal(int *ml, int *skipcnt, const char *str, int amt, int *attr, int newAttr);
  void draw_string_urlchk(int *ml, int *skipcnt, const char *str, int amt, int *attr, int newAttr);
  void draw_string(int *ml, int *skipcnt, const char *str, int amt, int *attr, int newAttr, bool dispAsString=false);

};

#endif
