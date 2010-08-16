/*
    WDL - ptrlist.h
    Copyright (C) 2005 and later, Cockos Incorporated
  
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

/*

  This file provides a simple templated class for a list of pointers. By default this list
  doesn't free any of the pointers, but you can call Empty(true) or Delete(x,true) to delete the pointer,
  or you can use Empty(true,free) etc to call free (or any other function).

  Note: on certain compilers, instantiating with WDL_PtrList<void> bla; will give a warning, since
  the template will create code for "delete (void *)x;" which isn't technically valid. Oh well.
 
*/

#ifndef _WDL_PTRLIST_H_
#define _WDL_PTRLIST_H_

#include "heapbuf.h"

template<class PTRTYPE> class WDL_PtrList 
{
  public:
    WDL_PtrList()
    {
    }
    ~WDL_PtrList()
    {
    }

    PTRTYPE **GetList() { return (PTRTYPE**)m_hb.Get(); }
    PTRTYPE *Get(int index) { if (GetList() && index >= 0 && index < GetSize()) return GetList()[index]; return NULL; }
    int GetSize(void) { return m_hb.GetSize()/sizeof(PTRTYPE *); }  

    int Find(PTRTYPE *p)
    {
      int x;
      if (p) for (x = 0; x < GetSize(); x ++) if (Get(x) == p) return x;
      return -1;
    }

    PTRTYPE *Add(PTRTYPE *item)
    {
      int s=GetSize();
      m_hb.Resize((s+1)*sizeof(PTRTYPE*));
      return Set(s,item);
    }

    PTRTYPE *Set(int index, PTRTYPE *item) 
    { 
      if (index >= 0 && index < GetSize() && GetList()) return GetList()[index]=item;
      return NULL;
    }

    PTRTYPE *Insert(int index, PTRTYPE *item)
    {
      int s=GetSize();
      m_hb.Resize((s+1)*sizeof(PTRTYPE*));

      if (index<0) index=0;
      else if (index > s) index=s;
      
      int x;
      for (x = s; x > index; x --)
      {
        GetList()[x]=GetList()[x-1];
      }
      return (GetList()[x] = item);
    }

    void Delete(int index, bool wantDelete=false, void (*delfunc)(void *)=NULL)
    {
      PTRTYPE **list=GetList();
      int size=GetSize();
      if (list && index >= 0 && index < size)
      {
        if (wantDelete)
        {
          if (delfunc) delfunc(Get(index));
          else delete Get(index);
        }
        if (index < --size) memcpy(list+index,list+index+1,sizeof(PTRTYPE *)*(size-index));
        m_hb.Resize(size * sizeof(PTRTYPE*));
      }
    }
    void Empty(bool wantDelete=false, void (*delfunc)(void *)=NULL)
    {
      if (wantDelete)
      {
        int x;
        for (x = GetSize()-1; x >= 0; x --)
        {
          if (delfunc) delfunc(Get(x));
          else delete Get(x);
          m_hb.Resize(x*sizeof(PTRTYPE *),false);
        }
      }
      m_hb.Resize(0);
    }

  private:
    WDL_HeapBuf m_hb;
};

#endif

