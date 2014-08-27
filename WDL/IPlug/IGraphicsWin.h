#ifndef _IGRAPHICSWIN_
#define _IGRAPHICSWIN_

#include "IGraphicsLice.h"

#include <windows.h>
#include <windowsx.h>
#include <winuser.h>

class IGraphicsWin : public IGraphicsLice
{
public:

	IGraphicsWin(IPlugBase* pPlug, int w, int h, int refreshFPS = 0);
	virtual ~IGraphicsWin();

  void SetHInstance(HINSTANCE hInstance) { mHInstance = hInstance; }
  
  void Resize(int w, int h);
  bool DrawScreen(IRECT* pR);  
  
	void* OpenWindow(void* pParentWnd);
	void CloseWindow();
	bool WindowIsOpen() { return (mPlugWnd); }

	void HostPath(WDL_String* pPath); 
  void PluginPath(WDL_String* pPath);

	void PromptForFile(WDL_String* pFilename, EFileAction action = kFileOpen, char* dir = "",
    char* extensions = "");   // extensions = "txt wav" for example.

  bool PromptForColor(IColor* pColor, char* prompt = "");
	void PromptUserInput(IControl* pControl, IParam* pParam);

  bool OpenURL(const char* url, 
    const char* msgWindowTitle = 0, const char* confirmMsg = 0, const char* errMsgOnFailure = 0);

    // Specialty use!
	void* GetWindow() { return mPlugWnd; }
  HWND GetParentWindow() { return mParentWnd; }
  HWND GetMainWnd();
  void SetMainWndClassName(char* name) { mMainWndClassName.Set(name); }
  void GetMainWndClassName(char* name) { strcpy(name, mMainWndClassName.Get()); }
  IRECT GetWindowRECT();
  void SetWindowTitle(char* str);

  bool DrawIText(IText* pText, char* str, IRECT* pR);

protected:
  LICE_IBitmap* OSLoadBitmap(int ID, const char* name);

private:
  bool mFontActive;
  IColor mActiveFontColor;

  HINSTANCE mHInstance;
  HWND mPlugWnd, mParamEditWnd;
	// Ed = being edited manually.
	IControl* mEdControl;
	IParam* mEdParam;
	WNDPROC mDefEditProc;
	int mParamEditMsg;
	int mIdleTicks;
	COLORREF* mCustomColorStorage;

  DWORD mPID;
  HWND mParentWnd, mMainWnd;
  WDL_String mMainWndClassName;

public:

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK ParamEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  static BOOL CALLBACK FindMainWindow(HWND hWnd, LPARAM lParam);
};

////////////////////////////////////////

#endif