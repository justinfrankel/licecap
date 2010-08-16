/*
  WDL - scsrc.cpp
  Copyright (C) 2007, Cockos Incorporated

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
  



*/

#ifdef _WIN32
#include <windows.h>
#else
#include <stdlib.h>
#include <string.h>
#include <pthread.h>


static char *lstrcpyn(char *dest, const char *src, int l)
{
  if (l<1) return dest;

  char *dsrc=dest;
  while (--l > 0)
  {
    char p=*src++;
    if (!p) break;
    *dest++=p;
  }
  *dest++=0;

  return dsrc;
}

#endif
#include <math.h>

#include "scsrc.h"
#include "jnetlib/jnetlib.h"
#define ERR_NOLAME -600
#define ERR_CREATINGENCODER -599

#define ERR_TIMEOUT -4
#define ERR_CONNECT -5
#define ERR_AUTH -6

#define ST_OK 0
#define ST_CONNECTING 1
#define ERR_DISCONNECTED_AFTER_SUCCESS 32
WDL_ShoutcastSource::WDL_ShoutcastSource(const char *host, const char *pass, const char *name, bool pub, 
                                         const char *genre, const char *url,
                                         int nch, int srate, int kbps, const char *ircchan)
{
  JNL::open_socketlib();
  m_host.Set(host);
  m_pass.Set(pass);
  if (name) m_name.Set(name);
  if (url) m_url.Set(url);
  if (genre) m_genre.Set(genre);
  if (ircchan) m_ircchan.Set(ircchan);
  m_pub=pub;
  m_br=kbps;

  m_state=ST_CONNECTING; // go to 0 when we 
  m_nch=nch==2?2:1;
  m_bytesout=0;
  m_srate=srate;
  m_last_samples[0]=m_last_samples[1]=0.0;
  m_rspos=0.0;
  m_sendcon=0;

  m_needtitle=0;
  m_title[0]=0;
  m_titlecon=0;
  m_titlecon_start=0;
  m_encoder_splsin=0;
  m_encoder=new LameEncoder(m_srate,m_nch,m_br);
  int s=m_encoder->Status();
  if (s == 1) m_state=ERR_NOLAME;
  else if (s) m_state=ERR_CREATINGENCODER;

  if (s) { delete m_encoder; m_encoder=0; }

  m_sendcon_start=time(NULL);
  if (m_encoder)
  {
    m_sendcon = new JNL_Connection(JNL_CONNECTION_AUTODNS,65536,65536);
    WDL_String hb(m_host.Get());
    int port=8000;
    char *p=strstr(hb.Get(),":");
    if (p)
    {
      *p++=0;
      port = atoi(p);
      if (!port) port=8000;
    }
    m_sendcon->connect(hb.Get(), port+1);

    m_sendcon->send_string(m_pass.Get());
    m_sendcon->send_string("\r\n");
  }

}

WDL_ShoutcastSource::~WDL_ShoutcastSource()
{
  delete m_titlecon;
  delete m_sendcon;
  delete m_encoder;
}

int WDL_ShoutcastSource::GetStatus() // returns 0 if connected/connecting, >0 if disconnected, -1 if failed connect (or other error) from the start
{
  if (m_state<ST_OK || m_state >= ERR_DISCONNECTED_AFTER_SUCCESS) return m_state;
  return 0;
}

void WDL_ShoutcastSource::GetStatusText(char *buf, int bufsz) // gets status text
{
  if (m_state == ST_OK) wsprintf(buf,"Connected. Sent %u bytes",m_bytesout);
  else if (m_state == ST_CONNECTING) lstrcpyn(buf,"Connecting...",bufsz);
  else if (m_state == ERR_DISCONNECTED_AFTER_SUCCESS) wsprintf(buf,"Disconnected after sending %u bytes",m_bytesout);
  else if (m_state == ERR_AUTH) lstrcpyn(buf,"Error authenticating with server",bufsz);
  else if (m_state == ERR_CONNECT) lstrcpyn(buf,"Error connecting to server",bufsz);
  else if (m_state == ERR_TIMEOUT) lstrcpyn(buf,"Timed out connecting to server",bufsz);
  else if (m_state == ERR_CREATINGENCODER) lstrcpyn(buf,"Error creating encoder",bufsz);
  else if (m_state == ERR_NOLAME) 
  #ifdef _WIN32
    lstrcpyn(buf,"Error loading lame_enc.dll",bufsz);
  #else
    lstrcpyn(buf,"Error loading libmp3lame.dylib",bufsz);
  #endif
  else lstrcpyn(buf,"Error creating encoder",bufsz);

}

void WDL_ShoutcastSource::SetCurTitle(const char *title)
{
  m_titlemutex.Enter();
  lstrcpyn(m_title,title,sizeof(m_title));
  m_needtitle=true;
  m_titlemutex.Leave();
  
}

void WDL_ShoutcastSource::OnSamples(float **samples, int nch, int chspread, int frames, double srate)
{
  m_samplemutex.Enter();

  if (fabs(srate-m_srate)<1.0)
  {
    float *ob=m_rsbuf.Resize(frames*m_nch,false);
    int x=frames;
    int a=0;
    if (nch < 2 && m_nch < 2)
    {
      while (x--)
      {
        *ob++ = samples[0][a];
        a+=chspread;
      }
    }
    else if (nch < 2 && m_nch > 1)
    {
      while (x--)
      {
        ob[0] = ob[1] = samples[0][a];
        ob+=2;
        a+=chspread;
      }
    }
    else if (nch > 1 && m_nch > 1)
    {
      while (x--)
      {
        *ob++ = samples[0][a];
        *ob++ = samples[1][a];
        a+=chspread;
      }
    }
    else 
    {
      while (x--)
      {
        *ob++ = (samples[0][a]+samples[1][a])*0.5f;
        a+=chspread;
      }
    }
    if (m_samplequeue.Available() < sizeof(float)*m_nch*96000*4 &&
        m_encoder && m_encoder->outqueue.Available() < 256*1024)
      m_samplequeue.Add(m_rsbuf.Get(),frames*m_nch*sizeof(float));
  }
  else
  {
    double rateratio = srate/m_srate;

    double fracpos=m_rspos;
    int outlen=(int) ((frames+fracpos)/rateratio);
    float *outptr=m_rsbuf.Resize(outlen*m_nch,false);

    int x;
    int loffs=-1000;
    double ls[2]={m_last_samples[0],m_last_samples[1]};
    for (x = 0; x < outlen; x ++)
    {
      int ioffs=(int)fracpos;
      double fp=fracpos-ioffs;

      ioffs *= chspread;

      if (ioffs>0)
      {
        ls[0]=samples[0][ioffs-chspread];
        ls[1]=samples[nch-1][ioffs-chspread];
      }

      if (m_nch>1)
      {
        *outptr++ = (float) (samples[0][ioffs] * fp + ls[0]*(1-fp));
        if (nch>1)
          *outptr++ = (float) (samples[1][ioffs] * fp + ls[1]*(1-fp));
        else
        {
          *outptr = outptr[-1];
          outptr++;
        }
      }
      else
      {
        if (nch>1)
        {
          *outptr++ = (float) ((((samples[0][ioffs]+samples[1][ioffs]) * fp + (ls[0]+ls[1])*(1-fp)))*0.5);
        }
        else
          *outptr++ = (float) (samples[0][ioffs] * fp + ls[0]*(1-fp));

      }

      fracpos += rateratio;
    }
    m_last_samples[0]=samples[0][frames-1];
    m_last_samples[1]=samples[nch-1][frames-1];

    if (m_samplequeue.Available() < sizeof(float)*m_nch*96000*4 &&
        m_encoder && m_encoder->outqueue.Available() < 256*1024)
      m_samplequeue.Add(m_rsbuf.Get(),outlen*m_nch*sizeof(float));

    m_rspos=fracpos - floor(fracpos);
  }

  m_samplemutex.Leave();
}


static void url_encode(char *in, char *out, int max_out)
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

int WDL_ShoutcastSource::RunStuff()
{
  int ret=0;
  // run connection

  if (m_sendcon)
  {
    if (m_encoder && m_encoder_splsin > 48000*60*60*3) // every 3 hours, reinit the mp3 encoder
    {
      m_encoder_splsin=0;

      WDL_Queue tmp;
      if (m_encoder->outqueue.GetSize()) tmp.Add(m_encoder->outqueue.Get(),m_encoder->outqueue.GetSize());

      delete m_encoder;

      int s=2;
      m_encoder=new LameEncoder(m_srate,m_nch,m_br);
      if (m_encoder && !(s=m_encoder->Status()))
      {
        // copy out queue from m_encoder to newnc
        if (tmp.GetSize()) m_encoder->outqueue.Add(tmp.Get(),tmp.GetSize());
      }
      else 
      {
        if (s == 1) m_state=ERR_NOLAME;
        else if (s) m_state=ERR_CREATINGENCODER;
        delete m_encoder;
        m_encoder=0;
      }

    }

    if (m_encoder)
    {
      // encode data from m_samplequeue
      int n=32;
      int maxl=1152*2;
      m_workbuf.Resize(maxl);
      while (n--)
      {
        m_samplemutex.Enter();
        int d=m_samplequeue.Available()/sizeof(float);
        if (d > 0)
        {
          if (d>maxl) d=maxl;
          m_samplequeue.GetToBuf(0,m_workbuf.Get(),d*sizeof(float));
          m_samplequeue.Advance(d*sizeof(float));
        }
        m_samplemutex.Leave();

        if (!d) break;

        m_encoder_splsin+=d/m_nch;
        m_encoder->Encode(m_workbuf.Get(),d/m_nch,1);
        ret=1;
      }

      if (m_encoder->outqueue.Available() > 128*1024)
      {
        m_encoder->outqueue.Advance(m_encoder->outqueue.Available()-64*1024);
      }

      if (m_state==ST_OK)
      {
        int mb=m_encoder->outqueue.Available();
        int mb2=m_sendcon->send_bytes_available();
        if (mb>mb2) mb=mb2;
        if (mb>0)
        {
          m_bytesout+=mb;
          m_sendcon->send_bytes(m_encoder->outqueue.Get(),mb);
          m_encoder->outqueue.Advance(mb);
          m_encoder->outqueue.Compact();
          ret=1;
        }
      }
    }

    m_sendcon->run();
    int s = m_sendcon->get_state();

    if (m_state == ST_CONNECTING)
    {
      if (m_sendcon->recv_lines_available()>0)
      {
        char buf[4096];
        m_sendcon->recv_line(buf, 4095);
        if (strcmp(buf, "OK2")) 
        {
          m_state=ERR_AUTH;
          delete m_sendcon;
          m_sendcon=0;
        }
        else 
        {
          m_state=ST_OK;
          m_sendcon->send_string("icy-name:");
          m_sendcon->send_string(m_name.Get());
          m_sendcon->send_string("\r\n");
          m_sendcon->send_string("icy-genre:");
          m_sendcon->send_string(m_genre.Get());
          m_sendcon->send_string("\r\n");
          m_sendcon->send_string("icy-pub:");
          m_sendcon->send_string(m_pub ? "1":"0");
          m_sendcon->send_string("\r\n");
          m_sendcon->send_string("icy-br:");
          char buf[64];
          wsprintf(buf,"%d",m_br);
          m_sendcon->send_string(buf);
          m_sendcon->send_string("\r\n");
          m_sendcon->send_string("icy-url:");
          m_sendcon->send_string(m_url.Get());
          m_sendcon->send_string("\r\n");
          if (m_ircchan.Get()[0])
          {
            m_sendcon->send_string("icy-irc:");
            m_sendcon->send_string(m_ircchan.Get());
            m_sendcon->send_string("\r\n");
          }
          m_sendcon->send_string("icy-genre:");
          m_sendcon->send_string(m_genre.Get());
          m_sendcon->send_string("\r\n");
          m_sendcon->send_string("icy-reset:1\r\n\r\n");
        }
      }
    }
    switch (s) 
    {
      case JNL_Connection::STATE_ERROR:
      case JNL_Connection::STATE_CLOSED:
        if (m_state==ST_OK) m_state=ERR_DISCONNECTED_AFTER_SUCCESS; 
        else if (m_state>ST_OK && m_state < ERR_DISCONNECTED_AFTER_SUCCESS) m_state = ERR_CONNECT;

        delete m_sendcon;
        m_sendcon=0;
      break;
      default:
        if (m_state > ST_OK && m_state < ERR_DISCONNECTED_AFTER_SUCCESS && time(NULL) > m_sendcon_start+30)
        {
          m_state=ERR_TIMEOUT;
          delete m_sendcon;
          m_sendcon=0;
        }

      break;
    }
  }

  if (m_titlecon)
  {
    if (m_titlecon->run() || time(NULL) > m_titlecon_start+30)
    {
      delete m_titlecon;
      m_titlecon=0;
    }
  }

  if (m_needtitle && m_state==ST_OK)
  {
    char title[512];
    m_titlemutex.Enter();
    url_encode(m_title,title,sizeof(title)-1);
    m_needtitle=false;
    m_titlemutex.Leave();


    WDL_String url;
    url.Append("http://");
    url.Append(m_host.Get());
    url.Append("/admin.cgi?pass=");
    url.Append(m_pass.Get());
    url.Append("&mode=updinfo&song=");
    
    url.Append(title);

    delete m_titlecon;
    m_titlecon=new JNL_HTTPGet;
    m_titlecon->addheader("User-Agent:Cockos Reacast (Mozilla)");
    m_titlecon->addheader("Accept:*/*");
    m_titlecon->connect(url.Get());
    m_titlecon_start=time(NULL);
  }

  return ret;
}