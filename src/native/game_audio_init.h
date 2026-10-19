#ifndef TITBIT_GAME_AUDIO_INIT_H
#define TITBIT_GAME_AUDIO_INIT_H
#include "winmm_volume.h"
#define ADR_NO_FORMAT_HELPERS
#include "../audiere/audiere.h"
#undef ADR_NO_FORMAT_HELPERS
extern MasterOutputVolume* game_volume;
extern audiere::AudioDevicePtr game_device;
bool InitializeGameAudio(void* unused, bool use_ds);
#endif
