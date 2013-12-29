#ifndef __EEL__STRINGS_H__
#define __EEL__STRINGS_H__

#include "ns-eel-int.h"
#include "../wdlcstring.h"
#include "../wdlstring.h"

// required for context
// #define EEL_STRING_GET_FOR_INDEX(x, wr) ((classname *)opaque)->GetStringForIndex(x, wr)
// #define EEL_STRING_ADDTOTABLE(x)  ((classname *)opaque)->AddString(x.Get())
// optional - #define EEL_STRING_GETFMTVAR(x) ((classname *)opaque)->GetVarForFormat(x)
// optional - #define EEL_STRING_GETNAMEDVAR(x,createOK) ((classname *)opaque)->GetNamedVar(x,createOK)


/*
  // writeable user-strings are 0..1023 (EEL_STRING_MAX_USER_STRINGS-1), and can be up to about EEL_STRING_MAXUSERSTRING_LENGTH_HINT bytes long

   printf("string %d blah");             -- output to log, allows %d %u %f etc, if host implements formats [1]
   sprintf(str,"string %d blah");        -- output to str [1]
   strlen(str);                          -- returns string length
   match("*test*", "this is a test")     -- search for first parameter regex-style in second parameter
   matchi("*test*", "this is a test")    -- search for first parameter regex-style in second parameter (case insensitive)
          // %s means 1 or more chars
          // %0s means 0 or more chars
          // %5s means exactly 5 chars
          // %5-s means 5 or more chars
          // %-10s means 1-10 chars
          // %3-5s means 3-5 chars. 
          // %0-5s means 0-5 chars. 

   strcpy(str, srcstr);                  -- replaces str with srcstr
   strcat(str, srcstr);                  -- appends srcstr to str 
   strcmp(str, str2)                     -- compares strings
   stricmp(str, str2)                    -- compares strings (ignoring case)
   strncmp(str, str2, maxlen)            -- compares strings up to maxlen bytes
   strnicmp(str, str2, maxlen)           -- compares strings (ignoring case) up to maxlen bytes
   strncpy(str, srcstr, maxlen);         -- replaces str with srcstr, up to maxlen (-1 for unlimited)
   strncat(str, srcstr, maxlen);         -- appends up to maxlen of srcstr to str (-1 for unlimited)
   strcpy_from(str,srcstr, offset);      -- copies srcstr to str, but starts reading srcstr at offset offset
   str_getchar(str, offset);             -- returns value at offset offset
   str_setchar(str, offset, value);      -- sets value at offset offset
   str_setlen(str, len);                 -- sets length of string (if increasing, will be space-padded)
   str_delsub(str, pos, len);            -- deletes len chars at pos
   str_insert(str, srcstr, pos);         -- inserts srcstr at pos

  [1]: note: printf/sprintf are NOT binary safe when using %s with modifiers (such as %100s etc) -- the source string being formatted
             will terminate at the first NULL character

also recommended, for the PHP fans:

  m_builtin_code = NSEEL_code_compile_ex(m_vm,

   "function strcpy_substr(dest, src, offset, maxlen) ("
   "  offset < 0 ? offset=strlen(src)+offset;"
   "  maxlen < 0 ? maxlen = strlen(src) - offset + maxlen;"
   "  strcpy_from(dest, src, offset);"
   "  maxlen > 0 && strlen(dest) > maxlen ? str_setlen(dest,maxlen);"
   ");\n"


  ,0,NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS

  );



  */


#ifndef EEL_STRING_MAXUSERSTRING_LENGTH_HINT
#define EEL_STRING_MAXUSERSTRING_LENGTH_HINT 16384
#endif

#ifndef EEL_STRING_MAX_USER_STRINGS
#define EEL_STRING_MAX_USER_STRINGS 1024
#endif

#ifndef EEL_STRING_STORAGECLASS 
#define EEL_STRING_STORAGECLASS WDL_FastString
#endif

static int eel_validate_format_specifier(const char *fmt_in, char *typeOut, 
                                         char *fmtOut, int fmtOut_sz, 
                                         char *varOut, int varOut_sz, 
                                         int *varOut_used
                                         )
{
  const char *fmt = fmt_in+1;
  int state=0;
  if (fmt_in[0] != '%') return 0; // ugh passed a non-specifier

  *varOut_used = 0;
  *varOut = 0;

  if (fmtOut_sz-- < 2) return 0;
  *fmtOut++ = '%';

  while (*fmt)
  {
    const char c = *fmt++;
    if (fmtOut_sz < 2) return 0;

    if (c == 'f'|| c=='e' || c=='E' || c=='g' || c=='G' || c == 'd' || c == 'u' || 
        c == 'x' || c == 'X' || c == 'c' || c =='s' || c=='S') 
    {
      *typeOut = c;
      fmtOut[0] = c;
      fmtOut[1] = 0;
      return (int) (fmt - fmt_in);
    }
    else if (c == '.') 
    {
      *fmtOut++ = c; fmtOut_sz--;
      if (state&(2|64)) break;
      state |= 2;
    }
    else if (c == '+') 
    {
      *fmtOut++ = c; fmtOut_sz--;
      if (state&(64|32|16|8|4)) break;
      state |= 8;
    }
    else if (c == '-') 
    {
      *fmtOut++ = c; fmtOut_sz--;
      if (state&(64|32|16|8|4)) break;
      state |= 16;
    }
    else if (c == ' ') 
    {
      *fmtOut++ = c; fmtOut_sz--;
      if (state&(64|32|16|8|4)) break;
      state |= 32;
    }
    else if (c >= '0' && c <= '9') 
    {
      *fmtOut++ = c; fmtOut_sz--;
      state|=4;
    }
    else if (c == '{')
    {
      if (state & 64) break;
      state|=64;
      if (*fmt == '.' || (*fmt >= '0' && *fmt <= '9')) return 0; // symbol name can't start with 0-9 or .

      while (*fmt != '}')
      {
        if ((*fmt >= 'a' && *fmt <= 'z') ||
            (*fmt >= 'A' && *fmt <= 'Z') ||
            (*fmt >= '0' && *fmt <= '9') ||
            *fmt == '_' || *fmt == '.')
        {
          if (varOut_sz < 2) return 0;
          *varOut++ = *fmt++;
          varOut_sz -- ;
        }
        else
        {
          return 0; // bad character in variable name
        }
      }
      fmt++;
      *varOut = 0;
      *varOut_used=1;
    }
    else
    {
      break;
    }
  }
  return 0;

}

static int eel_format_strings(void *opaque, const char *fmt, char *buf, int buf_sz)
{
  int rv=1;
  int fmt_parmpos = 0;
  char *op = buf;
  while (*fmt && op < buf+buf_sz-128)
  {
    if (fmt[0] == '%' && fmt[1] == '%') 
    {
      *op++ = '%';
      fmt+=2;
    }
    else if (fmt[0] == '%')
    {
      char ct=0;
      char fs[128];
      char varname[128];
      int varname_used=0;
      const int l=eel_validate_format_specifier(fmt,&ct,fs,sizeof(fs),varname,sizeof(varname),&varname_used);
      if (!l || !ct) 
      {
        *op=0;
        return -1;
      }

      const EEL_F *varptr = NULL;
      if (varname_used)
      {
#ifdef EEL_STRING_GETNAMEDVAR
        if (varname[0]) varptr=EEL_STRING_GETNAMEDVAR(varname,0);
#endif
      }
      else
      {
#ifdef EEL_STRING_GETFMTVAR
        varptr = EEL_STRING_GETFMTVAR(fmt_parmpos++);
#endif
      }
      const double v = varptr ? (double)*varptr : 0.0;

      if (ct == 's' || ct=='S')
      {
        WDL_FastString *wr=NULL;
        const char *str = EEL_STRING_GET_FOR_INDEX(v,&wr);
        const int maxl=(buf+buf_sz - 2 - op);
        if (wr && !fs[2]) // %s or %S -- todo: implement padding modes for binary compat too?
        {
          int l = wr->GetLength();
          if (l > maxl) l=maxl;
          memcpy(op,wr->Get(),l);
          op += l;
          *op=NULL;
        }
        else
        {
          snprintf(op,maxl,fs,str ? str : "");
        }
      }
      else if (ct == 'x' || ct == 'X' || ct == 'd' || ct == 'u')
      {
        snprintf(op,64,fs,(int) (v));
      }
      else if (ct == 'c')
      {
        *op++=(char) (int)v;
        *op=0;
      }
      else
        snprintf(op,64,fs,v);

      while (*op) op++;

      fmt += l;
    }
    else 
    {
      *op++ = *fmt++;
    }

  }
  *op=0;
  return op - buf;
}



static int eel_string_match(void *opaque, const char *fmt, const char *msg, int match_fmt_pos, int ignorecase, const char *fmt_endptr, const char *msg_endptr)
{
  // check for match, updating EEL_STRING_GETFMTVAR(*) as necessary
  // %d=12345
  // %f=12345[.678]
  // %c=any nonzero char, ascii value
  // %x=12354ab
  // %*, %?, %+, %% literals
  // * ? +  match minimal groups of 0+,1, or 1+ chars
  for (;;)
  {
    if (fmt>=fmt_endptr)
    {
      if (msg>=msg_endptr) return 1;
      return 0; // format ends before matching string
    }

    // if string ends and format is not on a wildcard, early-out to 0
    if (msg>=msg_endptr && *fmt != '*') return 0;

    switch (*fmt)
    {
      case '*':
      case '+':
        // if last char of search pattern, we're done!
        if (fmt+1>=fmt_endptr || (fmt[1] == '?' && fmt+2>=fmt_endptr)) return *fmt == '*' || msg<msg_endptr;

        if (fmt[0] == '+')  msg++; // skip a character for + . Note that in this case msg[1] is valid, because of the !*msg && *fmt != '*' check above

        fmt++;
        if (*fmt == '?')
        {
          // *? or +? are lazy matches
          fmt++;

          while (msg<msg_endptr && !eel_string_match(opaque,fmt, msg,match_fmt_pos,ignorecase,fmt_endptr, msg_endptr)) msg++;
          return msg<msg_endptr;
        }
        else
        {
          // greedy match
          int len = msg_endptr-msg;
          while (len >= 0 && !eel_string_match(opaque,fmt, msg+len,match_fmt_pos,ignorecase,fmt_endptr, msg_endptr)) len--;
          return len >= 0;
        }
      break;
      case '?':
        fmt++;
        msg++;
      break;
      case '%':
        {
          fmt++;
          unsigned short fmt_minlen = 1, fmt_maxlen = 0;
          if (*fmt >= '0' && *fmt <= '9')
          {
            fmt_minlen = *fmt++ - '0';
            while (*fmt >= '0' && *fmt <= '9') fmt_minlen = fmt_minlen * 10 + (*fmt++ - '0');
            fmt_maxlen = fmt_minlen;
          }
          if (*fmt == '-')
          {
            fmt++;
            fmt_maxlen = 0;
            while (*fmt >= '0' && *fmt <= '9') fmt_maxlen = fmt_maxlen * 10 + (*fmt++ - '0');
          }
          const char *dest_varname=NULL;
          if (*fmt == '{')
          {
            dest_varname=++fmt;
            while (*fmt && fmt < fmt_endptr && *fmt != '}') fmt++;
            if (fmt >= fmt_endptr-1 || *fmt != '}') return 0; // malformed %{var}s
            fmt++; // skip '}'
          }

          const char fmt_char = *fmt++;
          if (!fmt_char) return 0; // malformed

          if (fmt_char == '*' || 
              fmt_char == '?' || 
              fmt_char == '+' || 
              fmt_char == '%')
          {
            if (*msg++ != fmt_char) return 0;
          }
          else if (fmt_char == 'c')
          {
            EEL_F *varOut = NULL;
            if (!dest_varname) 
            {
#ifdef EEL_STRING_GETFMTVAR
              varOut = EEL_STRING_GETFMTVAR(match_fmt_pos++);
#endif
            }
            else
            {
#ifdef EEL_STRING_GETNAMEDVAR 
              char tmp[128];
              int idx=0;
              while (dest_varname < fmt_endptr && *dest_varname && *dest_varname != '}' && idx<sizeof(tmp)-1) tmp[idx++] = *dest_varname++;
              tmp[idx]=0;
              if (idx>0) varOut = EEL_STRING_GETNAMEDVAR(tmp,1);
#endif
            }
            if (msg >= msg_endptr) return 0; // out of chars

            const unsigned char c =  *(unsigned char *)msg++;
            if (varOut) *varOut = (EEL_F)c;
          }
          else 
          {
            int len=0;
            if (fmt_char == 's')
            {
              len = msg_endptr-msg;
            }
            else if (fmt_char == 'x' || fmt_char == 'X')
            {
              while ((msg[len] >= '0' && msg[len] <= '9') ||
                     (msg[len] >= 'A' && msg[len] <= 'F') ||
                     (msg[len] >= 'a' && msg[len] <= 'f')) len++;
            }
            else if (fmt_char == 'f')
            {
              while (msg[len] >= '0' && msg[len] <= '9') len++;
              if (msg[len] == '.') 
              { 
                len++; 
                while (msg[len] >= '0' && msg[len] <= '9') len++;
              }
            }
            else if (fmt_char == 'd' || fmt_char == 'u')
            {
              while (msg[len] >= '0' && msg[len] <= '9') len++;
            }
            else 
            {
              // bad format
              return 0;
            }

            if (fmt_maxlen>0 && len > fmt_maxlen) len = fmt_maxlen;

            if (!dest_varname) match_fmt_pos++;

            while (len >= fmt_minlen && !eel_string_match(opaque,fmt, msg+len,match_fmt_pos,ignorecase,fmt_endptr, msg_endptr)) len--;
            if (len < fmt_minlen) return 0;

            EEL_F *varOut = NULL;
            if (!dest_varname) 
            {
#ifdef EEL_STRING_GETFMTVAR
              varOut = EEL_STRING_GETFMTVAR(match_fmt_pos-1);
#endif
            }
            else
            {
#ifdef EEL_STRING_GETNAMEDVAR 
              char tmp[128];
              int idx=0;
              while (dest_varname < fmt_endptr && *dest_varname  && *dest_varname != '}' && idx<sizeof(tmp)-1) tmp[idx++] = *dest_varname++;
              tmp[idx]=0;
              if (idx>0) varOut = EEL_STRING_GETNAMEDVAR(tmp,1);
#endif
            }
            if (varOut)
            {
              if (fmt_char == 's')
              {
                EEL_STRING_STORAGECLASS *wr=NULL;
                EEL_STRING_GET_FOR_INDEX(*varOut, &wr);
                if (wr)
                {
                  if (msg_endptr >= wr->Get() && msg_endptr <= wr->Get() + wr->GetLength())
                  {
#ifdef EEL_STRING_DEBUGOUT
                    EEL_STRING_DEBUGOUT("match: destination specifier passed is also haystack, will not update");
#endif
                  }
                  else if (fmt_endptr >= wr->Get() && fmt_endptr <= wr->Get() + wr->GetLength())
                  {
#ifdef EEL_STRING_DEBUGOUT
                    EEL_STRING_DEBUGOUT("match: destination specifier passed is also format, will not update");
#endif
                  }
                  else 
                  {
                    wr->SetRaw(msg,len);
                  }
                }
                else
                {
#ifdef EEL_STRING_DEBUGOUT
                   EEL_STRING_DEBUGOUT("match: bad destination specifier passed as %d: %f",match_fmt_pos,*varOut);
#endif
                }
              }
              else
              {
                char tmp[128];
                lstrcpyn_safe(tmp,msg,min(len+1,sizeof(tmp)));
                char *bl=(char*)msg;
                if (fmt_char == 'u')
                  *varOut = (EEL_F)strtoul(tmp,&bl,10);
                else if (fmt_char == 'x' || fmt_char == 'X')
                  *varOut = (EEL_F)strtoul(msg,&bl,16);
                else
                  *varOut = (EEL_F)atof(tmp);
              }
            }
            return 1;
          }
        }
      break;
      default:
        if (ignorecase ? (toupper(*fmt) != toupper(*msg)) : (*fmt!= *msg)) return 0;
        fmt++;
        msg++;
      break;
    }
  }
}



static EEL_F NSEEL_CGEN_CALL _eel_sprintf(void *opaque, EEL_F *strOut, EEL_F *fmt_index)
{
  if (opaque)
  {
    EEL_STRING_STORAGECLASS *wr=NULL;
    EEL_STRING_GET_FOR_INDEX(*strOut, &wr);
    if (!wr)
    {
#ifdef EEL_STRING_DEBUGOUT
      EEL_STRING_DEBUGOUT("sprintf: bad destination specifier passed %f",*strOut);
#endif
    }
    else
    {
      const char *fmt = EEL_STRING_GET_FOR_INDEX(*fmt_index,NULL);
      if (fmt)
      {
        char buf[16384];
        const int fmt_len = eel_format_strings(opaque,fmt,buf,sizeof(buf));

        if (fmt_len>=0)
        {
          wr->SetRaw(buf,fmt_len);
        }
        else
        {
#ifdef EEL_STRING_DEBUGOUT
          EEL_STRING_DEBUGOUT("sprintf: bad format string %s",fmt);
#endif
        }
      }
      else
      {
#ifdef EEL_STRING_DEBUGOUT
        EEL_STRING_DEBUGOUT("sprintf: bad format specifier passed %f",*fmt_index);
#endif
      }
    }
  }
  return *strOut;
}


static EEL_F NSEEL_CGEN_CALL _eel_strncat(void *opaque, EEL_F *strOut, EEL_F *fmt_index, EEL_F *maxlen)
{
  if (opaque)
  {
    EEL_STRING_STORAGECLASS *wr=NULL, *wr_src=NULL;
    EEL_STRING_GET_FOR_INDEX(*strOut, &wr);
    if (!wr)
    {
#ifdef EEL_STRING_DEBUGOUT
      EEL_STRING_DEBUGOUT("str%scat: bad destination specifier passed %f",maxlen ? "n":"",*strOut);
#endif
    }
    else
    {
      const char *fmt = EEL_STRING_GET_FOR_INDEX(*fmt_index,&wr_src);
      if (fmt||wr_src)
      {
        if (wr->GetLength() > EEL_STRING_MAXUSERSTRING_LENGTH_HINT)
        {
#ifdef EEL_STRING_DEBUGOUT
          EEL_STRING_DEBUGOUT("str%scat: will not grow string since it is already %d bytes",maxlen ? "n":"",wr->GetLength());
#endif
        }
        else
        {
          int ml=0;
          if (maxlen && *maxlen > 0) ml = (int)*maxlen;
          if (wr_src)
          {
            EEL_STRING_STORAGECLASS tmp;
            if (wr_src == wr) *(wr_src=&tmp) = *wr;
            wr->AppendRaw(wr_src->Get(), ml > 0 && ml < wr_src->GetLength() ? ml : wr_src->GetLength());
          }
          else
            wr->Append(fmt, ml);
        }
      }
      else
      {
#ifdef EEL_STRING_DEBUGOUT
        EEL_STRING_DEBUGOUT("str%scat: bad format specifier passed %f",maxlen ? "n":"",*fmt_index);
#endif
      }
    }
  }
  return *strOut;
}

static EEL_F NSEEL_CGEN_CALL _eel_strcpyfrom(void *opaque, EEL_F *strOut, EEL_F *fmt_index, EEL_F *offs)
{
  if (opaque)
  {
    EEL_STRING_STORAGECLASS *wr=NULL, *wr_src=NULL;
    EEL_STRING_GET_FOR_INDEX(*strOut, &wr);
    if (!wr)
    {
#ifdef EEL_STRING_DEBUGOUT
      EEL_STRING_DEBUGOUT("strcpy_from: bad destination specifier passed %f",*strOut);
#endif
    }
    else
    {
      const char *fmt = EEL_STRING_GET_FOR_INDEX(*fmt_index,&wr_src);
      if (fmt||wr_src)
      {
        const int fmt_len  = wr_src ? wr_src->GetLength() : (int) strlen(fmt);
        int o = (int) *offs;
        if (o < 0) o=0;
        if (o >= fmt_len) wr->Set("");
        else 
        {
          if (wr_src) 
          {
            if (wr_src == wr)  
              wr->DeleteSub(0,o);
            else
              wr->SetRaw(wr_src->Get() + o, fmt_len - o);
          }
          else wr->Set(fmt+o);
        }
      }
      else
      {
#ifdef EEL_STRING_DEBUGOUT
        EEL_STRING_DEBUGOUT("strcpy_from: bad format specifier passed %f",*fmt_index);
#endif
      }
    }
  }
  return *strOut;
}


static EEL_F NSEEL_CGEN_CALL _eel_strncpy(void *opaque, EEL_F *strOut, EEL_F *fmt_index, EEL_F *maxlen)
{
  if (opaque)
  {
    EEL_STRING_STORAGECLASS *wr=NULL, *wr_src=NULL;
    EEL_STRING_GET_FOR_INDEX(*strOut, &wr);
    if (!wr)
    {
#ifdef EEL_STRING_DEBUGOUT
      EEL_STRING_DEBUGOUT("str%scpy: bad destination specifier passed %f",maxlen ? "n":"",*strOut);
#endif
    }
    else
    {
      const char *fmt = EEL_STRING_GET_FOR_INDEX(*fmt_index,&wr_src);
      if (fmt||wr_src)
      {
        int ml=-1;
        if (maxlen && *maxlen >= 0) ml = (int)*maxlen;

        if (wr_src == wr) 
        {
          if (ml>=0 && ml < wr->GetLength()) wr->SetLen(ml); // shorten string if strncpy(x,x,len) and len >=0
          return *strOut; 
        }

        if (wr_src) wr->SetRaw(wr_src->Get(), ml>0 && ml < wr_src->GetLength() ? ml : wr_src->GetLength());
        else wr->Set(fmt,ml);
      }
      else
      {
#ifdef EEL_STRING_DEBUGOUT
        EEL_STRING_DEBUGOUT("str%scpy: bad format specifier passed %f",maxlen ? "n":"",*fmt_index);
#endif
      }
    }
  }
  return *strOut;
}

static EEL_F _eel_strcmp_int(const char *a, int a_len, const char *b, int b_len, int ml, bool ignorecase)
{
  // binary-safe comparison (at least if a_len>=0 etc)
  int pos = 0;
  for (;;)
  {
    if (ml > 0 && pos == ml) return 0.0;
    const bool a_end = a_len >= 0 ? pos == a_len : !a[pos];
    const bool b_end = b_len >= 0 ? pos == b_len : !b[pos];
    if (a_end || b_end)
    {
      if (!b_end) return -1.0; // b[pos] is nonzero, a[pos] is zero
      if (!a_end) return 1.0;  
      return 0.0;
    }
    char av = a[pos];
    char bv = b[pos];
    if (ignorecase) 
    {
      av=toupper(av);
      bv=toupper(bv);
    }
    if (bv > av) return -1.0;
    if (av > bv) return 1.0;

    pos++;
  }
}

static EEL_F NSEEL_CGEN_CALL _eel_strncmp(void *opaque, EEL_F *aa, EEL_F *bb, EEL_F *maxlen)
{
  if (opaque)
  {
    EEL_STRING_STORAGECLASS *wr_a=NULL,*wr_b=NULL;
    const char *a = EEL_STRING_GET_FOR_INDEX(*aa,&wr_a);
    const char *b = EEL_STRING_GET_FOR_INDEX(*bb,&wr_b);
    if (!a || !b)
    {
#ifdef EEL_STRING_DEBUGOUT
      EEL_STRING_DEBUGOUT("str%scmp: bad specifier(s) passed %f/%f",maxlen ? "n" : "",*aa,*bb);
#endif
    }
    else
    {
      const int ml = maxlen ? (int) *maxlen : -1;
      if (!ml || a==b) return 0; // strncmp(x,y,0) == 0

      return _eel_strcmp_int(a,wr_a ? wr_a->GetLength() : -1,b,wr_b ? wr_b->GetLength() : -1, ml, false);
    }
  }
  return -1.0;
}

static EEL_F NSEEL_CGEN_CALL _eel_strnicmp(void *opaque, EEL_F *aa, EEL_F *bb, EEL_F *maxlen)
{
  if (opaque)
  {
    EEL_STRING_STORAGECLASS *wr_a=NULL,*wr_b=NULL;
    const char *a = EEL_STRING_GET_FOR_INDEX(*aa,&wr_a);
    const char *b = EEL_STRING_GET_FOR_INDEX(*bb,&wr_b);
    if (!a || !b)
    {
#ifdef EEL_STRING_DEBUGOUT
      EEL_STRING_DEBUGOUT("str%sicmp: bad specifier(s) passed %f/%f",maxlen ? "n" : "",*aa,*bb);
#endif
    }
    else
    {
      const int ml = maxlen ? (int) *maxlen : -1;
      if (!ml || a==b) return 0; // strncmp(x,y,0) == 0
      return _eel_strcmp_int(a,wr_a ? wr_a->GetLength() : -1,b,wr_b ? wr_b->GetLength() : -1, ml, true);
    }
  }
  return -1.0;
}


static EEL_F NSEEL_CGEN_CALL _eel_strcat(void *opaque, EEL_F *strOut, EEL_F *fmt_index)
{
  return _eel_strncat(opaque,strOut,fmt_index,NULL);
}

static EEL_F NSEEL_CGEN_CALL _eel_strcpy(void *opaque, EEL_F *strOut, EEL_F *fmt_index)
{
  return _eel_strncpy(opaque,strOut,fmt_index,NULL);
}


static EEL_F NSEEL_CGEN_CALL _eel_strcmp(void *opaque, EEL_F *strOut, EEL_F *fmt_index)
{
  return _eel_strncmp(opaque,strOut,fmt_index,NULL);
}

static EEL_F NSEEL_CGEN_CALL _eel_stricmp(void *opaque, EEL_F *strOut, EEL_F *fmt_index)
{
  return _eel_strnicmp(opaque,strOut,fmt_index,NULL);
}


static EEL_F NSEEL_CGEN_CALL _eel_strgetchar(void *opaque, EEL_F *strOut, EEL_F *idx)
{
  if (opaque)
  {
    EEL_STRING_STORAGECLASS *wr=NULL;
    const char *fmt = EEL_STRING_GET_FOR_INDEX(*strOut, &wr);
    if (!wr && !fmt)
    {
#ifdef EEL_STRING_DEBUGOUT
      EEL_STRING_DEBUGOUT("str_getchar: bad destination specifier passed %f",*strOut);
#endif
    }
    else
    {
      int l = (int) *idx;
      if (l >= 0)
      {
        if (wr)
        {
          if (l < wr->GetLength()) return ((unsigned char *)wr->Get())[l];
        }
        else
        {
          if (l < (int)strlen(fmt)) return ((unsigned char *)fmt)[l];
        }
      }
    }
  }
  return 0;
}

static EEL_F NSEEL_CGEN_CALL _eel_strsetchar(void *opaque, EEL_F *strOut, EEL_F *idx, EEL_F *val)
{
  if (opaque)
  {
    EEL_STRING_STORAGECLASS *wr=NULL;
    EEL_STRING_GET_FOR_INDEX(*strOut, &wr);
    if (!wr)
    {
#ifdef EEL_STRING_DEBUGOUT
      EEL_STRING_DEBUGOUT("str_setchar: bad destination specifier passed %f",*strOut);
#endif
    }
    else
    {
      const int l = (int) *idx;
      if (l >= 0 && l < wr->GetLength()) 
      {
        ((unsigned char *)wr->Get())[l]=((int)*val)&255; // allow putting nulls in string, strlen() will still get the full size
      }
    }
  }
  return *strOut;
}

static EEL_F NSEEL_CGEN_CALL _eel_strinsert(void *opaque, EEL_F *strOut, EEL_F *fmt_index, EEL_F *pos)
{
  if (opaque)
  {
    EEL_STRING_STORAGECLASS *wr=NULL, *wr_src=NULL;
    EEL_STRING_GET_FOR_INDEX(*strOut, &wr);
    if (!wr)
    {
#ifdef EEL_STRING_DEBUGOUT
      EEL_STRING_DEBUGOUT("str_insert: bad destination specifier passed %f",*strOut);
#endif
    }
    else
    {
      const char *fmt = EEL_STRING_GET_FOR_INDEX(*fmt_index,&wr_src);
      if (fmt||wr_src)
      {
        EEL_STRING_STORAGECLASS tmp;
        if (wr_src == wr) *(wr_src=&tmp) = *wr; // insert from copy

        // if wr_src, fmt is guaranteed to be wr_src.Get()
        int p = (int)*pos;
        int insert_l = wr_src ? wr_src->GetLength() : (int)strlen(fmt);
        if (p < 0) 
        {
          insert_l += p; // decrease insert_l
          fmt -= p; // advance fmt -- if fmt gets advanced past NULL term, insert_l will be < 0 anyway
          p=0;
        }

        if (insert_l>0)
        {
          if (wr->GetLength() > EEL_STRING_MAXUSERSTRING_LENGTH_HINT)
          {
#ifdef EEL_STRING_DEBUGOUT
            EEL_STRING_DEBUGOUT("str_insert: will not grow string since it is already %d bytes",wr->GetLength());
#endif
          }
          else
          {
            if (wr_src) 
            {
              wr->InsertRaw(fmt,p, insert_l); 
            }
            else
            {
              wr->Insert(fmt,p);
            }
          }
        }
      }
      else
      {
#ifdef EEL_STRING_DEBUGOUT
        EEL_STRING_DEBUGOUT("str_insert: bad source specifier passed %f",*fmt_index);
#endif
      }
    }
  }
  return *strOut;
}

static EEL_F NSEEL_CGEN_CALL _eel_strdelsub(void *opaque, EEL_F *strOut, EEL_F *pos, EEL_F *len)
{
  if (opaque)
  {
    EEL_STRING_STORAGECLASS *wr=NULL;
    EEL_STRING_GET_FOR_INDEX(*strOut, &wr);
    if (!wr)
    {
#ifdef EEL_STRING_DEBUGOUT
      EEL_STRING_DEBUGOUT("str_delsub: bad destination specifier passed %f",*strOut);
#endif
    }
    else
    {
      int p=(int)*pos;
      int l=(int)*len;
      if (p<0)
      {
        l+=p;
        p=0;
      }
      if (l>0)
        wr->DeleteSub(p,l);
    }
  }
  return *strOut;
}

static EEL_F NSEEL_CGEN_CALL _eel_strsetlen(void *opaque, EEL_F *strOut, EEL_F *newlen)
{
  if (opaque)
  {
    EEL_STRING_STORAGECLASS *wr=NULL;
    EEL_STRING_GET_FOR_INDEX(*strOut, &wr);
    if (!wr)
    {
#ifdef EEL_STRING_DEBUGOUT
      EEL_STRING_DEBUGOUT("str_setlen: bad destination specifier passed %f",*strOut);
#endif
    }
    else
    {
      int l = (int) *newlen;
      if (l < 0) l=0;
      if (l > EEL_STRING_MAXUSERSTRING_LENGTH_HINT)
      {
#ifdef EEL_STRING_DEBUGOUT
         EEL_STRING_DEBUGOUT("str_setlen: clamping requested length of %d to %d",l,EEL_STRING_MAXUSERSTRING_LENGTH_HINT);
#endif
        l=EEL_STRING_MAXUSERSTRING_LENGTH_HINT;
      }
      wr->SetLen(l);

    }
  }
  return *strOut;
}


static EEL_F NSEEL_CGEN_CALL _eel_strlen(void *opaque, EEL_F *fmt_index)
{
  if (opaque)
  {
    EEL_STRING_STORAGECLASS *wr=NULL;
    const char *fmt = EEL_STRING_GET_FOR_INDEX(*fmt_index,&wr);
    if (wr) return (EEL_F) wr->GetLength();
    if (fmt) return (EEL_F) strlen(fmt);
  }
  return 0.0;
}




static EEL_F NSEEL_CGEN_CALL _eel_printf(void *opaque, EEL_F *fmt_index)
{
  if (opaque)
  {
    const char *fmt = EEL_STRING_GET_FOR_INDEX(*fmt_index,NULL);
    if (fmt)
    {
      char buf[16384];
      const int len = eel_format_strings(opaque,fmt,buf,sizeof(buf));

      if (len >= 0)
      {
#ifdef EEL_STRING_STDOUT_WRITE
        EEL_STRING_STDOUT_WRITE(buf,len);
#endif
        return 1.0;
      }
      else
      {
#ifdef EEL_STRING_DEBUGOUT
        EEL_STRING_DEBUGOUT("printf: bad format string %s",fmt);
#endif
      }
    }
    else
    {
#ifdef EEL_STRING_DEBUGOUT
      EEL_STRING_DEBUGOUT("printf: bad format specifier passed %f",*fmt_index);
#endif
    }
  }
  return 0.0;
}



static EEL_F NSEEL_CGEN_CALL _eel_match(void *opaque, EEL_F *fmt_index, EEL_F *value_index)
{
  if (opaque)
  {
    EEL_STRING_STORAGECLASS *fmt_wr=NULL, *msg_wr=NULL;
    const char *fmt = EEL_STRING_GET_FOR_INDEX(*fmt_index,&fmt_wr);
    const char *msg = EEL_STRING_GET_FOR_INDEX(*value_index,&msg_wr);

    if (fmt && msg) return eel_string_match(opaque,fmt,msg,0,0, fmt + (fmt_wr?fmt_wr->GetLength():strlen(fmt)), msg + (msg_wr?msg_wr->GetLength():strlen(msg))) ? 1.0 : 0.0;
  }
  return 0.0;
}
static EEL_F NSEEL_CGEN_CALL _eel_matchi(void *opaque, EEL_F *fmt_index, EEL_F *value_index)
{
  // not yet binary safe
  if (opaque)
  {
    EEL_STRING_STORAGECLASS *fmt_wr=NULL, *msg_wr=NULL;
    const char *fmt = EEL_STRING_GET_FOR_INDEX(*fmt_index,&fmt_wr);
    const char *msg = EEL_STRING_GET_FOR_INDEX(*value_index,&msg_wr);

    if (fmt && msg) return eel_string_match(opaque,fmt,msg,0,1, fmt + (fmt_wr?fmt_wr->GetLength():strlen(fmt)), msg + (msg_wr?msg_wr->GetLength():strlen(msg))) ? 1.0 : 0.0;
  }
  return 0.0;
}

static void EEL_string_register()
{
  NSEEL_addfunctionex("strlen",1,(char *)_asm_generic1parm_retd,(char *)_asm_generic1parm_retd_end-(char *)_asm_generic1parm_retd,NSEEL_PProc_THIS,(void *)&_eel_strlen);
  NSEEL_addfunctionex("sprintf",2,(char *)_asm_generic2parm_retd,(char *)_asm_generic2parm_retd_end-(char *)_asm_generic2parm_retd,NSEEL_PProc_THIS,(void *)&_eel_sprintf);

  NSEEL_addfunctionex("strcat",2,(char *)_asm_generic2parm_retd,(char *)_asm_generic2parm_retd_end-(char *)_asm_generic2parm_retd,NSEEL_PProc_THIS,(void *)&_eel_strcat);
  NSEEL_addfunctionex("strcpy",2,(char *)_asm_generic2parm_retd,(char *)_asm_generic2parm_retd_end-(char *)_asm_generic2parm_retd,NSEEL_PProc_THIS,(void *)&_eel_strcpy);
  NSEEL_addfunctionex("strcmp",2,(char *)_asm_generic2parm_retd,(char *)_asm_generic2parm_retd_end-(char *)_asm_generic2parm_retd,NSEEL_PProc_THIS,(void *)&_eel_strcmp);
  NSEEL_addfunctionex("stricmp",2,(char *)_asm_generic2parm_retd,(char *)_asm_generic2parm_retd_end-(char *)_asm_generic2parm_retd,NSEEL_PProc_THIS,(void *)&_eel_stricmp);

  NSEEL_addfunctionex("strncat",3,(char *)_asm_generic3parm_retd,(char *)_asm_generic3parm_retd_end-(char *)_asm_generic3parm_retd,NSEEL_PProc_THIS,(void *)&_eel_strncat);
  NSEEL_addfunctionex("strncpy",3,(char *)_asm_generic3parm_retd,(char *)_asm_generic3parm_retd_end-(char *)_asm_generic3parm_retd,NSEEL_PProc_THIS,(void *)&_eel_strncpy);
  NSEEL_addfunctionex("strncmp",3,(char *)_asm_generic3parm_retd,(char *)_asm_generic3parm_retd_end-(char *)_asm_generic3parm_retd,NSEEL_PProc_THIS,(void *)&_eel_strncmp);
  NSEEL_addfunctionex("strnicmp",3,(char *)_asm_generic3parm_retd,(char *)_asm_generic3parm_retd_end-(char *)_asm_generic3parm_retd,NSEEL_PProc_THIS,(void *)&_eel_strnicmp);
  NSEEL_addfunctionex("str_getchar",2,(char *)_asm_generic2parm_retd,(char *)_asm_generic2parm_retd_end-(char *)_asm_generic2parm_retd,NSEEL_PProc_THIS,(void *)&_eel_strgetchar);
  NSEEL_addfunctionex("str_setlen",2,(char *)_asm_generic2parm_retd,(char *)_asm_generic2parm_retd_end-(char *)_asm_generic2parm_retd,NSEEL_PProc_THIS,(void *)&_eel_strsetlen);
  NSEEL_addfunctionex("strcpy_from",3,(char *)_asm_generic3parm_retd,(char *)_asm_generic3parm_retd_end-(char *)_asm_generic3parm_retd,NSEEL_PProc_THIS,(void *)&_eel_strcpyfrom);
  NSEEL_addfunctionex("str_setchar",3,(char *)_asm_generic3parm_retd,(char *)_asm_generic3parm_retd_end-(char *)_asm_generic3parm_retd,NSEEL_PProc_THIS,(void *)&_eel_strsetchar);
  NSEEL_addfunctionex("str_insert",3,(char *)_asm_generic3parm_retd,(char *)_asm_generic3parm_retd_end-(char *)_asm_generic3parm_retd,NSEEL_PProc_THIS,(void *)&_eel_strinsert);
  NSEEL_addfunctionex("str_delsub",3,(char *)_asm_generic3parm_retd,(char *)_asm_generic3parm_retd_end-(char *)_asm_generic3parm_retd,NSEEL_PProc_THIS,(void *)&_eel_strdelsub);

  NSEEL_addfunctionex("printf",1,(char *)_asm_generic1parm_retd,(char *)_asm_generic1parm_retd_end-(char *)_asm_generic1parm_retd,NSEEL_PProc_THIS,(void *)&_eel_printf);

  NSEEL_addfunctionex("match",2,(char *)_asm_generic2parm_retd,(char *)_asm_generic2parm_retd_end-(char *)_asm_generic2parm_retd,NSEEL_PProc_THIS,(void *)&_eel_match);
  NSEEL_addfunctionex("matchi",2,(char *)_asm_generic2parm_retd,(char *)_asm_generic2parm_retd_end-(char *)_asm_generic2parm_retd,NSEEL_PProc_THIS,(void *)&_eel_matchi);

}

void eel_preprocess_strings(void *opaque, EEL_STRING_STORAGECLASS &procOut, const char *rdptr)
{
  EEL_STRING_STORAGECLASS newstr;
  // preprocess to get strings from "", and replace with an index of someconstant+m_strings.GetSize()
  int comment_state=0; 
  // states:
  // 1 = comment to end of line
  // 2=comment til */
  while (*rdptr)
  {
    const char tc = *rdptr;
    switch (comment_state)
    {
      case 0:
        if (tc == '/')
        {
          if (rdptr[1] == '/') comment_state=1;
          else if (rdptr[1] == '*') comment_state=2;
        }


        if (tc == '$' && rdptr[1] == '\'' && rdptr[2] && rdptr[3] == '\'')
        {
          // ignore $'x' and do default processing
          procOut.Append(rdptr,4);
          rdptr+=4;
        }
        else if (tc == '"' || tc == '\'')
        {
          // scan tokens and replace with (idx) and padding
          newstr.Set("");
          const char *rdptr_start = rdptr;

          rdptr++; 

          while (*rdptr)
          {
            if (*rdptr == '\\') 
            {
              const char nc = rdptr[1];
              if (nc == 'r' || nc == 'R') { newstr.Append("\r"); rdptr += 2; }
              else if (nc == 'n' || nc == 'N') { newstr.Append("\n"); rdptr += 2; }
              else if (nc == 't' || nc == 'T') { newstr.Append("\t"); rdptr += 2; }
              else if (nc == '\'' || nc == '\"') { newstr.Append(&nc,1); rdptr+=2; }
              else if ((nc >= '0' && nc <= '9') || nc == 'x' || nc == 'X')
              {
                int base = 8;
                rdptr++;
                if (nc == 'X' || nc == 'x') { rdptr++; base=16; }

                unsigned char c=0;
                char thisc=toupper(*rdptr);
                while ((thisc >= '0' && thisc <= (base>=10 ? '9' : '7')) ||
                       (base == 16 && thisc >= 'A' && thisc <= 'F'))
                {
                  c *= base;
                  if (thisc >= 'A' && thisc <= 'F') c+=thisc - 'A' + 10;
                  else c += thisc - '0';
                  rdptr++;
                  thisc=toupper(*rdptr);
                }
                newstr.AppendRaw((char*)&c,1);
              }
              else 
              {
                const int n=rdptr[1] ? 2 :1;
                newstr.Append(rdptr,n);
                rdptr+=n;
              }
            }
            else 
            {
              if (*rdptr == tc)
              {
                if (rdptr[1] != tc) break;
                // "" converts to ", '' to '
                rdptr++;
              }
              newstr.Append(rdptr++,1);
            }
          }

          if (*rdptr) rdptr++; // skip trailing quote

          char t[128];
          if (tc == '\"') 
          {
            snprintf(t,sizeof(t),"(%u",EEL_STRING_ADDTOTABLE(newstr));
          }
          else
          {
            unsigned int val=0;
            const unsigned char *p = (const unsigned char *)newstr.Get();
            while (*p)
            {
              val <<= 8;
              val += *p++;
            }
            snprintf(t,sizeof(t),"(%u",val);
          }

          procOut.Append(t);

          // rdptr-rdptr_start is the original string length
          int pad_len = (int)(rdptr-rdptr_start) - (int)strlen(t) - 1;
          if (pad_len > 6)
          {
            // pad with /*blahhhhh*/
            pad_len -= 4;
            procOut.Append("/*");
            const char *rds = rdptr_start+1;
            while (pad_len-- > 0)
            {
              if (rds[0] == '*' && rds[1] == '/' && pad_len > 0)
              {
                procOut.Append("_/");
                rds+=2;
                pad_len--;
              }
              else
              {
                procOut.Append(rds,1);
                rds++;
              }
            }
            procOut.Append("*/");
          }
          else 
          {
            // pad with blanks
            while (pad_len-- > 0) procOut.Append(" ");
          }
          procOut.Append(")");
        }
        else
          procOut.Append(rdptr++,1);

      break;
      case 1:
        if (tc == '\n') comment_state=0;
        procOut.Append(rdptr++,1);
      break;
      case 2:
        if (tc == '*' && rdptr[1] == '/') 
        {
          procOut.Append(rdptr++,1);
          comment_state=0;
        }
        procOut.Append(rdptr++,1);
      break;
    }
  }
}

#endif

