#include "IParam.h"
#include <stdio.h>

#define MAX_PARAM_DISPLAY_PRECISION 6
#define MAX_PARAM_DISPLAY_LEN 8

IParam::IParam()
:	mType(kTypeNone), mValue(0.0), mMin(0.0), mMax(1.0), mStep(1.0), 
    mDisplayPrecision(0), mNegateDisplay(false), mShape(1.0)
{
    memset(mName, 0, MAX_PARAM_NAME_LEN * sizeof(char));
    memset(mLabel, 0, MAX_PARAM_NAME_LEN * sizeof(char));
}

IParam::~IParam()
{
}

void IParam::InitBool(const char* name, bool defaultVal, const char* label)
{
	if (mType == kTypeNone) {
		mType = kTypeBool;
	}
	InitEnum(name, (defaultVal ? 1 : 0), 2);

  SetDisplayText(0, "off");
  SetDisplayText(1, "on");
}

void IParam::InitEnum(const char* name, int defaultVal, int nEnums)
{
	if (mType == kTypeNone) {
		mType = kTypeEnum;
	}
	InitInt(name, defaultVal, 0, nEnums - 1);
}

void IParam::InitInt(const char* name, int defaultVal, int minVal, int maxVal, const char* label)
{
	if (mType == kTypeNone) {
		mType = kTypeInt;
	}
	InitDouble(name, (double) defaultVal, (double) minVal, (double) maxVal, 1.0, label);
}

void IParam::InitDouble(const char* name, double defaultVal, double minVal, double maxVal, double step, const char* label)
{
	if (mType == kTypeNone) {
		mType = kTypeDouble;
	}
	strcpy(mName, name);
	strcpy(mLabel, label);
	mValue = defaultVal;
	mMin = minVal;
	mMax = MAX(maxVal, minVal + step);
	mStep = step;

	for (mDisplayPrecision = 0; 
		mDisplayPrecision < MAX_PARAM_DISPLAY_PRECISION && step != floor(step);
		++mDisplayPrecision, step *= 10.0) {
		;
	}
}
    
void IParam::SetShape(double shape)
{
    if (shape != 0.0) {
        mShape = shape;
    }
}

void IParam::SetDisplayText(int value, const char* text) 
{
  int n = mDisplayTexts.GetSize();
  mDisplayTexts.Resize(n + 1);
  DisplayText* pDT = mDisplayTexts.Get() + n;
  pDT->mValue = value;
  strcpy(pDT->mText, text);
}

double IParam::DBToAmp()
{
	return ::DBToAmp(mValue);
}

void IParam::SetNormalized(double normalizedValue)
{
  mValue = FromNormalizedParam(normalizedValue, mMin, mMax, mShape);
	if (mType != kTypeDouble) {
		mValue = floor(0.5 + mValue / mStep) * mStep;
	}
	mValue = MIN(mValue, mMax);
}

double IParam::GetNormalized()
{
	return GetNormalized(mValue);
}

double IParam::GetNormalized(double nonNormalizedValue)
{
  nonNormalizedValue = BOUNDED(nonNormalizedValue, mMin, mMax);
  return ToNormalizedParam(nonNormalizedValue, mMin, mMax, mShape);
}

void IParam::GetDisplayForHost(double value, bool normalized, char* rDisplay)
{
  if (normalized) {
    value = FromNormalizedParam(value, mMin, mMax, mShape);
  }

  const char* displayText = GetDisplayText((int) value);
  if (CSTR_NOT_EMPTY(displayText)) {
    strcpy(rDisplay, displayText);
    return;
  }

	double displayValue = value;
	if (mNegateDisplay) {
		displayValue = -displayValue;
	}

	if (displayValue == 0.0) {
		strcpy(rDisplay, "0");
	}
	else
	if (mDisplayPrecision == 0) {
		sprintf(rDisplay, "%d", int(displayValue));
	} 
	else {
		char fmt[16];
		sprintf(fmt, "%%.%df", mDisplayPrecision);
		sprintf(rDisplay, fmt, displayValue);
	}
}

const char* IParam::GetNameForHost()
{
  return mName;
}

const char* IParam::GetLabelForHost()
{
  return mLabel;
}

int IParam::GetNDisplayTexts()
{
  return mDisplayTexts.GetSize();
}

const char* IParam::GetDisplayText(int value)
{
  int n = mDisplayTexts.GetSize();
  if (n) {
    DisplayText* pDT = mDisplayTexts.Get();
    for (int i = 0; i < n; ++i, ++pDT) {
      if (value == pDT->mValue) {
        return pDT->mText;
      }
    }
  }
  return "";
}

bool IParam::MapDisplayText(char* str, int* pValue)
{
  int n = mDisplayTexts.GetSize();
  if (n) {
    DisplayText* pDT = mDisplayTexts.Get();
    for (int i = 0; i < n; ++i, ++pDT) {
      if (!strcmp(str, pDT->mText)) {
        *pValue = pDT->mValue;
        return true;
      }
    }
  }
  return false;
}

void IParam::GetBounds(double* pMin, double* pMax)
{
  *pMin = mMin;
  *pMax = mMax;
}