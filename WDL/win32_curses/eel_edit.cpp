#ifdef _WIN32
#include <windows.h>
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
#include "../win32_utf8.h"
#include "../wdlcstring.h"
#include "../eel2/ns-eel-int.h"

EEL_Editor::EEL_Editor(void *cursesCtx) : MultiTab_Editor(cursesCtx), m_has_peek(true)
{
  init_pair(3, RGB(0,255,255),COLOR_BLACK); // highlight for known words
  init_pair(4, RGB(0,255,0),COLOR_BLACK); // numbers
  init_pair(5, RGB(96,128,192),COLOR_BLACK); // comments

  init_pair(6, COLOR_WHITE, COLOR_RED);
  init_pair(7, RGB(255,255,0), COLOR_BLACK);  

#ifdef WDL_IS_FAKE_CURSES
  init_pair(8, RGB(255,128,128), COLOR_BLACK); // regvar
  init_pair(9, RGB(0,192,255), COLOR_BLACK); // keyword
  init_pair(10, RGB(255,192,192), COLOR_BLACK); // string
  init_pair(11, RGB(192,255,128), COLOR_BLACK); // stringvar
#endif
}

EEL_Editor::~EEL_Editor()
{
}

//#define START_ON_VARS_KEYWORD
#ifdef START_ON_VARS_KEYWORD
#include "../assocarray.h"
static int s_lasttok_wasfunction;
static WDL_StringKeyedArray<bool> s_declaredVars(false), s_declaredFuncs(false), s_funcDef_vars(false);

static void sh_func_ontoken(const char *tok, int toklen)
{
  // todo: track whether we are in a function definition
  if (s_lasttok_wasfunction && (tok[0] == '_' || isalpha(tok[0])))
  {
    char buf[1024];
    if (toklen > sizeof(buf)-1) toklen=sizeof(buf)-1;
    lstrcpyn_safe(buf,tok,toklen+1);
    s_declaredFuncs.Insert(buf,1);
    s_lasttok_wasfunction=false;
  }
  else
  {
    s_lasttok_wasfunction = toklen == 8 && !strnicmp(tok,"function",8);
  }
}

#else
#define sh_func_ontoken(x,y)
#endif

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


  int x;
  for(x=0;;x++)
  {
    functionType *f = nseel_getFunctionFromTable(x);
    if (!f) break;
    if (f && !strnicmp(tokStart,f->name,len) && (int)strlen(f->name) == len)
    {
      return SYNTAX_FUNC;
    }
  }
#ifdef START_ON_VARS_KEYWORD
  // todo: need to lookahead to see if the next token is (, if so, look at s_declaredFuncs, otherwise s_declaredVars/s_funcDef_vars
  if (state != STATE_BEFORE_CODE && (s_declaredVars.GetSize()||s_declaredFuncs.GetSize()))
  {
    char buf[1024];
    if (len < sizeof(buf))
    {
      lstrcpyn_safe(buf,tokStart,len+1);
      int tl;
      const char *ep = tokStart+len;
      const char *nexttok=sh_tokenize(&ep, ep+strlen(ep), &tl, NULL);
      if (nexttok && nexttok[0] == '(')
      {
        if (!s_declaredFuncs.Get(buf)) return SYNTAX_ERROR;
      }
      else
      {
        if (s_declaredVars.GetSize() && !s_declaredVars.Get(buf)) return SYNTAX_ERROR;
      }
    }
  }
#endif
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

    if (isalpha(c)) 
    {
      return (int) (fmt - fmt_in);
    }

    if (c == '.' || c == '+' || c == '-' || c == ' ' || (c>='0' && c<='9')) 
    {
    }
    else if (c == '{')
    {
      if (*var_offs!=0) return 0; // already specified
      *var_offs = fmt-fmt_in;
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
      *var_len = (fmt-fmt_in) - *var_offs;
      fmt++;
    }
    else
    {
      break;
    }
  }
  return 0;
}


void EEL_Editor::draw_string(int *ml, int *skipcnt, const char *str, int amt, int *attr, int newAttr, int comment_string_state)
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
        const int sz=wdl_min(str_scan-str,amt);
        draw_string_urlchk(ml,skipcnt,str,sz,attr,newAttr);
        str += sz;
        amt -= sz;
      }

      {
        const int sz=(varlen>0) ? wdl_min(varpos,amt) : wdl_min(l,amt);
        if (sz>0) 
        {
          draw_string_internal(ml,skipcnt,str,sz,attr,SYNTAX_HIGHLIGHT2);
          str += sz;
          amt -= sz;
        }
      }

      if (varlen>0) 
      {
        int sz = wdl_min(varlen,amt);
        if (sz>0)
        {
          draw_string_internal(ml,skipcnt,str,sz,attr,*str == '#' ? SYNTAX_STRINGVAR : SYNTAX_HIGHLIGHT1);
          amt -= sz;
          str += sz;
        }

        sz = wdl_min(l - varpos - varlen, amt);
        if (sz>0)
        {
          draw_string_internal(ml,skipcnt,str,sz,attr,SYNTAX_HIGHLIGHT2);
          amt-=sz;
          str+=sz;
        }
      }
    }
  }
  draw_string_urlchk(ml,skipcnt,str,amt,attr,newAttr);
}

void EEL_Editor::draw_string_urlchk(int *ml, int *skipcnt, const char *str, int amt, int *attr, int newAttr)
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
               (strncmp(str_scan,"http://",7) || (sstr != str_scan && isalnum(str_scan[-1]))) &&
               str_scan < str+amt) str_scan++;
        if (!*str_scan || str_scan >= str+amt) break;
        while (str_scan[l] && str_scan[l] != ')' && str_scan[l] != '\"' && str_scan[l] != ')' && str_scan[l] != ' ' && str_scan[l] != '\t') l++;
      }
      if (!*str_scan || str_scan >= str+amt) break; // allow default processing to happen if we reached the end of the string
      
      if (l > amt) l=amt;
      
      if (str_scan > str)
      {
        const int sz=wdl_min(str_scan-str,amt);
        draw_string_internal(ml,skipcnt,str,sz,attr,newAttr);
        str += sz;
        amt -= sz;
      }
      
      const int sz=wdl_min(l,amt);
      if (sz>0)
      {
        draw_string_internal(ml,skipcnt,str,sz,attr,SYNTAX_HIGHLIGHT1);
        str += sz;
        amt -= sz;
      }
    }
  }
  draw_string_internal(ml,skipcnt,str,amt,attr,newAttr);
}
  
void EEL_Editor::draw_string_internal(int *ml, int *skipcnt, const char *str, int amt, int *attr, int newAttr)
{
  if (amt>0)
  {
    if (amt > *ml) amt = *ml;
    *ml -= amt;

    const int sk = *skipcnt;
    if (amt < sk) 
    { 
      *skipcnt=sk - amt; 
    } 
    else
    {
      if (sk>0) 
      {
        str += sk; 
        amt -= sk; 
        *skipcnt=0; 
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


void EEL_Editor::mvaddnstr_highlight(int y, int x, const char *p, int ml, int *c_comment_state, int skipcnt)
{
  int last_attr = A_NORMAL;
  attrset(last_attr);
  move(y, x);
  int rv = do_draw_line(p, ml, c_comment_state, skipcnt, last_attr);
  attrset(rv< 0 ? SYNTAX_ERROR : A_NORMAL);
  if (rv)
  {
    clrtoeol();
    if (rv < 0) attrset(A_NORMAL);
  }
}

int EEL_Editor::do_draw_line(const char *p, int ml, int *c_comment_state, int skipcnt, int last_attr)
{
  if (is_code_start_line(p)) 
  {
    *c_comment_state=0;
    s_draw_parentokenstack.Resize(0,false);
  }

  int ignoreSyntaxState=0;
#ifdef START_ON_VARS_KEYWORD
  if (!strnicmp(p,"var",3) && isspace(p[3]))
  {
    ignoreSyntaxState=-2;
    draw_string(&ml,&skipcnt,p,3,&last_attr,SYNTAX_HIGHLIGHT1);
    p+=3;
  }
  else
#endif
    ignoreSyntaxState = overrideSyntaxDrawingForLine(&ml, &skipcnt, &p, c_comment_state, &last_attr);

  if (ignoreSyntaxState>0)
  {
    draw_string(&ml,&skipcnt,p,strlen(p),&last_attr,ignoreSyntaxState==100 ? SYNTAX_ERROR : 
        ignoreSyntaxState==2 ? SYNTAX_COMMENT : A_NORMAL);
    return ml>0;
  }


  // syntax highlighting
  const char *endptr = p+strlen(p);
  const char *tok;
  const char *lp = p;
  int toklen=0;
  int last_comment_state=*c_comment_state;
  while (NULL != (tok = sh_tokenize(&p,endptr,&toklen,c_comment_state)) || lp < endptr)
  {
    if (last_comment_state>0) // if in a multi-line string or comment
    {
      // draw empty space between lp and p as a string. in this case, tok/toklen includes our string, so we quickly finish after
      draw_string(&ml,&skipcnt,lp,p-lp,&last_attr, last_comment_state==1 ? SYNTAX_COMMENT:SYNTAX_STRING, last_comment_state);
      last_comment_state=0;
      lp = p;
      continue;
    }
    sh_func_ontoken(tok,toklen);

    // draw empty space between lp and tok/endptr as normal
    const char *adv_to = tok ? tok : endptr;
    if (adv_to > lp) draw_string(&ml,&skipcnt,lp,adv_to-lp,&last_attr, A_NORMAL);

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
    else if (isalpha(tok[0]) || tok[0] == '_' || tok[0] == '#')
    {
      int def_attr = A_NORMAL;
      bool isf=true;
      if (tok[0] == '#')
      {
        def_attr = SYNTAX_STRINGVAR;
        draw_string(&ml,&skipcnt,tok,1,&last_attr,def_attr);
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
          const int attr=isf?namedTokenHighlight(tok,this_len,*c_comment_state):def_attr;
          draw_string(&ml,&skipcnt,tok,this_len,&last_attr,attr==A_NORMAL?def_attr:attr);
          tok += this_len;
          toklen -= this_len;
        }
        if (toklen > 0)
        {
          draw_string(&ml,&skipcnt,tok,1,&last_attr,SYNTAX_HIGHLIGHT1);
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
        err_left=1;
    }

    if (ignoreSyntaxState) err_left = err_right = 0;

    if (err_left > 0) 
    {
      if (err_left > toklen) err_left=toklen;
      draw_string(&ml,&skipcnt,tok,err_left,&last_attr,SYNTAX_ERROR);
      tok+=err_left;
      toklen -= err_left;
    }
    if (err_right > toklen) err_right=toklen;

    draw_string(&ml, &skipcnt, tok, toklen - err_right, &last_attr, attr, start_of_tok);

    if (err_right > 0)
      draw_string(&ml,&skipcnt,tok+toklen-err_right,err_right,&last_attr,SYNTAX_ERROR);

    if (ignoreSyntaxState == -1 && tok[0] == '>')
    {
      draw_string(&ml,&skipcnt,p,strlen(p),&last_attr,ignoreSyntaxState==2 ? SYNTAX_COMMENT : A_NORMAL);
      break;
    }
  }
  return ml > 0;
}

int EEL_Editor::GetCommentStateForLineStart(int line)
{
  
#ifdef START_ON_VARS_KEYWORD
  s_declaredFuncs.DeleteAll();
  s_declaredVars.DeleteAll();
#endif
  m_indent_size=2;
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

      #ifdef START_ON_VARS_KEYWORD
      if (!strnicmp(p,"var",3) && isspace(p[3]))
      {
        const char *endp=p + t->GetLength();
        const char *tok;
        int toklen;
        p+=4;
        while (NULL != (tok = sh_tokenize(&p,endp,&toklen,NULL)))
        {
          if (isalpha(tok[0]) || tok[0] == '_' || tok[0] == '#')
          {
            char buf[512];
            if (toklen > sizeof(buf)-1) toklen=sizeof(buf)-1;
            lstrcpyn_safe(buf,tok,toklen+1);
            s_declaredVars.AddUnsorted(buf,1);
          }
        }
      }
      #endif
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

  #ifdef START_ON_VARS_KEYWORD
  s_declaredVars.Resort();
  #endif
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
  if (txt)
  {
    const char *special_start = txt + spos;
    const char *special_end = txt + spos + slen;
#ifdef START_ON_VARS_KEYWORD
    if (!strnicmp(txt,"var",3) && isspace(txt[3])) return true;
#endif
    if (*txt == '@') return true;
    while (*txt)
    {
      const char c = txt[0];
      if (c == '\"' || c == '\'') return true;
      if (c == '*' && txt[1] == '/') return true;
      if (c == '/' && txt[1] == '*') return true;
      if (txt >= special_start && txt < special_end)
      {
        if (c == '(' || c == '[' || c == ')' || c == ']' || c == ':' || c == ';' || c == '?') return true;
      }
#ifdef START_ON_VARS_KEYWORD
      if (!strnicmp(txt,"function",8)) return true;
#endif
      txt++;
    }
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
        if (sz) toklist->Get()[sz-1].add_linecnt((tok ? p:endp) - start_p);
      }
      else
      {
        if (tok) switch (tok[0])
        {
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
              eel_sh_token t(l,tok-start_p,toklen,tok[0]);
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
    if (m_curs_x >= curstr->GetLength()) m_curs_x=curstr->GetLength()-1;
    if (m_curs_x<0) m_curs_x=0;

    int new_x,new_y;
    if (eel_sh_get_matching_pos_for_pos(&m_text, m_curs_x,m_curs_y,&new_x,&new_y,&errmsg,this))
    {
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


void EEL_Editor::doWatchInfo(int c)
{
  if (!m_has_peek) return;

    // determine the word we are on, check its value in the effect
  char sstr[512];
  lstrcpyn_safe(sstr,"Use this on a valid symbol name", sizeof(sstr));
  WDL_FastString *t=m_text.Get(m_curs_y);
  char curChar=0;
  if (t)
  {
    const char *p=t->Get();
    if (m_curs_x >= 0 && m_curs_x < t->GetLength()) curChar = p[m_curs_x];
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
            if (x == miny) sx=max(minx,0);
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
        Watch_Code(code.Get(),sstr,sizeof(sstr));
      }
      // compile+execute code within () as debug_watch_value = ( code )
      // show value (or err msg)
    }
    else if (curChar && (isalnum(curChar) || curChar == '_' || curChar == '.' || curChar == '#')) 
    {
      const char *lp=p+m_curs_x;
      const char *rp=p+m_curs_x+1;
      while (lp >= p && (isalnum(*lp) || *lp == '_' || *lp == '.')) lp--;
      if (lp < p || *lp != '#') lp++;
      while (*rp && (isalnum(*rp) || *rp == '_' || *rp == '.')) rp++;

      if (Watch_OnContextHelp(lp,rp,c, sstr,sizeof(sstr))) return;

    }
  }
  if (curChar && Watch_OnContextHelp(&curChar,&curChar,c,sstr,sizeof(sstr))) return;

  if (sstr[0]) draw_message(sstr);
  setCursor();
}


void EEL_Editor::draw_bottom_line()
{
#define BOLD(x) { attrset(m_color_bottomline|A_BOLD); addstr(x); attrset(m_color_bottomline&~A_BOLD); }
  BOLD(" S"); addstr("ave");
  if (m_has_peek)
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

int EEL_Editor::onChar(int c)
{
  if (!m_state && !SHIFT_KEY_DOWN && !ALT_KEY_DOWN) switch (c)
  {
  case KEY_F1:
  case 'K'-'A'+1:
    doWatchInfo(c);
  return 0;
  case 'S'-'A'+1:
     if(updateFile())
     {
       draw_message("Error writing file, changes not saved!");
     }
     setCursor();
  return 0;

  case 'I'-'A'+1:
    if (!m_selecting)
    {
      WDL_FastString *txtstr=m_text.Get(m_curs_y);
      const char *txt=txtstr?txtstr->Get():NULL;
      if (txt && !strncmp(txt,"import",6) && isspace(txt[6]))
      {
        char fnp[512];
        {
          const char *p=txt+6;
          while (*p == ' ') p++;
          lstrcpyn_safe(fnp,p,sizeof(fnp));
        }
        //remove trailing whitespace
        {
          char *ep = fnp;
          while (*ep) ep++;
          while (ep > fnp && (ep[-1] == '\n' || ep[-1] == '\t' || ep[-1] == ' ')) *--ep = 0;
        }
        if (!fnp[0]) return 0; // no filename

        MultiTab_Editor::OpenFileInTab(fnp);
      }
    }
  return 0;
  case KEY_F4:
  case 'T'-'A'+1:
    doParenMatching();
  return 0;
  }

  return MultiTab_Editor::onChar(c);
}


void EEL_Editor::onRightClick(HWND hwnd)
{
  doWatchInfo(0);
}



LRESULT EEL_Editor::onMouseMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_LBUTTONDBLCLK:
      if (CURSES_INSTANCE && CURSES_INSTANCE->m_font_w && CURSES_INSTANCE->m_font_h)
      {
        const int y = ((short)HIWORD(lParam)) / CURSES_INSTANCE->m_font_h - m_top_margin;
        const int x = ((short)LOWORD(lParam)) / CURSES_INSTANCE->m_font_w + m_offs_x;
        WDL_FastString *fs=m_text.Get(y + m_paneoffs_y[m_curpane]);
        if (fs && y >= 0)
        {
          if (!strncmp(fs->Get(),"import",6) && isspace(fs->Get()[6]))
          {
            onChar('I'-'A'+1); // open imported file
            return 1;
          }
        }
      }

    break;

  }
  return MultiTab_Editor::onMouseMessage(hwnd,uMsg,wParam,lParam);
}

void EEL_Editor::Watch_Code(const char *code, char *sstr, size_t sstr_size)
{
  lstrcpyn_safe(sstr,"not implemented",sstr_size);
}
bool EEL_Editor::Watch_OnContextHelp(const char *lp, const char *rp, int c, char *sstr, size_t sstr_size)
{
  lstrcpyn_safe(sstr,"not implemented",sstr_size);
  return false;
}
