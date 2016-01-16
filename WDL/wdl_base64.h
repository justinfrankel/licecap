#ifndef _WDL_BASE64_H_
#define _WDL_BASE64_H_

#ifndef WDL_BASE64_FUNCDECL
#define WDL_BASE64_FUNCDECL static
#endif

static const char wdl_base64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
WDL_BASE64_FUNCDECL void wdl_base64encode(const unsigned char *in, char *out, int len)
{
  char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int shift = 0;
  int accum = 0;

  while (len>0)
  {
    len--;
    accum <<= 8;
    shift += 8;
    accum |= *in++;
    while ( shift >= 6 )
    {
      shift -= 6;
      *out++ = alphabet[(accum >> shift) & 0x3F];
    }
  }
  if (shift == 4)
  {
    *out++ = alphabet[(accum & 0xF)<<2];
    *out++='=';  
  }
  else if (shift == 2)
  {
    *out++ = alphabet[(accum & 0x3)<<4];
    *out++='=';  
    *out++='=';  
  }

  *out++=0;
}

WDL_BASE64_FUNCDECL int wdl_base64decode(const char *src, unsigned char *dest, int destsize)
{
  int accum=0, nbits=0, wpos=0;
  while (*src && wpos < destsize)
  {
    int x=0;
    char c=*src++;
    if (c >= 'A' && c <= 'Z') x=c-'A';
    else if (c >= 'a' && c <= 'z') x=c-'a' + 26;
    else if (c >= '0' && c <= '9') x=c-'0' + 52;
    else if (c == '+') x=62;
    else if (c == '/') x=63;
    else break;

    accum = (accum << 6) | x;
    nbits += 6;   

    while (nbits >= 8 && wpos < destsize)
    {
      nbits-=8;
      dest[wpos++] = (char)((accum>>nbits)&0xff);
    }
  }
  return wpos;
}

#endif
