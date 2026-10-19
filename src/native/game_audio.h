#ifndef TITBIT_GAME_AUDIO_H
#define TITBIT_GAME_AUDIO_H
#define ADR_NO_FORMAT_HELPERS
#include "../audiere/audiere.h"
#undef ADR_NO_FORMAT_HELPERS
/* Authored observed state, not historical class names. */
struct ObservedSoundState {
 unsigned slots[8],a,b,c;
 ObservedSoundState();
 void Reset();
};
class SoundFocusPrefix {
public:
 virtual ~SoundFocusPrefix();
 ObservedSoundState state;
 unsigned unknown30;
 audiere::OutputStreamPtr stream;
 SoundFocusPrefix();
 void Stop();void Resume();
};
struct ActiveSoundEntry {SoundFocusPrefix* sound;int pause_depth;unsigned unknown;};
extern ActiveSoundEntry* active_sounds;
extern int active_sound_count;
extern "C" void PauseGameAudio();
extern "C" void RestoreGameAudio();
#endif
