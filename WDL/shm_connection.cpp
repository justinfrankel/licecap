#include <windows.h>

#include "shm_connection.h"

#if WDL_SHM_IO_BUFFER_SIZE < 1024
#error too small WDL_SHM_IO_BUFFER_SIZE
#endif

WDL_SHM_Connection::WDL_SHM_Connection(const char *uniquestring, int rdIndex/*0 or 1*/)
{
  m_mapsize=0;
  m_recvmsg_curseq=0;
  m_file=INVALID_HANDLE_VALUE;
  m_filemap=NULL;
  m_mem=NULL;
  m_events[0]=m_events[1]=NULL;
  m_rdIndex=rdIndex;


  char buf[512];
  GetTempPath(sizeof(buf)-4,buf);
  if (!buf[0]) lstrcpyn(buf,"C:\\",32);
  if (buf[strlen(buf)-1] != '/' && buf[strlen(buf)-1] != '\\') strcat(buf,"\\");
  m_tempfn.Set(buf);
  m_tempfn.Append("WDL_SHM_");
  m_tempfn.Append(uniquestring);
  m_tempfn.Append(".tmp");

  DeleteFile(m_tempfn.Get()); // this will fail if another process has it locked

  m_file=CreateFile(m_tempfn.Get(),GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE ,
        NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_TEMPORARY,NULL);
  
  if (m_file != INVALID_HANDLE_VALUE && ((m_mapsize=GetFileSize(m_file,NULL)) < 2048+8+2 || m_mapsize == 0xFFFFFFFF))
  {
    WDL_HeapBuf tmp;
    m_mapsize=(WDL_SHM_IO_BUFFER_SIZE+4)*2 + 2;
    memset(tmp.Resize(m_mapsize),0,m_mapsize);
    DWORD d;
    WriteFile(m_file,tmp.Get(),tmp.GetSize(),&d,NULL);
  }
  if (m_file!=INVALID_HANDLE_VALUE)
    m_filemap=CreateFileMapping(m_file,NULL,PAGE_READWRITE,0,0,NULL);

  if (m_filemap)
  {
    m_mem=(unsigned char *)MapViewOfFile(m_filemap,FILE_MAP_WRITE,0,0,0);

    WDL_String tmp;
    tmp.Set("WDL_SHM_");
    tmp.Append(uniquestring);
    tmp.Append(".1");
    m_events[!rdIndex]=CreateEvent(NULL,false,false,tmp.Get());

    tmp.Set("WDL_SHM_");
    tmp.Append(uniquestring);
    tmp.Append(".2");
    m_events[!!rdIndex]=CreateEvent(NULL,false,false,tmp.Get());
  }

  if (m_mem)
  {
    int rdoffs2=(m_mapsize-2)/2;
    m_mem[!!m_rdIndex] = 1;
  }
}


WDL_SHM_Connection::~WDL_SHM_Connection()
{
  if (m_mem) 
  {
    int rdoffs2=(m_mapsize-2)/2;
    m_mem[!!m_rdIndex] = 0;

    UnmapViewOfFile(m_mem);
  }
  if (m_filemap) CloseHandle(m_filemap);
  if (m_file != INVALID_HANDLE_VALUE) CloseHandle(m_file);
  DeleteFile(m_tempfn.Get());

  if (m_events[0]) CloseHandle(m_events[0]);
  if (m_events[1]) CloseHandle(m_events[1]);
}

int WDL_SHM_Connection::Recv(void **buf, bool remove, int maxWait)
{
  if (!m_mem||m_mapsize<1024) return 0;
  unsigned char *rdptr = (m_mem+2 + (m_mapsize-2)/2);

  bool firstPass=false;

  int curblocksize=*(int *)rdptr;
  while (m_recvmsg_curseq||!m_recvmsg.GetSize()) 
  {
    if (!curblocksize)
    {
      if (maxWait < 0) return 0;

      DWORD prev = GetTickCount();
      WaitForSingleObject(m_events[1],max(maxWait,0));     
      maxWait -= GetTickCount()-prev;
      if (!(curblocksize=*(int *)rdptr)) continue;
    }
    // add block
    if (curblocksize >= 5 && curblocksize<=(int)(m_mapsize-2)/2-4) // clear this block out if invalid
    {
      unsigned char *block = rdptr+4;

      unsigned char type=*block;
      int seq=*(int *)(block+1);
      // 1 byte flag (flag=1, start of block, flag=0 middle, flag=2 = end)
      // 4 byte sequence ID
      // 4 byte size verify (if end)
      if (type == 1)  // new message
      {
        m_recvmsg_curseq = seq;
        m_recvmsg.Clear();
      }

      if (seq == m_recvmsg_curseq && (type != 2 || curblocksize>=9))
      {
        m_recvmsg.Add(type==2 ? block+9 : block+5, type==2 ? (curblocksize-9):(curblocksize-5));
        if (type == 2)
        {
          m_recvmsg_curseq=0;
          if (m_recvmsg.GetSize()!= *(int *)(block+5)) m_recvmsg.Clear(); // verify size!
        }
      }
    }

    curblocksize = *(int *)rdptr = 0; // done reading the message
    // todo: signal some event that we've read?
  }

  *buf = m_recvmsg.Get();
  int rv=m_recvmsg.GetSize();

  if (remove) m_recvmsg.Advance(rv); 

  return rv;
  
}