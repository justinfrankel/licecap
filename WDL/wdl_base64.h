#ifndef _WDL_BASE64_H_
#define _WDL_BASE64_H_

#ifndef WDL_BASE64_FUNCDECL
#define WDL_BASE64_FUNCDECL static
#endif

static const char wdl_base64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
WDL_BASE64_FUNCDECL void wdl_base64encode(const unsigned char *in, char *out, int len)
{
  while (len >= 6)
  {
    const int accum = (in[0] << 16) + (in[1] << 8) + in[2];
    const int accum2 = (in[3] << 16) + (in[4] << 8) + in[5];
    out[0] = wdl_base64_alphabet[(accum >> 18) & 0x3F];
    out[1] = wdl_base64_alphabet[(accum >> 12) & 0x3F];
    out[2] = wdl_base64_alphabet[(accum >> 6) & 0x3F];
    out[3] = wdl_base64_alphabet[accum & 0x3F];
    out[4] = wdl_base64_alphabet[(accum2 >> 18) & 0x3F];
    out[5] = wdl_base64_alphabet[(accum2 >> 12) & 0x3F];
    out[6] = wdl_base64_alphabet[(accum2 >> 6) & 0x3F];
    out[7] = wdl_base64_alphabet[accum2 & 0x3F];
    out+=8;
    in+=6;
    len-=6;
  }

  if (len >= 3)
  {
    const int accum = (in[0]<<16)|(in[1]<<8)|in[2];
    out[0] = wdl_base64_alphabet[(accum >> 18) & 0x3F];
    out[1] = wdl_base64_alphabet[(accum >> 12) & 0x3F];
    out[2] = wdl_base64_alphabet[(accum >> 6) & 0x3F];
    out[3] = wdl_base64_alphabet[accum & 0x3F];    
    in+=3;
    len-=3;
    out+=4;
  }

  if (len>0)
  {
    if (len == 2)
    {
      const int accum = (in[0] << 8) | in[1];
      out[0] = wdl_base64_alphabet[(accum >> 10) & 0x3F];
      out[1] = wdl_base64_alphabet[(accum >> 4) & 0x3F];
      out[2] = wdl_base64_alphabet[(accum & 0xF)<<2];
    }
    else
    {
      const int accum = in[0];
      out[0] = wdl_base64_alphabet[(accum >> 2) & 0x3F];
      out[1] = wdl_base64_alphabet[(accum & 0x3)<<4];
      out[2]='=';  
    }
    out[3]='=';  
    out+=4;
  }

  out[0]=0;
}

WDL_BASE64_FUNCDECL int wdl_base64decode(const char *src, unsigned char *dest, int destsize)
{
  static unsigned char tab[256];
  int accum=0, nbits=0, wpos=0, x;

  if (!tab['/']) for(x=0;x<64;x++) tab[wdl_base64_alphabet[x]]=x+1;
  if (destsize <= 0) return 0;

  for (;;)
  {
    const int v=(int)tab[*(unsigned char *)src++];
    if (!v) return wpos;

    accum = (accum << 6) | (v-1);
    nbits += 6;   

    while (nbits >= 8)
    {
      nbits-=8;
      dest[wpos] = (accum>>nbits)&0xff;
      if (++wpos >= destsize) return wpos;
    }
  }
}


#endif
