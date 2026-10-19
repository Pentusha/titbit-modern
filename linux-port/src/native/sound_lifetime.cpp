/* Authored Sound state/lifetime; complete56-byte allocation is independently
 * evidenced by CreateSound. Original names/property meanings remain unknown. */
#include "game_audio.h"
extern "C" void *__cdecl memset(void*,int,unsigned);
extern "C" void *__cdecl memmove(void*,const void*,unsigned);
#pragma intrinsic(memset)
ObservedSoundState::ObservedSoundState(){Reset();}
void ObservedSoundState::Reset(){memset(slots,0,sizeof slots);a=0;b=0;c=0;}
SoundFocusPrefix::SoundFocusPrefix(){stream=0;}
SoundFocusPrefix::~SoundFocusPrefix(){
 int n=active_sound_count;
 for(int i=0;i<n;++i){
  if(active_sounds[i].sound==this){
   int remaining=active_sound_count-i-1;
   if(remaining)memmove(&active_sounds[i],&active_sounds[i+1],remaining*sizeof(ActiveSoundEntry));
   --active_sound_count;
  }
 }
}
