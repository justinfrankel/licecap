#ifndef _CONTAINERS_
#define _CONTAINERS_

#ifdef WIN32
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 
#undef WINVER
#define WINVER 0x0501
#pragma warning(disable:4018 4267)	// size_t/signed/unsigned mismatch..
#pragma warning(disable:4800)		// if (pointer) ...
#pragma warning(disable:4805)		// Compare bool and BOOL.
#endif

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "../mutex.h"
#include "../wdlstring.h"
#include "../ptrlist.h"

#define FREE_NULL(p) {free(p);p=0;}
#define DELETE_NULL(p) {delete(p); p=0;}
#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)<(y)?(y):(x))
#define BOUNDED(x,lo,hi) ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))
#define CSTR_NOT_EMPTY(cStr) ((cStr) && (cStr)[0] != '\0')

#define MAKE_QUOTE(str) #str
#define MAKE_STR(str) MAKE_QUOTE(str)

#define PI 3.141592653589793238
#define AMP_DB 8.685889638065036553
#define IAMP_DB 0.11512925464970

inline double DBToAmp(double dB)
{
    return exp(IAMP_DB * dB);
}

inline double AmpToDB(double amp)
{
	return AMP_DB * log(fabs(amp));
}

#ifndef REMINDER
  #if defined WIN32
    // This enables: #pragma REMINDER("change this line!") with click-through from VC++.
    #define REMINDER(msg) message(__FILE__   "(" MAKE_STR(__LINE__) "): " msg)
  #else if defined __APPLE__
    #define REMINDER(msg) WARNING msg
  #endif
#endif

template <class T> inline void SWAP(T& a, T& b) 
{
    T tmp = a; a = b; b = tmp; 
}

typedef unsigned char BYTE;
class ByteChunk
{
public:

	ByteChunk() {}
	~ByteChunk() {}

	template <class T> inline int Put(const T* pVal) 
  {
		int n = mBytes.GetSize();
		mBytes.Resize(n + sizeof(T));
		memcpy(mBytes.Get() + n, (BYTE*) pVal, sizeof(T));
		return mBytes.GetSize();
	}

	template <class T> inline int Get(T* pVal, int startPos) 
  {
    int endPos = startPos + sizeof(T);
    if (startPos >= 0 && endPos <= mBytes.GetSize()) {
      memcpy((BYTE*) pVal, mBytes.Get() + startPos, sizeof(T));
      return endPos;
    }
    return -1;
	}

	inline int PutStr(const char* str) 
  {
    int slen = strlen(str);
		Put(&slen);
		int n = mBytes.GetSize();
		mBytes.Resize(n + slen);
		memcpy(mBytes.Get() + n, (BYTE*) str, slen);
		return mBytes.GetSize();
	}

	inline int GetStr(WDL_String* pStr, int startPos)
  {
		int len;
    int strStartPos = Get(&len, startPos);
    if (strStartPos >= 0) {
      int strEndPos = strStartPos + len;
      if (strEndPos <= mBytes.GetSize() && len > 0) {
        pStr->Set((char*) (mBytes.Get() + strStartPos), len);
      }
      return strEndPos;
    }
    return -1;
	}

  inline int PutBool(bool b)
  {
    int n = mBytes.GetSize();
    mBytes.Resize(n + 1);
    *(mBytes.Get() + n) = (BYTE) (b ? 1 : 0);
    return mBytes.GetSize();
  }

  inline int GetBool(bool* pB, int startPos)
  {
    int endPos = startPos + 1;
    if (startPos >= 0 && endPos <= mBytes.GetSize()) {
      BYTE byt = *(mBytes.Get() + startPos);
      *pB = (byt);
      return endPos;
    }
    return -1;
  }

  inline int PutChunk(ByteChunk* pRHS)
  {
    int n = mBytes.GetSize();
    int nRHS = pRHS->Size();
    mBytes.Resize(n + nRHS);
    memcpy(mBytes.Get() + n, pRHS->GetBytes(), nRHS);
    return mBytes.GetSize();
  }

  inline void Clear() 
  {
    mBytes.Resize(0);
  }

  inline int Size() 
  {
    return mBytes.GetSize();
  }

  inline int Resize(int newSize) 
  {
    int n = mBytes.GetSize();
    mBytes.Resize(newSize);
    if (newSize > n) {
      memset(mBytes.Get() + n, 0, (newSize - n));
    }
    return n;
  }

  inline BYTE* GetBytes()
  {
    return mBytes.Get();
  }

  inline bool IsEqual(ByteChunk* pRHS)
  {
    return (pRHS && pRHS->Size() == Size() && !memcmp(pRHS->GetBytes(), GetBytes(), Size()));
  }

private:

  WDL_TypedBuf<unsigned char> mBytes;
};

#endif
