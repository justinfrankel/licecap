#include "IPlugStructs.h"
#include "Log.h"

//bool IText::operator==(const IText& rhs) const
//{
//	int s = sizeof(*this);
//	int r = sizeof(rhs);
//	return (s == r && !strcmp(mFont.Get(), rhs.mFont.Get()) && memcmp(this, &rhs, s) == 0);
//}
//
//bool IText::operator<(const IText& rhs) const
//{
//	if (mFont != rhs.mFont) {
//		return (mFont < rhs.mFont);
//	}
//	int s = sizeof(*this);
//	int r = sizeof(rhs);
//	if (s != r) {
//		return (s < r);
//	}
//	return memcmp(this, &rhs, MIN(s, r));
//}

void IMidiMsg::MakeNoteOnMsg(int noteNumber, int velocity, int offset)
{
  Clear();
  mStatus = kNoteOn << 4;
  mData1 = noteNumber;
  mData2 = velocity;
  mOffset = offset;
}

void IMidiMsg::MakeNoteOffMsg(int noteNumber, int offset)
{
  Clear();
  mStatus = kNoteOff << 4;
  mData1 = noteNumber;
  mOffset = offset;
}

void IMidiMsg::MakePitchWheelMsg(double value)
{
  Clear();
  mStatus = kPitchWheel << 4;
  int i = 8192 + (int) (value * 8192.0);
  i = BOUNDED(i, 0, 16383);
  mData2 = i>>7;
  mData1 = i&0x7F;
}

void IMidiMsg::MakeControlChangeMsg(EControlChangeMsg idx, double value)
{
  Clear();
  mStatus = kControlChange << 4;
  mData1 = idx;
  mData2 = (int) (value * 127.0);
}

IMidiMsg::EStatusMsg IMidiMsg::StatusMsg() const
{
	unsigned int e = mStatus >> 4;
	if (e < kNoteOff || e > kPitchWheel) {
		return kNone;
	}
	return (EStatusMsg) e;
}

int IMidiMsg::NoteNumber() const
{
	switch (StatusMsg()) {
		case kNoteOn:
		case kNoteOff:
		case kPolyAftertouch:
			return mData1;
		default:
			return -1;
	}
}

int IMidiMsg::Velocity() const
{
	switch (StatusMsg()) {
		case kNoteOn:
		case kNoteOff:
			return mData2;
		default:
			return -1;
	}
}

int IMidiMsg::Program() const
{
  if (StatusMsg() == kProgramChange) {
    return mData1;
  }
  return -1;
}

double IMidiMsg::PitchWheel() const
{
  if (StatusMsg() == kPitchWheel) {
    int iVal = (mData2 << 7) + mData1;
    return (double) (iVal - 8192) / 8192.0;
  }
  return 0.0;
}

IMidiMsg::EControlChangeMsg IMidiMsg::ControlChangeIdx() const
{
  return (EControlChangeMsg) mData1;
}

double IMidiMsg::ControlChange(EControlChangeMsg idx) const
{
  if (StatusMsg() == kControlChange && ControlChangeIdx() == idx) {
    return (double) mData2 / 127.0;
  }
  return -1.0;
}

void IMidiMsg::Clear()
{
  mOffset = 0;
  mStatus = mData1 = mData2 = 0;
}

const char* StatusMsgStr(IMidiMsg::EStatusMsg msg)
{
  switch (msg) {
    case IMidiMsg::kNone: return "none";
    case IMidiMsg::kNoteOff: return "noteoff";
    case IMidiMsg::kNoteOn: return "noteon";
    case IMidiMsg::kPolyAftertouch: return "aftertouch";
    case IMidiMsg::kControlChange: return "controlchange";
    case IMidiMsg::kProgramChange: return "programchange";
    case IMidiMsg::kChannelAftertouch: return "channelaftertouch";
    case IMidiMsg::kPitchWheel: return "pitchwheel";
    default:  return "unknown";
	};
}

void IMidiMsg::LogMsg()
{
#ifdef TRACER_BUILD
  Trace(TRACELOC, "midi:(%s:%d:%d)", StatusMsgStr(StatusMsg()), NoteNumber(), Velocity());
#endif
}