#ifndef __EEL_FILES_H__
#define __EEL_FILES_H__

// should include eel_strings.h before this, probably
//#define EEL_FILE_OPEN(fn,mode) ((instance)opaque)->OpenFile(fn,mode)
//#define EEL_FILE_GETFP(fp) ((instance)opaque)->GetFileFP(fp)
//#define EEL_FILE_CLOSE(fpindex) ((instance)opaque)->CloseFile(fpindex)

static EEL_F NSEEL_CGEN_CALL _eel_fopen(void *opaque, EEL_F *fn_index, EEL_F *mode_index)
{
  const char *fn = EEL_STRING_GET_FOR_INDEX(*fn_index,NULL);
  const char *mode = EEL_STRING_GET_FOR_INDEX(*mode_index,NULL);
  if (!fn || !mode) return -1;
  return (EEL_F) EEL_FILE_OPEN(fn,mode);
}
static EEL_F NSEEL_CGEN_CALL _eel_fclose(void *opaque, EEL_F *fpp) { return EEL_FILE_CLOSE((int)*fpp); }
static EEL_F NSEEL_CGEN_CALL _eel_fgetc(void *opaque, EEL_F *fpp) 
{
  FILE *fp = EEL_FILE_GETFP((int)*fpp);
  if (fp) return (EEL_F)fgetc(fp);
  return -1.0;
}

static EEL_F NSEEL_CGEN_CALL _eel_ftell(void *opaque, EEL_F *fpp) 
{
  FILE *fp = EEL_FILE_GETFP((int)*fpp);
  if (fp) return (EEL_F)ftell(fp);
  return -1.0;
}
static EEL_F NSEEL_CGEN_CALL _eel_fflush(void *opaque, EEL_F *fpp) 
{
  FILE *fp = EEL_FILE_GETFP((int)*fpp);
  if (fp) { fflush(fp); return 0.0; }
  return -1.0;
}

static EEL_F NSEEL_CGEN_CALL _eel_feof(void *opaque, EEL_F *fpp) 
{
  FILE *fp = EEL_FILE_GETFP((int)*fpp);
  if (fp) return feof(fp) ? 1.0 : 0.0;
  return -1.0;
}
static EEL_F NSEEL_CGEN_CALL _eel_fseek(void *opaque, EEL_F *fpp, EEL_F *wh, EEL_F *offset) 
{
  FILE *fp = EEL_FILE_GETFP((int)*fpp);
  if (fp) return fseek(fp,*wh<0 ? SEEK_SET : *wh > 0 ? SEEK_END : SEEK_CUR, (int) *offset);
  return -1.0;
}
static EEL_F NSEEL_CGEN_CALL _eel_fgets(void *opaque, EEL_F *fpp, EEL_F *strOut)
{
  EEL_STRING_STORAGECLASS *wr=NULL;
  EEL_STRING_GET_FOR_INDEX(*strOut, &wr);

  FILE *fp = EEL_FILE_GETFP((int)*fpp);
  if (!fp)
  {
    if (wr) wr->Set("");
    return 0.0;
  }
  char buf[16384];
  buf[0]=0;
  fgets(buf,sizeof(buf),fp);
  if (wr)
  {
    wr->Set(buf);
    return (EEL_F)wr->GetLength();
  }
  else
  {
#ifdef EEL_STRING_DEBUGOUT
    EEL_STRING_DEBUGOUT("fgets: bad destination specifier passed %f, throwing away %d bytes",*strOut, strlen(buf));
#endif
    return strlen(buf);
  }
}
static EEL_F NSEEL_CGEN_CALL _eel_fread(void *opaque, EEL_F *fpp, EEL_F *strOut, EEL_F *flen)
{
  int use_len = (int) *flen;
  if (use_len < 1) return 0.0;

  EEL_STRING_STORAGECLASS *wr=NULL;
  EEL_STRING_GET_FOR_INDEX(*strOut, &wr);
  if (!wr)
  {
#ifdef EEL_STRING_DEBUGOUT
    EEL_STRING_DEBUGOUT("fread: bad destination specifier passed %f, not reading %d bytes",*strOut, use_len);
#endif
    return -1;
  }

  FILE *fp = EEL_FILE_GETFP((int)*fpp);
  if (!fp)
  {
    if (wr) wr->Set("");
    return 0.0;
  }

  wr->SetLen(use_len);
  if (wr->GetLength() != use_len)
  {
#ifdef EEL_STRING_DEBUGOUT
    EEL_STRING_DEBUGOUT("fread: error allocating storage for read of %d bytes", use_len);
#endif
    return -1.0;
  }
  use_len = fread((char *)wr->Get(),1,use_len,fp);
  wr->SetLen(use_len > 0 ? use_len : 0, true);
  return (EEL_F) use_len;
}

static EEL_F NSEEL_CGEN_CALL _eel_fwrite(void *opaque, EEL_F *fpp, EEL_F *strOut, EEL_F *flen)
{
  int use_len = (int) *flen;

  EEL_STRING_STORAGECLASS *wr=NULL;
  const char *str=EEL_STRING_GET_FOR_INDEX(*strOut, &wr);
  if (!wr && !str)
  {
#ifdef EEL_STRING_DEBUGOUT
    EEL_STRING_DEBUGOUT("fwrite: bad source specifier passed %f, not writing %d bytes",*strOut, use_len);
#endif
    return -1.0;
  }
  if (use_len < 1 && wr) use_len = wr->GetLength();
  if (wr && use_len > wr->GetLength()) use_len = wr->GetLength();

  if (use_len < 1) return 0.0;

  FILE *fp = EEL_FILE_GETFP((int)*fpp);
  if (!fp) return 0.0;

  return (EEL_F) fwrite(str,1,use_len,fp);
}

static EEL_F NSEEL_CGEN_CALL _eel_fprintf(void *opaque, EEL_F *fpp, EEL_F *fmt_index)
{
  if (opaque)
  {
    FILE *fp = EEL_FILE_GETFP((int)*fpp);
    if (!fp) return 0.0;

    const char *fmt = EEL_STRING_GET_FOR_INDEX(*fmt_index,NULL);
    if (fmt)
    {
      char buf[16384];
      const int len = eel_format_strings(opaque,fmt,buf,sizeof(buf));

      if (len >= 0)
      {
        return (EEL_F) fwrite(buf,1,len,fp);
      }
      else
      {
#ifdef EEL_STRING_DEBUGOUT
        EEL_STRING_DEBUGOUT("fprintf: bad format string %s",fmt);
#endif
      }
    }
    else
    {
#ifdef EEL_STRING_DEBUGOUT
      EEL_STRING_DEBUGOUT("fprintf: bad format specifier passed %f",*fmt_index);
#endif
    }
  }
  return 0.0;
}

void EEL_file_register()
{
  NSEEL_addfunctionex("fopen",2,(char *)_asm_generic2parm_retd,(char *)_asm_generic2parm_retd_end-(char *)_asm_generic2parm_retd,NSEEL_PProc_THIS,(void *)&_eel_fopen);

  NSEEL_addfunctionex("fread",3,(char *)_asm_generic3parm_retd,(char *)_asm_generic3parm_retd_end-(char *)_asm_generic3parm_retd,NSEEL_PProc_THIS,(void *)&_eel_fread);
  NSEEL_addfunctionex("fgets",2,(char *)_asm_generic2parm_retd,(char *)_asm_generic2parm_retd_end-(char *)_asm_generic2parm_retd,NSEEL_PProc_THIS,(void *)&_eel_fgets);
  NSEEL_addfunctionex("fgetc",1,(char *)_asm_generic1parm_retd,(char *)_asm_generic1parm_retd_end-(char *)_asm_generic1parm_retd,NSEEL_PProc_THIS,(void *)&_eel_fgetc);

  NSEEL_addfunctionex("fwrite",3,(char *)_asm_generic3parm_retd,(char *)_asm_generic3parm_retd_end-(char *)_asm_generic3parm_retd,NSEEL_PProc_THIS,(void *)&_eel_fwrite);
  NSEEL_addfunctionex("fprintf",2,(char *)_asm_generic2parm_retd,(char *)_asm_generic2parm_retd_end-(char *)_asm_generic2parm_retd,NSEEL_PProc_THIS,(void *)&_eel_fprintf);

  NSEEL_addfunctionex("fseek",3,(char *)_asm_generic3parm_retd,(char *)_asm_generic3parm_retd_end-(char *)_asm_generic3parm_retd,NSEEL_PProc_THIS,(void *)&_eel_fseek);
  NSEEL_addfunctionex("ftell",1,(char *)_asm_generic1parm_retd,(char *)_asm_generic1parm_retd_end-(char *)_asm_generic1parm_retd,NSEEL_PProc_THIS,(void *)&_eel_ftell);
  NSEEL_addfunctionex("feof",1,(char *)_asm_generic1parm_retd,(char *)_asm_generic1parm_retd_end-(char *)_asm_generic1parm_retd,NSEEL_PProc_THIS,(void *)&_eel_feof);
  NSEEL_addfunctionex("fflush",1,(char *)_asm_generic1parm_retd,(char *)_asm_generic1parm_retd_end-(char *)_asm_generic1parm_retd,NSEEL_PProc_THIS,(void *)&_eel_fflush);
  NSEEL_addfunctionex("fclose",1,(char *)_asm_generic1parm_retd,(char *)_asm_generic1parm_retd_end-(char *)_asm_generic1parm_retd,NSEEL_PProc_THIS,(void *)&_eel_fclose);
}

#endif
