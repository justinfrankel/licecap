// used for HDC/HGDIOBJ pooling (to avoid excess heap use), used by swell-gdi.mm and swell-gdi-generic.cpp

static WDL_Mutex *m_ctxpool_mutex;
static HDC__ *m_ctxpool;
static int m_ctxpool_size;
static HGDIOBJ__ *m_objpool;
static int m_objpool_size;

HDC__ *SWELL_GDP_CTX_NEW()
{
  if (!m_ctxpool_mutex) m_ctxpool_mutex=new WDL_Mutex;
  
  HDC__ *p=NULL;
  if (m_ctxpool)
  {
    m_ctxpool_mutex->Enter();
    if ((p=m_ctxpool))
    { 
      m_ctxpool=p->_next;
      m_ctxpool_size--;
      memset(p,0,sizeof(HDC__));
    }
    m_ctxpool_mutex->Leave();
  }
  if (!p) 
  {
//    printf("alloc ctx\n");
    p=(HDC__ *)calloc(sizeof(HDC__)+128,1); // extra space in case things want to use it (i.e. swell-gdi-lice does)
  }
  return p;
}
static void SWELL_GDP_CTX_DELETE(HDC__ *p)
{
  if (!m_ctxpool_mutex) m_ctxpool_mutex=new WDL_Mutex;

  if (p && m_ctxpool_size<100)
  {
    m_ctxpool_mutex->Enter();
    p->_next = m_ctxpool;
    m_ctxpool = p;
    m_ctxpool_size++;
    m_ctxpool_mutex->Leave();
  }
  else 
  {
  //  printf("free ctx\n");
    free(p);
  }
}
static HGDIOBJ__ *GDP_OBJECT_NEW()
{
  HGDIOBJ__ *p=NULL;
  if (m_objpool)
  {
    if (!m_ctxpool_mutex) m_ctxpool_mutex=new WDL_Mutex;
    m_ctxpool_mutex->Enter();
    if ((p=m_objpool))
    {
      m_objpool = p->_next;
      m_objpool_size--;
      memset(p,0,sizeof(HGDIOBJ__));
    }
    m_ctxpool_mutex->Leave();
  }
  if (!p) 
  {
    //   printf("alloc obj\n");
    p=(HGDIOBJ__ *)calloc(sizeof(HGDIOBJ__),1);    
  }
  return p;
}
static void GDP_OBJECT_DELETE(HGDIOBJ__ *p)
{
  if (p && m_objpool_size<200)
  {
    if (!m_ctxpool_mutex) m_ctxpool_mutex=new WDL_Mutex;
    m_ctxpool_mutex->Enter();
    p->_next = m_objpool;
    m_objpool = p;
    m_objpool_size++;
    m_ctxpool_mutex->Leave();
  }
  else
  {
    //    printf("free obj\n");
    free(p);
  }
}
