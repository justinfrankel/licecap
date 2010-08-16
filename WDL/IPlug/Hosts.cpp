#include "Hosts.h"
#include <ctype.h>
#include "Log.h"

EHost LookUpHost(const char* inHost)
{
  char host[256];
  ToLower(host, inHost);
  
	// C4 is version >= 8.2
	if (strstr(host, "cubase")) {
		return kHostCubase;
	}
  if (strstr(host, "reaper")) {
		return kHostReaper;
	}
  if (strstr(host, "nuendo")) {
		return kHostNuendo;
	}
	if (strstr(host, "cakewalk")) {
		return kHostSonar;
	}
	if (strstr(host, "samplitude")) {
		return kHostSamplitude;
	}
	if (strstr(host, "fruity")) {
		return kHostFL;
	}
	if (strstr(host, "live")) {
		return kHostAbletonLive;
	}
	if (strstr(host, "melodyne")) {
		return kHostMelodyneStudio;
	}
	if (strstr(host, "vstmanlib")) {
		return kHostVSTScanner;
  }
  if (strstr(host, "aulab")) {
    return kHostAULab;
  }
  if (strstr(host, "forte")) {
    return kHostForte;
  }
  if (strstr(host, "chainer")) {
    return kHostChainer;
  }
  if (strstr(host, "audition")) {
    return kHostAudition;
  }
  if (strstr(host, "orion")) {
    return kHostOrion;
  }
  if (strstr(host, "sawstudio")) {
    return kHostSAWStudio;
  }
	if (strstr(host, "logic")) {
		return kHostLogic;
	}
	if (strstr(host, "digital")) {
		return kHostDigitalPerformer;
	}
	return kHostUnknown;
}