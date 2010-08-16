#ifndef _IPLUGSTRUCTS_
#define _IPLUGSTRUCTS_

#include "Containers.h"

// Abstracting the graphics made it easy to go ahead and abstract the OS ... 
// the cost is this crap redefining some basic stuff.

struct IBitmap 
{
	void* mData;
	int W, H, N;		// N = number of states (for multibitmaps).
	IBitmap(void* pData = 0, int w = 0, int h = 0, int n = 1) : mData(pData), W(w), H(h), N(n) {}
};

struct IColor 
{
	int A, R, G, B;
	IColor(int a = 255, int r = 0, int g = 0, int b = 0) : A(a), R(r), G(g), B(b) {} 
    bool operator==(const IColor& rhs) { return (rhs.A == A && rhs.R == R && rhs.G == G && rhs.B == B); }
    bool operator!=(const IColor& rhs) { return !operator==(rhs); }
    bool Empty() const { return A == 0 && R == 0 && G == 0 && B == 0; }
    void Clamp() { A = MIN(A, 255); R = MIN(R, 255); G = MIN(G, 255); B = MIN(B, 255); }
};

const IColor COLOR_TRANSPARENT(0, 0, 0, 0);
const IColor COLOR_BLACK(255, 0, 0, 0);
const IColor COLOR_GRAY(255, 127, 127, 127);
const IColor COLOR_WHITE(255, 255, 255, 255);
const IColor COLOR_RED(255, 255, 0, 0);
const IColor COLOR_GREEN(255, 0, 255, 0);
const IColor COLOR_BLUE(255, 0, 0, 255);
const IColor COLOR_YELLOW(255, 255, 255, 0);
const IColor COLOR_ORANGE(255, 255, 127, 0);

struct IChannelBlend 
{
	enum EBlendMethod {
		kBlendNone,		// Copy over whatever is already there, but look at src alpha.
		kBlendClobber,	// Copy completely over whatever is already there.
		kBlendAdd,
        kBlendColorDodge,
		// etc
	};
	EBlendMethod mMethod;
	float mWeight;

	IChannelBlend(EBlendMethod method = kBlendNone, float weight = 1.0f) : mMethod(method), mWeight(weight) {}
};

const int DEFAULT_TEXT_SIZE = 14;
const IColor DEFAULT_TEXT_COLOR = COLOR_BLACK;
const char* const DEFAULT_FONT = "Arial";
const int FONT_LEN = 32;

struct IText 
{
	char mFont[FONT_LEN];
	int mSize;
	IColor mColor;
	enum EStyle { kStyleNormal, kStyleBold, kStyleItalic } mStyle;
	enum EAlign { kAlignNear, kAlignCenter, kAlignFar } mAlign;
	int mOrientation;   // Degrees ccwise from normal.

	IText(int size = DEFAULT_TEXT_SIZE, const IColor* pColor = 0, char* font = 0,
		EStyle style = kStyleNormal, EAlign align = kAlignCenter, int orientation = 0)
    :	mSize(size), mColor(pColor ? *pColor : DEFAULT_TEXT_COLOR), //mFont(font ? font : DEFAULT_FONT),
        mStyle(style), mAlign(align), mOrientation(orientation) 
    {
        strcpy(mFont, (font ? font : DEFAULT_FONT));     
    }

    IText(const IColor* pColor) 
	:	mSize(DEFAULT_TEXT_SIZE), mColor(*pColor), //mFont(DEFAULT_FONT), 
        mStyle(kStyleNormal), mAlign(kAlignCenter), mOrientation(0)
    {
        strcpy(mFont, DEFAULT_FONT);     
    }

 //   bool operator==(const IText& rhs) const;
 //   bool operator!=(const IText& rhs) const { return !operator==(rhs); }
 //	bool operator<(const IText& rhs) const;	// For sorting.
};

struct IRECT 
{ 
	int L, T, R, B;

	IRECT() { L = T = R = B = 0; }
	IRECT(int l, int t, int r, int b) : L(l), R(r), T(t), B(b) {} 
	IRECT(int x, int y, IBitmap* pBitmap) : L(x), T(y), R(x + pBitmap->W), B(y + pBitmap->H / pBitmap->N) {}

	bool Empty() const {
		return (L == 0 && T == 0 && R == 0 && B == 0); 
	}
  void Clear() {
      L = T = R = B = 0;
  }
  bool operator==(const IRECT& rhs) const {
      return (L == rhs.L && T == rhs.T && R == rhs.R && B == rhs.B);
  }
  bool operator!=(const IRECT& rhs) const {
      return !(*this == rhs);
  }

  inline int W() const { return R - L; }
  inline int H() const { return B - T; }
  inline float MW() const { return 0.5f * (float) (L + R); }
  inline float MH() const { return 0.5f * (float) (T + B); }

	inline IRECT Union(IRECT* pRHS) {
		if (Empty()) { return *pRHS; }
		if (pRHS->Empty()) { return *this; }
		return IRECT(MIN(L, pRHS->L), MIN(T, pRHS->T), MAX(R, pRHS->R), MAX(B, pRHS->B));
	}
	inline IRECT Intersect(IRECT* pRHS) {
		if (Intersects(pRHS)) {
			return IRECT(MAX(L, pRHS->L), MAX(T, pRHS->T), MIN(R, pRHS->R), MIN(B, pRHS->B));
		}
		return IRECT();
	}
	inline bool Intersects(IRECT* pRHS) {
		return (!Empty() && !pRHS->Empty() && R >= pRHS->L && L < pRHS->R && B >= pRHS->T && T < pRHS->B);
	}
	inline bool Contains(IRECT* pRHS) {
		return (!Empty() && !pRHS->Empty() && pRHS->L >= L && pRHS->R <= R && pRHS->T >= T && pRHS->B <= B);
	}
	inline bool Contains(int x, int y) {
		return (!Empty() && x >= L && x < R && y >= T && y < B);
	}

  void Clank(IRECT* pRHS) {
    if (L < pRHS->L) {
      R = MIN(pRHS->R - 1, R + pRHS->L - L);
      L = pRHS->L;
    }
    if (T < pRHS->T) {
      B = MIN(pRHS->B - 1, B + pRHS->T - T);
      T = pRHS->T;
    }
    if (R >= pRHS->R) {
      L = MAX(pRHS->L, L - (R - pRHS->R + 1));
      R = pRHS->R - 1;
    }
    if (B >= pRHS->B) {
      T = MAX(pRHS->T, T - (B - pRHS->B + 1));
      B = pRHS->B - 1;
    }
  }
};

struct IMouseMod 
{
	bool L, R, S, C, A;
	IMouseMod(bool l = false, bool r = false, bool s = false, bool c = false, bool a = false)
    : L(l), R(r), S(s), C(c), A(a) {} 
};

struct IMidiMsg
{
	int mOffset;
	BYTE mStatus, mData1, mData2;

  enum EStatusMsg {
		kNone = 0,
		kNoteOff = 8,
		kNoteOn = 9,
		kPolyAftertouch = 10,
		kControlChange = 11,
		kProgramChange = 12,
		kChannelAftertouch = 13,
		kPitchWheel = 14
	};

  enum EControlChangeMsg {
    kModWheel = 1,
    kBreathController = 2,
    kUndefined003 = 3,
    kFootController = 4,
    kPortamentoTime = 5,
    kChannelVolume = 7,
    kBalance = 8,
    kUndefined009 = 9,
    kPan = 10,
    kExpressionController = 11,
    kEffectControl1 = 12,
    kEffectControl2 = 13,
    kUndefined014 = 14,
    kUndefined015 = 15,
    kGeneralPurposeController1 = 16,
    kGeneralPurposeController2 = 17,
    kGeneralPurposeController3 = 18,
    kGeneralPurposeController4 = 19,
    kUndefined020 = 20,
    kUndefined021 = 21,
    kUndefined022 = 22,
    kUndefined023 = 23,
    kUndefined024 = 24,
    kUndefined025 = 25,
    kUndefined026 = 26,
    kUndefined027 = 27,
    kUndefined028 = 28,
    kUndefined029 = 29,
    kUndefined030 = 30,
    kUndefined031 = 31,
    kSustainOnOff = 64,
    kPortamentoOnOff = 65,
    kSustenutoOnOff = 66,
    kSoftPedalOnOff = 67,
    kLegatoOnOff = 68,
    kHold2OnOff = 69,
    kSoundVariation = 70,
    kResonance = 71,
    kReleaseTime = 72,
    kAttackTime = 73,
    kCutoffFrequency = 74,
    kDecayTime = 75,
    kVibratoRate = 76,
    kVibratoDepth = 77,
    kVibratoDelay = 78,
    kSoundControllerUndefined = 79,
    kUndefined085 = 85,
    kUndefined086 = 86,
    kUndefined087 = 87,
    kUndefined088 = 88,
    kUndefined089 = 89,
    kUndefined090 = 90,
    kTremoloDepth = 92,
    kChorusDepth = 93,
    kPhaserDepth = 95,
    kUndefined102 = 102,
    kUndefined103 = 103,
    kUndefined104 = 104,
    kUndefined105 = 105,
    kUndefined106 = 106,
    kUndefined107 = 107,
    kUndefined108 = 108,
    kUndefined109 = 109,
    kUndefined110 = 110,
    kUndefined111 = 111,
    kUndefined112 = 112,
    kUndefined113 = 113,
    kUndefined114 = 114,
    kUndefined115 = 115,
    kUndefined116 = 116,
    kUndefined117 = 117,
    kUndefined118 = 118,
    kUndefined119 = 119,
    kAllNotesOff = 123
  };

	IMidiMsg(int offs = 0, BYTE s = 0, BYTE d1 = 0, BYTE d2 = 0) : mOffset(offs), mStatus(s), mData1(d1), mData2(d2) {}

  void MakeNoteOnMsg(int noteNumber, int velocity, int offset);
  void MakeNoteOffMsg(int noteNumber, int offset);
  void MakePitchWheelMsg(double value);    // Value in [-1, 1], converts to [0, 16384) where 8192 = no pitch change.
  void MakeControlChangeMsg(EControlChangeMsg idx, double value);           //  Value in [0, 1].

	EStatusMsg StatusMsg() const;
	int NoteNumber() const;		  // Returns [0, 128), -1 if NA.
	int Velocity() const;		    // Returns [0, 128), -1 if NA.
  int Program() const;        // Returns [0, 128), -1 if NA.
  double PitchWheel() const;  // Returns [-1.0, 1.0], zero if NA.
  EControlChangeMsg ControlChangeIdx() const;
  double ControlChange(EControlChangeMsg idx) const;      // return [0, 1], -1 if NA.
  static bool ControlChangeOnOff(double msgValue) { return (msgValue >= 0.5); }  // true = on.
  void Clear();
  void LogMsg();
};

const int MAX_PRESET_NAME_LEN = 256;
#define UNUSED_PRESET_NAME "empty"

struct IPreset
{
  bool mInitialized;
  char mName[MAX_PRESET_NAME_LEN];
  ByteChunk mChunk;

  IPreset(int idx)
  : mInitialized(false)
  {
    sprintf(mName, "- %d -", idx+1);
  }
};

enum 
{
  KEY_SPACE,
  KEY_UPARROW,
  KEY_DOWNARROW,
  KEY_LEFTARROW,
  KEY_RIGHTARROW,
  KEY_DIGIT_0,
  KEY_DIGIT_9=KEY_DIGIT_0+9,
  KEY_ALPHA_A,
  KEY_ALPHA_Z=KEY_ALPHA_A+25
};

#endif