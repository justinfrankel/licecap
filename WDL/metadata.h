#ifndef _WDL_METADATA_H_
#define _WDL_METADATA_H_

#include <math.h>
#include "wdlstring.h"
#include "xmlparse.h"
#include "fileread.h"
#include "filewrite.h"
#include "queue.h"
#include "win32_utf8.h"


char *tag_strndup(const char *src, int len)
{
  if (!src || !len) return NULL;
  int n=0;
  while (n < len && src[n]) ++n;
  char *dest=(char*)malloc(n+1);
  if (!dest) return NULL;
  memcpy(dest, src, n);
  dest[n]=0;
  return dest;
}

WDL_UINT64 ParseUInt64(const char *val)
{
  WDL_UINT64 i=0;
  if (val)
  {
    const char *p=val;
    while (*p)
    {
      int d=*p-'0';
      if (d < 0 || d > 9) break;
      i=(10*i)+d;
      ++p;
    }
    if (*p) i=0;
  }
  return i;
}

void XMLCompliantAppend(WDL_FastString *str, const char *txt)
{
  if (str && txt) for (;;)
  {
    char c = *txt++;
    switch (c)
    {
      case 0: return;
      case '<': str->Append("&lt;"); break;
      case '>': str->Append("&gt;"); break;
      case '&': str->Append("&amp;"); break;
      default: str->Append(&c,1); break;
    }
  }
}


const char *XMLHasOpenTag(WDL_FastString *str, const char *tag) // tag like "<FOO>")
{
  // stupid
  int taglen=strlen(tag);
  const char *open=strstr(str->Get(), tag);
  while (open)
  {
    const char *close=strstr(open+taglen, tag+1);
    if (!close || WDL_NOT_NORMALLY(close[-1] != '/')) break;
    open=strstr(close+taglen-1, tag);
  }
  return open;
}

void UnpackXMLElement(const char *pre, wdl_xml_element *elem,
  WDL_StringKeyedArray<char*> *metadata)
{
  WDL_FastString key;
  if (stricmp(elem->name, "BWFXML"))
  {
    key.SetFormatted(512, "%s:%s", pre, elem->name);
    pre=key.Get();
  }
  if (elem->value.Get()[0])
  {
    metadata->Insert(key.Get(), strdup(elem->value.Get()));
  }
  for (int i=0; i < elem->elements.GetSize(); ++i)
  {
    wdl_xml_element *elem2=elem->elements.Get(i);
    UnpackXMLElement(pre, elem2, metadata);
  }
}

bool UnpackIXMLChunk(const char *buf, int buflen,
  WDL_StringKeyedArray<char*> *metadata)
{
  if (!buf || !buflen || !metadata) return false;

  while (buflen > 20 && strnicmp(buf, "<BWFXML>", 8))
  {
    ++buf;
    --buflen;
  }
  if (buflen >= 20)
  {
    wdl_xml_parser xml(buf, buflen);
    if (!xml.parse() && xml.element_root)
    {
      UnpackXMLElement("IXML", xml.element_root, metadata);
      return true;
    }
  }

  return false;
}

bool IsXMPMetadata(const char *name, WDL_FastString *key)
{
  if (!name || !name[0] || !key) return false;

  // returns true if this XMP schema is one we know/care about
  if (!strnicmp(name, "xmpDM:", 6) && name[6])
  {
    key->SetFormatted(512, "XMP:dm/%s", name+6);
    return true;
  }
  if (!strnicmp(name, "dc:", 3) && name[3])
  {
    key->SetFormatted(512, "XMP:dc/%s", name+3);
    return true;
  }
  return false;
}

double UnpackXMPTimestamp(wdl_xml_element *elem)
{
  double tval=-1.0;
  int num=0, denom=0;
  for (int i=0; i < elem->attributes.GetSize(); ++i)
  {
    char *attr;
    const char *val=elem->attributes.Enumerate(i, &attr);
    if (!strcmp(attr, "xmpDM:scale") && val && val[0])
    {
      if (sscanf(val, "%d/%d", &num, &denom) != 2) num=denom=0;
    }
    else if (!strcmp(attr, "xmpDM:value") && val && val[0])
    {
      tval=atof(val);
    }
  }
  if (tval >= 0.0 && num > 0 && denom > 0)
  {
    return tval*(double)num/(double)denom;
  }
  return -1.0;
}

void UnpackXMPDescription(const char *curkey, wdl_xml_element *elem,
  WDL_StringKeyedArray<char*> *metadata)
{
  if (!strcmp(elem->name, "xmpDM:Tracks"))
  {
    // xmp "tracks" are collections of markers and other related data,
    // todo maybe parse the markers but for now we know we can ignore this entire block
    return;
  }

  if (!strcmp(elem->name, "xmpDM:relativeTimestamp"))
  {
    double tval=UnpackXMPTimestamp(elem);
    if (tval >= 0.0)
    {
      char buf[512];
      snprintf(buf, sizeof(buf), "%.0f", floor(tval*1000.0));
      metadata->Insert("XMP:dm/relativeTimestamp", strdup(buf));
    }
    return;
  }

  WDL_FastString key;
  int i;
  for (i=0; i < elem->attributes.GetSize(); ++i)
  {
    char *attr;
    const char *val=elem->attributes.Enumerate(i, &attr);
    if (IsXMPMetadata(attr, &key) && val && val[0])
    {
      metadata->Insert(key.Get(), strdup(val));
    }
  }

  if (IsXMPMetadata(elem->name, &key)) curkey=key.Get();
  if (curkey && elem->value.Get()[0])
  {
    metadata->Insert(curkey, strdup(elem->value.Get()));
  }

  for (i=0; i < elem->elements.GetSize(); ++i)
  {
    wdl_xml_element *elem2=elem->elements.Get(i);
    UnpackXMPDescription(curkey, elem2, metadata);
  }
}

void UnpackXMPElement(wdl_xml_element *elem, WDL_StringKeyedArray<char*> *metadata)
{
  if (!strcmp(elem->name, "rdf:Description"))
  {
    // everything we care about is in this block
    UnpackXMPDescription(NULL, elem, metadata);
    return;
  }

  for (int i=0; i < elem->elements.GetSize(); ++i)
  {
    wdl_xml_element *elem2=elem->elements.Get(i);
    UnpackXMPElement(elem2, metadata);
  }
}

bool UnpackXMPChunk(const char *buf, int buflen,
  WDL_StringKeyedArray<char*> *metadata)
{
  if (!buf || !buflen || !metadata) return false;

  wdl_xml_parser xmp(buf, buflen);
  if (!xmp.parse() && xmp.element_root)
  {
    UnpackXMPElement(xmp.element_root, metadata);
    return true;
  }

  return false;
}


// metadata is passed as an assocarray of id=>value strings,
// where id has the form "scheme:identifier"
// example "ID3:TIT2", "INFO:IPRD", "VORBIS:ALBUM", etc
// for user-defined metadata, the form is extended to "scheme:identifier:key",
// example "ID3:TXXX:mykey", "VORBIS:USER:mykey", etc
// id passed to this function is just "identifier:key" part
// NOTE: WDL/lameencdec and WDL/vorbisencdec have copies of this, so edit them if this changes
bool ParseUserDefMetadata(const char *id, const char *val,
  const char **k, const char **v, int *klen, int *vlen)
{
  const char *sep=strchr(id, ':');
  if (sep) // key encoded in id, version >= 6.12
  {
    *k=sep+1;
    *klen=strlen(*k);
    *v=val;
    *vlen=strlen(*v);
    return true;
  }

  sep=strchr(val, '=');
  if (sep) // key encoded in value, version <= 6.11
  {
    *k=val;
    *klen=sep-val;
    *v=sep+1;
    *vlen=strlen(*v);
    return true;
  }

  // no key, version <= 6.11
  *k="User";
  *klen=strlen(*k);
  *v=val;
  *vlen=strlen(*v);
  return false;
}


bool HasScheme(const char *scheme, WDL_StringKeyedArray<char*> *metadata)
{
  if (!scheme || !scheme[0] || !metadata) return false;

  bool ismatch=false;
  int idx=metadata->LowerBound(scheme, &ismatch);
  const char *key=NULL;
  metadata->Enumerate(idx, &key);
  if (key && !strnicmp(key, scheme, strlen(scheme))) return true;
  return false;
}


WDL_INT64 _ATOI64(const char *str)
{
  bool neg=false;
  if (*str == '-') { neg=true; str++; }
  WDL_INT64 v=0;
  while (*str >= '0' && *str <= '9')
  {
    v = (v*10) + (WDL_INT64) (neg ? -(*str - '0') : (*str - '0'));
    str++;
  }
  return v;
}


int PackIXMLChunk(WDL_HeapBuf *hb, WDL_StringKeyedArray<char*> *metadata)
{
  if (!hb || !metadata) return 0;

  if (!HasScheme("IXML", metadata) && !HasScheme("BWF", metadata)) return 0;

  int olen=hb->GetSize();

  WDL_FastString ixml;
  ixml.Set("<?xml version=\"1.0\" encoding=\"UTF-8\"?><BWFXML>");
  // metadata keys are sorted BWF<IXML,
  // so we'll need to open/close BEXT and USER in that order
  bool has_ixml=false, has_bext=false, has_user=false;
  for (int i=0; i < metadata->GetSize(); ++i)
  {
    const char *key;
    const char *val=metadata->Enumerate(i, &key);
    if (!key || !key[0] || !val || !val[0]) continue;

    if (!strncmp(key, "BWF:", 4) && key[4])
    {
      key += 4;

      const char *elem=NULL;
      if (!strcmp(key, "Description")) elem="BWF_DESCRIPTION";
      else if (!strcmp(key, "Originator")) elem="BWF_ORIGINATOR";
      else if (!strcmp(key, "OriginatorReference")) elem="BWF_ORIGINATOR_REFERENCE";
      else if (!strcmp(key, "OriginationDate")) elem="BWF_ORIGINATION_DATE";
      else if (!strcmp(key, "OriginationTime")) elem="BWF_ORIGINATION_TIME";
      else if (!strcmp(key, "TimeReference")) elem="BWF_TIME_REFERENCE";
      if (elem)
      {
        has_ixml=true;
        if (!has_bext) { has_bext=true; ixml.Append("<BEXT>"); }

        if (!strcmp(elem, "BWF_TIME_REFERENCE"))
        {
          WDL_UINT64 pos=_ATOI64(val);
          int hi=pos>>32, lo=(pos&0xFFFFFFFF);
          ixml.AppendFormatted(4096, "<%s_HIGH>%d</%s_HIGH>", elem, hi, elem);
          ixml.AppendFormatted(4096, "<%s_LOW>%d</%s_LOW>", elem, lo, elem);
        }
        else
        {
          ixml.AppendFormatted(2048, "<%s>", elem);
          XMLCompliantAppend(&ixml, val);
          ixml.AppendFormatted(2048, "</%s>", elem);
        }
      }
    }
    else if (!strncmp(key, "IXML:", 5) && key[5])
    {
      key += 5;
      has_ixml=true;
      if (has_bext) { has_bext=false; ixml.Append("</BEXT>"); }

      if (!strncmp(key, "USER", 4))
      {
        if (!has_user) { has_user=true; ixml.Append("<USER>"); }

        const char *k, *v;
        int klen, vlen;
        ParseUserDefMetadata(key, val, &k, &v, &klen, &vlen);
        ixml.Append("<");
        XMLCompliantAppend(&ixml, k);
        ixml.Append(">");
        XMLCompliantAppend(&ixml, v);
        ixml.Append("</");
        XMLCompliantAppend(&ixml, k);
        ixml.Append(">");
      }
      else
      {
        if (has_user) { has_user=false; ixml.Append("</USER>"); }

        ixml.AppendFormatted(2048, "<%s>", key);
        XMLCompliantAppend(&ixml, val);
        ixml.AppendFormatted(2048, "</%s>", key);
      }
      // specs say no specific whitespace or newline needed
    }
  }

  if (has_ixml)
  {
    if (has_bext) ixml.Append("</BEXT>");
    if (has_user) ixml.Append("</USER>");
    ixml.Append("</BWFXML>");

    int len=ixml.GetLength()+1; // nul-terminate just in case
    unsigned char *p=(unsigned char*)hb->Resize(olen+len+(len&1));
    if (p)
    {
      memcpy(p+olen, ixml.Get(), len);
      if (len&1) p[olen+len]=0;
    }
  }

  return hb->GetSize()-olen;
}


int PackXMPChunk(WDL_HeapBuf *hb, WDL_StringKeyedArray<char*> *metadata)
{
  if (!hb || !metadata) return 0;

  if (!HasScheme("XMP", metadata)) return 0;

  int olen=hb->GetSize();

  const char *xmp_hdr=
    "<?xpacket begin=\"\xEF\xBB\xBF\" id=\"W5M0MpCehiHzreSzNTczkc9d\"?>"
    "<x:xmpmeta xmlns:x=\"adobe:ns:meta/\">"
      "<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">"
        "<rdf:Description"
        " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
        " xmlns:xmpDM=\"http://ns.adobe.com/xmp/1.0/DynamicMedia/\""; // unclosed
  const char *xmp_ftr=
        "</rdf:Description>"
      "</rdf:RDF>"
    "</x:xmpmeta>"
    "<?xpacket end=\"w\"?>";

  WDL_FastString xmp(xmp_hdr);

  for (int pass=0; pass < 2; ++pass) // attributes, then elements
  {
    if (pass) xmp.Append(">");
    for (int i=0; i < metadata->GetSize(); ++i)
    {
      const char *key;
      const char *val=metadata->Enumerate(i, &key);
      if (!key || !key[0] || !val || !val[0]) continue;

      if (!strncmp(key, "XMP:", 4) && key[4])
      {
        key += 4;
        const char *prefix = // xmp schema
          !strncmp(key, "dc/", 3) ? "dc" :
          !strncmp(key, "dm/", 3) ? "xmpDM" : NULL;
        if (prefix && key[3])
        {
          if (!strcmp(key, "dc/description") || !strcmp(key, "dc/title"))
          {
            // elements
            if (!pass) continue;

            key += 3;
            const char *lang=metadata->Get("XMP:dc/language");
            if (!lang) lang="x-default";
            xmp.AppendFormatted(1024, "<%s:%s>", prefix, key);
            xmp.AppendFormatted(1024, "<rdf:Alt><rdf:li xml:lang=\"%s\">", lang);
            xmp.Append(val);
            xmp.Append("</rdf:li></rdf:Alt>");
            xmp.AppendFormatted(1024, "</%s:%s>", prefix, key);
          }
          else if (!strcmp(key, "dm/relativeTimestamp"))
          {
            // element
            if (!pass) continue;

            key += 3;
            xmp.AppendFormatted(1024, "<%s:%s xmpDM:value=\"%s\" xmpDM:scale=\"1/1000\"/>",
              prefix, key, val);
          }
          else
          {
            // attributes
            if (pass) continue;

            key += 3;
            xmp.AppendFormatted(1024, " %s:%s=\"%s\"", prefix, key, val);
          }
        }
      }
    }
  }
  xmp.Append(xmp_ftr);

  int len=xmp.GetLength()+1; // nul-terminate just in case
  unsigned char *p=(unsigned char*)hb->Resize(olen+len+(len&1));
  if (p)
  {
    memcpy(p+olen, xmp.Get(), xmp.GetLength()+1);
    if (len&1) p[olen+len]=0;
  }

  return hb->GetSize()-olen;
}

int PackVorbisFrame(WDL_HeapBuf *hb, WDL_StringKeyedArray<char*> *metadata, bool for_vorbis)
{
  if (!hb || !metadata) return 0;

  // for vorbis, we need an empty frame even if there's no metadata
  if (!for_vorbis && !HasScheme("VORBIS", metadata)) return 0;

  int olen=hb->GetSize();

  const char *vendor="REAPER";
  const int vendorlen=strlen(vendor);
  int framelen=4+vendorlen+4+for_vorbis;

  int i, tagcnt=0;
  for (i=0; i < metadata->GetSize(); ++i)
  {
    const char *key;
    const char *val=metadata->Enumerate(i, &key);
    if (!key || !key[0] || !val || !val[0]) continue;
    if (!strncmp(key, "VORBIS:", 7) && key[7])
    {
      key += 7;
      const char *k=key, *v=val;
      int klen=strlen(k), vlen=strlen(v);
      if (!strncmp(key, "USER", 4))
      {
        ParseUserDefMetadata(key, val, &k, &v, &klen, &vlen);
      }

      int taglen=4+klen+1+vlen;
      if (framelen+taglen >= 0xFFFFFF) break;
      framelen += taglen;
      ++tagcnt;
    }
  }

  unsigned char *buf=(unsigned char*)hb->Resize(olen+framelen)+olen;
  if (buf)
  {
    unsigned char *p=buf;
    memcpy(p, &vendorlen, 4);
    p += 4;
    memcpy(p, vendor, vendorlen);
    p += vendorlen;
    memcpy(p, &tagcnt, 4);
    p += 4;

    for (i=0; i < metadata->GetSize(); ++i)
    {
      const char *key;
      const char *val=metadata->Enumerate(i, &key);
      if (!key || !key[0] || !val || !val[0]) continue;
      if (!strncmp(key, "VORBIS:", 7) && key[7])
      {
        key += 7;
        const char *k=key, *v=val;
        int klen=strlen(k), vlen=strlen(v);
        if (!strncmp(key, "USER", 4))
        {
          ParseUserDefMetadata(key, val, &k, &v, &klen, &vlen);
        }

        int taglen=klen+1+vlen;
        memcpy(p, &taglen, 4);
        p += 4;
        while (*k)
        {
          *p++ = (*k >= ' ' && *k <= '}' && *k != '=') ? *k : ' ';
          k++;
        }
        *p++='=';
        memcpy(p, v, vlen);
        p += vlen;

        if (!--tagcnt) break;
      }
    }

    if (for_vorbis) *p++=1; // framing bit

    if (WDL_NOT_NORMALLY(p-buf != framelen) || framelen > 0xFFFFFF)
    {
      hb->Resize(olen);
    }
  }

  return hb->GetSize()-olen;
}

bool UnpackVorbisFrame(unsigned char *frame, int framelen,
  WDL_StringKeyedArray<char*> *metadata)
{
  if (!frame || !framelen || !metadata) return 0;

  char *p=(char*)frame;

  int vendor_len=*(int*)p;
  if (4+vendor_len+4 > framelen) return false;

  p += 4+vendor_len;
  int tagcnt=*(int*)p;
  p += 4;

  WDL_String str;
  int pos=4+vendor_len+4;
  while (pos < framelen && tagcnt--)
  {
    int taglen=*(int*)p;
    p += 4;
    if (pos+taglen > framelen) return false;
    str.Set("VORBIS:");
    str.Append(p, taglen);
    p += taglen;
    const char *sep=strchr(str.Get()+7, '=');
    if (!sep) return false;
    *(char*)sep=0;
    metadata->Insert(str.Get(), strdup(sep+1));
  }

  return (pos == framelen && tagcnt == 0);
}


#define _AddInt32LE(i) \
  *p++=((i)&0xFF); \
  *p++=(((i)>>8)&0xFF); \
  *p++=(((i)>>16)&0xFF); \
  *p++=(((i)>>24)&0xFF);

#define _GetInt32LE(p) \
  (((p)[0])|((p)[1]<<8)|((p)[2]<<16)|((p)[3]<<24))


int PackApeChunk(WDL_HeapBuf *hb, WDL_StringKeyedArray<char*> *metadata)
{
  if (!hb || !metadata) return false;

  if (!HasScheme("APE", metadata)) return false;

  int olen=hb->GetSize();

  int i, apelen=0, cnt=0;
  for (i=0; i < metadata->GetSize(); ++i)
  {
    const char *key;
    const char *val=metadata->Enumerate(i, &key);
    if (strlen(key) < 5 || strncmp(key, "APE:", 4) || !val || !val[0]) continue;
    key += 4;
    if (!apelen) apelen += 64;
    if (!strncmp(key, "User Defined", 12))
    {
      const char *k, *v;
      int klen, vlen;
      ParseUserDefMetadata(key, val, &k, &v, &klen, &vlen);
      apelen += 8+klen+1+vlen;
    }
    else
    {
      apelen += 8+strlen(key)+1+strlen(val);
    }
    ++cnt;
  }
  if (!apelen) return false;

  unsigned char *buf=(unsigned char*)hb->Resize(olen+apelen)+olen;
  if (buf)
  {
    unsigned char *p=buf;
    memcpy(p, "APETAGEX", 8);
    p += 8;
    _AddInt32LE(2000); // version
    _AddInt32LE(apelen-32);
    _AddInt32LE(cnt);
    _AddInt32LE((1<<31)|(1<<30)|(1<<29)); // tag contains header and footer, this is the header
    _AddInt32LE(0);
    _AddInt32LE(0);

    for (i=0; i < metadata->GetSize(); ++i)
    {
      const char *key;
      const char *val=metadata->Enumerate(i, &key);
      if (strlen(key) < 5 || strncmp(key, "APE:", 4) || !val || !val[0]) continue;
      key += 4;
      const char *k=key, *v=val;
      int klen, vlen;
      if (!strncmp(key, "User Defined", 12))
      {
        ParseUserDefMetadata(key, val, &k, &v, &klen, &vlen);
      }
      else
      {
        klen=strlen(k);
        vlen=strlen(v);
      }
      _AddInt32LE(vlen);
      _AddInt32LE(0);
      while (klen--)
      {
        *p = (*k >= 0x20 && *k <= 0x7E ? *k : ' ');
        ++p;
        ++k;
      }
      *p++=0;
      memcpy(p, v, vlen);
      p += vlen;
    }

    memcpy(p, "APETAGEX", 8);
    p += 8;
    _AddInt32LE(2000); // version
    _AddInt32LE(apelen-32);
    _AddInt32LE(cnt);
    _AddInt32LE((1<<31)|(1<<30)|(1<<28)); // tag contains header and footer, this is the footer
    _AddInt32LE(0);
    _AddInt32LE(0);

    if (WDL_NOT_NORMALLY(p-buf != apelen)) hb->Resize(olen);
  }

  return hb->GetSize()-olen;
}


const char *EnumMetadataSchemeFromFileType(const char *filetype, int idx)
{
  if (!filetype || !filetype[0]) return NULL;

  if (filetype[0] == '.') ++filetype;
  if (!stricmp(filetype, "bwf")) filetype="wav";
  else if (!stricmp(filetype, "opus")) filetype="ogg";
  else if (!stricmp(filetype, "aiff")) filetype="aif";

  static const char *WAV_SCHEMES[]=
  {
    "BWF", "INFO", "IXML", "CART", "XMP", "AXML", "ID3",
  };
  static const char *MP3_SCHEMES[]=
  {
    "ID3", "APE", "IXML", "XMP",
  };
  static const char *FLAC_SCHEMES[]=
  {
    "VORBIS", "BWF", "IXML", "XMP",
  };
  static const char *OGG_SCHEMES[]=
  {
    "VORBIS",
  };
  static const char *WV_SCHEMES[]=
  {
    "BWF", "APE",
  };
  static const char *AIF_SCHEMES[]=
  {
    "IFF", "XMP",
  };
  static const char *RX2_SCHEMES[]=
  {
    "REX",
  };

#define DO_SCHEME_MAP(X) if (!stricmp(filetype, #X)) \
  return idx < sizeof(X##_SCHEMES)/sizeof(X##_SCHEMES[0]) ? X##_SCHEMES[idx] : NULL;

  DO_SCHEME_MAP(WAV);
  DO_SCHEME_MAP(MP3);
  DO_SCHEME_MAP(FLAC);
  DO_SCHEME_MAP(OGG);
  DO_SCHEME_MAP(WV);
  DO_SCHEME_MAP(AIF);
  DO_SCHEME_MAP(RX2);

#undef DO_SCHEME_MAP

  return NULL;
}


const char *EnumMetadataKeyFromMexKey(const char *mexkey, int idx)
{
  if (!mexkey || !mexkey[0] || idx < 0) return NULL;

  // TO_DO_IF_METADATA_UPDATE
  // "TITLE", "ARTIST", "ALBUM", "YEAR", "GENRE", "COMMENT", "DESC", "BPM", "KEY", "DB_CUSTOM", "TRACKNUMBER"

  if (!strcmp(mexkey, "DATE")) mexkey="YEAR";
  else if (!strcmp(mexkey, "REAPER")) mexkey="DB_CUSTOM";
  // callers handle PREFPOS

  // !!!
  // if anything is added here that might get embedded in ID3,
  // also update AddMexID3Raw()
  // !!!

  // general priority order here:
  // BWF
  // INFO
  // ID3
  // APE
  // VORBIS
  // CART
  // IXML
  // XMP
  // IFF
  // REX

  static const char *TITLE_KEYS[]=
  {
    "INFO:INAM",
    "ID3:TIT2",
    "APE:Title",
    "VORBIS:TITLE",
    "CART:Title",
    "IXML:PROJECT",
    "XMP:dc/title",
    "IFF:NAME",
    "REX:Name",
  };
  static const char *ARTIST_KEYS[]=
  {
    "INFO:IART",
    "ID3:TPE1",
    "APE:Artist",
    "VORBIS:ARTIST",
    "CART:Artist",
    "XMP:dm/artist",
    "IFF:AUTH",
  };
  static const char *ALBUM_KEYS[]=
  {
    "INFO:IALB",
    "INFO:IPRD",
    "ID3:TALB",
    "APE:Album",
    "VORBIS:ALBUM",
    "XMP:dm/album",
  };
  static const char *YEAR_KEYS[]= // really DATE
  {
    "BWF:OriginationDate",
    "INFO:ICRD",
    "ID3:TYER",
    "ID3:TDRC",
    "APE:Year",
    "APE:Record Date",
    "VORBIS:DATE",
    "CART:StartDate",
    "XMP:dc/date",
  };
  static const char *GENRE_KEYS[]=
  {
    "INFO:IGNR",
    "ID3:TCON",
    "APE:Genre",
    "VORBIS:GENRE",
    "CART:Category",
    "XMP:dm/genre",
  };
  static const char *COMMENT_KEYS[]=
  {
    "INFO:ICMT",
    "ID3:COMM",
    "APE:Comment",
    "VORBIS:COMMENT",
    "CART:TagText",
    "IXML:NOTE",
    "XMP:dm/logComment",
    "REX:FreeText",
  };
  static const char *DESC_KEYS[]=
  {
    "BWF:Description",
    "INFO:ISBJ",
    "INFO:IKEY",
    "ID3:TIT3",
    "APE:Subtitle",
    "VORBIS:DESCRIPTION",
    "XMP:dc/description",
    "IFF:ANNO",
  };
  static const char *BPM_KEYS[]=
  {
    "ID3:TBPM",
    "APE:BPM",
    "VORBIS:BPM",
    "XMP:dm/tempo",
  };
  static const char *KEY_KEYS[]=
  {
    "ID3:TKEY",
    "APE:Key",
    "VORBIS:KEY",
    "XMP:dm/key",
  };
  static const char *DB_CUSTOM_KEYS[]=
  {
    "ID3:TXXX:REAPER",
    "APE:REAPER",
    "VORBIS:REAPER",
    "IXML:USER:REAPER",
  };
  static const char *TRACKNUMBER_KEYS[]=
  {
    "INFO:TRCK",
    "ID3:TRCK",
    "APE:Track",
    "VORBIS:TRACKNUMBER",
    "CART:CutID",
    // "IXML:TRACK",
    "XMP:dm/trackNumber",
  };

#define DO_MEXKEY_MAP(K) if (!strcmp(mexkey, #K)) \
  return idx < sizeof(K##_KEYS)/sizeof(K##_KEYS[0]) ? K##_KEYS[idx] : NULL;

  DO_MEXKEY_MAP(TITLE);
  DO_MEXKEY_MAP(ARTIST);
  DO_MEXKEY_MAP(ALBUM);
  DO_MEXKEY_MAP(TRACKNUMBER);
  DO_MEXKEY_MAP(YEAR);
  DO_MEXKEY_MAP(GENRE);
  DO_MEXKEY_MAP(COMMENT);
  DO_MEXKEY_MAP(DESC);
  DO_MEXKEY_MAP(BPM);
  DO_MEXKEY_MAP(KEY);
  DO_MEXKEY_MAP(DB_CUSTOM);

#undef DO_MEXKEY_MAP

  return NULL;
}


bool HandleMexMetadataRequest(const char *mexkey, char *buf, int buflen,
  WDL_StringKeyedArray<char*> *metadata)
{
  if (!mexkey || !mexkey[0] || !buf || !buflen || !metadata) return false;

  buf[0]=0;
  int i=0;
  const char *key;
  while ((key=EnumMetadataKeyFromMexKey(mexkey, i++)))
  {
    const char *val=metadata->Get(key);
    if (val && val[0])
    {
      lstrcpyn(buf, val, buflen);
      return true;
    }
  }

  return false;
}


void WriteMetadataPrefPos(double prefpos, int srate,
  WDL_StringKeyedArray<char*> *metadata)
{
  if (!metadata) return;

  metadata->Delete("BWF:TimeReference");
  metadata->Delete("ID3:TXXX:TIME_REFERENCE");
  metadata->Delete("IXML:BEXT:BWF_TIME_REFERENCE_HIGH");
  metadata->Delete("IXML:BEXT:BWF_TIME_REFERENCE_LOW");
  metadata->Delete("XMP:dm/relativeTimestamp");
  metadata->Delete("VORBIS:TIME_REFERENCE");

  if (prefpos > 0.0 && srate > 1)
  {
    char buf[128];
    if (srate > 0.0)
    {
      snprintf(buf, sizeof(buf), "%.0f", floor(prefpos*(double)srate));
      metadata->Insert("BWF:TimeReference", strdup(buf));
      // BWF:TimeReference causes IXML:BEXT element to be written as well
      metadata->Insert("ID3:TXXX:TIME_REFERENCE", strdup(buf));
      metadata->Insert("VORBIS:TIME_REFERENCE", strdup(buf));
    }
    snprintf(buf, sizeof(buf), "%.0f", floor(prefpos*1000.0));
    metadata->Insert("XMP:dm/relativeTimestamp", strdup(buf));
  }
}


void AddMexMetadata(WDL_StringKeyedArray<char*> *mex_metadata,
  WDL_StringKeyedArray<char*> *metadata, int srate)
{
  if (!mex_metadata || !metadata) return;

  for (int idx=0; idx < mex_metadata->GetSize(); ++idx)
  {
    const char *mexkey;
    const char *val=mex_metadata->Enumerate(idx, &mexkey);

    if (!strcmp(mexkey, "PREFPOS"))
    {
      WDL_UINT64 ms = val && val[0] ? ParseUInt64(val) : 0;
      WriteMetadataPrefPos((double)ms/1000.0, srate, metadata);
      // caller may still have to do stuff if prefpos is represented
      // in some other way outside the metadata we handle, like wavpack
      continue;
    }

    int i=0;
    const char *key;
    while ((key=EnumMetadataKeyFromMexKey(mexkey, i++)))
    {
      if (val && val[0]) metadata->Insert(key, strdup(val));
      else metadata->Delete(key);
    }
  }
}


void DumpMetadata(WDL_FastString *str, WDL_StringKeyedArray<char*> *metadata)
{
  if (!str || !metadata || !metadata->GetSize()) return;

  char scheme[256];
  scheme[0]=0;

  // TO_DO_IF_METADATA_UPDATE
  // note these are for display, so not necessarily in the same order as elsewhere
  static const char *mexkey[]=
    {"TITLE", "ARTIST", "ALBUM", "TRACKNUMBER", "YEAR", "GENRE", "COMMENT", "DESC", "BPM", "KEY", "DB_CUSTOM"};
  static const char *dispkey[]=
    {"Title", "Artist", "Album", "Track", "Date", "Genre", "Comment", "Description", "BPM", "Key", "Media Explorer Tags"};
  char buf[2048];
  for (int j=0; j < sizeof(mexkey)/sizeof(mexkey[0]); ++j)
  {
    if (HandleMexMetadataRequest(mexkey[j], buf, sizeof(buf), metadata))
    {
      if (!scheme[0])
      {
        lstrcpyn(scheme, "mex", sizeof(scheme));
        str->Append("Metadata:\r\n");
      }
      str->AppendFormatted(4096, "    %s: %s\r\n", dispkey[j], buf);
    }
  }

  int i;
  for (i=0; i < metadata->GetSize(); ++i)
  {
    const char *key;
    const char *val=metadata->Enumerate(i, &key);
    if (!key || !key[0] || !val || !val[0] || !strncmp(val, "[Binary data]", 13)) continue;

    const char *sep=strchr(key, ':');
    if (sep)
    {
      int slen=wdl_min(sep-key, sizeof(scheme)-1);
      if (strncmp(scheme, key, slen))
      {
        lstrcpyn(scheme, key, slen+1);
        str->AppendFormatted(256, "%s tags:\r\n", scheme);
      }
      key += slen+1;
    }
    str->AppendFormatted(4096, "    %s: %s\r\n", key, val);
  }

  int unk_cnt=0;
  for (i=0; i < metadata->GetSize(); ++i)
  {
    const char *key;
    const char *val=metadata->Enumerate(i, &key);
    if (key && key[0] && val && (!val[0] || !strncmp(val, "[Binary data]", 13)))
    {
      if (!unk_cnt++) str->Append("Other file sections:\r\n");
      str->AppendFormatted(4096, "    %s%s\r\n", key,
        !strnicmp(key, "smed", 4) ?
        " (proprietary Soundminer metadata)" : "");
    }
  }
}

void CopyMetadata(WDL_StringKeyedArray<char*> *src, WDL_StringKeyedArray<char*> *dest)
{
  if (!dest || !src) return;

  dest->DeleteAll(false);
  for (int i=0; i < src->GetSize(); ++i)
  {
    const char *key;
    const char *val=src->Enumerate(i, &key);
    dest->AddUnsorted(key, strdup(val));
  }
  dest->Resort(); // safe in case src/dest have diff sort attributes
}


bool CopyFileData(WDL_FileRead *fr, WDL_FileWrite *fw, WDL_INT64 len)
{
  while (len)
  {
    char tmp[32768];
    const int amt = (int) (wdl_min(len,sizeof(tmp)));
    const int rd = fr->Read(tmp, amt);
    if (rd != amt) return false;
    if (fw->Write(tmp, rd) != rd) return false;
    len -= rd;
  }
  return true;
}


bool EnumVorbisChapters(WDL_StringKeyedArray<char*> *metadata, int idx,
  double *pos, const char **name)
{
  if (!metadata) return false;

  int cnt=0;
  bool ismatch=false;
  const char *prev=NULL;
  int i=metadata->LowerBound("VORBIS:CHAPTER", &ismatch);
  for (; i < metadata->GetSize(); ++i)
  {
    const char *key;
    const char *val=metadata->Enumerate(i, &key);
    if (strncmp(key, "VORBIS:CHAPTER", 14)) return false;
    if (!prev || strncmp(key, prev, 17))
    {
      prev=key;
      if (idx == cnt)
      {
        if (!key[17] && val && val[0])
        {
          // VORBIS:CHAPTER001 => 00:00:00.000
          if (pos)
          {
            int hh=0, mm=0, ss=0, ms=0;
            if (sscanf(val, "%d:%d:%d.%d", &hh, &mm, &ss, &ms) == 4)
            {
              *pos=(double)hh*3600.0+(double)mm*60.0+(double)ss+(double)ms*0.001;
            }
          }
          if (name)
          {
            val=metadata->Enumerate(i+1, &key);
            if (!strncmp(key, prev, 17) && !strcmp(key+17, "NAME"))
            {
              // VORBIS:CHAPTER001NAME => chapter name
              *name=val;
            }
          }
          return true;
        }
        return false;
      }
      ++cnt;
    }
  }
  return false;
}


#define _AddSyncSafeInt32(i) \
*p++=(((i)>>21)&0x7F); \
*p++=(((i)>>14)&0x7F); \
*p++=(((i)>>7)&0x7F); \
*p++=((i)&0x7F);

#define _AddInt32(i) \
*p++=(((i)>>24)&0xFF); \
*p++=(((i)>>16)&0xFF); \
*p++=(((i)>>8)&0xFF); \
*p++=((i)&0xFF);

#define _GetSyncSafeInt32(p) \
(((p)[0]<<21)|((p)[1]<<14)|((p)[2]<<7)|((p)[3]))

void WriteSyncSafeInt32(WDL_FileWrite *fw, int i)
{
  unsigned char buf[4];
  unsigned char *p=buf;
  _AddSyncSafeInt32(i);
  fw->Write(buf, 4);
}


#define CTOC_NAME "TOC" // arbitrary name of table of contents element


static bool _isnum(const char *v, int pos, int len)
{
  for (int i=pos; i < pos+len; ++i)
  {
    if (v[i] < '0' || v[i] > '9') return false;
  }
  return true;
}

int IsID3TimeVal(const char *v)
{
  if (strlen(v) == 4 && _isnum(v, 0, 4)) return 1;
  if (strlen(v) == 5 && _isnum(v, 0, 2) && _isnum(v, 3, 2)) return 2;
  return 0;
}

struct ID3RawTag
{
  char key[8]; // we only use 4 chars + nul
  WDL_HeapBuf val; // includes everything after taglen (flags, etc)
};

int ReadID3Raw(WDL_FileRead *fr, WDL_PtrList<ID3RawTag> *rawtags)
{
  if (!fr || !fr->IsOpen() || !rawtags) return 0;

  unsigned char buf[16];
  if (fr->Read(buf, 10) != 10) return 0;
  if (memcmp(buf, "ID3\x04", 4) && memcmp(buf, "ID3\x03", 4)) return 0;
  int id3len=_GetSyncSafeInt32(buf+6);
  if (!id3len) return 0;

  int rdlen=0;
  while (rdlen < id3len)
  {
    if (fr->Read(buf, 8) != 8) return 0;
    if (!buf[0]) return 10+id3len; // padding
    if ((buf[0] < 'A' || buf[0] > 'Z') && (buf[0] < '0' || buf[0] > '9')) return 0; // unexpected

    int taglen=_GetSyncSafeInt32(buf+4)+2; // include flags in taglen

    ID3RawTag *rawtag=rawtags->Add(new ID3RawTag);
    memcpy(rawtag->key, buf, 4);
    rawtag->key[4]=0;
    unsigned char *p=(unsigned char*)rawtag->val.ResizeOK(taglen);
    if (!p || fr->Read(p, taglen) != taglen) return 0;
    rdlen += 8+taglen;
    if (rdlen == id3len) return 10+id3len;
  }
  return 0;
}

int WriteID3Raw(WDL_FileWrite *fw, WDL_PtrList<ID3RawTag> *rawtags)
{
  if (!fw || !fw->IsOpen() || !rawtags || !rawtags->GetSize()) return 0;

  WDL_INT64 fpos=fw->GetPosition();
  fw->Write("ID3\x04\0\0\0\0\0\0", 10);

  int id3len=0;
  for (int i=0; i < rawtags->GetSize(); ++i)
  {
    ID3RawTag *rawtag=rawtags->Get(i);
    int taglen=rawtag->val.GetSize(); // flags included in taglen
    fw->Write(rawtag->key, 4);
    WriteSyncSafeInt32(fw, taglen-2);
    fw->Write(rawtag->val.Get(), taglen);

    id3len += 8+taglen;
  }

  WDL_INT64 epos=fw->GetPosition();
  fw->SetPosition(fpos+6);
  WriteSyncSafeInt32(fw, id3len);
  fw->SetPosition(epos);

  return id3len;
}


void AddMexID3Raw(WDL_StringKeyedArray<char*> *metadata,
  WDL_PtrList<ID3RawTag> *rawtags)
{
  if (!metadata || !rawtags) return;

  // this is a super specialized function, basically the accumulated
  // technical debt from all of the abstraction in how we handle metadata
  // in general and in the media explorer, conflicting with the highly
  // structured nature of ID3.

  // in theory we could add a general translation layer between
  // our metadata and ID3RawTag, but there are a lot of specific rules
  // that are hard to generalize, for example the rule that there
  // can be multiple TXXX tags but only one with a given variable-length
  // identifier that starts 2 bytes into the value part of the tag.
  // also the only application for this translation is rewriting metadata
  // from the media explorer.

  // TO_DO_IF_METADATA_UPDATE
  // we just have to know all this, and also how to handle each one
  static const char *MEXID3KEYS[]=
  {
    "ID3:TIT2",
    "ID3:TPE1",
    "ID3:TALB",
    "ID3:TRCK",
    "ID3:TYER",
    "ID3:TDRC",
    "ID3:TCON",
    "ID3:COMM",
    "ID3:TIT3",
    "ID3:TBPM",
    "ID3:TKEY",
    "ID3:TXXX:REAPER",
    "ID3:TXXX:TIME_REFERENCE",
    "ID3:PRIV:iXML",
    "ID3:PRIV:XMP",
  };

  WDL_HeapBuf hb;
  for (int k=0; k < sizeof(MEXID3KEYS)/sizeof(MEXID3KEYS[0]); ++k)
  {
    const char *key=MEXID3KEYS[k];
    if (!strcmp(key, "ID3:TXXX:TIME_REFERENCE") && !metadata->Exists(key))
    {
      continue; // only clear prefpos if the caller provides it
    }

    for (int i=0; i < rawtags->GetSize(); ++i)
    {
      ID3RawTag *rawtag=rawtags->Get(i);
      if (!strncmp(rawtag->key, key+4, 4))
      {
        if (!strncmp(key+4, "TXXX", 4))
        {
          const char *subkey=key+9;
          if (rawtag->val.GetSize() < 3+strlen(subkey)+1 ||
            memcmp((unsigned char*)rawtag->val.Get()+3, subkey, strlen(subkey)))
          {
            continue;
          }
        }
        else if (!strncmp(key+4, "PRIV", 4))
        {
          const char *subkey=key+9;
          if (rawtag->val.GetSize() < 2+strlen(subkey)+1 ||
            memcmp((unsigned char*)rawtag->val.Get()+2, subkey, strlen(subkey)))
          {
            continue;
          }
        }

        // note that spec allows multiple COMM tags, but we won't
        rawtags->Delete(i--, true);
      }
    }

    const char *val=NULL;
    int vallen=0;
    if (!strncmp(key+4, "PRIV", 4))
    {
      const char *subkey=key+9;
      hb.Resize(0, false);
      if (!stricmp(subkey, "IXML")) PackIXMLChunk(&hb, metadata);
      else if (!stricmp(subkey, "XMP")) PackXMPChunk(&hb, metadata);
      val=(const char*)hb.Get();
      vallen=hb.GetSize();
    }
    else
    {
      val=metadata->Get(key);
      vallen = val ? strlen(val) : 0;
    }
    if (!val || !vallen) continue;

    ID3RawTag *rawtag=new ID3RawTag;
    memcpy(rawtag->key, key+4, 4);
    rawtag->key[4]=0;

    if (!strncmp(key+4, "TXXX", 4))
    {
      const char *subkey=key+9;
      int subkeylen=strlen(subkey);
      unsigned char *p=(unsigned char*)rawtag->val.Resize(3+subkeylen+1+vallen);
      if (p)
      {
        memcpy(p, "\0\0\3", 3); // UTF-8
        p += 3;
        memcpy(p, subkey, subkeylen+1);
        p += subkeylen+1;
        memcpy(p, val, vallen);
      }
    }
    else if (!strncmp(key+4, "PRIV", 4))
    {
      const char *subkey=key+9;
      int subkeylen=strlen(subkey);
      unsigned char *p=(unsigned char*)rawtag->val.Resize(2+subkeylen+1+vallen);
      if (p)
      {
        memcpy(p, "\0\0", 2);
        p += 2;
        memcpy(p, subkey, subkeylen+1);
        p += subkeylen+1;
        memcpy(p, val, vallen);
      }
    }
    else if (!strncmp(key+4, "COMM", 4))
    {
      // see comments in PackID3Chunk
      unsigned char *p=(unsigned char*)rawtag->val.Resize(3+4+vallen);
      if (p)
      {
        memcpy(p, "\0\0\3", 3); // UTF-8
        p += 3;
        const char *lang=metadata->Get("ID3:COMM_LANG");
        if (lang && strlen(lang) >= 3 &&
          tolower(lang[0]) >= 'a' && tolower(lang[0]) <= 'z' &&
          tolower(lang[1]) >= 'a' && tolower(lang[1]) <= 'z' &&
          tolower(lang[2]) >= 'a' && tolower(lang[2]) <= 'z')
        {
          *p++=tolower(*lang++);
          *p++=tolower(*lang++);
          *p++=tolower(*lang++);
          *p++=0;
        }
        else
        {
          memcpy(p, "XXX\0", 4);
          p += 4;
        }
        memcpy(p, val, vallen);
      }
    }
    else if (key[4] == 'T')
    {
      unsigned char *p=(unsigned char*)rawtag->val.Resize(2+1+vallen);
      if (p)
      {
        memcpy(p, "\0\0\3", 3); // UTF-8
        p += 3;
        memcpy(p, val, vallen);
      }
    }

    if (rawtag->val.GetSize()) rawtags->Add(rawtag);
    else delete rawtag;
  }
}


int PackID3Chunk(WDL_HeapBuf *hb, WDL_StringKeyedArray<char*> *metadata, bool want_ixml_xmp)
{
  if (!hb || !metadata) return false;

  bool want_ixml = want_ixml_xmp && HasScheme("IXML", metadata);
  bool want_xmp = want_ixml_xmp && HasScheme("XMP", metadata);
  if (!HasScheme("ID3", metadata) && !want_ixml && !want_xmp) return false;

  int olen=hb->GetSize();

  int id3len=0, chapcnt=0;
  WDL_TypedQueue<char> toc;
  int i;
  for (i=0; i < metadata->GetSize(); ++i)
  {
    const char *key;
    const char *val=metadata->Enumerate(i, &key);
    if (strlen(key) < 8 || strncmp(key, "ID3:", 4) || !val) continue;
    key += 4;
    if (!strncmp(key, "TXXX", 4))
    {
      const char *k, *v;
      int klen, vlen;
      ParseUserDefMetadata(key, val, &k, &v, &klen, &vlen);
      id3len += 10+1+klen+1+vlen;
    }
    else if (!strncmp(key, "TIME", 4))
    {
      if (IsID3TimeVal(val)) id3len += 10+1+4;
    }
    else if (key[0] == 'T' && strlen(key) == 4)
    {
      id3len += 10+1+strlen(val);
    }
    else if (!strcmp(key, "COMM"))
    {
      id3len += 10+5+strlen(val);
    }
    else if (!strncmp(key, "CHAP", 4) && chapcnt < 255)
    {
      const char *c1=strchr(val, ':');
      const char *c2 = c1 ? strchr(c1+1, ':') : NULL;
      if (c1)
      {
        ++chapcnt;
        const char *toc_entry=key; // use "CHAP001", etc as the internal toc entry
        const char *chap_name = c2 ? c2+1 : NULL;
        toc.Add(toc_entry, strlen(toc_entry)+1);
        id3len += 10+strlen(toc_entry)+1+16;
        if (chap_name) id3len += 10+1+strlen(chap_name)+1;
      }
    }
  }
  if (chapcnt)
  {
    id3len += 10+strlen(CTOC_NAME)+1+2+toc.GetSize();
  }

  WDL_HeapBuf apic_hdr;
  int apic_datalen=0;
  const char *apic_fn=metadata->Get("ID3:APIC_FILE");
  if (apic_fn && apic_fn[0])
  {
    const char *mime=NULL;
    const char *ext=WDL_get_fileext(apic_fn);
    if (ext && (!stricmp(ext, ".jpg") || !stricmp(ext, ".jpeg"))) mime="image/jpeg";
    else if (ext && !stricmp(ext, ".png")) mime="image/png";
    if (mime)
    {
      FILE *fp=fopenUTF8(apic_fn, "rb"); // could stat but let's make sure we can open the file
      if (fp)
      {
        fseek(fp, 0, SEEK_END);
        apic_datalen=ftell(fp);
        fclose(fp);
      }
    }
    if (apic_datalen)
    {
      const char *t=metadata->Get("ID3:APIC_TYPE");
      int type=-1;
      if (t && t[0] >= '0' && t[0] <= '9') type=atoi(t);
      if (type < 0 || type >= 16) type=3; // default "Cover (front)"

      const char *desc=metadata->Get("ID3:APIC_DESC");
      if (!desc) desc="";
      int desclen=wdl_min(strlen(desc), 63);

      int apic_hdrlen=1+strlen(mime)+1+1+desclen+1;
      char *p=(char*)apic_hdr.Resize(apic_hdrlen);
      if (p)
      {
        *p++=3; // UTF-8
        memcpy(p, mime, strlen(mime)+1);
        p += strlen(mime)+1;
        *p++=type;
        memcpy(p, desc, desclen);
        p += desclen;
        *p++=0;
        id3len += 10+apic_hdrlen+apic_datalen;
      }
    }
  }

  WDL_HeapBuf ixml;
  if (want_ixml)
  {
    PackIXMLChunk(&ixml, metadata);
    if (ixml.GetSize()) id3len += 10+5+ixml.GetSize();
  }

  WDL_HeapBuf xmp;
  if (want_xmp)
  {
    PackXMPChunk(&xmp, metadata);
    if (xmp.GetSize()) id3len += 10+4+xmp.GetSize();
  }

  if (id3len)
  {
    id3len += 10;
    unsigned char *buf=(unsigned char*)hb->Resize(olen+id3len)+olen;
    if (buf)
    {
      chapcnt=0;
      unsigned char *p=buf;
      memcpy(p,"ID3\x04\x00\x00", 6);
      p += 6;
      _AddSyncSafeInt32(id3len-10);
      for (i=0; i < metadata->GetSize(); ++i)
      {
        const char *key;
        const char *val=metadata->Enumerate(i, &key);
        if (strlen(key) < 8 || strncmp(key, "ID3:", 4) || !val) continue;
        key += 4;
        if (!strncmp(key, "TXXX", 4))
        {
          memcpy(p, key, 4);
          p += 4;
          const char *k, *v;
          int klen, vlen;
          ParseUserDefMetadata(key, val, &k, &v, &klen, &vlen);
          _AddSyncSafeInt32(1+klen+1+vlen);
          memcpy(p, "\x00\x00\x03", 3); // UTF-8
          p += 3;
          memcpy(p, k, klen);
          p += klen;
          *p++=0;
          memcpy(p, v, vlen);
          p += vlen;
        }
        else if (!strncmp(key, "TIME", 4))
        {
          int tv=IsID3TimeVal(val);
          if (tv)
          {
            memcpy(p, key, 4);
            p += 4;
            _AddSyncSafeInt32(1+4);
            memcpy(p, "\x00\x00\x03", 3); // UTF-8
            p += 3;
            memcpy(p, val, 2);
            if (tv == 1) memcpy(p+2, val+2, 2);
            else memcpy(p+2, val+3, 2);
            p += 4;
          }
        }
        else if (key[0] == 'T' && strlen(key) == 4)
        {
          memcpy(p, key, 4);
          p += 4;
          int len=strlen(val);
          _AddSyncSafeInt32(1+len);
          memcpy(p, "\x00\x00\x03", 3); // UTF-8
          p += 3;
          memcpy(p, val, len);
          p += len;
        }
        else if (!strcmp(key, "COMM"))
        {
          // http://www.loc.gov/standards/iso639-2/php/code_list.php
          // most apps ignore this, itunes wants "eng" or something locale-specific
          const char *lang=metadata->Get("ID3:COMM_LANG");

          memcpy(p, key, 4);
          p += 4;
          int len=strlen(val);
          _AddSyncSafeInt32(5+len);
          memcpy(p, "\x00\x00\x03", 3); // UTF-8
          p += 3;
          if (lang && strlen(lang) >= 3 &&
              tolower(*lang) >= 'a' && tolower(*lang) <= 'z')
          {
            *p++=tolower(*lang++);
            *p++=tolower(*lang++);
            *p++=tolower(*lang++);
            *p++=0;
          }
          else
          {
            // some apps write "XXX" for "no particular language"
            memcpy(p, "XXX\x00", 4);
            p += 4;
          }
          memcpy(p, val, len);
          p += len;
        }
        else if (!strncmp(key, "CHAP", 4) && chapcnt < 255)
        {
          const char *c1=strchr(val, ':');
          const char *c2 = c1 ? strchr(c1+1, ':') : NULL;
          if (c1)
          {
            // note, the encoding ignores the chapter number (CHAP001, etc)

            ++chapcnt;
            const char *toc_entry=key; // use "CHAP001", etc as the internal toc entry
            const char *chap_name = c2 ? c2+1 : NULL;
            int st=atoi(val);
            int et=atoi(c1+1);

            int framelen=strlen(toc_entry)+1+16;
            if (chap_name) framelen += 10+1+strlen(chap_name)+1;

            memcpy(p, "CHAP", 4);
            p += 4;
            _AddSyncSafeInt32(framelen);
            memset(p, 0, 2);
            p += 2;
            memcpy(p, toc_entry, strlen(toc_entry)+1);
            p += strlen(toc_entry)+1;
            _AddInt32(st);
            _AddInt32(et);
            memset(p, 0, 8);
            p += 8;

            if (chap_name)
            {
              int name_framelen=1+strlen(chap_name)+1;
              memcpy(p, "TIT2", 4);
              p += 4;
              _AddSyncSafeInt32(name_framelen);
              memcpy(p, "\x00\x00\x03", 3); // UTF-8
              p += 3;
              memcpy(p, chap_name, strlen(chap_name)+1);
              p += strlen(chap_name)+1;
            }
          }
        }
      }

      if (chapcnt)
      {
        int toc_framelen=strlen(CTOC_NAME)+1+2+toc.GetSize();
        memcpy(p, "CTOC", 4);
        p += 4;
        _AddSyncSafeInt32(toc_framelen);
        memset(p, 0, 2);
        p += 2;
        memcpy(p, CTOC_NAME, strlen(CTOC_NAME)+1);
        p += strlen(CTOC_NAME)+1;
        *p++=3; // CTOC flags: &1=top level, &2=ordered
        *p++=(chapcnt&0xFF);
        memcpy(p, toc.Get(), toc.GetSize());
        p += toc.GetSize();
      }

      if (apic_hdr.GetSize() && apic_datalen)
      {
        memcpy(p, "APIC", 4);
        p += 4;
        int len=apic_hdr.GetSize()+apic_datalen;
        _AddSyncSafeInt32(len);
        memcpy(p, "\x00\x00", 2);
        p += 2;
        memcpy(p, apic_hdr.Get(), apic_hdr.GetSize());
        p += apic_hdr.GetSize();
        FILE *fp=fopenUTF8(apic_fn, "rb");
        if (WDL_NORMALLY(fp))
        {
          fread(p, 1, apic_datalen, fp);
          fclose(fp);
        }
        else // uh oh
        {
          memset(p, 0, apic_datalen);
        }
        p += apic_datalen;
      }

      if (ixml.GetSize())
      {
        memcpy(p, "PRIV", 4);
        p += 4;
        int len=ixml.GetSize()+5;
        _AddSyncSafeInt32(len);
        memcpy(p, "\x00\x00", 2);
        p += 2;
        memcpy(p, "iXML\x00", 5);
        p += 5;
        memcpy(p, ixml.Get(), ixml.GetSize());
        p += ixml.GetSize();
      }

      if (xmp.GetSize())
      {
        memcpy(p, "PRIV", 4);
        p += 4;
        int len=xmp.GetSize()+4;
        _AddSyncSafeInt32(len);
        memcpy(p, "\x00\x00", 2);
        p += 2;
        memcpy(p, "XMP\x00", 4);
        p += 4;
        memcpy(p, xmp.Get(), xmp.GetSize());
        p += xmp.GetSize();
      }

      if (WDL_NOT_NORMALLY(p-buf != id3len)) hb->Resize(olen);
    }
  }

  return hb->GetSize()-olen;
}

double ReadMetadataPrefPos(WDL_StringKeyedArray<char*> *metadata, double srate)
{
  if (!metadata) return -1.0;

  const char *v=metadata->Get("BWF:TimeReference");
  if (!v || !v[0]) v=metadata->Get("ID3:TXXX:TIME_REFERENCE");
  if (!v || !v[0]) v=metadata->Get("VORBIS:TIME_REFERENCE");
  if (v && v[0] && srate > 0.0)
  {
    WDL_UINT64 i=ParseUInt64(v);
    return (double)i/srate;
  }

  v=metadata->Get("IXML:BEXT:BWF_TIME_REFERENCE_LOW");
  if (v && v[0] && srate > 0.0)
  {
    WDL_UINT64 ipos=atoi(v);
    v=metadata->Get("IXML:BEXT:BWF_TIME_REFERENCE_HIGH");
    if (v[0]) ipos |= ((WDL_UINT64)atoi(v))<<32;
    return (double)ipos/srate;
  }

  v=metadata->Get("XMP:dm/relativeTimestamp");
  if (v && v[0])
  {
    WDL_UINT64 i=ParseUInt64(v);
    return (double)i/1000.0;
  }

  return -1.0;
}

#endif // _METADATA_H_
