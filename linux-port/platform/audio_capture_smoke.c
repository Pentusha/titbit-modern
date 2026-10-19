#include "titbit_platform.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv){
 unsigned volume=argc>1?(unsigned)strtoul(argv[1],NULL,10):65535;
 TBAudio *audio=tb_audio_open(44100,2,16);if(!audio){fprintf(stderr,"%s\n",tb_error());return 1;}
 assert(tb_audio_master_set(volume));int16_t samples[4410*2];for(unsigned i=0;i<4410;i++)samples[i*2]=samples[i*2+1]=(i%100<50?10000:-10000);
 TBWaveHeader header;memset(&header,0,sizeof(header));header.data=(char*)samples;header.length=sizeof(samples);header.flags=TB_WAVE_PREPARED;assert(tb_audio_queue(audio,&header));
 uint32_t start=tb_milliseconds();while(!(header.flags&TB_WAVE_DONE)&&tb_milliseconds()-start<3000)tb_sleep(5);
 assert(header.flags&TB_WAVE_DONE);tb_audio_close(audio);tb_shutdown();printf("PASS PCM capture volume=%u\n",volume);return 0;
}
