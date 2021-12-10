#ifndef __WDL_EEL_EDITOR_H_
#define __WDL_EEL_EDITOR_H_

#define STATE_BEFORE_CODE -1

#include "curses_editor.h"
#include "../assocarray.h"

class suggested_matchlist {
    struct rec { char *val; int score, allocsz, mode; };
    WDL_TypedBuf<rec> m_list;
    int m_list_valid;
  public:

    enum { MODE_FUNC=0, MODE_USERFUNC, MODE_VAR, MODE_REGVAR };

    suggested_matchlist(int maxsz=80)
    {
      rec *list = m_list.ResizeOK(maxsz,false);
      if (list) memset(list,0,maxsz*sizeof(*list));
    }
    ~suggested_matchlist()
    {
      for (int x = 0; x < m_list.GetSize(); x ++) free(m_list.Get()[x].val);
    }

    int get_size() const { return m_list_valid; }
    const char *get(int idx, int *mode=NULL, int *score=NULL) const {
      if (idx < 0 || idx >= m_list_valid) return NULL;
      if (mode) *mode = m_list.Get()[idx].mode;
      if (score) *score=m_list.Get()[idx].score;
      return m_list.Get()[idx].val;
    }

    void clear() { m_list_valid = 0; }
    void add(const char *p, int score=0x7FFFFFFF, int mode=MODE_FUNC)
    {
      rec *list = m_list.Get();
      int insert_after;
      for (insert_after = m_list_valid-1; insert_after >= 0 && score > list[insert_after].score; insert_after--);

      const int maxsz = m_list.GetSize();
      WDL_ASSERT(m_list_valid <= maxsz);
#ifdef _DEBUG
      { for (int  y = 0; y < m_list_valid; y ++) { WDL_ASSERT(list[y].val != NULL); } }
#endif

      if (insert_after+1 >= maxsz) return;

      for (int y=insert_after; y>=0 && list[y].score == score; y--)
      {
        int d = strcmp(p,list[y].val);
        if (!d) return;
        if (d < 0) insert_after = y;
      }

      if (m_list_valid < maxsz) m_list_valid++;
      rec r = list[maxsz-1];
      WDL_ASSERT(r.val || (!r.allocsz && !r.score));

      r.mode = mode;
      r.score = score;
      const size_t plen = strlen(p);
      if (!r.val || (plen+1) > r.allocsz)
      {
        free(r.val);
        r.val = (char *)malloc( r.allocsz = wdl_max(plen,80)+36 );
      }
      if (WDL_NORMALLY(r.val)) strcpy(r.val,p);

      m_list.Delete(maxsz-1);
      m_list.Insert(r,insert_after+1);
      WDL_ASSERT(maxsz == m_list.GetSize());
    }
};


extern int g_eel_editor_max_vis_suggestions;
extern int g_eel_editor_flags; // &1=doubleclick function name defaults to jump

// add EEL syntax highlighting and paren matching, hooks for watch/etc
class EEL_Editor : public WDL_CursesEditor
{
public:
  EEL_Editor(void *cursesCtx);
  virtual ~EEL_Editor();

  virtual void draw_line_highlight(int y, const char *p, int *c_comment_state, int line_n);
  virtual int do_draw_line(const char *p, int *c_comment_state, int last_attr);
  virtual int GetCommentStateForLineStart(int line); 
  virtual bool LineCanAffectOtherLines(const char *txt, int spos, int slen); // if multiline comment etc
  virtual void doWatchInfo(int c);
  virtual void doParenMatching();

  virtual int onChar(int c);
  virtual void onRightClick(HWND hwnd);
  virtual void draw_bottom_line();

#ifdef WDL_IS_FAKE_CURSES
  virtual LRESULT onMouseMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

  virtual int overrideSyntaxDrawingForLine(int *skipcnt, const char **p, int *c_comment_state, int *last_attr) { return 0; }
  virtual int namedTokenHighlight(const char *tokStart, int len, int state);

  virtual int is_code_start_line(const char *p) { return 0; } // pass NULL to see if code-start-lines are even used

  virtual void draw_string_internal(int *skipcnt, const char *str, int amt, int *attr, int newAttr);
  virtual void draw_string_urlchk(int *skipcnt, const char *str, int amt, int *attr, int newAttr);
  virtual void draw_string(int *skipcnt, const char *str, int amt, int *attr, int newAttr, int comment_string_state=0);

  virtual bool sh_draw_parenttokenstack_pop(char c);
  virtual bool sh_draw_parentokenstack_update(const char *tok, int toklen);
  virtual const char *sh_tokenize(const char **ptr, const char *endptr, int *lenOut, int *state);

  virtual bool peek_want_VM_funcs() { return false; } // implement if syntax highlighting should (and can safely) call peek_get_VM()
  virtual void *peek_get_VM() { return NULL; } // returns NSEEL_VMCTX (if supported)
  virtual int peek_get_named_string_value(const char *name, char *sstr, size_t sstr_sz) { return -1; } // returns >=0 (index) if found
  virtual bool peek_get_numbered_string_value(double idx, char *sstr, size_t sstr_sz) { return false; }

  virtual void peek_lock() { }
  virtual void peek_unlock() { }
  virtual void on_help(const char *str, int curChar) { } // curChar is current character if str is NULL

  virtual bool line_has_openable_file(const char *line, int cursor_bytepos, char *fnout, size_t fnout_sz) { return false; }

  enum {
    KEYWORD_MASK_BUILTIN_FUNC=1,
    KEYWORD_MASK_USER_FUNC=4,
    KEYWORD_MASK_USER_VAR=8
  };

  // chkmask: KEYWORD_MASK_* or ~0 for all
  // ignoreline= line to ignore function defs on.
  virtual int peek_get_token_info(const char *name, char *sstr, size_t sstr_sz, int chkmask, int ignoreline);
  virtual void get_suggested_token_names(const char *fname, int chkmask, suggested_matchlist *list);
  virtual int fuzzy_match(const char *codestr, const char *refstr); // returns score

  virtual void draw_top_line();

  virtual void open_import_line();

  virtual void get_extra_filepos_names(WDL_LogicalSortStringKeyedArray<int> * list, int pass) { }

  // static helpers
  static WDL_TypedBuf<char> s_draw_parentokenstack;
  static int parse_format_specifier(const char *fmt_in, int *var_offs, int *var_len);

  WDL_FastString m_suggestion;
  int m_suggestion_x,m_suggestion_y;
  HWND m_suggestion_hwnd;
  suggested_matchlist m_suggestion_list;
  int m_suggestion_hwnd_sel;
  int m_suggestion_tokpos, m_suggestion_toklen; // bytepos/len
  int m_suggestion_curline_comment_state;

  void ensure_code_func_cache_valid();

  // list of code functions, first 4 bytes of the pointer is integer line offset
  // followed by name
  // followed by (parameter block)
  // followed by trailing info
  WDL_PtrList<char> m_code_func_cache;
  DWORD m_code_func_cache_time; // last GetTickCount() for invalidation
  int m_code_func_cache_lines; // last m_text.GetSize() for invalidation


  bool m_case_sensitive; // for function detection, and maybe other places
  const char *m_function_prefix; // defaults to "function "
  const char *m_comment_str; // defaults to "//"
};

#endif
