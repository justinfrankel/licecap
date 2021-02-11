#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#else
#include "../swell/swell.h"
#endif
#include <stdlib.h>
#include <string.h>
#ifndef CURSES_INSTANCE
#define CURSES_INSTANCE ((win32CursesCtx *)m_cursesCtx)
#endif
#include "curses.h"
#include "eel_edit.h"
#include "../wdlutf8.h"
#include "../win32_utf8.h"
#include "../wdlcstring.h"
#include "../eel2/ns-eel-int.h"

int g_eel_editor_max_vis_suggestions = 50;

EEL_Editor::EEL_Editor(void *cursesCtx) : WDL_CursesEditor(cursesCtx)
{
  m_suggestion_curline_comment_state=0;
  m_suggestion_x=m_suggestion_y=-1;
  m_case_sensitive=false;
  m_comment_str="//"; // todo IsWithinComment() or something?
  m_function_prefix = "function ";

  m_suggestion_hwnd=NULL;
  m_code_func_cache_lines=0;
  m_code_func_cache_time=0;
}

EEL_Editor::~EEL_Editor()
{
  if (m_suggestion_hwnd) DestroyWindow(m_suggestion_hwnd);
  m_code_func_cache.Empty(true,free);
}

#define sh_func_ontoken(x,y)

int EEL_Editor::namedTokenHighlight(const char *tokStart, int len, int state)
{
  if (len == 4 && !strnicmp(tokStart,"this",4)) return SYNTAX_KEYWORD;
  if (len == 7 && !strnicmp(tokStart,"_global",7)) return SYNTAX_KEYWORD;
  if (len == 5 && !strnicmp(tokStart,"local",5)) return SYNTAX_KEYWORD;
  if (len == 8 && !strnicmp(tokStart,"function",8)) return SYNTAX_KEYWORD;
  if (len == 6 && !strnicmp(tokStart,"static",6)) return SYNTAX_KEYWORD;
  if (len == 8 && !strnicmp(tokStart,"instance",8)) return SYNTAX_KEYWORD;
  if (len == 6 && !strnicmp(tokStart,"global",6)) return SYNTAX_KEYWORD;
  if (len == 7 && !strnicmp(tokStart,"globals",7)) return SYNTAX_KEYWORD;

  if (len == 5 && !strnicmp(tokStart,"while",5)) return SYNTAX_KEYWORD;
  if (len == 4 && !strnicmp(tokStart,"loop",4)) return SYNTAX_KEYWORD;

  if (len == 17 && !strnicmp(tokStart,"__denormal_likely",17)) return SYNTAX_FUNC;
  if (len == 19 && !strnicmp(tokStart,"__denormal_unlikely",19)) return SYNTAX_FUNC;

  char buf[512];
  lstrcpyn_safe(buf,tokStart,wdl_min(sizeof(buf),len+1));
  NSEEL_VMCTX vm = peek_want_VM_funcs() ? peek_get_VM() : NULL;
  if (nseel_getFunctionByName((compileContext*)vm,buf,NULL)) return SYNTAX_FUNC;

  return A_NORMAL;
}

int EEL_Editor::parse_format_specifier(const char *fmt_in, int *var_offs, int *var_len)
{
  const char *fmt = fmt_in+1;
  *var_offs = 0;
  *var_len = 0;
  if (fmt_in[0] != '%') return 0; // passed a non-specifier

  while (*fmt)
  {
    const char c = *fmt++;

    if (c>0 && isalpha(c)) 
    {
      return (int) (fmt - fmt_in);
    }

    if (c == '.' || c == '+' || c == '-' || c == ' ' || (c>='0' && c<='9')) 
    {
    }
    else if (c == '{')
    {
      if (*var_offs!=0) return 0; // already specified
      *var_offs = (int)(fmt-fmt_in);
      if (*fmt == '.' || (*fmt >= '0' && *fmt <= '9')) return 0; // symbol name can't start with 0-9 or .

      while (*fmt != '}')
      {
        if ((*fmt >= 'a' && *fmt <= 'z') ||
            (*fmt >= 'A' && *fmt <= 'Z') ||
            (*fmt >= '0' && *fmt <= '9') ||
            *fmt == '_' || *fmt == '.' || *fmt == '#')
        {
          fmt++;
        }
        else
        {
          return 0; // bad character in variable name
        }
      }
      *var_len = (int)((fmt-fmt_in) - *var_offs);
      fmt++;
    }
    else
    {
      break;
    }
  }
  return 0;
}


void EEL_Editor::draw_string(int *skipcnt, const char *str, int amt, int *attr, int newAttr, int comment_string_state)
{
  if (amt > 0 && comment_string_state=='"')
  {
    while (amt > 0 && *str)
    {
      const char *str_scan = str;
      int varpos,varlen,l=0;

      while (!l && *str_scan)
      {
        while (*str_scan && *str_scan != '%' && str_scan < str+amt) str_scan++;
        if (str_scan >= str+amt) break;

        l = parse_format_specifier(str_scan,&varpos,&varlen);
        if (!l && *str_scan)  if (*++str_scan == '%') str_scan++;
      }
      if (!*str_scan || str_scan >= str+amt) break; // allow default processing to happen if we reached the end of the string

      if (l > amt) l=amt;

      if (str_scan > str) 
      {
        const int sz=wdl_min((int)(str_scan-str),amt);
        draw_string_urlchk(skipcnt,str,sz,attr,newAttr);
        str += sz;
        amt -= sz;
      }

      {
        const int sz=(varlen>0) ? wdl_min(varpos,amt) : wdl_min(l,amt);
        if (sz>0) 
        {
          draw_string_internal(skipcnt,str,sz,attr,SYNTAX_HIGHLIGHT2);
          str += sz;
          amt -= sz;
        }
      }

      if (varlen>0) 
      {
        int sz = wdl_min(varlen,amt);
        if (sz>0)
        {
          draw_string_internal(skipcnt,str,sz,attr,*str == '#' ? SYNTAX_STRINGVAR : SYNTAX_HIGHLIGHT1);
          amt -= sz;
          str += sz;
        }

        sz = wdl_min(l - varpos - varlen, amt);
        if (sz>0)
        {
          draw_string_internal(skipcnt,str,sz,attr,SYNTAX_HIGHLIGHT2);
          amt-=sz;
          str+=sz;
        }
      }
    }
  }
  draw_string_urlchk(skipcnt,str,amt,attr,newAttr);
}

void EEL_Editor::draw_string_urlchk(int *skipcnt, const char *str, int amt, int *attr, int newAttr)
{
  if (amt > 0 && (newAttr == SYNTAX_COMMENT || newAttr == SYNTAX_STRING))
  {
    const char *sstr=str;
    while (amt > 0 && *str)
    {
      const char *str_scan = str;
      int l=0;
      
      while (l < 10 && *str_scan)
      {
        str_scan += l;
        l=0;
        while (*str_scan &&
               (strncmp(str_scan,"http://",7) || (sstr != str_scan && str_scan[-1] > 0 && isalnum(str_scan[-1]))) &&
               str_scan < str+amt) str_scan++;
        if (!*str_scan || str_scan >= str+amt) break;
        while (str_scan[l] && str_scan[l] != ')' && str_scan[l] != '\"' && str_scan[l] != ')' && str_scan[l] != ' ' && str_scan[l] != '\t') l++;
      }
      if (!*str_scan || str_scan >= str+amt) break; // allow default processing to happen if we reached the end of the string
      
      if (l > amt) l=amt;
      
      if (str_scan > str)
      {
        const int sz=wdl_min((int)(str_scan-str),amt);
        draw_string_internal(skipcnt,str,sz,attr,newAttr);
        str += sz;
        amt -= sz;
      }
      
      const int sz=wdl_min(l,amt);
      if (sz>0)
      {
        draw_string_internal(skipcnt,str,sz,attr,SYNTAX_HIGHLIGHT1);
        str += sz;
        amt -= sz;
      }
    }
  }
  draw_string_internal(skipcnt,str,amt,attr,newAttr);
}
  
void EEL_Editor::draw_string_internal(int *skipcnt, const char *str, int amt, int *attr, int newAttr)
{
  // *skipcnt is in characters, amt is in bytes
  while (*skipcnt > 0 && amt > 0)
  {
    const int clen = wdl_utf8_parsechar(str,NULL);
    str += clen;
    amt -= clen;
    *skipcnt -= 1;
  }

  if (amt>0)
  {
    if (*attr != newAttr) 
    {
      attrset(newAttr);
      *attr = newAttr;
    }
    addnstr(str,amt);
  }
}


WDL_TypedBuf<char> EEL_Editor::s_draw_parentokenstack;
bool EEL_Editor::sh_draw_parenttokenstack_pop(char c)
{
  int sz = s_draw_parentokenstack.GetSize();
  while (--sz >= 0)
  {
    char tc = s_draw_parentokenstack.Get()[sz];
    if (tc == c)
    {
      s_draw_parentokenstack.Resize(sz,false);
      return false;
    }

    switch (c)
    {
      case '?': 
        // any open paren or semicolon is enough to cause error for ?:

      return true;
      case '(':
        if (tc == '[') return true;
      break;
      case '[':
        if (tc == '(') return true;
      break;
    }
  }

  return true;
}
bool EEL_Editor::sh_draw_parentokenstack_update(const char *tok, int toklen)
{
  if (toklen == 1)
  {
    switch (*tok)
    {
      case '(':
      case '[':
      case ';':
      case '?':
        s_draw_parentokenstack.Add(*tok);
      break;
      case ':': return sh_draw_parenttokenstack_pop('?');
      case ')': return sh_draw_parenttokenstack_pop('(');
      case ']': return sh_draw_parenttokenstack_pop('[');
    }
  }
  return false;
}


void EEL_Editor::draw_line_highlight(int y, const char *p, int *c_comment_state, int line_n)
{
  if (line_n == m_curs_y)
    m_suggestion_curline_comment_state = *c_comment_state;
  int last_attr = A_NORMAL;
  attrset(last_attr);
  move(y, 0);
  int rv = do_draw_line(p, c_comment_state, last_attr);
  attrset(rv< 0 ? SYNTAX_ERROR : A_NORMAL);
  clrtoeol();
  if (rv < 0) attrset(A_NORMAL);
}

int EEL_Editor::do_draw_line(const char *p, int *c_comment_state, int last_attr)
{
  //skipcnt = m_offs_x
  if (is_code_start_line(p)) 
  {
    *c_comment_state=0;
    s_draw_parentokenstack.Resize(0,false);
  }

  int skipcnt = m_offs_x;
  int ignoreSyntaxState = overrideSyntaxDrawingForLine(&skipcnt, &p, c_comment_state, &last_attr);

  if (ignoreSyntaxState>0)
  {
    int len = (int)strlen(p);
    draw_string(&skipcnt,p,len,&last_attr,ignoreSyntaxState==100 ? SYNTAX_ERROR : 
        ignoreSyntaxState==2 ? SYNTAX_COMMENT : A_NORMAL);
    return len-m_offs_x < COLS;
  }


  // syntax highlighting
  const char *endptr = p+strlen(p);
  const char *tok;
  const char *lp = p;
  int toklen=0;
  int last_comment_state=*c_comment_state;
  while (NULL != (tok = sh_tokenize(&p,endptr,&toklen,c_comment_state)) || lp < endptr)
  {
    if (tok && *tok < 0 && toklen == 1)
    {
      while (tok[toklen] < 0) {p++; toklen++; } // utf-8 skip
    }
    if (last_comment_state>0) // if in a multi-line string or comment
    {
      // draw empty space between lp and p as a string. in this case, tok/toklen includes our string, so we quickly finish after
      draw_string(&skipcnt,lp,(int)(p-lp),&last_attr, last_comment_state==1 ? SYNTAX_COMMENT:SYNTAX_STRING, last_comment_state);
      last_comment_state=0;
      lp = p;
      continue;
    }
    sh_func_ontoken(tok,toklen);

    // draw empty space between lp and tok/endptr as normal
    const char *adv_to = tok ? tok : endptr;
    if (adv_to > lp) draw_string(&skipcnt,lp,(int)(adv_to-lp),&last_attr, A_NORMAL);

    if (adv_to >= endptr) break;

    last_comment_state=0;
    lp = p;
    if (adv_to == p) continue;

    // draw token
    int attr = A_NORMAL;
    int err_left=0;
    int err_right=0;
    int start_of_tok = 0;

    if (tok[0] == '/' && toklen > 1 && (tok[1] == '*' || tok[1] == '/'))
    {
      attr = SYNTAX_COMMENT;
    }
    else if (tok[0] > 0 && (isalpha(tok[0]) || tok[0] == '_' || tok[0] == '#'))
    {
      int def_attr = A_NORMAL;
      bool isf=true;
      if (tok[0] == '#')
      {
        def_attr = SYNTAX_STRINGVAR;
        draw_string(&skipcnt,tok,1,&last_attr,def_attr);
        tok++;
        toklen--;
      }
      while (toklen > 0)
      {
        // divide up by .s, if any
        int this_len=0;
        while (this_len < toklen && tok[this_len] != '.') this_len++;
        if (this_len > 0)
        {
          int attr=isf?namedTokenHighlight(tok,this_len,*c_comment_state):def_attr;
          if (isf && attr == A_NORMAL)
          {
            int ntok_len=0, cc = *c_comment_state;
            const char *pp=lp,*ntok = sh_tokenize(&pp,endptr,&ntok_len,&cc);
            if (ntok && ntok_len>0 && *ntok == '(') def_attr = attr = SYNTAX_FUNC2;
          }

          draw_string(&skipcnt,tok,this_len,&last_attr,attr==A_NORMAL?def_attr:attr);
          tok += this_len;
          toklen -= this_len;
        }
        if (toklen > 0)
        {
          draw_string(&skipcnt,tok,1,&last_attr,SYNTAX_HIGHLIGHT1);
          tok++;
          toklen--;
        }
        isf=false;
      }
      continue;
    }
    else if (tok[0] == '.' ||
             (tok[0] >= '0' && tok[0] <= '9') ||
             (toklen > 1 && tok[0] == '$' && (tok[1] == 'x' || tok[1]=='X')))
    {
      attr = SYNTAX_HIGHLIGHT2;

      int x=1,mode=0;
      if (tok[0] == '.') mode=1;
      else if (toklen > 1 && (tok[0] == '$' || tok[0] == '0') && (tok[1] == 'x' || tok[1] == 'X')) { mode=2;  x++; }
      for(;x<toklen;x++)
      {
        if (tok[x] == '.'  && !mode) mode=1;
        else if (tok[x] < '0' || tok[x] > '9') 
        {
          if (mode != 2 || ((tok[x] < 'a' || tok[x] > 'f') && (tok[x] < 'A' || tok[x] > 'F')))
                break;
        }
      }
      if (x<toklen) err_right=toklen-x;
    }
    else if (tok[0] == '\'' || tok[0] == '\"')
    {
      start_of_tok = tok[0];
      attr = SYNTAX_STRING;
    }
    else if (tok[0] == '$')
    {
      attr = SYNTAX_HIGHLIGHT2;

      if (toklen >= 3 && !strnicmp(tok,"$pi",3)) err_right = toklen - 3;
      else if (toklen >= 2 && !strnicmp(tok,"$e",2)) err_right = toklen - 2;
      else if (toklen >= 4 && !strnicmp(tok,"$phi",4)) err_right = toklen - 4;
      else if (toklen == 4 && tok[1] == '\'' && tok[3] == '\'') { }
      else if (toklen > 1 && tok[1] == '~')
      {
        int x;
        for(x=2;x<toklen;x++) if (tok[x] < '0' || tok[x] > '9') break;
        if (x<toklen) err_right=toklen-x;
      }
      else err_right = toklen;
    }
    else if (ignoreSyntaxState==-1 && (tok[0] == '{' || tok[0] == '}'))
    {
      attr = SYNTAX_HIGHLIGHT1;
    }
    else
    {
      const char *h="()+*-=/,|&%;!<>?:^!~[]";
      while (*h && *h != tok[0]) h++;
      if (*h)
      {
        if (*c_comment_state != STATE_BEFORE_CODE && sh_draw_parentokenstack_update(tok,toklen))
          attr = SYNTAX_ERROR;
        else
          attr = SYNTAX_HIGHLIGHT1;
      }
      else 
      {
        err_left=1;
        if (tok[0] < 0) while (err_left < toklen && tok[err_left]<0) err_left++; // utf-8 skip
      }
    }

    if (ignoreSyntaxState) err_left = err_right = 0;

    if (err_left > 0) 
    {
      if (err_left > toklen) err_left=toklen;
      draw_string(&skipcnt,tok,err_left,&last_attr,SYNTAX_ERROR);
      tok+=err_left;
      toklen -= err_left;
    }
    if (err_right > toklen) err_right=toklen;

    draw_string(&skipcnt, tok, toklen - err_right, &last_attr, attr, start_of_tok);

    if (err_right > 0)
      draw_string(&skipcnt,tok+toklen-err_right,err_right,&last_attr,SYNTAX_ERROR);

    if (ignoreSyntaxState == -1 && tok[0] == '>')
    {
      draw_string(&skipcnt,p,strlen(p),&last_attr,ignoreSyntaxState==2 ? SYNTAX_COMMENT : A_NORMAL);
      break;
    }
  }
  return 1;
}

int EEL_Editor::GetCommentStateForLineStart(int line)
{
  if (m_write_leading_tabs<=0) m_indent_size=2;
  const bool uses_code_start_lines = !!is_code_start_line(NULL);

  int state=0;
  int x=0;

  if (uses_code_start_lines)
  {
    state=STATE_BEFORE_CODE;
    for (;;x++)
    {
      WDL_FastString *t = m_text.Get(x);
      if (!t || is_code_start_line(t->Get())) break;
    
      const char *p=t->Get();

      if (!strnicmp(p,"tabsize:",8))
      {
        int a = atoi(p+8);
        if (a>0 && a < 32) m_indent_size = a;
      }
    }


    // scan backwards to find line starting with @
    for (x=line;x>=0;x--)
    {
      WDL_FastString *t = m_text.Get(x);
      if (!t) break;
      if (is_code_start_line(t->Get()))
      {
        state=0;
        break;
      }
    }
    x++;
  }

  s_draw_parentokenstack.Resize(0,false);

  for (;x<line;x++)
  {
    WDL_FastString *t = m_text.Get(x);
    const char *p = t?t->Get():"";
    if (is_code_start_line(p)) 
    {
      s_draw_parentokenstack.Resize(0,false);
      state=0; 
    }
    else if (state != STATE_BEFORE_CODE)
    {
      const int ll=t?t->GetLength():0;
      const char *endp = p+ll;
      int toklen;
      const char *tok;
      while (NULL != (tok=sh_tokenize(&p,endp,&toklen,&state))) // eat all tokens, updating state
      {
        sh_func_ontoken(tok,toklen);
        sh_draw_parentokenstack_update(tok,toklen);
      }
    }
  }
  return state;
}

const char *EEL_Editor::sh_tokenize(const char **ptr, const char *endptr, int *lenOut, int *state)
{
  return nseel_simple_tokenizer(ptr, endptr, lenOut, state);
}


bool EEL_Editor::LineCanAffectOtherLines(const char *txt, int spos, int slen) // if multiline comment etc
{
  const char *special_start = txt + spos;
  const char *special_end = txt + spos + slen;
  while (*txt)
  {
    if (txt >= special_start-1 && txt < special_end)
    {
      const char c = txt[0];
      if (c == '*' && txt[1] == '/') return true;
      if (c == '/' && (txt[1] == '/' || txt[1] == '*')) return true;
      if (c == '\\' && (txt[1] == '\"' || txt[1] == '\'')) return true;

      if (txt >= special_start)
      {
        if (c == '\"' || c == '\'') return true;
        if (c == '(' || c == '[' || c == ')' || c == ']' || c == ':' || c == ';' || c == '?') return true;
      }
    }
    txt++;
  }
  return false;
}


struct eel_sh_token
{
  int line, col, end_col; 
  unsigned int data; // packed char for token type, plus 24 bits for linecnt (0=single line, 1=extra line, etc)

  eel_sh_token(int _line, int _col, int toklen, unsigned char c)
  {
    line = _line;
    col = _col;
    end_col = col + toklen;
    data = c;
  }
  ~eel_sh_token() { }

  void add_linecnt(int endcol) { data += 256; end_col = endcol; }
  int get_linecnt() const { return (data >> 8); }
  char get_c() const { return (char) (data & 255); }

  bool is_comment() const {
    return get_c() == '/' && (get_linecnt() || end_col>col+1);
  };
};

static int eel_sh_get_token_for_pos(const WDL_TypedBuf<eel_sh_token> *toklist, int line, int col, bool *is_after)
{
  const int sz = toklist->GetSize();
  int x;
  for (x=0; x < sz; x ++)
  {
    const eel_sh_token *tok = toklist->Get()+x;
    const int first_line = tok->line;
    const int last_line = first_line+tok->get_linecnt(); // last affected line (usually same as first)

    if (last_line >= line) // if this token doesn't end before the line we care about
    {
      // check to see if the token starts after our position
      if (first_line > line || (first_line == line && tok->col > col)) break; 

      // token started before line/col, see if it ends after line/col
      if (last_line > line || tok->end_col > col) 
      {
        // direct hit
        *is_after = false;
        return x;
      } 
    }
  }
  *is_after = true;
  return x-1;
}

static void eel_sh_generate_token_list(const WDL_PtrList<WDL_FastString> *lines, WDL_TypedBuf<eel_sh_token> *toklist, int start_line, EEL_Editor *editor)
{
  toklist->Resize(0,false);
  int state=0;
  int l;
  int end_line = lines->GetSize();
  if (editor->is_code_start_line(NULL))
  {
    for (l = start_line; l < end_line; l ++)
    {
      WDL_FastString *s = lines->Get(l);
      if (s && editor->is_code_start_line(s->Get()))
      {
        end_line = l;
        break;
      }
    }
    for (; start_line >= 0; start_line--)
    {
      WDL_FastString *s = lines->Get(start_line);
      if (s && editor->is_code_start_line(s->Get())) break;
    }
    if (start_line < 0) return; // before any code
  
    start_line++;
  }
  else
  {
    start_line = 0;
  }
  

  for (l=start_line;l<end_line;l++)
  {
    WDL_FastString *t = lines->Get(l);
    const int ll = t?t->GetLength():0;
    const char *start_p = t?t->Get():"";
    const char *p = start_p;
    const char *endp = start_p+ll;
   
    const char *tok;
    int last_state=state;
    int toklen;
    while (NULL != (tok=editor->sh_tokenize(&p,endp,&toklen,&state))||last_state)
    {
      if (last_state == '\'' || last_state == '"' || last_state==1)
      {
        const int sz=toklist->GetSize();
        // update last token to include this data
        if (sz) toklist->Get()[sz-1].add_linecnt((int) ((tok ? p:endp) - start_p));
      }
      else
      {
        if (tok) switch (tok[0])
        {
          case '{':
          case '}':
          case '?':
          case ':':
          case ';':
          case '(':
          case '[':
          case ')':
          case ']':
          case '\'':
          case '"':
          case '/': // comment
            {
              eel_sh_token t(l,(int)(tok-start_p),toklen,tok[0]);
              toklist->Add(t);
            }
          break;
        }
      }
      last_state=0;
    }
  }
}

static bool eel_sh_get_matching_pos_for_pos(WDL_PtrList<WDL_FastString> *text, int curx, int cury, int *newx, int *newy, const char **errmsg, EEL_Editor *editor)
{
  static WDL_TypedBuf<eel_sh_token> toklist;
  eel_sh_generate_token_list(text,&toklist, cury,editor);
  bool is_after;
  const int hit_tokidx = eel_sh_get_token_for_pos(&toklist, cury, curx, &is_after);
  const eel_sh_token *hit_tok = hit_tokidx >= 0 ? toklist.Get() + hit_tokidx : NULL;

  if (!is_after && hit_tok && (hit_tok->get_c() == '"' || hit_tok->get_c() == '\'' || hit_tok->is_comment()))
  {
    eel_sh_token tok = *hit_tok; // save a copy, toklist might get destroyed recursively here
    hit_tok = &tok;

    //if (tok.get_c() == '"')
    {
      // the user could be editing code in code, tokenize it and see if we can make sense of it
      WDL_FastString start, end;
      WDL_PtrList<WDL_FastString> tmplist;
      WDL_FastString *s = text->Get(tok.line);
      if (s && s->GetLength() > tok.col+1)
      {
        int maxl = tok.get_linecnt()>0 ? 0 : tok.end_col - tok.col - 2;
        start.Set(s->Get() + tok.col+1, maxl);
      }
      tmplist.Add(&start);
      const int linecnt = tok.get_linecnt();
      if (linecnt>0)
      {
        for (int a=1; a < linecnt; a ++)
        {
          s = text->Get(tok.line + a);
          if (s) tmplist.Add(s);
        }
        s = text->Get(tok.line + linecnt);
        if (s)
        {
          if (tok.end_col>1) end.Set(s->Get(), tok.end_col-1);
          tmplist.Add(&end);
        }
      }

      int lx = curx, ly = cury - tok.line;
      if (cury == tok.line) lx -= (tok.col+1);

      // this will destroy the token 
      if (eel_sh_get_matching_pos_for_pos(&tmplist, lx, ly, newx, newy, errmsg, editor))
      {
        *newy += tok.line;
        if (cury == tok.line) *newx += tok.col + 1;
        return true;
      }
    }

    // if within a string or comment, move to start, unless already at start, move to end
    if (cury == hit_tok->line && curx == hit_tok->col)
    {
      *newx=hit_tok->end_col-1;
      *newy=hit_tok->line + hit_tok->get_linecnt();
    }
    else
    {
      *newx=hit_tok->col;
      *newy=hit_tok->line;
    }
    return true;
  }

  if (!hit_tok) return false;

  const int toksz=toklist.GetSize();
  int tokpos = hit_tokidx;
  int pc1=0,pc2=0; // (, [ count
  int pc3=0; // : or ? count depending on mode
  int dir=-1, mode=0;  // default to scan to previous [(
  if (!is_after) 
  {
    switch (hit_tok->get_c())
    {
      case '(': mode=1; dir=1; break;
      case '[': mode=2; dir=1; break;
      case ')': mode=3; dir=-1; break;
      case ']': mode=4; dir=-1; break;
      case '?': mode=5; dir=1; break;
      case ':': mode=6; break;
      case ';': mode=7; break;
    }
    // if hit a token, exclude this token from scanning
    tokpos += dir;
  }

  while (tokpos>=0 && tokpos<toksz)
  {
    const eel_sh_token *tok = toklist.Get() + tokpos;
    const char this_c = tok->get_c();
    if (!pc1 && !pc2)
    {
      bool match=false, want_abort=false;
      switch (mode)
      {
        case 0: match = this_c == '(' || this_c == '['; break;
        case 1: match = this_c == ')'; break;
        case 2: match = this_c == ']'; break;
        case 3: match = this_c == '('; break;
        case 4: match = this_c == '['; break;
        case 5: 
          // scan forward to nearest : or ;
          if (this_c == '?') pc3++;
          else if (this_c == ':')
          {
            if (pc3>0) pc3--;
            else match=true;
          }
          else if (this_c == ';') match=true;
          else if (this_c == ')' || this_c == ']') 
          {
            want_abort=true; // if you have "(x<y?z)", don't match for the ?
          }
        break;
        case 6:  // scanning back from : to ?, if any
        case 7:  // semicolon searches same as colon, effectively
          if (this_c == ':') pc3++;
          else if (this_c == '?')
          {
            if (pc3>0) pc3--;
            else match = true;
          }
          else if (this_c == ';' || this_c == '(' || this_c == '[') 
          {
            want_abort=true;
          }
        break;
      }

      if (want_abort) break;
      if (match)
      {
        *newx=tok->col;
        *newy=tok->line;
        return true;
      }
    }
    switch (this_c)
    {
      case '[': pc2++; break;
      case ']': pc2--; break;
      case '(': pc1++; break;
      case ')': pc1--; break;
    }
    tokpos+=dir;
  }    

  if (errmsg)
  {
    if (!mode) *errmsg = "Could not find previous [ or (";
    else if (mode == 1) *errmsg = "Could not find matching )";
    else if (mode == 2) *errmsg = "Could not find matching ]";
    else if (mode == 3) *errmsg = "Could not find matching (";
    else if (mode == 4) *errmsg = "Could not find matching [";
    else if (mode == 5) *errmsg = "Could not find matching : or ; for ?";
    else if (mode == 6) *errmsg = "Could not find matching ? for :";
    else if (mode == 7) *errmsg = "Could not find matching ? for ;";
  }
  return false;
}


void EEL_Editor::doParenMatching()
{
  WDL_FastString *curstr;
  const char *errmsg = "";
  if (NULL != (curstr=m_text.Get(m_curs_y)))
  {
    int bytex = WDL_utf8_charpos_to_bytepos(curstr->Get(),m_curs_x);
    if (bytex >= curstr->GetLength()) bytex=curstr->GetLength()-1;
    if (bytex<0) bytex = 0;

    int new_x,new_y;
    if (eel_sh_get_matching_pos_for_pos(&m_text, bytex,m_curs_y,&new_x,&new_y,&errmsg,this))
    {
      curstr = m_text.Get(new_y);
      if (curstr) new_x = WDL_utf8_bytepos_to_charpos(curstr->Get(),new_x);

      m_curs_x=new_x;
      m_curs_y=new_y;
      m_want_x=-1;
      draw();
      setCursor(1);
    }
    else if (errmsg[0])
    {
      draw_message(errmsg);
      setCursor(0);
    }
  }
}

static int word_len(const char *p)
{
  int l = 0;
  if (*p >= 'a' && *p <='z')
  {
    l++;
    // lowercase word
    while (p[l] && p[l] != '_' && p[l] != '.' && !(p[l] >= 'A' && p[l] <='Z')) l++;
  }
  else if (*p >= 'A' && *p <= 'Z')
  {
    if (!strcmp(p,"MIDI")) return 4;
    l++;
    if (p[l] >= 'A'  && p[l] <='Z') // UPPERCASE word
      while (p[l] && p[l] != '_' && p[l] != '.' && !(p[l] >= 'a' && p[l] <='z')) l++;
    else // Titlecase word
      while (p[l] && p[l] != '_' && p[l] != '.' && !(p[l] >= 'A' && p[l] <='Z')) l++;
  }
  return l;
}

static int search_str_partial(const char *str, int reflen, const char *word, int wordlen)
{
  // find the longest leading segment of word in str
  int best_len = 0;
  for (int y = 0; y < reflen; y ++)
  {
    while (y < reflen && !strnicmp(str+y,word,best_len+1))
    {
      if (++best_len >= wordlen) return best_len;
      reflen--;
    }
  }
  return best_len;
}

static int fuzzy_match2(const char *codestr, int codelen, const char *refstr, int reflen)
{
  // codestr is user-typed, refstr is the reference function name
  // our APIs are gfx_blah_blah or TrackBlahBlah so breaking the API up by words
  // and searching the user-entered code should be effective
  int lendiff = reflen - codelen;
  if (lendiff < 0) lendiff = -lendiff;

  const char *word = refstr;
  int score = 0;
  for (;;)
  {
    while (*word == '_' || *word == '.') word++;
    const int wordlen = word_len(word);
    if (!wordlen) break;
    char word_buf[128];
    lstrcpyn_safe(word_buf,word,wordlen+1);

    if (WDL_stristr(refstr,word_buf)==word)
    {
      int max_match_len = search_str_partial(codestr,codelen,word,wordlen);
      if (max_match_len < 2 && wordlen == 5 && !strnicmp(word,"Count",5))
      {
        max_match_len = search_str_partial(codestr,codelen,"Num",3);
      }
      if (max_match_len > (wordlen <= 2 ? 1 : 2))
        score += max_match_len;
    }
    word += wordlen;
  }

  if (!score) return 0;

  return score * 1000 + 1000 - wdl_clamp(lendiff*2,0,200);
}

int EEL_Editor::fuzzy_match(const char *codestr, const char *refstr)
{
  const int codestr_len = (int)strlen(codestr), refstr_len = (int)strlen(refstr);
  if (refstr_len >= codestr_len && !strnicmp(codestr,refstr,codestr_len)) return 1000000000;
  int score1 = fuzzy_match2(refstr,refstr_len,codestr,codestr_len);
  int score2 = fuzzy_match2(codestr,codestr_len,refstr,refstr_len);
  if (score2 > score1) return score2 | 1;
  return score1&~1;
}

static int eeledit_varenumfunc(const char *name, EEL_F *val, void *ctx)
{
  void **parms = (void **)ctx;
  int score = ((EEL_Editor*)parms[2])->fuzzy_match((const char *)parms[1], name);
  if (score > 0) ((suggested_matchlist*)parms[0])->add(name,score,suggested_matchlist::MODE_VAR);
  return 1;
}

void EEL_Editor::get_suggested_token_names(const char *fname, int chkmask, suggested_matchlist *list)
{
  int x;
  if (chkmask & (KEYWORD_MASK_BUILTIN_FUNC|KEYWORD_MASK_USER_VAR))
  {
    peek_lock();
    NSEEL_VMCTX vm = peek_get_VM();
    compileContext *fvm = vm && peek_want_VM_funcs() ? (compileContext*)vm : NULL;
    if (chkmask&KEYWORD_MASK_BUILTIN_FUNC) for (x=0;;x++)
    {
      functionType *p = nseel_enumFunctions(fvm,x);
      if (!p) break;
      int score = fuzzy_match(fname,p->name);
      if (score>0) list->add(p->name,score);
    }
    if (vm && (chkmask&KEYWORD_MASK_USER_VAR))
    {
      const void *parms[3] = { list, fname, this };
      NSEEL_VM_enumallvars(vm, eeledit_varenumfunc, parms);
    }
    peek_unlock();
  }

  if (chkmask & KEYWORD_MASK_USER_FUNC)
  {
    ensure_code_func_cache_valid();
    for (int x=0;x< m_code_func_cache.GetSize();x++)
    {
      const char *k = m_code_func_cache.Get(x) + 4;
      if (WDL_NORMALLY(k))
      {
        int score = fuzzy_match(fname,k);
        if (score > 0) list->add(k,score,suggested_matchlist::MODE_USERFUNC);
      }
    }
  }
}

int EEL_Editor::peek_get_token_info(const char *name, char *sstr, size_t sstr_sz, int chkmask, int ignoreline)
{
  if (chkmask&KEYWORD_MASK_USER_FUNC)
  {
    ensure_code_func_cache_valid();
    for (int i = 0; i < m_code_func_cache.GetSize(); i ++)
    {
      const char *cacheptr = m_code_func_cache.Get(i);
      const char *nameptr = cacheptr + sizeof(int);
      if (!(m_case_sensitive ? strcmp(nameptr, name):stricmp(nameptr,name)) &&
          *(int *)cacheptr != ignoreline)
      {
        const char *parms = nameptr+strlen(nameptr)+1;
        const char *trail = parms+strlen(parms)+1;
        snprintf(sstr,sstr_sz,"%s%s%s%s",nameptr,parms,*trail?" " :"",trail);
        return 4;
      }
    }
  }

  if (chkmask & (KEYWORD_MASK_BUILTIN_FUNC|KEYWORD_MASK_USER_VAR))
  {
    int rv = 0;
    peek_lock();
    NSEEL_VMCTX vm = peek_want_VM_funcs() ? peek_get_VM() : NULL;
    functionType *f = (chkmask&KEYWORD_MASK_BUILTIN_FUNC) ? nseel_getFunctionByName((compileContext*)vm,name,NULL) : NULL;
    double v;
    if (f)
    {
      snprintf(sstr,sstr_sz,"'%s' is a function that requires %d parameters", f->name,f->nParams&0xff);
      rv = KEYWORD_MASK_BUILTIN_FUNC;
    }
    else if (chkmask & KEYWORD_MASK_USER_VAR)
    {
      if (!vm) vm = peek_get_VM();
      EEL_F *vptr=NSEEL_VM_getvar(vm,name);
      if (vptr)
      {
        v = *vptr;
        rv = KEYWORD_MASK_USER_VAR;
      }
    }
    peek_unlock();

    if (rv == KEYWORD_MASK_USER_VAR)
    {
      int good_len=-1;
      snprintf(sstr,sstr_sz,"%s=%.14f",name,v);

      if (v > -1.0 && v < NSEEL_RAM_ITEMSPERBLOCK*NSEEL_RAM_BLOCKS)
      {
        const unsigned int w = (unsigned int) (v+NSEEL_CLOSEFACTOR);
        EEL_F *dv = NSEEL_VM_getramptr_noalloc(vm,w,NULL);
        if (dv)
        {
          snprintf_append(sstr,sstr_sz," [0x%06x]=%.14f",w,*dv);
          good_len=-2;
        }
        else
        {
          good_len = strlen(sstr);
          snprintf_append(sstr,sstr_sz," [0x%06x]=<uninit>",w);
        }
      }

      char buf[512];
      buf[0]=0;
      if (peek_get_numbered_string_value(v,buf,sizeof(buf)))
      {
        if (good_len==-2)
          snprintf_append(sstr,sstr_sz," %.0f(str)=%s",v,buf);
        else
        {
          if (good_len>=0) sstr[good_len]=0; // remove [addr]=<uninit> if a string and no ram
          snprintf_append(sstr,sstr_sz," (str)=%s",buf);
        }
      }
    }

    if (rv) return rv;
  }

  return 0;
}


void EEL_Editor::doWatchInfo(int c)
{
    // determine the word we are on, check its value in the effect
  char sstr[512], buf[512];
  lstrcpyn_safe(sstr,"Use this on a valid symbol name", sizeof(sstr));
  WDL_FastString *t=m_text.Get(m_curs_y);
  char curChar=0;
  if (t)
  {
    const char *p=t->Get();
    const int bytex = WDL_utf8_charpos_to_bytepos(p,m_curs_x);
    if (bytex >= 0 && bytex < t->GetLength()) curChar = p[bytex];
    if (c != KEY_F1 && (m_selecting || 
             curChar == '(' || 
             curChar == '[' ||
             curChar == ')' ||
             curChar == ']'
             ))
    {
      WDL_FastString code;
      int miny,maxy,minx,maxx;
      bool ok = false;
      if (!m_selecting)
      {
        if (eel_sh_get_matching_pos_for_pos(&m_text,minx=m_curs_x,miny=m_curs_y,&maxx, &maxy,NULL,this))
        {
          if (maxy==miny)
          {
            if (maxx < minx)
            {
              int tmp = minx;
              minx=maxx;
              maxx=tmp;
            }
          }
          else if (maxy < miny)
          {
            int tmp=maxy;
            maxy=miny;
            miny=tmp;
            tmp = minx;
            minx=maxx;
            maxx=tmp;
          }
          ok = true;
          minx++; // skip leading (
        }
      }
      else
      {
        ok=true; 
        getselectregion(minx,miny,maxx,maxy); 
        WDL_FastString *s;
        s = m_text.Get(miny);
        if (s) minx = WDL_utf8_charpos_to_bytepos(s->Get(),minx);
        s = m_text.Get(maxy);
        if (s) maxx = WDL_utf8_charpos_to_bytepos(s->Get(),maxx);
      }

      if (ok)
      {
        int x;
        for (x = miny; x <= maxy; x ++)
        {
          WDL_FastString *s=m_text.Get(x);
          if (s) 
          {
            const char *str=s->Get();
            int sx,ex;
            if (x == miny) sx=wdl_max(minx,0);
            else sx=0;
            int tmp=s->GetLength();
            if (sx > tmp) sx=tmp;
      
            if (x == maxy) ex=wdl_min(maxx,tmp);
            else ex=tmp;
      
            if (code.GetLength()) code.Append("\r\n");
            code.Append(ex-sx?str+sx:"",ex-sx);
          }
        }
      }
      if (code.Get()[0])
      {
        if (m_selecting && (GetAsyncKeyState(VK_SHIFT)&0x8000))
        {
          peek_lock();
          NSEEL_CODEHANDLE ch;
          NSEEL_VMCTX vm = peek_get_VM();

          if (vm && (ch = NSEEL_code_compile_ex(vm,code.Get(),1,0)))
          {
            codeHandleType *p = (codeHandleType*)ch;
            code.Ellipsize(3,20);
            const char *errstr = "failed writing to";
            if (p->code)
            {
              buf[0]=0;
              GetTempPath(sizeof(buf)-64,buf);
              lstrcatn(buf,"jsfx-out",sizeof(buf));
              FILE *fp = fopen(buf,"wb");
              if (fp)
              {
                errstr="wrote to";
                fwrite(p->code,1,p->code_size,fp);
                fclose(fp);
              }
            }
            snprintf(sstr,sizeof(sstr),"Expression '%s' compiled to %d bytes, %s temp/jsfx-out",code.Get(),p->code_size, errstr);
            NSEEL_code_free(ch);
          }
          else
          {
            code.Ellipsize(3,20);
            snprintf(sstr,sizeof(sstr),"Expression '%s' could not compile",code.Get());
          }
          peek_unlock();
        }
        else
        {
          WDL_FastString code2;
          code2.Set("__debug_watch_value = (((((");
          code2.Append(code.Get());
          code2.Append(")))));");
      
          peek_lock();

          NSEEL_VMCTX vm = peek_get_VM();

          EEL_F *vptr=NULL;
          double v=0.0;
          const char *err="Invalid context";
          if (vm)
          {
            NSEEL_CODEHANDLE ch = NSEEL_code_compile_ex(vm,code2.Get(),1,0);
            if (!ch) err = "Error parsing";
            else
            {
              NSEEL_code_execute(ch);
              NSEEL_code_free(ch);
              vptr = NSEEL_VM_getvar(vm,"__debug_watch_value");
              if (vptr) v = *vptr;
            }
          }

          peek_unlock();

          {
            // remove whitespace from code for display
            int x;
            bool lb=true;
            for (x=0;x<code.GetLength();x++)
            {
              if (isspace(code.Get()[x]))
              {
                if (lb) code.DeleteSub(x--,1);
                lb=true;
              }
              else
              {
                lb=false;
              }
            }
            if (lb && code.GetLength()>0) code.SetLen(code.GetLength()-1);
          }

          code.Ellipsize(3,20);
          if (vptr)
          {
            snprintf(sstr,sizeof(sstr),"Expression '%s' evaluates to %.14f",code.Get(),v);
          }
          else
          {
            snprintf(sstr,sizeof(sstr),"Error evaluating '%s': %s",code.Get(),err?err:"Unknown error");
          }
        }
      }
      // compile+execute code within () as debug_watch_value = ( code )
      // show value (or err msg)
    }
    else if (curChar>0 && (isalnum(curChar) || curChar == '_' || curChar == '.' || curChar == '#')) 
    {
      const int bytex = WDL_utf8_charpos_to_bytepos(p,m_curs_x);
      const char *lp=p+bytex;
      const char *rp=lp + WDL_utf8_charpos_to_bytepos(lp,1);
      while (lp >= p && *lp > 0 && (isalnum(*lp) || *lp == '_' || (*lp == '.' && (lp==p || lp[-1]!='.')))) lp--;
      if (lp < p || *lp != '#') lp++;
      while (*rp && *rp > 0 && (isalnum(*rp) || *rp == '_' || (*rp == '.' && rp[1] != '.'))) rp++;

      if (*lp == '#' && rp > lp+1)
      {
        WDL_FastString n;
        lp++;
        n.Set(lp,(int)(rp-lp));
        int idx;
        if ((idx=peek_get_named_string_value(n.Get(),buf,sizeof(buf)))>=0) snprintf(sstr,sizeof(sstr),"#%s(%d)=%s",n.Get(),idx,buf);
        else snprintf(sstr,sizeof(sstr),"#%s not found",n.Get());
      }
      else if (*lp > 0 && (isalpha(*lp) || *lp == '_') && rp > lp)
      {
        WDL_FastString n;
        n.Set(lp,(int)(rp-lp));

        if (c==KEY_F1)
        {
          if (m_suggestion.GetLength())
            goto help_from_sug;
          on_help(n.Get(),0);
          return;
        }

        int f = peek_get_token_info(n.Get(),sstr,sizeof(sstr),~0,-1);
        if (!f) snprintf(sstr,sizeof(sstr),"'%s' NOT FOUND",n.Get());
      }
    }
  }
  if (c==KEY_F1)
  {
help_from_sug:
    WDL_FastString t;
    if (m_suggestion.GetLength())
    {
      const char *p = m_suggestion.Get();
      int l;
      for (l = 0; isalnum(p[l]) || p[l] == '_' || p[l] == '.'; l ++);
      if (l>0) t.Set(m_suggestion.Get(),l);
    }
    on_help(t.GetLength() > 2 ? t.Get() : NULL,(int)curChar);
    return;
  }

  setCursor();
  draw_message(sstr);
}


void EEL_Editor::draw_bottom_line()
{
#define BOLD(x) { attrset(COLOR_BOTTOMLINE|A_BOLD); addstr(x); attrset(COLOR_BOTTOMLINE&~A_BOLD); }
  addstr("ma"); BOLD("T"); addstr("ch");
  BOLD(" S"); addstr("ave");
  if (peek_get_VM())
  {
    addstr(" pee"); BOLD("K");
  }
  if (GetTabCount()>1)
  {
    addstr(" | tab: ");
    BOLD("[], F?"); addstr("=switch ");
    BOLD("W"); addstr("=close");
  }
#undef BOLD
}

#define CTRL_KEY_DOWN (GetAsyncKeyState(VK_CONTROL)&0x8000)
#define SHIFT_KEY_DOWN (GetAsyncKeyState(VK_SHIFT)&0x8000)
#define ALT_KEY_DOWN (GetAsyncKeyState(VK_MENU)&0x8000)

static const char *suggestion_help_text = "(up/down to select, tab to insert)";
static const char *suggestion_help_text2 = "(tab or return to insert)";
static LRESULT WINAPI suggestionProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  EEL_Editor *editor;
  switch (uMsg)
  {
    case WM_DESTROY:
      editor = (EEL_Editor *)GetWindowLongPtr(hwnd,GWLP_USERDATA);
      if (editor) editor->m_suggestion_hwnd = NULL;
    break;
    case WM_LBUTTONDOWN:
    case WM_MOUSEMOVE:
      editor = (EEL_Editor *)GetWindowLongPtr(hwnd,GWLP_USERDATA);
      if (editor)
      {
        win32CursesCtx *ctx = (win32CursesCtx *)editor->m_cursesCtx;
        if (ctx && ctx->m_font_h)
        {
          RECT r;
          GetClientRect(hwnd,&r);
          SetForegroundWindow(GetParent(hwnd));
          SetFocus(GetParent(hwnd));

          const int max_vis = r.bottom / ctx->m_font_h - 1, sel = editor->m_suggestion_hwnd_sel;
          int hit = GET_Y_LPARAM(lParam) / ctx->m_font_h;
          if (hit >= max_vis) return 0;
          if (sel >= max_vis) hit += 1 + sel - max_vis;

          if (uMsg == WM_LBUTTONDOWN && !SHIFT_KEY_DOWN && !ALT_KEY_DOWN && !CTRL_KEY_DOWN)
          {
            editor->m_suggestion_hwnd_sel = hit;
            editor->onChar('\t');
          }
          else if (sel != hit)
          {
            editor->m_suggestion_hwnd_sel = hit;
            InvalidateRect(hwnd,NULL,FALSE);

            char sug[512];
            sug[0]=0;
            const char *p = editor->m_suggestion_list.get(hit);
            if (p && editor->peek_get_token_info(p,sug,sizeof(sug),~0,-1))
            {
              editor->m_suggestion.Set(sug);
              editor->draw_top_line();
              editor->setCursor();
            }
          }
        }
      }
    return 0;
    case WM_PAINT:
      editor = (EEL_Editor *)GetWindowLongPtr(hwnd,GWLP_USERDATA);
      if (editor)
      {
        PAINTSTRUCT ps;
        if (BeginPaint(hwnd,&ps))
        {
          RECT r;
          GetClientRect(hwnd,&r);

          win32CursesCtx *ctx = (win32CursesCtx *)editor->m_cursesCtx;
          HBRUSH br = CreateSolidBrush(ctx->colortab[WDL_CursesEditor::COLOR_TOPLINE][1]);
          FillRect(ps.hdc,&r,br);
          DeleteObject(br);

          suggested_matchlist *ml = &editor->m_suggestion_list;

          HGDIOBJ oldObj = SelectObject(ps.hdc,ctx->mOurFont);
          SetBkMode(ps.hdc,TRANSPARENT);

          const int fonth = wdl_max(ctx->m_font_h,1);
          const int maxv = r.bottom / fonth - 1;
          const int selpos = wdl_max(editor->m_suggestion_hwnd_sel,0);
          int ypos = 0;

          const bool show_scores = (GetAsyncKeyState(VK_SHIFT)&0x8000) && (GetAsyncKeyState(VK_CONTROL)&0x8000) && (GetAsyncKeyState(VK_MENU)&0x8000);

          for (int x = (selpos >= maxv ? 1+selpos-maxv : 0); x < ml->get_size() && ypos <= r.bottom-fonth*2; x ++)
          {
            int mode, score;
            const char *p = ml->get(x,&mode,&score);
            if (WDL_NORMALLY(p))
            {
              const bool sel = x == selpos;
              SetTextColor(ps.hdc,ctx->colortab[
                  (mode == suggested_matchlist::MODE_VAR ? WDL_CursesEditor::COLOR_TOPLINE :
                   mode == suggested_matchlist::MODE_USERFUNC ? WDL_CursesEditor::SYNTAX_FUNC2 :
                   mode == suggested_matchlist::MODE_REGVAR ? WDL_CursesEditor::SYNTAX_REGVAR :
                   WDL_CursesEditor::SYNTAX_KEYWORD)
                  | (sel ? A_BOLD:0)][0]);
              RECT tr = {4, ypos, r.right-4, ypos+fonth };
              DrawTextUTF8(ps.hdc,p,-1,&tr,DT_SINGLELINE|DT_NOPREFIX|DT_TOP|DT_LEFT);
              if (show_scores)
              {
                char tmp[128];
                snprintf(tmp,sizeof(tmp),"(%d)",score);
                DrawTextUTF8(ps.hdc,tmp,-1,&tr,DT_SINGLELINE|DT_NOPREFIX|DT_TOP|DT_RIGHT);
              }
            }
            ypos += fonth;
          }
          {
            const COLORREF mix = ((ctx->colortab[WDL_CursesEditor::COLOR_TOPLINE][1]&0xfefefe)>>1) +
                                 ((ctx->colortab[WDL_CursesEditor::COLOR_TOPLINE][0]&0xfefefe)>>1);
            SetTextColor(ps.hdc,mix);
            RECT tr = {4, r.bottom-fonth, r.right-4, r.bottom };
            DrawTextUTF8(ps.hdc,
                editor->m_suggestion_hwnd_sel >= 0 ? suggestion_help_text2 : suggestion_help_text,
                -1,&tr,DT_SINGLELINE|DT_NOPREFIX|DT_TOP|DT_CENTER);
          }
          SelectObject(ps.hdc,oldObj);

          EndPaint(hwnd,&ps);
        }
      }
    return 0;
  }
  return DefWindowProc(hwnd,uMsg,wParam,lParam);
}

void EEL_Editor::open_import_line()
{
  WDL_FastString *txtstr=m_text.Get(m_curs_y);
  const char *txt=txtstr?txtstr->Get():NULL;
  char fnp[2048];
  if (txt && line_has_openable_file(txt,WDL_utf8_charpos_to_bytepos(txt,m_curs_x),fnp,sizeof(fnp)))
  {
    WDL_CursesEditor::OpenFileInTab(fnp);
  }
}

int EEL_Editor::onChar(int c)
{
  if ((m_ui_state == UI_STATE_NORMAL || m_ui_state == UI_STATE_MESSAGE) && 
      (c == 'K'-'A'+1 || c == 'S'-'A'+1 || c == 'R'-'A'+1 || !SHIFT_KEY_DOWN) && !ALT_KEY_DOWN) switch (c)
  {
  case KEY_F1:
    if (CTRL_KEY_DOWN) break;
  case 'K'-'A'+1:
    doWatchInfo(c);
  return 0;
  case 'S'-'A'+1:
   {
     WDL_DestroyCheck chk(&destroy_check);
     if(updateFile())
     {
       if (chk.isOK())
         draw_message("Error writing file, changes not saved!");
     }
     if (chk.isOK())
       setCursor();
   }
  return 0;

  // case 'I': note stupidly we map Ctrl+I to VK_TAB, bleh
  case 'R'-'A'+1:
    if (!SHIFT_KEY_DOWN) break;
    if (!m_selecting)
    {
      open_import_line();
    }
  return 0;
  case KEY_F4:
  case 'T'-'A'+1:
    doParenMatching();
  return 0;
  }


  int rv = 0;
  int do_sug = (g_eel_editor_max_vis_suggestions > 0 &&
                m_ui_state == UI_STATE_NORMAL &&
                !m_selecting && m_top_margin > 0 &&
                (c == 'L'-'A'+1 || (!CTRL_KEY_DOWN && !ALT_KEY_DOWN))) ? 1 : 0;
  bool did_fuzzy = false;
  char sug[1024];
  sug[0]=0;

  if (do_sug)
  {
    if (m_suggestion_hwnd)
    {
      // insert if tab or shift+enter or (enter if arrow-navigated)
      if ((c == '\t' && !SHIFT_KEY_DOWN) ||
          (c == '\r' && (m_suggestion_hwnd_sel>=0 || SHIFT_KEY_DOWN)))
      {
        char buf[512];
        int sug_mode;
        const char *ptr = m_suggestion_list.get(wdl_max(m_suggestion_hwnd_sel,0), &sug_mode);
        lstrcpyn_safe(buf,ptr?ptr:"",sizeof(buf));

        DestroyWindow(m_suggestion_hwnd);

        WDL_FastString *l=m_text.Get(m_curs_y);
        if (buf[0] && l &&
            WDL_NORMALLY(m_suggestion_tokpos>=0 && m_suggestion_tokpos < l->GetLength()) &&
            WDL_NORMALLY(m_suggestion_toklen>0) &&
            WDL_NORMALLY(m_suggestion_tokpos+m_suggestion_toklen <= l->GetLength()))
        {
          preSaveUndoState();
          l->DeleteSub(m_suggestion_tokpos,m_suggestion_toklen);
          l->Insert(buf,m_suggestion_tokpos);
          int pos = m_suggestion_tokpos + strlen(buf);
          if (sug_mode == suggested_matchlist::MODE_FUNC || sug_mode == suggested_matchlist::MODE_USERFUNC)
          {
            if (pos >= l->GetLength() || l->Get()[pos] != '(')
              l->Insert("(",pos++);
          }
          m_curs_x = WDL_utf8_bytepos_to_charpos(l->Get(),pos);
          draw();
          saveUndoState();
          setCursor();
          goto run_suggest;
        }
        return 0;
      }
      if ((c == KEY_UP || c==KEY_DOWN || c == KEY_NPAGE || c == KEY_PPAGE) && !SHIFT_KEY_DOWN)
      {
        m_suggestion_hwnd_sel = wdl_max(m_suggestion_hwnd_sel,0) +
          (c==KEY_UP ? -1 : c==KEY_DOWN ? 1 : c==KEY_NPAGE ? 4 : -4);

        if (m_suggestion_hwnd_sel >= m_suggestion_list.get_size())
          m_suggestion_hwnd_sel = m_suggestion_list.get_size()-1;
        if (m_suggestion_hwnd_sel < 0)
          m_suggestion_hwnd_sel=0;

        InvalidateRect(m_suggestion_hwnd,NULL,FALSE);

        const char *p = m_suggestion_list.get(m_suggestion_hwnd_sel);
        if (p) peek_get_token_info(p,sug,sizeof(sug),~0,m_curs_y);
        goto finish_sug;
      }
    }

    if (c==27 ||
        c=='L'-'A'+1 ||
        c=='\r' ||
        c=='\t' ||
        (c>=KEY_DOWN && c<= KEY_F12 && c!=KEY_DC)) do_sug = 2; // no fuzzy window
    else if (c=='\b' && !m_suggestion_hwnd) do_sug=2; // backspace will update but won't show suggestions
  }

  rv = WDL_CursesEditor::onChar(c);

run_suggest:
  if (do_sug)
  {
    WDL_FastString *l=m_text.Get(m_curs_y);
    if (l)
    {
      const int MAX_TOK=512;
      struct {
        const char *tok;
        int toklen;
      } token_list[MAX_TOK];

      const char *cursor = l->Get() + WDL_utf8_charpos_to_bytepos(l->Get(),m_curs_x);
      int ntok = 0;
      {
        const char *p = l->Get(), *endp = p + l->GetLength();
        int state = m_suggestion_curline_comment_state, toklen = 0, bcnt = 0, pcnt = 0;
        const char *tok;
                                       // if no parens/brackets are open, use a peekable token that starts at cursor
        while ((tok=sh_tokenize(&p,endp,&toklen,&state)) && cursor > tok-(!pcnt && !bcnt && (tok[0] < 0 || tok[0] == '_' || isalpha(tok[0]))))
        {
          if (!state)
          {
            if (WDL_NOT_NORMALLY(ntok >= MAX_TOK))
            {
              memmove(token_list,token_list+1,sizeof(token_list) - sizeof(token_list[0]));
              ntok--;
            }
            switch (*tok)
            {
              case '[': bcnt++; break;
              case ']': bcnt--; break;
              case '(': pcnt++; break;
              case ')': pcnt--; break;
            }
            token_list[ntok].tok = tok;
            token_list[ntok].toklen = toklen;
            ntok++;
          }
        }
      }

      int t = ntok;
      int comma_cnt = 0;
      while (--t >= 0)
      {
        const char *tok = token_list[t].tok;
        int toklen = token_list[t].toklen;
        const int lc = tok[0], ac = lc==')' ? '(' : lc==']' ? '[' : 0;
        if (ac)
        {
          int cnt=1;
          while (--t>=0)
          {
            const int c = token_list[t].tok[0];
            if (c == lc) cnt++;
            else if (c == ac && !--cnt) break;
          }
          if (t > 0)
          {
            const int c = token_list[t-1].tok[0];
            if (c != ',' && c != ')' && c != ']') t--;
            continue;
          }
        }
        if (t<0) break;

        if (tok[0] == ',') comma_cnt++;
        else if ((tok[0] < 0 || tok[0] == '_' || isalpha(tok[0])) && (cursor <= tok + toklen || (t < ntok-1 && token_list[t+1].tok[0] == '(')))
        {
          // if cursor is within or at end of token, or is a function (followed by open-paren)
          char buf[512];
          lstrcpyn_safe(buf,tok,wdl_min(toklen+1, sizeof(buf)));
          if (peek_get_token_info(buf,sug,sizeof(sug),~0,m_curs_y))
          {
            if (comma_cnt > 0)
            {
              // hide previous parameters from sug's parameter fields so we know which parameters
              // we are (hopefully on)
              char *pstart = sug;
              while (*pstart && *pstart != '(') pstart++;
              if (*pstart == '(') pstart++;
              int comma_pos = 0;
              char *p = pstart;
              while (comma_pos < comma_cnt)
              {
                while (*p == ' ') p++;
                while (*p && *p != ',' && *p != ')') p++;
                if (*p == ')') break;
                if (*p) p++;
                comma_pos++;
              }

              if (*p && *p != ')')
              {
                *pstart=0;
                lstrcpyn_safe(buf,p,sizeof(buf));
                snprintf_append(sug,sizeof(sug), "... %s",buf);
              }
            }
            break;
          }

          // only use token up to cursor for suggestions
          if (cursor >= tok && cursor <= tok+toklen)
          {
            toklen = (int) (cursor-tok);
            lstrcpyn_safe(buf,tok,wdl_min(toklen+1, sizeof(buf)));
          }

          if (c == '\b' && cursor == tok)
          {
          }
          else if (do_sug != 2 && t == ntok-1 && toklen >= 3 && cursor <= tok + toklen)
          {
            m_suggestion_list.clear();
            get_suggested_token_names(buf,~0,&m_suggestion_list);

            win32CursesCtx *ctx = (win32CursesCtx *)m_cursesCtx;
            if (m_suggestion_list.get_size()>0 &&
                WDL_NORMALLY(ctx->m_hwnd) && WDL_NORMALLY(ctx->m_font_w) && WDL_NORMALLY(ctx->m_font_h))
            {
              m_suggestion_toklen = toklen;
              m_suggestion_tokpos = (int)(tok-l->Get());
              m_suggestion_hwnd_sel = -1;
              if (!m_suggestion_hwnd)
              {
#ifdef _WIN32
                static HINSTANCE last_inst;
                const char *classname = "eel_edit_predicto";
                HINSTANCE inst = (HINSTANCE)GetWindowLongPtr(ctx->m_hwnd,GWLP_HINSTANCE);
                if (inst != last_inst)
                {
                  last_inst = inst;
                  WNDCLASS wc={CS_DBLCLKS,suggestionProc,};
                  wc.lpszClassName=classname;
                  wc.hInstance=inst;
                  wc.hCursor=LoadCursor(NULL,IDC_ARROW);
                  RegisterClass(&wc);
                }
                m_suggestion_hwnd = CreateWindowEx(0,classname,"", WS_CHILD, 0,0,0,0, ctx->m_hwnd, NULL, inst, NULL);
#else
                m_suggestion_hwnd = CreateDialogParam(NULL,NULL,ctx->m_hwnd, suggestionProc, 0);
#endif
                if (m_suggestion_hwnd) SetWindowLongPtr(m_suggestion_hwnd,GWLP_USERDATA,(LPARAM)this);
              }
              if (m_suggestion_hwnd)
              {
                const int fontw = ctx->m_font_w, fonth = ctx->m_font_h;
                int xpos = (WDL_utf8_bytepos_to_charpos(l->Get(),m_suggestion_tokpos) - m_offs_x) * fontw;
                RECT par_cr;
                GetClientRect(ctx->m_hwnd,&par_cr);

                int use_w = (int)strlen(suggestion_help_text);
                int use_h = (wdl_min(g_eel_editor_max_vis_suggestions,m_suggestion_list.get_size()) + 1)*fonth;
                for (int x = 0; x < m_suggestion_list.get_size(); x ++)
                {
                  const char *p = m_suggestion_list.get(x);
                  if (WDL_NORMALLY(p!=NULL))
                  {
                    const int l = (int) strlen(p);
                    if (l > use_w) use_w=l;
                  }
                }

                use_w = 8 + use_w * fontw;
                if (use_w > par_cr.right - xpos)
                {
                  xpos = wdl_max(par_cr.right - use_w,0);
                  use_w = par_cr.right - xpos;
                }

                const int cursor_line_y = ctx->m_cursor_y * fonth;
                int ypos = cursor_line_y + fonth;
                if (ypos + use_h > par_cr.bottom)
                {
                  if (cursor_line_y-fonth > par_cr.bottom - ypos)
                  {
                    // more room at the top, but enough?
                    ypos = cursor_line_y - use_h;
                    if (ypos<fonth)
                    {
                      ypos = fonth;
                      use_h = cursor_line_y-fonth;
                    }
                  }
                  else
                    use_h = par_cr.bottom - ypos;
                }

                SetWindowPos(m_suggestion_hwnd,NULL,xpos,ypos,use_w,use_h, SWP_NOZORDER|SWP_NOACTIVATE);
                InvalidateRect(m_suggestion_hwnd,NULL,FALSE);
                ShowWindow(m_suggestion_hwnd,SW_SHOWNA);
              }
              did_fuzzy = true;
              const char *p = m_suggestion_list.get(wdl_max(m_suggestion_hwnd_sel,0));
              if (p && peek_get_token_info(p,sug,sizeof(sug),~0,m_curs_y)) break;
            }
          }
        }
      }
    }
  }
  if (!did_fuzzy && m_suggestion_hwnd) DestroyWindow(m_suggestion_hwnd);

finish_sug:
  if (strcmp(sug,m_suggestion.Get()) && m_ui_state == UI_STATE_NORMAL)
  {
    m_suggestion.Set(sug);
    if (sug[0])
    {
      m_suggestion_x=m_curs_x;
      m_suggestion_y=m_curs_y;
      draw_top_line();
      setCursor();
    }
  }
  if (!sug[0] && m_suggestion_y>=0 && m_ui_state == UI_STATE_NORMAL)
  {
    m_suggestion_x=m_suggestion_y=-1;
    m_suggestion.Set("");
    if (m_top_margin>0) draw_top_line();
    else draw();
    setCursor();
  }

  return rv;
}

void EEL_Editor::draw_top_line()
{
  if (m_curs_x >= m_suggestion_x && m_curs_y == m_suggestion_y && m_suggestion.GetLength() && m_ui_state == UI_STATE_NORMAL)
  {
    const char *p=m_suggestion.Get();
    char str[512];
    if (WDL_utf8_get_charlen(m_suggestion.Get()) > COLS)
    {
      int l = WDL_utf8_charpos_to_bytepos(m_suggestion.Get(),COLS-4);
      if (l > sizeof(str)-6) l = sizeof(str)-6;
      lstrcpyn(str, m_suggestion.Get(), l+1);
      strcat(str, "...");
      p=str;
    }

    attrset(COLOR_TOPLINE|A_BOLD);
    bkgdset(COLOR_TOPLINE);
    move(0, 0);
    addstr(p);
    clrtoeol();
    attrset(0);
    bkgdset(0);
  }
  else
  {
    m_suggestion_x=m_suggestion_y=-1;
    if (m_suggestion.GetLength()) m_suggestion.Set("");
    WDL_CursesEditor::draw_top_line();
  }
}


void EEL_Editor::onRightClick(HWND hwnd)
{
  WDL_LogicalSortStringKeyedArray<int> flist(m_case_sensitive);
  int i;
  if (!(GetAsyncKeyState(VK_CONTROL)&0x8000))
  {
    m_code_func_cache_lines = -1; // invalidate cache
    ensure_code_func_cache_valid();
    for (i = 0; i < m_code_func_cache.GetSize(); i ++)
    {
      const char *p = m_code_func_cache.Get(i);
      const int line = *(int *)p;
      p += sizeof(int);

      const char *q = p+strlen(p)+1;
      char buf[512];
      snprintf(buf,sizeof(buf),"%s%s",p,q);
      flist.AddUnsorted(buf,line);
      p += 4;
    }
  }
  if (flist.GetSize())
  {
    flist.Resort();
    if (m_case_sensitive) flist.Resort(WDL_LogicalSortStringKeyedArray<int>::cmpistr);

    HMENU hm=CreatePopupMenu();
    int pos=0;
    for (i=0; i < flist.GetSize(); ++i)
    {
      const char* fname=NULL;
      int line=flist.Enumerate(i, &fname);
      InsertMenu(hm, pos++, MF_STRING|MF_BYPOSITION, line+1, fname);
    }
    POINT p;
    GetCursorPos(&p);
    int ret=TrackPopupMenu(hm, TPM_NONOTIFY|TPM_RETURNCMD, p.x, p.y, 0, hwnd, NULL);
    DestroyMenu(hm);
    if (ret > 0)
    {
      GoToLine(ret-1,true);
    }
  }
  else
  {
    doWatchInfo(0);
  }
}

void EEL_Editor::ensure_code_func_cache_valid()
{
  const char *prefix = m_function_prefix;
  if (!prefix || !*prefix) return;

  const DWORD now = GetTickCount();
  if (m_text.GetSize()==m_code_func_cache_lines && (now-m_code_func_cache_time)<5000) return;

  m_code_func_cache_lines = m_text.GetSize();
  m_code_func_cache_time = now;

  m_code_func_cache.Empty(true,free);

  const int prefix_len = (int) strlen(m_function_prefix);
  for (int i=0; i < m_text.GetSize(); ++i)
  {
    WDL_FastString* s=m_text.Get(i);
    if (WDL_NORMALLY(s))
    {
      const char *p = s->Get();
      while (*p)
      {
        if (m_case_sensitive ? !strncmp(p,prefix,prefix_len) : !strnicmp(p,prefix,prefix_len))
        {
          p+=prefix_len;
          while (*p == ' ') p++;
          if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '_')
          {
            const char *q = p+1;
            while ((*q >= '0' && *q <= '9') ||
                   (*q >= 'a' && *q <= 'z') ||
                   (*q >= 'A' && *q <= 'Z') ||
                   *q == ':' || // lua
                   *q == '_' || *q == '.') q++;

            const char *endp = q;
            while (*q == ' ') q++;
            if (*q == '(')
            {
              const char *endq = q;
              while (*endq && *endq != ')') endq++;
              if (*endq) endq++;
              const char *r = endq;
              while (*r == ' ') r++;

              const int p_len = (int) (endp - p);
              const int q_len = (int) (endq - q);
              const int r_len = (int) strlen(r);

              // add function
              char *v = (char *)malloc(sizeof(int) + p_len + q_len + r_len + 3), *wr = v;
              if (WDL_NORMALLY(v))
              {
                *(int *)wr = i;  wr += sizeof(int);
                lstrcpyn_safe(wr,p,p_len+1); wr += p_len+1;
                lstrcpyn_safe(wr,q,q_len+1); wr += q_len+1;
                lstrcpyn_safe(wr,r,r_len+1); wr += r_len+1;

                m_code_func_cache.Add(v);
              }
              p = r; // continue parsing after parentheses
            }
          }
        }
        if (*p) p++;
      }
    }
  }
}


#ifdef WDL_IS_FAKE_CURSES

LRESULT EEL_Editor::onMouseMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_LBUTTONDBLCLK:
      if (m_suggestion_hwnd) DestroyWindow(m_suggestion_hwnd);
      if (CURSES_INSTANCE && CURSES_INSTANCE->m_font_w && CURSES_INSTANCE->m_font_h)
      {
        const int y = ((short)HIWORD(lParam)) / CURSES_INSTANCE->m_font_h - m_top_margin;
        //const int x = ((short)LOWORD(lParam)) / CURSES_INSTANCE->m_font_w + m_offs_x;
        WDL_FastString *fs=m_text.Get(y + m_paneoffs_y[m_curpane]);
        if (fs && y >= 0)
        {
          if (!strncmp(fs->Get(),"import",6) && isspace(fs->Get()[6]))
          {
            open_import_line();
            return 1;
          }
        }

        // doubleclicking a function goes to it
        if (!SHIFT_KEY_DOWN)
        {
          WDL_FastString *l=m_text.Get(m_curs_y);
          if (l)
          {
            const char *p = l->Get(), *endp = p + l->GetLength(), *cursor = p + WDL_utf8_charpos_to_bytepos(p,m_curs_x);
            int state = 0, toklen = 0;
            const char *tok;
            while ((tok=sh_tokenize(&p,endp,&toklen,&state)) && cursor > tok+toklen);

            if (tok && cursor <= tok+toklen)
            {
              ensure_code_func_cache_valid();

              while (toklen > 0)
              {
                for (int i = 0; i < m_code_func_cache.GetSize(); i ++)
                {
                  const char *p = m_code_func_cache.Get(i);
                  int line = *(int *)p;
                  p+=sizeof(int);
                  if (line != m_curs_y && strlen(p) == toklen && (m_case_sensitive ? !strncmp(p,tok,toklen) : !strnicmp(p,tok,toklen)))
                  {
                    GoToLine(line,true);
                    return 0;
                  }
                }

                // try removing any foo. prefixes
                while (toklen > 0 && *tok != '.') { tok++; toklen--; }
                tok++;
                toklen--;
              }
            }
          }
        }
      }
    break;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
      if (m_suggestion_hwnd) DestroyWindow(m_suggestion_hwnd);
    break;

  }
  return WDL_CursesEditor::onMouseMessage(hwnd,uMsg,wParam,lParam);
}
#endif
