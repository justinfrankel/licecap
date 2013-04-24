#include "IGraphicsWin.h"
#include "IControl.h"
#include "Log.h"
#include <wininet.h>

#pragma warning(disable:4244)	// Pointer size cast mismatch.
#pragma warning(disable:4312)	// Pointer size cast mismatch.
#pragma warning(disable:4311)	// Pointer size cast mismatch.

static int nWndClassReg = 0;
static const char* wndClassName = "IPlugWndClass";
static double sFPS = 0.0;

#define MAX_PARAM_LEN 32
#define PARAM_EDIT_ID 99

enum EParamEditMsg {
	kNone,
	kEditing,
	kUpdate,
	kCancel,
	kCommit
};

#define IPLUG_TIMER_ID 2

inline IMouseMod GetMouseMod(WPARAM wParam)
{
	return IMouseMod((wParam & MK_LBUTTON), (wParam & MK_RBUTTON), 
        (wParam & MK_SHIFT), (wParam & MK_CONTROL), GetKeyState(VK_MENU) < 0);
}

// static
LRESULT CALLBACK IGraphicsWin::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (msg == WM_CREATE) {
    LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LPARAM) (lpcs->lpCreateParams));
		int mSec = int(1000.0 / sFPS);
		SetTimer(hWnd, IPLUG_TIMER_ID, mSec, NULL);
		return 0;
	}

	IGraphicsWin* pGraphics = (IGraphicsWin*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
	char txt[MAX_PARAM_LEN];
	double v;

	if (!pGraphics || hWnd != pGraphics->mPlugWnd) {
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	if (pGraphics->mParamEditWnd && pGraphics->mParamEditMsg == kEditing) {
		if (msg == WM_RBUTTONDOWN) {
			pGraphics->mParamEditMsg = kCancel;
			return 0;
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	switch (msg) {

		case WM_TIMER: {
			if (wParam == IPLUG_TIMER_ID) {

				if (pGraphics->mParamEditWnd && pGraphics->mParamEditMsg != kNone) {
					switch (pGraphics->mParamEditMsg) {
            case kUpdate: {
							pGraphics->mEdParam->GetDisplayForHost(txt);
							SendMessage(pGraphics->mParamEditWnd, WM_SETTEXT, 0, (LPARAM) txt);
							break;
            }
            case kCommit: {
							SendMessage(pGraphics->mParamEditWnd, WM_GETTEXT, MAX_PARAM_LEN, (LPARAM) txt);
              if (pGraphics->mEdParam->GetNDisplayTexts()) {
                int vi = 0;
                pGraphics->mEdParam->MapDisplayText(txt, &vi);
                v = (double) vi;
							}
							else {
								v = atof(txt);
								if (pGraphics->mEdParam->DisplayIsNegated()) {
									v = -v;
								}
							}
							pGraphics->mEdControl->SetValueFromUserInput(pGraphics->mEdParam->GetNormalized(v));
							// Fall through.
            }
            case kCancel:
			      {
							SetWindowLongPtr(pGraphics->mParamEditWnd, GWLP_WNDPROC, (LPARAM) pGraphics->mDefEditProc);
							DestroyWindow(pGraphics->mParamEditWnd);
							pGraphics->mParamEditWnd = 0;
							pGraphics->mEdParam = 0;
							pGraphics->mEdControl = 0;
							pGraphics->mDefEditProc = 0;
            }
            break;            
          }
					pGraphics->mParamEditMsg = kNone;
					return 0;
				}

        IRECT dirtyR;
        if (pGraphics->IsDirty(&dirtyR)) {
          RECT r = { dirtyR.L, dirtyR.T, dirtyR.R, dirtyR.B };
          InvalidateRect(hWnd, &r, FALSE);
          UpdateWindow(hWnd);
          if (pGraphics->mParamEditWnd) {
            pGraphics->mParamEditMsg = kUpdate;
          }
        }
      }
      return 0;
    }
    case WM_RBUTTONDOWN: {
			if (pGraphics->mParamEditWnd) {
				pGraphics->mParamEditMsg = kCancel;
				return 0;
			}
			// Else fall through.
    }
    case WM_LBUTTONDOWN: {
			SetCapture(hWnd);
			pGraphics->OnMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), &GetMouseMod(wParam));
			return 0;
    }
    case WM_MOUSEMOVE: {
			if (!(wParam & (MK_LBUTTON | MK_RBUTTON))) { 
        if (pGraphics->OnMouseOver(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), &GetMouseMod(wParam))) {
          TRACKMOUSEEVENT eventTrack = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hWnd, HOVER_DEFAULT };
          TrackMouseEvent(&eventTrack);
        }
			}
      else
			if (GetCapture() == hWnd) {
				pGraphics->OnMouseDrag(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), &GetMouseMod(wParam));
			}
			return 0;
    }
    case WM_MOUSELEAVE: {
      pGraphics->OnMouseOut();
      return 0;
    }
    case WM_LBUTTONUP:
    case WM_RBUTTONUP: {
      ReleaseCapture();
			pGraphics->OnMouseUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), &GetMouseMod(wParam));
			return 0;
    }
    case WM_LBUTTONDBLCLK: {
      if (pGraphics->OnMouseDblClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), &GetMouseMod(wParam))) {
        SetCapture(hWnd);
      }
			return 0;
    }
		case WM_MOUSEWHEEL: {
			int d = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
			int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
			RECT r;
			GetWindowRect(hWnd, &r);
			pGraphics->OnMouseWheel(x - r.left, y - r.top, &GetMouseMod(wParam), d);
			return 0;
		}

    case WM_KEYDOWN:
    {
      bool ok = true;
      int key;     

      if (wParam == VK_SPACE) key = KEY_SPACE;
      else if (wParam == VK_UP) key = KEY_UPARROW;
      else if (wParam == VK_DOWN) key = KEY_DOWNARROW;
      else if (wParam == VK_LEFT) key = KEY_LEFTARROW;
      else if (wParam == VK_RIGHT) key = KEY_RIGHTARROW;
      else if (wParam >= '0' && wParam <= '9') key = KEY_DIGIT_0+wParam-'0';
      else if (wParam >= 'A' && wParam <= 'Z') key = KEY_ALPHA_A+wParam-'A';
      else if (wParam >= 'a' && wParam <= 'z') key = KEY_ALPHA_A+wParam-'a';
      else ok = false;

      if (ok)
      {
        POINT p;
        GetCursorPos(&p); 
        ScreenToClient(hWnd, &p);
        pGraphics->OnKeyDown(p.x, p.y, key);
      }
    }
    return 0;

		case WM_PAINT: {
      RECT r;
      if (GetUpdateRect(hWnd, &r, FALSE)) {
        IRECT ir(r.left, r.top, r.right, r.bottom);
        pGraphics->Draw(&ir);
      }
			return 0;
		}

		//case WM_CTLCOLOREDIT: {
		//	// An edit control just opened.
		//	HDC dc = (HDC) wParam;
		//	SetTextColor(dc, ///);
		//	return 0;
		//}

		case WM_CLOSE: {
			pGraphics->CloseWindow();
			return 0;
		}
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

// static 
LRESULT CALLBACK IGraphicsWin::ParamEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	IGraphicsWin* pGraphics = (IGraphicsWin*) GetWindowLongPtr(hWnd, GWLP_USERDATA);

	if (pGraphics && pGraphics->mParamEditWnd && pGraphics->mParamEditWnd == hWnd) 
  {
		switch (msg) {
			case WM_KEYDOWN: {
				if (wParam == VK_RETURN) {
					pGraphics->mParamEditMsg = kCommit;
					return 0;
				}
				break;
			}
			case WM_SETFOCUS: {
				pGraphics->mParamEditMsg = kEditing;
				break;
			}
			case WM_KILLFOCUS: {
				pGraphics->mParamEditMsg = kNone;
				break;
			}
			case WM_COMMAND: {
				switch HIWORD(wParam) {
					case CBN_SELCHANGE: {
						if (pGraphics->mParamEditWnd) {
							pGraphics->mParamEditMsg = kCommit;
							return 0;
						}
					}
				
				}
				break;	// Else let the default proc handle it.
			}
		}
		return CallWindowProc(pGraphics->mDefEditProc, hWnd, msg, wParam, lParam);
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

IGraphicsWin::IGraphicsWin(IPlugBase* pPlug, int w, int h, int refreshFPS)
:	IGraphicsLice(pPlug, w, h, refreshFPS), mPlugWnd(0), mParamEditWnd(0), 
  mPID(0), mParentWnd(0), mMainWnd(0), mCustomColorStorage(0),
	mEdControl(0), mEdParam(0), mDefEditProc(0), mParamEditMsg(kNone), mIdleTicks(0),
  mFontActive(false), mHInstance(0)
{
}

IGraphicsWin::~IGraphicsWin()
{
	CloseWindow();
  FREE_NULL(mCustomColorStorage);
}

LICE_IBitmap* IGraphicsWin::OSLoadBitmap(int ID, const char* name)
{
  const char* ext = name+strlen(name)-1;
  while (ext > name && *ext != '.') --ext;
  ++ext;

  if (!stricmp(ext, "png")) return _LICE::LICE_LoadPNGFromResource(mHInstance, ID, 0);
  if (!stricmp(ext, "jpg") || !stricmp(ext, "jpeg")) return _LICE::LICE_LoadJPGFromResource(mHInstance, ID, 0);
  return 0;
}

void GetWindowSize(HWND pWnd, int* pW, int* pH)
{
  if (pWnd) {
    RECT r;
    GetWindowRect(pWnd, &r);
    *pW = r.right - r.left;
    *pH = r.bottom - r.top;
  }
  else {
    *pW = *pH = 0;
  }
}

bool IsChildWindow(HWND pWnd)
{
  if (pWnd) {
    int style = GetWindowLong(pWnd, GWL_STYLE);
    int exStyle = GetWindowLong(pWnd, GWL_EXSTYLE);
    return ((style & WS_CHILD) && !(exStyle & WS_EX_MDICHILD));
  }
  return false;
}

#define SETPOS_FLAGS SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE

void IGraphicsWin::Resize(int w, int h)
{
  int dw = w - Width(), dh = h - Height();
  IGraphics::Resize(w, h);
  if (mDrawBitmap) {
    mDrawBitmap->resize(w, h);
  }
  if (WindowIsOpen()) {
    HWND pParent = 0, pGrandparent = 0;
    int w = 0, h = 0, parentW = 0, parentH = 0, grandparentW = 0, grandparentH = 0;
    GetWindowSize(mPlugWnd, &w, &h);
    if (IsChildWindow(mPlugWnd)) {
      pParent = GetParent(mPlugWnd);
      GetWindowSize(pParent, &parentW, &parentH);
      if (IsChildWindow(pParent)) {
        pGrandparent = GetParent(pParent);
        GetWindowSize(pGrandparent, &grandparentW, &grandparentH);
      }
    }
    SetWindowPos(mPlugWnd, 0, 0, 0, w + dw, h + dh, SETPOS_FLAGS);
    if (pParent) {
      SetWindowPos(pParent, 0, 0, 0, parentW + dw, parentH + dh, SETPOS_FLAGS);
    }
    if (pGrandparent) {
      SetWindowPos(pGrandparent, 0, 0, 0, grandparentW + dw, grandparentH + dh, SETPOS_FLAGS);
    }
          
    RECT r = { 0, 0, w, h };
    InvalidateRect(mPlugWnd, &r, FALSE);
  }
}

bool IGraphicsWin::DrawScreen(IRECT* pR)
{
  PAINTSTRUCT ps;
  HWND hWnd = (HWND) GetWindow();
	HDC dc = BeginPaint(hWnd, &ps);
	BitBlt(dc, pR->L, pR->T, pR->W(), pR->H(), mDrawBitmap->getDC(), pR->L, pR->T, SRCCOPY);
	EndPaint(hWnd, &ps);
	return true;
}

void* IGraphicsWin::OpenWindow(void* pParentWnd)
{
  int x = 0, y = 0, w = Width(), h = Height();
  mParentWnd = (HWND) pParentWnd;

	if (mPlugWnd) {
		RECT pR, cR;
		GetWindowRect((HWND) pParentWnd, &pR);
		GetWindowRect(mPlugWnd, &cR);
		CloseWindow();
		x = cR.left - pR.left;
		y = cR.top - pR.top;
		w = cR.right - cR.left;
		h = cR.bottom - cR.top;
	}

	if (nWndClassReg++ == 0) {
		WNDCLASS wndClass = { CS_DBLCLKS, WndProc, 0, 0, mHInstance, 0, LoadCursor(NULL, IDC_ARROW), 0, 0, wndClassName };
		RegisterClass(&wndClass);
	}

  sFPS = FPS();
  mPlugWnd = CreateWindow(wndClassName, "IPlug", WS_CHILD | WS_VISIBLE, // | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		x, y, w, h, (HWND) pParentWnd, 0, mHInstance, this);
	//SetWindowLong(mPlugWnd, GWL_USERDATA, (LPARAM) this);

	if (!mPlugWnd && --nWndClassReg == 0) {
		UnregisterClass(wndClassName, mHInstance);
	}
  else {
    SetAllControlsDirty();
  }  

	return mPlugWnd;
}

#define MAX_CLASSNAME_LEN 128
void GetWndClassName(HWND hWnd, WDL_String* pStr)
{
    char cStr[MAX_CLASSNAME_LEN];
    cStr[0] = '\0';
    GetClassName(hWnd, cStr, MAX_CLASSNAME_LEN);
    pStr->Set(cStr);
}

BOOL CALLBACK IGraphicsWin::FindMainWindow(HWND hWnd, LPARAM lParam)
{
    IGraphicsWin* pGraphics = (IGraphicsWin*) lParam;
    if (pGraphics) {
        DWORD wPID;
        GetWindowThreadProcessId(hWnd, &wPID);
        WDL_String str;
        GetWndClassName(hWnd, &str);
        if (wPID == pGraphics->mPID && !strcmp(str.Get(), pGraphics->mMainWndClassName.Get())) {
            pGraphics->mMainWnd = hWnd;
            return FALSE;   // Stop enumerating.
        }
    }
    return TRUE;
}

HWND IGraphicsWin::GetMainWnd()
{
    if (!mMainWnd) {
        if (mParentWnd) {
            HWND parentWnd = mParentWnd;
            while (parentWnd) {
                mMainWnd = parentWnd;
                parentWnd = GetParent(mMainWnd);
            }
            GetWndClassName(mMainWnd, &mMainWndClassName);
        }
        else
        if (CSTR_NOT_EMPTY(mMainWndClassName.Get())) {
            mPID = GetCurrentProcessId();
            EnumWindows(FindMainWindow, (LPARAM) this);
        }
    }
    return mMainWnd;
}

#define TOOLWIN_BORDER_W 6
#define TOOLWIN_BORDER_H 23

IRECT IGraphicsWin::GetWindowRECT()
{
    if (mPlugWnd) {
        RECT r;
        GetWindowRect(mPlugWnd, &r);
        r.right -= TOOLWIN_BORDER_W;
        r.bottom -= TOOLWIN_BORDER_H;
        return IRECT(r.left, r.top, r.right, r.bottom);
    }
    return IRECT();
}

void IGraphicsWin::SetWindowTitle(char* str)
{
    SetWindowText(mPlugWnd, str);
}

void IGraphicsWin::CloseWindow()
{
	if (mPlugWnd) {
		DestroyWindow(mPlugWnd);
		mPlugWnd = 0;

		if (--nWndClassReg == 0) {
			UnregisterClass(wndClassName, mHInstance);
		}
	}
}

#define PARAM_EDIT_W 36
#define PARAM_EDIT_H 16
#define PARAM_EDIT_H_PER_ENUM 36
#define PARAM_LIST_MIN_W 24
#define PARAM_LIST_W_PER_CHAR 8

void IGraphicsWin::PromptUserInput(IControl* pControl, IParam* pParam)
{
	if (!pControl || !pParam || mParamEditWnd) {
		return;
	}

	IRECT* pR = pControl->GetRECT();
	int cX = int(pR->MW()), cY = int(pR->MH());
  char currentText[MAX_PARAM_NAME_LEN];
  pParam->GetDisplayForHost(currentText);

  int n = pParam->GetNDisplayTexts();
  if (n) {
    int i, currentIdx = -1;
		int w = PARAM_LIST_MIN_W, h = PARAM_EDIT_H_PER_ENUM * (n + 1);
		for (i = 0; i < n; ++i) {
      const char* str = pParam->GetDisplayText(i);
      w = MAX(w, PARAM_LIST_MIN_W + strlen(str) * PARAM_LIST_W_PER_CHAR);
			if (!strcmp(str, currentText)) {
				currentIdx = i;
			}
		}			

		mParamEditWnd = CreateWindow("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
			cX - w/2, cY, w, h, mPlugWnd, (HMENU) PARAM_EDIT_ID, mHInstance, 0);

		for (i = 0; i < n; ++i) {
      const char* str = pParam->GetDisplayText(i);
			SendMessage(mParamEditWnd, CB_ADDSTRING, 0, (LPARAM) str);
		}
		SendMessage(mParamEditWnd, CB_SETCURSEL, (WPARAM) currentIdx, 0);
	}
	else {
		int w = PARAM_EDIT_W, h = PARAM_EDIT_H;
		mParamEditWnd = CreateWindow("EDIT", currentText, WS_CHILD | WS_VISIBLE | ES_CENTER | ES_MULTILINE,
			cX - w/2, cY - h/2, w, h, mPlugWnd, (HMENU) PARAM_EDIT_ID, mHInstance, 0);
	}

	mDefEditProc = (WNDPROC) SetWindowLongPtr(mParamEditWnd, GWLP_WNDPROC, (LONG_PTR) ParamEditProc);
  SetWindowLong(mParamEditWnd, GWLP_USERDATA, (LPARAM) this);

  IText txt;
	HFONT font = CreateFont(txt.mSize, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, txt.mFont);
	SendMessage(mParamEditWnd, WM_SETFONT, (WPARAM) font, 0);
	//DeleteObject(font);	

	mEdControl = pControl;
	mEdParam = pParam;	
}

#define MAX_PATH_LEN 256

void GetModulePath(HMODULE hModule, WDL_String* pPath) 
{
  pPath->Set("");
	char pathCStr[MAX_PATH_LEN];
	pathCStr[0] = '\0';
	if (GetModuleFileName(hModule, pathCStr, MAX_PATH_LEN)) {
    int s = -1;
    for (int i = 0; i < strlen(pathCStr); ++i) {
      if (pathCStr[i] == '\\') {
        s = i;
      }
    }
    if (s >= 0 && s + 1 < strlen(pathCStr)) {
      pPath->Set(pathCStr, s + 1);
    }
  }
}

void IGraphicsWin::HostPath(WDL_String* pPath)
{
  GetModulePath(0, pPath);
}

void IGraphicsWin::PluginPath(WDL_String* pPath)
{
  GetModulePath(mHInstance, pPath);
}

void IGraphicsWin::PromptForFile(WDL_String* pFilename, EFileAction action, char* dir, char* extensions)
{
  pFilename->Set("");
	if (!WindowIsOpen()) { 
		return;
	}

  WDL_String pathStr;
	char fnCStr[MAX_PATH_LEN], dirCStr[MAX_PATH_LEN];
	fnCStr[0] = '\0';
	dirCStr[0] = '\0';
	if (CSTR_NOT_EMPTY(dir)) {
  pathStr.Set(dir);
		strcpy(dirCStr, dir);
	}
  else {
    HostPath(&pathStr);
  }
	
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = mPlugWnd;
	ofn.lpstrFile = fnCStr;
	ofn.nMaxFile = MAX_PATH_LEN - 1;
	ofn.lpstrInitialDir = dirCStr;
	ofn.Flags = OFN_PATHMUSTEXIST;

    //if (!extensions.empty()) {
        //static char extStr[256];
        //static char defExtStr[16];
        //int i, j, p;

        //for (j = 0, p = 0; j < extensions.size(); ++j) {
        //    extStr[p++] = extensions[j++];
        //}
        //extStr[p++] = '\0';

        //StrVector exts = SplitStr(extensions);
        //for (i = 0, p = 0; i < exts.size(); ++i) {
        //    const std::string& ext = exts[i];
        //    if (i) {
        //        extStr[p++] = ';';
        //    }
        //    extStr[p++] = '*';
        //    extStr[p++] = '.';
        //    for (j = 0; j < ext.size(); ++j) {
        //        extStr[p++] = ext[j];
        //    }
        //}
        //extStr[p++] = '\0';
        //extStr[p++] = '\0';
        //ofn.lpstrFilter = extStr;
        //
        //strcpy(defExtStr, exts.front().c_str());
        //ofn.lpstrDefExt = defExtStr;
    //}

    bool rc = false;
    switch (action) {
        case kFileSave:
            ofn.Flags |= OFN_OVERWRITEPROMPT;
            rc = GetSaveFileName(&ofn);
            break;

        case kFileOpen:     
        default:
            ofn.Flags |= OFN_FILEMUSTEXIST;
    	    rc = GetOpenFileName(&ofn);
            break;
    }

    if (rc) {
        pFilename->Set(ofn.lpstrFile);
    }
}

UINT_PTR CALLBACK CCHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    if (uiMsg == WM_INITDIALOG && lParam) {
        CHOOSECOLOR* cc = (CHOOSECOLOR*) lParam;
        if (cc && cc->lCustData) {
            char* str = (char*) cc->lCustData;
            SetWindowText(hdlg, str);
        }
    }
    return 0;
}

bool IGraphicsWin::PromptForColor(IColor* pColor, char* prompt)
{
    if (!mPlugWnd) {
        return false;
    }
    if (!mCustomColorStorage) {
        mCustomColorStorage = (COLORREF*) calloc(16, sizeof(COLORREF));
    }
    CHOOSECOLOR cc;
    memset(&cc, 0, sizeof(CHOOSECOLOR));
    cc.lStructSize = sizeof(CHOOSECOLOR);
    cc.hwndOwner = mPlugWnd;
    cc.rgbResult = RGB(pColor->R, pColor->G, pColor->B);
    cc.lpCustColors = mCustomColorStorage;
    cc.lCustData = (LPARAM) prompt;
    cc.lpfnHook = CCHookProc;
    cc.Flags = CC_RGBINIT | CC_ANYCOLOR | CC_FULLOPEN | CC_SOLIDCOLOR | CC_ENABLEHOOK;
    
    if (ChooseColor(&cc)) {
        pColor->R = GetRValue(cc.rgbResult);
        pColor->G = GetGValue(cc.rgbResult);
        pColor->B = GetBValue(cc.rgbResult);
        return true;
    }
    return false;
}

#define MAX_INET_ERR_CODE 32
bool IGraphicsWin::OpenURL(const char* url, 
  const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
  if (confirmMsg && MessageBox(mPlugWnd, confirmMsg, msgWindowTitle, MB_YESNO) != IDYES) {
    return false;
  }
  DWORD inetStatus = 0;
  if (InternetGetConnectedState(&inetStatus, 0)) {
    if ((int) ShellExecute(mPlugWnd, "open", url, 0, 0, SW_SHOWNORMAL) > MAX_INET_ERR_CODE) {
      return true;
    }
  }
  if (errMsgOnFailure) {
    MessageBox(mPlugWnd, errMsgOnFailure, msgWindowTitle, MB_OK);
  }
  return false;
}

bool IGraphicsWin::DrawIText(IText* pText, char* str, IRECT* pR)
{
  if (!str || str == '\0') {
      return true;
  }

	HDC pDC = mDrawBitmap->getDC();
	
  bool setColor = (pText->mColor != mActiveFontColor);
	if (!mFontActive) {
		int h = pText->mSize;
		int esc = 10 * pText->mOrientation;
		int wt = (pText->mStyle == IText::kStyleBold ? FW_BOLD : 0);
		int it = (pText->mStyle == IText::kStyleItalic ? 1 : 0);
		HFONT font = CreateFont(h, 0, esc, esc, wt, it, 0, 0, 0, 0, 0, 0, 0, pText->mFont);
		SelectObject(pDC, font);  // leak?
		SetBkMode(pDC, TRANSPARENT);
		mFontActive = true;
    setColor = true;
	}
	
	if (setColor) {
		SetTextColor(pDC, RGB(pText->mColor.R, pText->mColor.G, pText->mColor.B));
		mActiveFontColor = pText->mColor;
	}
    
	UINT fmt = DT_NOCLIP;
	switch(pText->mAlign) {
		case IText::kAlignCenter:	fmt |= DT_CENTER; break;
		case IText::kAlignFar:		fmt |= DT_RIGHT; break;
		case IText::kAlignNear:
		default:					        fmt |= DT_LEFT; break;
	}

	RECT R = { pR->L, pR->T, pR->R, pR->B };
	return !!DrawText(pDC, str, strlen(str), &R, fmt);
}

