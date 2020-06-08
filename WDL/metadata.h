#ifndef _WDL_METADATA_H_
#define _WDL_METADATA_H_

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


void XMLCompliantAppend(WDL_FastString *str, const char *txt)
{
  if (!str) return;
  int pos=str->GetLength();
  str->Append(txt);
  int len=str->GetLength();
  for (int i=pos; i < len; ++i)
  {
    char c=str->Get()[i];
    const char *repl=NULL;
    if (c == '<') repl="&lt;";
    else if (c == '>') repl="&gt;";
    else if (c == '&') repl="&amp;";
    if (repl)
    {
      str->DeleteSub(i, 1);
      str->Insert(repl, i);
      pos += strlen(repl)-1;
      len += strlen(repl)-1;
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

bool UnpackIXMLChunk(WDL_HeapBuf *hb, WDL_StringKeyedArray<char*> *metadata)
{
  if (!hb || !hb->GetSize() || !metadata) return false;

  const char *p=(const char*)hb->Get();
  int len=hb->GetSize();
  while (len > 20 && strnicmp(p, "<BWFXML>", 8))
  {
    ++p;
    --len;
  }
  if (len >= 20)
  {
    wdl_xml_parser xml(p, len);
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

void UnpackXMPDescription(const char *curkey, wdl_xml_element *elem,
  WDL_StringKeyedArray<char*> *metadata)
{
  if (!strcmp(elem->name, "xmpDM:Tracks"))
  {
    // xmp "tracks" are collections of markers and other related data,
    // todo maybe parse the markers but for now we know we can ignore this entire block
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

bool UnpackXMPChunk(WDL_HeapBuf *hb, WDL_StringKeyedArray<char*> *metadata)
{
  if (!hb || !hb->GetSize() || !metadata) return false;

  const char *p=(const char*)hb->Get();
  int len=hb->GetSize();
  wdl_xml_parser xmp(p, len);
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
  if (key && !strncmp(key, scheme, strlen(scheme))) return true;
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

int PackVorbisFrame(WDL_HeapBuf *hb, WDL_StringKeyedArray<char*> *metadata)
{
  if (!hb || !metadata) return 0;

  if (!HasScheme("VORBIS", metadata)) return 0;

  int olen=hb->GetSize();

  const char *vendor="REAPER";
  const int vendorlen=strlen(vendor);

  int i, framelen=0, tagcnt=0;
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

      if (!framelen) framelen=4+vendorlen+4;
      framelen +=4+klen+1+vlen; // +1?
      ++tagcnt;
    }
  }
  if (framelen && framelen < 0xFFFFFF)
  {
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

          int taglen=klen+1+vlen; // +1?
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
        }
      }

      if (WDL_NOT_NORMALLY(p-buf != framelen) || framelen > 0xFFFFFF)
      {
        hb->Resize(olen);
      }
    }
  }
  return hb->GetSize()-olen;
}

bool UnpackVorbisFrame(WDL_HeapBuf *hb, WDL_StringKeyedArray<char*> *metadata)
{
  if (!hb || !metadata) return 0;

  char *p=(char*)hb->Get();
  int framelen=hb->GetSize();

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
  // only used for rewriting metadata atm

  if (!filetype || !filetype[0]) return NULL;

  if (!stricmp(filetype, ".wav") || !strcmp(filetype, ".bwf"))
  {
    if (idx == 0) return "BWF";
    if (idx == 1) return "INFO";
    if (idx == 2) return "IXML";
    if (idx == 3) return "CART";
    if (idx == 4) return "XMP";
    if (idx == 5) return "ID3";
    return NULL;
  }
  if (!stricmp(filetype, ".mp3"))
  {
    if (idx == 0) return "ID3";
    if (idx == 1) return "APE";
    if (idx == 2) return "XMP";
    return NULL;
  }
  if (!stricmp(filetype, ".flac"))
  {
    if (idx == 0) return "VORBIS";
    if (idx == 1) return "BWF";
    if (idx == 2) return "IXML";
    if (idx == 3) return "XMP";
    return NULL;
  }
  if (!stricmp(filetype, ".ogg") || !stricmp(filetype, ".opus"))
  {
    if (idx == 0) return "VORBIS";
    return NULL;
  }
  if (!stricmp(filetype, ".wv"))
  {
    if (idx == 0) return "APE";
    return NULL;
  }
  if (!stricmp(filetype, ".aif") || !stricmp(filetype, ".aiff"))
  {
    if (idx == 0) return "IFF";
    if (idx == 1) return "XMP";
    return NULL;
  }
  if (!stricmp(filetype, ".rx2"))
  {
    if (idx == 0) return "REX";
    return NULL;
  }
  return NULL;
}


const char *EnumMetadataKeyFromMexKey(const char *mexkey, int idx)
{
  if (!mexkey || !mexkey[0] || idx < 0) return NULL;

  // TO_DO_IF_METADATA_UPDATE
  // "TITLE", "ARTIST", "ALBUM", "YEAR", "GENRE", "COMMENT", "DESC", "BPM", "KEY", "DB_CUSTOM"

  if (!strcmp(mexkey, "DATE")) mexkey="YEAR";
  else if (!strcmp(mexkey, "REAPER")) mexkey="DB_CUSTOM";

  // general priority order here:
  // BWF
  // INFO
  // IXML
  // XMP
  // ID3
  // APE
  // VORBIS
  // CART
  // IFF
  // REX

  static const char *TITLE_KEYS[]=
  {
    "INFO:INAM",
    "IXML:PROJECT",
    "XMP:dc/title"
    "ID3:TIT2",
    "APE:Title",
    "VORBIS:TITLE",
    "CART:Title",
    "IFF:NAME",
    "REX:Name",
  };
  static const char *ARTIST_KEYS[]=
  {
    "INFO:IART",
    "XMP:dm/artist"
    "ID3:TPE1",
    "APE:Artist",
    "VORBIS:ARTIST",
    "CART:Artist",
    "IFF:AUTH",
  };
  static const char *ALBUM_KEYS[]=
  {
    "INFO:IALB",
    "INFO:IPRD",
    "XMP:dm/album",
    "ID3:TALB",
    "APE:Album",
    "VORBIS:ALBUM",
  };
  static const char *YEAR_KEYS[]= // really DATE
  {
    "BWF:OriginationDate",
    "INFO:ICRD",
    "XMP:dc/date"
    "ID3:TYER",
    "APE:Year",
    "APE:Record Date",
    "VORBIS:DATE",
    "CART:StartDate",
  };
  static const char *GENRE_KEYS[]=
  {
    "INFO:IGNR",
    "XMP:dm/genre",
    "ID3:TCON",
    "APE:Genre",
    "VORBIS:GENRE",
    "CART:Category",
  };
  static const char *COMMENT_KEYS[]=
  {
    "INFO:ICMT",
    "IXML:NOTE",
    "XMP:dm/logComment",
    "ID3:COMM",
    "APE:Comment",
    "VORBIS:COMMENT",
    "CART:TagText",
    "REX:FreeText",
  };
  static const char *DESC_KEYS[]=
  {
    "BWF:Description",
    "INFO:ISBJ",
    "INFO:IKEY",
    "XMP:dc/description",
    "ID3:TIT3",
    "APE:Subtitle",
    "VORBIS:DESCRIPTION",
    "IFF:ANNO",
  };
  static const char *BPM_KEYS[]=
  {
    "XMP:dm/tempo",
    "ID3:TBPM",
    "APE:BPM",
    "VORBIS:BPM",
  };
  static const char *KEY_KEYS[]=
  {
    "XMP:dm/key",
    "ID3:TKEY",
    "APE:Key",
    "VORBIS:KEY",
  };
  static const char *DB_CUSTOM_KEYS[]=
  {
    "IXML:USER:REAPER",
    "ID3:TXXX:REAPER",
    "APE:REAPER",
    "VORBIS:REAPER",
  };

#define DO_MEXKEY_MAP(K) if (!strcmp(mexkey, #K)) \
  return idx < sizeof(K##_KEYS)/sizeof(K##_KEYS[0]) ? K##_KEYS[idx] : NULL;

  DO_MEXKEY_MAP(TITLE);
  DO_MEXKEY_MAP(ARTIST);
  DO_MEXKEY_MAP(ALBUM);
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


bool AddMexMetadata(WDL_StringKeyedArray<char*> *mex_metadata,
  const char *filetype, WDL_StringKeyedArray<char*> *metadata)
{
  if (!mex_metadata || !metadata || !filetype || !filetype[0]) return false;

  bool did_edit=false;
  for (int idx=0; idx < mex_metadata->GetSize(); ++idx)
  {
    const char *mexkey;
    const char *val=mex_metadata->Enumerate(idx, &mexkey);

    int s=0;
    const char *scheme;
    while ((scheme=EnumMetadataSchemeFromFileType(filetype, s++)))
    {
      int i=0;
      const char *key;
      while ((key=EnumMetadataKeyFromMexKey(mexkey, i++)))
      {
        if (val && val[0]) metadata->Insert(key, strdup(val));
        else metadata->Delete(key);
        did_edit=true;
      }
    }
  }
  return did_edit;
}


void DumpMetadata(WDL_FastString *str, WDL_StringKeyedArray<char*> *metadata)
{
  if (!str || !metadata || !metadata->GetSize()) return;

  char scheme[256];
  scheme[0]=0;

  // TO_DO_IF_METADATA_UPDATE
  static const char *mexkey[]=
    {"TITLE", "ARTIST", "ALBUM", "YEAR", "GENRE", "COMMENT", "DESC", "BPM", "KEY", "DB_CUSTOM"};
  static const char *dispkey[]=
    {"Title", "Artist", "Album", "Date", "Genre", "Comment", "Description", "BPM", "Key", "Media Explorer Tags"};
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

  for (int i=0; i < metadata->GetSize(); ++i)
  {
    const char *key;
    const char *val=metadata->Enumerate(i, &key);
    if (!key || !key[0] || !val || !val[0]) continue;

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
}

void CopyMetadata(WDL_StringKeyedArray<char*> *src, WDL_StringKeyedArray<char*> *dest)
{
  if (!dest || !src) return;

  dest->DeleteAll(false);
  for (int i=0; i < src->GetSize(); ++i)
  {
    const char *key;
    const char *val=src->Enumerate(i, &key);
    dest->AddUnsorted(key, strdup(val)); // already sorted
  }
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

int PackID3Chunk(WDL_HeapBuf *hb, WDL_StringKeyedArray<char*> *metadata, bool want_xmp)
{
  if (!hb || !metadata) return false;

  if (want_xmp) want_xmp=HasScheme("XMP", metadata);
  if (!HasScheme("ID3", metadata) && !want_xmp) return false;

  int olen=hb->GetSize();

  int id3len=0, chapcnt=0;
  WDL_TypedQueue<char> toc;
  int i;
  for (i=0; i < metadata->GetSize(); ++i)
  {
    const char *id;
    const char *val=metadata->Enumerate(i, &id);
    if (strlen(id) < 8 || strncmp(id, "ID3:", 4) || !val) continue;
    id += 4;
    if (!strncmp(id, "TXXX", 4))
    {
      const char *k, *v;
      int klen, vlen;
      ParseUserDefMetadata(id, val, &k, &v, &klen, &vlen);
      id3len += 10+1+klen+1+vlen;
    }
    else if (!strncmp(id, "TIME", 4))
    {
      if (IsID3TimeVal(val)) id3len += 10+1+4;
    }
    else if (id[0] == 'T' && strlen(id) == 4)
    {
      id3len += 10+1+strlen(val);
    }
    else if (!strcmp(id, "COMM"))
    {
      id3len += 10+5+strlen(val);
    }
    else if (!strncmp(id, "CHAP", 4) && chapcnt < 255)
    {
      const char *c1=strchr(val, ':');
      const char *c2 = c1 ? strchr(c1+1, ':') : NULL;
      if (c1)
      {
        ++chapcnt;
        const char *toc_entry=id; // use "CHAP1", etc as the internal toc entry
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
        const char *id;
        const char *val=metadata->Enumerate(i, &id);
        if (strlen(id) < 8 || strncmp(id, "ID3:", 4) || !val) continue;
        id += 4;
        if (!strncmp(id, "TXXX", 4))
        {
          memcpy(p, id, 4);
          p += 4;
          const char *k, *v;
          int klen, vlen;
          ParseUserDefMetadata(id, val, &k, &v, &klen, &vlen);
          _AddSyncSafeInt32(1+klen+1+vlen);
          memcpy(p, "\x00\x00\x03", 3); // UTF-8
          p += 3;
          memcpy(p, k, klen);
          p += klen;
          *p++=0;
          memcpy(p, v, vlen);
          p += vlen;
        }
        else if (!strncmp(id, "TIME", 4))
        {
          int tv=IsID3TimeVal(val);
          if (tv)
          {
            memcpy(p, id, 4);
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
        else if (id[0] == 'T' && strlen(id) == 4)
        {
          memcpy(p, id, 4);
          p += 4;
          int len=strlen(val);
          _AddSyncSafeInt32(1+len);
          memcpy(p, "\x00\x00\x03", 3); // UTF-8
          p += 3;
          memcpy(p, val, len);
          p += len;
        }
        else if (!strcmp(id, "COMM"))
        {
          // http://www.loc.gov/standards/iso639-2/php/code_list.php
          // most apps ignore this, itunes wants "eng" or something locale-specific
          const char *lang=metadata->Get("ID3:COMM_LANG");

          memcpy(p, id, 4);
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
        else if (!strncmp(id, "CHAP", 4) && chapcnt < 255)
        {
          const char *c1=strchr(val, ':');
          const char *c2 = c1 ? strchr(c1+1, ':') : NULL;
          if (c1)
          {
            // note, the encoding ignores the chapter number (CHAP1, etc)

            ++chapcnt;
            const char *toc_entry=id; // use "CHAP1", etc as the internal toc entry
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


#endif // _METADATA_H_
