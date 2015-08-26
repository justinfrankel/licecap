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

  virtual void draw(int lineidx=-1);

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


#endif
