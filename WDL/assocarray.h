#ifndef _WDL_ASSOCARRAY_H_
#define _WDL_ASSOCARRAY_H_

#include "heapbuf.h"


// on all of these, if valdispose is set, the array will dispose of values as needed.
// if keydup/keydispose are set, copies of (any)key data will be made/destroyed as necessary


template <class KEY, class VAL> class WDL_AssocArray
{
public:

  WDL_AssocArray(int (*keycmp)(KEY *k1, KEY *k2), KEY (*keydup)(KEY)=0, void (*keydispose)(KEY)=0, void (*valdispose)(VAL)=0)
  {
    m_keycmp = keycmp;
    m_keydup = keydup;
    m_keydispose = keydispose;
    m_valdispose = valdispose;
  }
 
  ~WDL_AssocArray() 
  {
    DeleteAll();
  }

  VAL* GetPtr(KEY key, KEY *keyPtrOut=NULL)
  {
    bool ismatch = false;
    int i = LowerBound(key, &ismatch);
    if (ismatch)
    {
      KeyVal* kv = m_data.Get()+i;
      if (keyPtrOut) *keyPtrOut = kv->key;
      return &(kv->val);
    }
    return 0;
  }

  VAL Get(KEY key, VAL notfound=0)  
  {
    VAL* p = GetPtr(key);
    if (p) return *p;
    return notfound;
  }

  bool Exists(KEY key)
  {
    bool ismatch = false;
    LowerBound(key, &ismatch);
    return ismatch;
  }

  void Insert(KEY key, VAL val, KEY *keyPtrOut=NULL) 
  {
    bool ismatch = false;
    int i = LowerBound(key, &ismatch);
    if (ismatch)
    {
      KeyVal* kv = m_data.Get()+i;
      if (m_valdispose) m_valdispose(kv->val);
      kv->val = val;
      if (keyPtrOut) *keyPtrOut = kv->key;
    }
    else
    {
      KeyVal* kv = m_data.Resize(m_data.GetSize()+1)+i;
      memmove(kv+1, kv, (m_data.GetSize()-i-1)*sizeof(KeyVal));
      if (m_keydup) key = m_keydup(key);
      kv->key = key;
      kv->val = val;
      if (keyPtrOut) *keyPtrOut = key;
    }
  }

  void Delete(KEY key) 
  {
    bool ismatch = false;
    int i = LowerBound(key, &ismatch);
    if (ismatch)
    {
      KeyVal* kv = m_data.Get()+i;
      if (m_keydispose) m_keydispose(kv->key);
      if (m_valdispose) m_valdispose(kv->val);
      memmove(kv, kv+1, (m_data.GetSize()-i-1)*sizeof(KeyVal));
      m_data.Resize(m_data.GetSize()-1);
    }
  }

  void DeleteByIndex(int idx)
  {
    if (idx>=0&&idx<GetSize())
    {
      KeyVal* kv = m_data.Get()+idx;
      if (m_keydispose) m_keydispose(kv->key);
      if (m_valdispose) m_valdispose(kv->val);
      memmove(kv, kv+1, (m_data.GetSize()-idx-1)*sizeof(KeyVal));
      m_data.Resize(m_data.GetSize()-1);
    }
  }

  void DeleteAll(bool resizedown=false)
  {
    if (m_keydispose || m_valdispose)
    {
      int i;
      for (i = 0; i < m_data.GetSize(); ++i)
      {
        KeyVal* kv = m_data.Get()+i;
        if (m_keydispose) m_keydispose(kv->key);
        if (m_valdispose) m_valdispose(kv->val);
      }
    }
    m_data.Resize(0, resizedown);
  }

  int GetSize()
  {
    return m_data.GetSize();
  }

  VAL* EnumeratePtr(int i, KEY* key=0)
  {
    if (i >= 0 && i < m_data.GetSize()) 
    {
      KeyVal* kv = m_data.Get()+i;
      if (key) *key = kv->key;
      return &(kv->val);
    }
    return 0;
  }

  VAL Enumerate(int i, KEY* key=0, VAL notfound=0)
  {
    VAL* p = EnumeratePtr(i, key);
    if (p) return *p;
    return notfound; 
  }

  KEY ReverseLookup(VAL val, KEY notfound=0)
  {
    int i;
    for (i = 0; i < m_data.GetSize(); ++i)
    {
      KeyVal* kv = m_data.Get()+i;
      if (kv->val == val) return kv->key;
    }
    return notfound;
  }

  void ChangeKey(KEY oldkey, KEY newkey)
  {
    bool ismatch=false;
    int i = LowerBound(oldkey, &ismatch);
    if (ismatch)
    {
      KeyVal* kv = m_data.Get()+i;
      if (m_keydispose) m_keydispose(kv->key);
      if (m_keydup) newkey = m_keydup(newkey);
      kv->key = newkey;
      Resort();
    }
  }

  // fast add-block mode
  void AddUnsorted(KEY key, VAL val)
  {
    int i=m_data.GetSize();
    KeyVal* kv = m_data.Resize(i+1)+i;
    if (m_keydup) key = m_keydup(key);
    kv->key = key;
    kv->val = val;
  }

  void Resort()
  {
    if (m_data.GetSize()>1 && m_keycmp)
      qsort(m_data.Get(),m_data.GetSize(),sizeof(KeyVal),(int(*)(const void *,const void *))m_keycmp);
  }

  int LowerBound(KEY key, bool* ismatch)
  {
    int a = 0;
    int c = m_data.GetSize();
    while (a != c)
    {
      int b = (a+c)/2;
      int cmp = m_keycmp(&key, &m_data.Get()[b].key);
      if (cmp > 0) a = b+1;
      else if (cmp < 0) c = b;
      else
      {
        *ismatch = true;
        return b;
      }
    }
    *ismatch = false;
    return a;
  }

  int GetIdx(KEY key)
  {
    bool ismatch=false;
    int i = LowerBound(key, &ismatch);
    if (ismatch) return i;
    return -1;
  }

private:

  struct KeyVal
  {
    KEY key;
    VAL val;
  };
  WDL_TypedBuf<KeyVal> m_data;

  int (*m_keycmp)(KEY *k1, KEY *k2);
  KEY (*m_keydup)(KEY);
  void (*m_keydispose)(KEY);
  void (*m_valdispose)(VAL);

};


template <class VAL> class WDL_IntKeyedArray : public WDL_AssocArray<int, VAL>
{
public:

  WDL_IntKeyedArray(void (*valdispose)(VAL)=0) : WDL_AssocArray<int, VAL>(cmpint, NULL, NULL, valdispose) {}
  ~WDL_IntKeyedArray() {}

private:

  static int cmpint(int *i1, int *i2) { return *i1-*i2; }
};


template <class VAL> class WDL_StringKeyedArray : public WDL_AssocArray<const char *, VAL>
{
public:

  WDL_StringKeyedArray(bool caseSensitive=true, void (*valdispose)(VAL)=0) : WDL_AssocArray<const char*, VAL>(caseSensitive?cmpstr:cmpistr, dupstr, freestr, valdispose) {}
  
  ~WDL_StringKeyedArray() { }

private:

  static const char *dupstr(const char *s) { return strdup(s);  } // these might not be necessary but depending on the libc maybe...
  static int cmpstr(const char **s1, const char **s2) { return strcmp(*s1, *s2); }
  static int cmpistr(const char **a, const char **b) { return stricmp(*a,*b); }
  static void freestr(const char* s) { free((void*)s); }

public:
  static void freecharptr(char *p) { free(p); }


};


#endif

