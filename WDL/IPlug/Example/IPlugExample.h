#ifndef __PLUGEXAMPLE__
#define __PLUGEXAMPLE__

// In the project settings, define either VST_API or AU_API.
#include "../IPlug_include_in_plug_hdr.h"

class PlugExample : public IPlug
{
public:

	PlugExample(IPlugInstanceInfo instanceInfo);
	~PlugExample() {}	// Nothing to clean up.

	// Implement these if your audio or GUI logic requires doing something 
	// when params change or when audio processing stops/starts.
	void Reset() {}
	void OnParamChange(int paramIdx) {}

	void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

private:

	int mMeterIdx_L, mMeterIdx_R;
	double prevL, prevR;
};

////////////////////////////////////////

#endif
