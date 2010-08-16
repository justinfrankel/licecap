#ifndef _IPARAM_
#define _IPARAM_

#include "Containers.h"
#include <math.h>

const int MAX_PARAM_NAME_LEN = 32;

inline double ToNormalizedParam(double nonNormalizedValue, double min, double max, double shape)
{
  return pow((nonNormalizedValue - min) / (max - min), 1.0 / shape);
}

inline double FromNormalizedParam(double normalizedValue, double min, double max, double shape)
{
  return min + pow((double) normalizedValue, shape) * (max - min);
}

class IParam
{
public:

  enum EParamType { kTypeNone, kTypeBool, kTypeInt, kTypeEnum, kTypeDouble };

	IParam();
  ~IParam();

  EParamType Type() { return mType; }
	
	void InitBool(const char* name, bool defaultVal, const char* label = "");
	void InitEnum(const char* name, int defaultVal, int nEnums);
	void InitInt(const char* name, int defaultVal, int minVal, int maxVal, const char* label = "");
  void InitDouble(const char* name, double defaultVal, double minVal, double maxVal, double step, const char* label = "");

  void Set(double value) { mValue = BOUNDED(value, mMin, mMax); }
	void SetDisplayText(int value, const char* text);

  // The higher the shape, the more resolution around host value zero.
  void SetShape(double shape);

	// Call this if your param is (x, y) but you want to always display (-x, -y).
	void NegateDisplay() { mNegateDisplay = true; }
	bool DisplayIsNegated() const { return mNegateDisplay; }

	// Accessors / converters.
	// These all return the readable value, not the VST (0,1).
	double Value() const { return mValue; }
	bool Bool() const { return (mValue >= 0.5); }
	int Int() const { return int(mValue); }
	double DBToAmp();

	void SetNormalized(double normalizedValue);
	double GetNormalized();
	double GetNormalized(double nonNormalizedValue);
  void GetDisplayForHost(char* rDisplay) { GetDisplayForHost(mValue, false, rDisplay); }
  void GetDisplayForHost(double value, bool normalized, char* rDisplay);
	const char* GetNameForHost();
	const char* GetLabelForHost();

  int GetNDisplayTexts();
	const char* GetDisplayText(int value);
	bool MapDisplayText(char* str, int* pValue);	// Reverse map back to value.
  void GetBounds(double* pMin, double* pMax);

private:

	// All we store is the readable values.
	// SetFromHost() and GetForHost() handle conversion from/to (0,1).
  EParamType mType;
	double mValue, mMin, mMax, mStep, mShape;	
	int mDisplayPrecision;
	char mName[MAX_PARAM_NAME_LEN], mLabel[MAX_PARAM_NAME_LEN];
	bool mNegateDisplay;
  
  struct DisplayText {
    int mValue;
    char mText[MAX_PARAM_NAME_LEN];
  };
  WDL_TypedBuf<DisplayText> mDisplayTexts;
};

#endif