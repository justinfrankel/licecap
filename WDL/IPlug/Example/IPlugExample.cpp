#include "IPlugExample.h"
#include "../IPlug_include_in_plug_src.h"
#include "../IControl.h"
#include "resource.h"
#include <math.h>

const int kNumPrograms = 1;

enum EParams {
	kGainL = 0,
	kGainR,
	kPan,
	kChannelSw,
	kNumParams
};

enum EChannelSwitch {
	kDefault = 0,
	kReversed,
	kAllLeft,
	kAllRight,
	kOff,
	kNumChannelSwitchEnums
};

enum ELayout
{
	kW = 400,
	kH = 200,

	kSwitch_N = 5,	// # of sub-bitmaps.
	kMeter_N = 51,	// # of sub-bitmaps.

	kFader_Len = 150,
	kGainL_X = 80,
	kGainL_Y = 20,
	kGainR_X = 350,
	kGainR_Y = 20,

	kSwitch_X = 20,
	kSwitch_Y = 40,

	kPan_X = 225,
	kPan_Y = 145,

	kMeterL_X = 135,
	kMeterL_Y = 20,
	kMeterR_X = 250,
	kMeterR_Y = 20
};

PlugExample::PlugExample(IPlugInstanceInfo instanceInfo)
:	IPLUG_CTOR(kNumParams, 6, instanceInfo), prevL(0.0), prevR(0.0)
{
  TRACE;

	// Define parameter ranges, display units, labels.

	GetParam(kGainL)->InitDouble("Gain L", 0.0, -44.0, 12.0, 0.1, "dB");
	GetParam(kGainL)->NegateDisplay();
	GetParam(kGainR)->InitDouble("Gain R", 0.0, -44.0, 12.0, 0.1, "dB");
	GetParam(kPan)->InitInt("Pan", 0, -100, 100, "%");

	// Params can be enums.

	GetParam(kChannelSw)->InitEnum("Channel", kDefault, kNumChannelSwitchEnums);
	GetParam(kChannelSw)->SetDisplayText(kDefault, "default");
	GetParam(kChannelSw)->SetDisplayText(kReversed, "reversed");
	GetParam(kChannelSw)->SetDisplayText(kAllLeft, "all L");
	GetParam(kChannelSw)->SetDisplayText(kAllRight, "all R");
	GetParam(kChannelSw)->SetDisplayText(kOff, "mute");

  MakePreset("preset 1", -5.0, 5.0, 17, kReversed);
  MakePreset("preset 2", -15.0, 25.0, 37, kAllRight);
  MakeDefaultPreset("-", 4);

	// Instantiate a graphics engine.

	IGraphics* pGraphics = MakeGraphics(this, kW, kH); // MakeGraphics(this, kW, kH);
	pGraphics->AttachBackground(BG_ID, BG_FN);

  // Attach controls to the graphics engine.  Controls are automatically associated
	// with a parameter if you construct the control with a parameter index.

	// Attach a couple of meters, not associated with any parameter,
	// which we keep indexes for, so we can push updates from the plugin class.

	IBitmap bitmap = pGraphics->LoadIBitmap(METER_ID, METER_FN, kMeter_N);
  mMeterIdx_L = pGraphics->AttachControl(new IBitmapControl(this, kMeterL_X, kMeterL_Y, &bitmap));
	mMeterIdx_R = pGraphics->AttachControl(new IBitmapControl(this, kMeterR_X, kMeterR_Y, &bitmap));

	// Attach a couple of faders, associated with the parameters GainL and GainR.

	bitmap = pGraphics->LoadIBitmap(FADER_ID, FADER_FN);
	pGraphics->AttachControl(new IFaderControl(this, kGainL_X, kGainL_Y, kFader_Len, kGainL, &bitmap, kVertical));
	pGraphics->AttachControl(new IFaderControl(this, kGainR_X, kGainR_Y, kFader_Len, kGainR, &bitmap, kVertical));

	// Attach a 5-position switch associated with the ChannelSw parameter.

	bitmap = pGraphics->LoadIBitmap(TOGGLE_ID, TOGGLE_FN, kSwitch_N);
	pGraphics->AttachControl(new ISwitchControl(this, kSwitch_X, kSwitch_Y, kChannelSw, &bitmap));

	// Attach a rotating knob associated with the Pan parameter.

	bitmap = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN);
	pGraphics->AttachControl(new IKnobRotaterControl(this, kPan_X, kPan_Y, kPan, &bitmap));

	// See IControl.h for other control types,
	// IKnobMultiControl, ITextControl, IBitmapOverlayControl, IFileSelectorControl, IGraphControl, etc.

	// Attach the graphics engine to the plugin.

	AttachGraphics(pGraphics);

	// No cleanup necessary, the graphics engine manages all of its resources and cleans up when closed.
}

void PlugExample::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked for us.

	double* in1 = inputs[0];
	double* in2 = inputs[1];
	double* out1 = outputs[0];
	double* out2 = outputs[1];

	double gain1 = GetParam(kGainL)->DBToAmp();
	double gain2 = GetParam(kGainR)->DBToAmp();
	double pan = 0.01 * GetParam(kPan)->Value();
	EChannelSwitch chanSwitch = (EChannelSwitch) int(GetParam(kChannelSw)->Value());
	double peakL = 0.0, peakR = 0.0;

	for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2) {

		*out1 = *in1 * gain1 * (1.0 - pan);
		*out2 = *in2 * gain2 * (1.0 + pan);

    // In an actual plugin you'd switch outside of the sample loop,
    // it's very inefficient to switch on every sample like this.

		switch (chanSwitch) {

			case kReversed:
				SWAP(*out1, *out2);
				break;

			case kAllLeft:
				*out1 += *out2;
				*out2 = 0.0;
				break;

			case kAllRight:
				*out2 += *out1;
				*out1 = 0.0;
				break;

			case kOff:
				*out1 = *out2 = 0.0;
				break;

			default:
				break;
		}

		peakL = MAX(peakL, fabs(*out1));
		peakR = MAX(peakR, fabs(*out2));
	}

	const double METER_ATTACK = 0.6, METER_DECAY = 0.1;
	double xL = (peakL < prevL ? METER_DECAY : METER_ATTACK);
	double xR = (peakR < prevR ? METER_DECAY : METER_ATTACK);

  peakL = peakL * xL + prevL * (1.0 - xL);
	peakR = peakR * xR + prevR * (1.0 - xR);

	prevL = peakL;
	prevR = peakR;

  if (GetGUI()) {
    GetGUI()->SetControlFromPlug(mMeterIdx_L, peakL);
    GetGUI()->SetControlFromPlug(mMeterIdx_R, peakR);
  }
}


