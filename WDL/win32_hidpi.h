#include "wdltypes.h"

#ifdef _WIN32
#ifdef WDL_WIN32_HIDPI_IMPL
#ifndef _WDL_WIN32_HIDPI_H_IMPL
#define _WDL_WIN32_HIDPI_H_IMPL

WDL_WIN32_HIDPI_IMPL void *_WDL_dpi_func(void *p)
{
  static void *(WINAPI *__SetThreadDpiAwarenessContext)(void *);
  if (!__SetThreadDpiAwarenessContext)
  {
    HINSTANCE huser32 = LoadLibrary("user32.dll");
    if (huser32) *(void **)&__SetThreadDpiAwarenessContext = GetProcAddress(huser32,"SetThreadDpiAwarenessContext");
    if (!__SetThreadDpiAwarenessContext) *(UINT_PTR *)&__SetThreadDpiAwarenessContext = 1;
  }
  return (UINT_PTR)__SetThreadDpiAwarenessContext > 1 ? __SetThreadDpiAwarenessContext(p) : NULL;
}
#endif
#else
void *_WDL_dpi_func(void *p);
#endif
#endif

#ifndef _WDL_WIN32_HIDPI_H_
#define _WDL_WIN32_HIDPI_H_

static WDL_STATICFUNC_UNUSED void *WDL_dpi_enter_aware(int mode) // -1 = DPI_AWARENESS_CONTEXT_UNAWARE, -2=aware, -3=mm aware, -4=mmaware2, -5=gdiscaled
{
#ifdef _WIN32
  return _WDL_dpi_func((void *)(INT_PTR)mode);
#else
  return NULL;
#endif
}
static WDL_STATICFUNC_UNUSED void WDL_dpi_leave_aware(void **p)
{
#ifdef _WIN32
  if (p)
  {
    if (*p) _WDL_dpi_func(*p);
    *p = NULL;
  }
#endif
}

#ifdef __cplusplus
struct WDL_dpi_aware_scope {
#ifdef _WIN32
  WDL_dpi_aware_scope(int mode=-1) { c = mode ? WDL_dpi_enter_aware(mode) : NULL; }
  ~WDL_dpi_aware_scope() { if (c) WDL_dpi_leave_aware(&c); }
  void *c;
#else
  WDL_dpi_aware_scope() { }
#endif
};
#endif

#endif
