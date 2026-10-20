#ifndef TITBIT_PORTABLE_MMREG_H
#define TITBIT_PORTABLE_MMREG_H
/* MinGW's mmreg-first path omits the const WAVEFORMATEX aliases required by
 * dsound.h. Loading the common multimedia declarations first provides the
 * same canonical format record to both headers. */
#include <mmsystem.h>
#include_next <mmreg.h>
#endif
