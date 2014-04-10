#ifndef __WDL_EEL_EDITOR_H_
#define __WDL_EEL_EDITOR_H_

#include "multitab_edit.h"

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
