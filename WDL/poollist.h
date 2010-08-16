/*
  WDL - poollist.h
  Copyright (C) 2006 and later, Cockos Incorporated

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
  


  This file defines a template class for managing WDL's time based resource pools. 

  It stores a sorted list of WDL_ResourcePool objects (keyed by filename), and allows the caller
  to retreive (and retain) a pool by filename, then later release the pool..

  The pool itself can contain a list of resources (objects) -- see rpool.h for more info.

  This is pretty esoteric. But we use it for some things.

*/


#include <stdlib.h>


#ifndef _WDL_POOLLIST_H_
#define _WDL_POOLLIST_H_

#include "rpool.h"

template<class RPTYPE> class WDL_PoolList
{
public:

  WDL_PoolList()
  {
  }
  ~WDL_PoolList()
  {
    int x;
    for (x = 0; x < pool.GetSize(); x ++) delete pool.Get(x);
  }

  WDL_ResourcePool<RPTYPE,char *, int> *GetPool(const char *filename)
  {
    WDL_MutexLock lock(&m_mutex);

    WDL_ResourcePool<RPTYPE,char *, int> tmp; 
    tmp.user1=(char *)filename;
    WDL_ResourcePool<RPTYPE,char *, int> *t=&tmp;
    WDL_ResourcePool<RPTYPE,char *, int> **_tmp=&t;

    if (pool.GetSize() && (_tmp = (WDL_ResourcePool<RPTYPE,char *, int>**)bsearch(_tmp,pool.GetList(),pool.GetSize(),sizeof(void *),_sortfunc)) && *_tmp)
    {
      t = *_tmp;
      t->user2++;
      return t;
    }

    t = new WDL_ResourcePool<RPTYPE,char *, int>;
    t->user1=strdup(filename);
    t->user2=1;

    pool.Add(t);

    qsort(pool.GetList(),pool.GetSize(),sizeof(void*),_sortfunc);

    return t;
  }

  void ReleasePool(WDL_ResourcePool<RPTYPE,char *, int> *tp)
  {
    if (!tp) return;
    WDL_MutexLock lock(&m_mutex);

    if (!--tp->user2)
    {
      int x;
      for (x = 0; x < pool.GetSize() && pool.Get(x) != tp; x ++);
      if (x<pool.GetSize()) 
      {
        pool.Delete(x);
      }
      free(tp->user1);
      delete tp;
      // remove from list
    }
  }

private:
  WDL_PtrList< WDL_ResourcePool<RPTYPE,char *, int> > pool;

  WDL_Mutex m_mutex;
  static int _sortfunc(const void *a, const void *b)
  {
    WDL_ResourcePool<RPTYPE,char *, int> *ta = *(WDL_ResourcePool<RPTYPE,char *, int> **)a;
    WDL_ResourcePool<RPTYPE,char *, int> *tb = *(WDL_ResourcePool<RPTYPE,char *, int> **)b;

    return stricmp(ta->user1,tb->user1);
  }
};


#endif