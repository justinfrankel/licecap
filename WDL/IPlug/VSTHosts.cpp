#include "VSTHosts.h"
#include <ctype.h>

void ToLower(char* cStr)
{
    int i, n = (int) strlen(cStr);
	for (i = 0; i < n; ++i) {
		cStr[i] = tolower(cStr[i]);
	}
}

EVSTHost LookUpHost(char* vendorStr, char* productStr, int version)
{
	ToLower(vendorStr);
	ToLower(productStr);

	// Cockos:REAPER:1990
	if (strstr(productStr, "reaper")) {
		return kHostReaper;
	}
	// Steinberg:Cubase VST:8200 ... C4 = (version >= 8200)
	if (strstr(productStr, "cubase")) {
		return kHostCubase;
	}
    // Steinberg:Nuendo:2200
    if (strstr(productStr, "nuendo")) {
		return kHostNuendo;
	}
	// Twelve Tone Systems:Cakewalk VST Wizard 4.5:4
	if (strstr(productStr, "cakewalk")) {
		return kHostSonar;
	}
	// MAGIX:Samplitude:9001
	if (strstr(productStr, "samplitude")) {
		return kHostSamplitude;
	}
	// Image-Line:Fruity Wrapper:6400
	if (strstr(productStr, "fruity")) {
		return kHostFL;
	}
	// Ableton:Live:5020 
	if (strstr(vendorStr, "ableton") && strstr(productStr, "live")) {
		return kHostAbletonLive;
	}
	// Celemony Software GmbH:Melodyne:0 
	if (strstr(productStr, "melodyne")) {
		return kHostMelodyneStudio;
	}
	// Vincent Burel Inc:VSTMANLIB:1031
	if (strstr(productStr, "vstmanlib")) {
		return kHostVSTScanner;
	}

	return kHostUnknown;
}