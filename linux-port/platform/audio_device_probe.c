/* Hardware QA: sends zero-valued PCM only. It never initializes video. */
#include "titbit_platform.h"
#include <SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
int main(void){
 TBAudio *first=tb_audio_open(22050,1,16);printf("probe22050mono=%s driver=%s\n",first?"open":"failed",SDL_GetCurrentAudioDriver());if(!first){fprintf(stderr,"%s\n",tb_error());return 1;}tb_audio_close(first);
 TBAudio *audio=tb_audio_open(44100,2,16);if(!audio){fprintf(stderr,"44100 stereo: %s\n",tb_error());return 2;}
 printf("playback44100stereo=open driver=%s\n",SDL_GetCurrentAudioDriver());
 TBAudio *second=tb_audio_open(44100,2,16);printf("concurrentSecondDevice=%s error=%s\n",second?"open":"failed",second?"":tb_error());if(second)tb_audio_close(second);
 int16_t silence[4410*2]={0};TBWaveHeader header;memset(&header,0,sizeof(header));header.data=(char*)silence;header.length=sizeof(silence);header.flags=TB_WAVE_PREPARED;
 if(!tb_audio_queue(audio,&header)){fprintf(stderr,"queue failed\n");return 3;}
 uint32_t start=tb_milliseconds();while(!(header.flags&TB_WAVE_DONE)&&tb_milliseconds()-start<3000)tb_sleep(5);
 int done=(header.flags&TB_WAVE_DONE)!=0;tb_audio_close(audio);tb_shutdown();if(!done){fprintf(stderr,"PCM did not complete\n");return 4;}
 puts("PASS native audio device, completed silence-only PCM");return 0;
}
