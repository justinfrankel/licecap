/*
  WDL - sharedpool.h
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
  


  This file defines a template for a simple object pool. 

  Objects are keyed by string (filename, or otherwise). The caller can add or get an object,
  increase or decrease its reference count (if it reaches zero the object is deleted).
  

  If you delete the pool itself, all objects are deleted, regardless of their reference count.

*/


#ifndef _WDL_SHAREDPOOL_H_
#define _WDL_SHAREDPOOL_H_

#include "ptrlist.h"

template<class OBJ> class WDL_SharedPool
{
  public:
    WDL_SharedPool() { }
    ~WDL_SharedPool() { m_list.Empty(true); }

    void Add(OBJ *obj, const char *n) // no need to AddRef() after add, it defaults to a reference count of 1.
    {
      if (obj && n)
      {
        int x;
        for(x=0;x<m_list.GetSize();x++) if (stricmp(m_list.Get(x)->name,n)>0) break;
        m_list.Insert(x,new Ent(obj,n));
      }
    }

    OBJ *Get(const char *s)
    {
      Ent tmp(NULL,s);
      Ent *t=&tmp;
      Ent **_tmp=&t;

      if (m_list.GetSize() && (_tmp = (Ent **)bsearch(_tmp,m_list.GetList(),m_list.GetSize(),sizeof(void *),_sortfunc)) && (t=*_tmp))
      {
        if (t->obj)
        {
          t->refcnt++;
          return t->obj;
        }
      }

      return 0;

    }

    void AddRef(OBJ *obj)
    {
      int x;
      if (obj) for (x = 0; x < m_list.GetSize(); x ++)
      {
        if (m_list.Get(x)->obj == obj)
        {          
          m_list.Get(x)->refcnt++;
          return;
        }
      }
    }

    void Release(OBJ *obj)
    {
      int x;
      if (obj) for (x = 0; x < m_list.GetSize(); x ++)
      {
        if (m_list.Get(x)->obj == obj)
        {          
          if (!--m_list.Get(x)->refcnt) m_list.Delete(x,true);
          return;
        }
      }
    }

    OBJ *EnumItems(int x)
    {
      Ent *e=m_list.Get(x);
      return e?e->obj:NULL;
    }

  private:

    class Ent
    {
      public:
        Ent(OBJ *o, const char *n) { obj=o; name=strdup(n); refcnt=1; }
        ~Ent() { delete obj; free(name); }

        OBJ *obj;
        int refcnt;
        char *name;
    };



    static int _sortfunc(const void *a, const void *b)
    {
      Ent *ta = *(Ent **)a;
      Ent *tb = *(Ent **)b;

      return stricmp(ta->name,tb->name);
    }
    
    WDL_PtrList<Ent> m_list;


};


#endif//_WDL_SHAREDPOOL_H_