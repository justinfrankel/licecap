#ifndef _VSTHOSTS_
#define _VSTHOSTS_

#include <stdlib.h>
#include <string.h>

enum EVSTHost {
	kHostUnknown = 0,
	kHostReaper,
	kHostProTools,
	kHostCubase,
    kHostNuendo,
	kHostSonar,
	kHostVegas,
	kHostFL,
	kHostSamplitude,
	kHostAbletonLive,
	kHostTracktion,
	kHostNTracks,
	kHostMelodyneStudio,
	kHostVSTScanner,

	// These hosts don't report the host name:
	// MiniHost
};

EVSTHost LookUpHost(char* vendorStr, char* productStr, int version);

#endif
				

	