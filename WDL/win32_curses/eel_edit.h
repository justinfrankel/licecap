#ifndef __WDL_EEL_EDITOR_H_
#define __WDL_EEL_EDITOR_H_

#include "multitab_edit.h"

// add EEL syntax highlighting and paren matching, hooks for watch/etc
class EEL_Editor : public MultiTab_Editor
{
public:
  EEL_Editor(void *cursesCtx);
  virtual ~EEL_Editor();

  virtual void mvaddnstr_highlight(int y, int x, const char *p, int ml, int *c_comment_state, int skipcnt);
  virtual int do_draw_line(const char *p, int ml, int *c_comment_state, int skipcnt, int last_attr);
  virtual int GetCommentStateForLineStart(int line); 
  virtual bool LineCanAffectOtherLines(const char *txt, int spos, int slen); // if multiline comment etc
  virtual void doWatchInfo(int c);
  virtual void doParenMatching();

  virtual int onChar(int c);
  virtual void onRightClick(HWND hwnd);
  virtual void draw_bottom_line();

  virtual LRESULT onMouseMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  virtual int overrideSyntaxDrawingForLine(int *ml, int *skipcnt, const char **p, int *c_comment_state, int *last_attr) { return 0; }
  virtual int namedTokenHighlight(const char *tokStart, int len, int state);

  virtual void Watch_Code(const char *code, char *sstr, size_t sstr_size);
  virtual bool Watch_OnContextHelp(const char *lp, const char *rp, int c, char *sstr, size_t sstr_size);

  virtual int is_code_start_line(const char *p) { return 0; } // pass NULL to see if code-start-lines are even used

  virtual void draw_string_internal(int *ml, int *skipcnt, const char *str, int amt, int *attr, int newAttr);
  virtual void draw_string_urlchk(int *ml, int *skipcnt, const char *str, int amt, int *attr, int newAttr);
  virtual void draw_string(int *ml, int *skipcnt, const char *str, int amt, int *attr, int newAttr, int comment_string_state=0);

  virtual bool sh_draw_parenttokenstack_pop(char c);
  virtual bool sh_draw_parentokenstack_update(const char *tok, int toklen);
  virtual const char *sh_tokenize(const char **ptr, const char *endptr, int *lenOut, int *state);

  bool m_has_peek;

  // static helpers
  static WDL_TypedBuf<char> s_draw_parentokenstack;
  static int parse_format_specifier(const char *fmt_in, int *var_offs, int *var_len);
};

#endif
