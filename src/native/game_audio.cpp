#include "game_audio.h"

ActiveSoundEntry* active_sounds;
int active_sound_count;

void SoundFocusPrefix::Stop() { stream->stop(); }
void SoundFocusPrefix::Resume() { stream->play(); }

// Returning the old value matches the native inlined counter operation. The
// caller deliberately re-reads the entry through the global table afterwards.
static inline int decrement_depth(int& value) { return value--; }

extern "C" void PauseGameAudio()
{
    if (active_sound_count > 0) {
        int remaining = active_sound_count;
        int i = 0;
        do {
            if (active_sounds[i].sound->stream->isPlaying() || active_sounds[i].pause_depth) {
                ++active_sounds[i].pause_depth;
                active_sounds[i].sound->Stop();
            }
            ++i;
        } while (--remaining);
    }
}

extern "C" void RestoreGameAudio()
{
    if (active_sound_count > 0) {
        int remaining = active_sound_count;
        int i = 0;
        do {
            if (active_sounds[i].pause_depth) {
                decrement_depth(active_sounds[i].pause_depth);
                if (!active_sounds[i].pause_depth)
                    active_sounds[i].sound->Resume();
            }
            ++i;
        } while (--remaining);
    }
}
