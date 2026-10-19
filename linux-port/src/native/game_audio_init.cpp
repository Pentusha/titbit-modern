#include "game_audio_init.h"
extern "C" void __cdecl PickingTrace(const char*, ...);
bool InitializeGameAudio(void* unused, bool use_ds)
{
    game_volume = new MasterOutputVolume;
    if (game_volume) game_volume->IsAvailable();
    game_volume->Enable();
    if (!game_device) {
        if (use_ds) {
            game_device = audiere::OpenDevice("autodetect", 0);
            PickingTrace("UseDS");
        } else {
            game_device = audiere::OpenDevice("winmm", 0);
        }
        return game_device;
    }
}
