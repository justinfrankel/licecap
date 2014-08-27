/*
** JNetLib
** Copyright (C) 2000-2003 Nullsoft, Inc.
** Author: Justin Frankel
** File: webserver.cpp - Generic simple webserver baseclass
** License: see jnetlib.h
** see test.cpp for an example of how to use this class
*/

#ifdef _WIN32
#include <windows.h>
#endif
#include "jnetlib.h"
#include "webserver.h"

class WS_ItemList 
{
  public:
    WS_ItemList() { m_size=0; m_list=0; }
    ~WS_ItemList() { ::free(m_list); }

    void *Add(void *i);
    void *Get(int w);
    void Del(int idx);
    int GetSize(void) { return m_size; }
  protected:
    void **m_list;
    int m_size;
};

class WS_conInst
{
public:
  WS_conInst(JNL_IConnection *c, int which_port);
  ~WS_conInst();

  // these will be used by WebServerBaseClass::onConnection yay
  JNL_HTTPServ m_serv;
  IPageGenerator *m_pagegen;


  int m_port; // port this came in on
  time_t m_connect_time;
};



void *WS_ItemList::Add(void *i)
{
  if (!m_list || !(m_size&31))
  {
    m_list=(void**)::realloc(m_list,sizeof(void*)*(m_size+32));
  }
  m_list[m_size++]=i;
  return i;
}
void *WS_ItemList::Get(int w) { if (w >= 0 && w < m_size) return m_list[w]; return NULL; }

void WS_ItemList::Del(int idx)
{
  if (m_list && idx >= 0 && idx < m_size)
  {
    m_size--;
    if (idx != m_size) ::memcpy(m_list+idx,m_list+idx+1,sizeof(void *)*(m_size-idx));
    if (!(m_size&31)&&m_size) // resize down
    {
      m_list=(void**)::realloc(m_list,sizeof(void*)*m_size);
    }
  }
}


WS_conInst::WS_conInst(JNL_IConnection *c, int which_port) : m_serv(c), m_port(which_port)
{
  m_pagegen=0;
  time(&m_connect_time);
}

WS_conInst::~WS_conInst()
{
  delete m_pagegen;
}

WebServerBaseClass::~WebServerBaseClass()
{
  int x;
  for (x = 0; x < m_connections->GetSize(); x ++)
  {
    delete (WS_conInst *)m_connections->Get(x);
  }
  delete m_connections;
  for (x = 0; x < m_listeners->GetSize(); x ++)
  {
    delete (JNL_Listen *)m_listeners->Get(x);
  }
  delete m_listeners;
}

WebServerBaseClass::WebServerBaseClass()
{
  m_connections=new WS_ItemList;
  m_listeners=new WS_ItemList;
  m_listener_rot=0;
  m_timeout_s=30;
  m_max_con=100;
}


void WebServerBaseClass::setMaxConnections(int max_con)
{
  m_max_con=max_con;
}

void WebServerBaseClass::setRequestTimeout(int timeout_s)
{
  m_timeout_s=timeout_s;
}

int WebServerBaseClass::addListenPort(int port, unsigned int which_interface)
{
  removeListenPort(port);

  JNL_Listen *p=new JNL_Listen(port,which_interface);
  m_listeners->Add((void *)p);
  if (p->is_error()) return -1;
  return 0;
}

void WebServerBaseClass::removeListenPort(int port)
{
  int x;
  for (x = 0; x < m_listeners->GetSize(); x ++)
  {
    JNL_Listen *p=(JNL_Listen *)m_listeners->Get(x);
    if (p->port()==port)
    {
      delete p;
      m_listeners->Del(x);
      break;
    }
  }
}

void WebServerBaseClass::removeListenIdx(int idx)
{
  if (idx >= 0 && idx < m_listeners->GetSize())
  {
    JNL_Listen *p=(JNL_Listen *)m_listeners->Get(idx);
    delete p;
    m_listeners->Del(idx);
  }
}

int WebServerBaseClass::getListenPort(int idx, int *err)
{
  JNL_Listen *p=(JNL_Listen *)m_listeners->Get(idx);
  if (p)
  {
    if (err) *err=p->is_error();
    return p->port();
  }
  return 0;
}

void WebServerBaseClass::attachConnection(JNL_IConnection *con, int port)
{
  m_connections->Add((void*)new WS_conInst(con,port));
}

void WebServerBaseClass::run(void)
{
  int nl;
  if (m_connections->GetSize() < m_max_con && (nl=m_listeners->GetSize()))
  {
    JNL_Listen *l=(JNL_Listen *)m_listeners->Get(m_listener_rot++ % nl);
    JNL_IConnection *c=l->get_connect();
    if (c)
    {
//      char buf[512];
//      sprintf(buf,"got new connection at %.3f",GetTickCount()/1000.0);
//      OutputDebugString(buf);
      attachConnection(c,l->port());
    }
  }
  int x;
  for (x = 0; x < m_connections->GetSize(); x ++)
  {
    WS_conInst *ci = (WS_conInst *)m_connections->Get(x);
    int rv = run_connection(ci);

    if (rv<0)
    {
      JNL_IConnection *c=ci->m_serv.steal_con();
      if (c) 
      {
        if (c->get_state() == JNL_Connection::STATE_CONNECTED)
          attachConnection(c,ci->m_port);
        else delete c;
      }
    }

    if (rv)
    {
      delete ci;
      m_connections->Del(x--);
    }
  }
}

int WebServerBaseClass::run_connection(WS_conInst *con)
{
  int s=con->m_serv.run();
  if (s < 0)
  {
    // m_serv.geterrorstr()
    return 1;
  }
  if (s < 2)
  {
    // return 1 if we timed out
    return time(NULL)-con->m_connect_time > m_timeout_s;    
  }
  if (s < 3)
  {
    con->m_pagegen=onConnection(&con->m_serv,con->m_port);
    return 0;
  }
  if (s < 4)
  {
    if (!con->m_pagegen) 
    {
      if (con->m_serv.canKeepAlive()) return -1;

      return !con->m_serv.bytes_inqueue();
    }
    char buf[16384];
    int l=con->m_serv.bytes_cansend();
    if (l > 0)
    {
      if (l > sizeof(buf))l=sizeof(buf);
      l=con->m_pagegen->GetData(buf,l);
      if (l < (con->m_pagegen->IsNonBlocking() ? 0 : 1)) // if nonblocking, this is l < 0, otherwise it's l<1
      {
        if (con->m_serv.canKeepAlive()) return -1;
        return !con->m_serv.bytes_inqueue();
      }
      if (l>0)
        con->m_serv.write_bytes(buf,l);
    }
    return 0;
  }
  if (con->m_serv.canKeepAlive()) return -1;
  return 1; // we're done by this point
}



void WebServerBaseClass::url_encode(const char *in, char *out, int max_out)
{
  while (*in && max_out > 4)
  {
    if ((*in >= 'A' && *in <= 'Z')||
	      (*in >= 'a' && *in <= 'z')||
	      (*in >= '0' && *in <= '9')|| *in == '.' || *in == '_' || *in == '-') 
    {
      *out++=*in++;
      max_out--;
    }
    else
	  {
  	  int i=*in++;
      *out++ = '%';
      int b=(i>>4)&15;
      if (b < 10) *out++='0'+b;
      else *out++='A'+b-10;
      b=i&15;
      if (b < 10) *out++='0'+b;
      else *out++='A'+b-10;
      max_out-=3;
	  }
  }
  *out=0;
}


void WebServerBaseClass::url_decode(const char *in, char *out, int maxlen)
{
  while (*in && maxlen>1)
  {
    if (*in == '+') 
    {
      in++;
      *out++=' ';
    }
	  else if (*in == '%' && in[1] != '%' && in[1])
	  {
		  int a=0;
		  int b=0;
		  for ( b = 0; b < 2; b ++)
		  {
			  int r=in[1+b];
			  if (r>='0'&&r<='9') r-='0';
			  else if (r>='a'&&r<='z') r-='a'-10;
			  else if (r>='A'&&r<='Z') r-='A'-10;
			  else break;
			  a*=16;
			  a+=r;
		  }
		  if (b < 2) *out++=*in++;
		  else { *out++=a; in += 3;}
	  }
	  else *out++=*in++;
	  maxlen--;
  }
  *out=0;
}





void WebServerBaseClass::base64decode(const char *src, char *dest, int destsize)
{
  int accum=0;
  int nbits=0;
  while (*src)
  {
    int x=0;
    char c=*src++;
    if (c >= 'A' && c <= 'Z') x=c-'A';
    else if (c >= 'a' && c <= 'z') x=c-'a' + 26;
    else if (c >= '0' && c <= '9') x=c-'0' + 52;
    else if (c == '+') x=62;
    else if (c == '/') x=63;
    else break;

    accum <<= 6;
    accum |= x;
    nbits += 6;   

    while (nbits >= 8)
    {
      if (--destsize<=0) break;
      nbits-=8;
      *dest++ = (char)((accum>>nbits)&0xff);
    }

  }
  *dest=0;
}

void WebServerBaseClass::base64encode(const char *in, char *out)
{
  char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int shift = 0;
  int accum = 0;

  while (*in)
  {
    if (*in)
    {
      accum <<= 8;
      shift += 8;
      accum |= *in++;
    }
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

int WebServerBaseClass::parseAuth(const char *auth_header, char *out, int out_len)//returns 0 on unknown auth, 1 on basic
{
  const char *authstr=auth_header;
  *out=0;
  if (!auth_header || !*auth_header) return 0;
  while (*authstr == ' ') authstr++;
  if (strnicmp(authstr,"basic ",6)) return 0;
  authstr+=6;
  while (*authstr == ' ') authstr++;
  base64decode(authstr,out,out_len);
  return 1;
}
