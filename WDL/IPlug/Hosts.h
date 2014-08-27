#ifndef _PLUGINHOSTS_
#define _PLUGINHOSTS_

#include <stdlib.h>
#include <string.h>

enum EHost {
  kHostUninit = -1,
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
  kHostAULab,
  kHostForte,
  kHostChainer,
  kHostAudition,
  kHostOrion,
  kHostBias,
  kHostSAWStudio,
	kHostLogic,
	kHostDigitalPerformer,

	// These hosts don't report the host name:
  // EnergyXT2
	// MiniHost
};

EHost LookUpHost(const char* host);

#endif
				

	