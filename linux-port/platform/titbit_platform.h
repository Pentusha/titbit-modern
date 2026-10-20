#ifndef TITBIT_PLATFORM_H
#define TITBIT_PLATFORM_H
/* Native platform boundary. Fixed-width types keep this API independent of
 * the reconstructed engine's 32-bit class layouts and compiler ABI. */
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct TBWindow TBWindow;
typedef struct TBAudio TBAudio;
typedef struct TBEvent { uint32_t message; uintptr_t wparam; intptr_t lparam; uint32_t time; int32_t x,y; } TBEvent;
typedef struct TBInput { uint8_t keys[256]; int32_t dx,dy,wheel; uint8_t buttons[4]; } TBInput;
typedef struct TBWaveHeader { char *data; uint32_t length,recorded; uintptr_t user; volatile uint32_t flags; uint32_t loops; struct TBWaveHeader *next; uintptr_t reserved; } TBWaveHeader;
enum { TB_WAVE_DONE=1, TB_WAVE_PREPARED=2, TB_WAVE_INQUEUE=16 };
const char *tb_error(void);
int tb_init(void);
void tb_shutdown(void);
TBWindow *tb_window_create(const char *title,int width,int height,int fullscreen);
void tb_window_destroy(TBWindow *window);
int tb_window_show(TBWindow *window,int show);
int tb_window_move(TBWindow *window,int x,int y,int width,int height);
int tb_window_focus(TBWindow *window);
int tb_window_opengl(TBWindow *window);
void *tb_project_icon(void);
void *tb_arrow_cursor(void);
void tb_window_icon(TBWindow *window,void *icon);
int tb_display_mode(int index,int *width,int *height,int *refresh,int *bits);
void tb_window_size(TBWindow *window,int *width,int *height);
void *tb_gl_context(TBWindow *window);
int tb_gl_make_current(TBWindow *window,void *context);
void tb_gl_delete_context(void *context);
void *tb_gl_proc(const char *name);
void tb_gl_viewport(int x,int y,int width,int height);
void tb_swap(TBWindow *window);
typedef struct TBFrameStats {int32_t width,height;uint64_t pixels,nonblack;double mean_red,mean_green,mean_blue;uint32_t color_bins;} TBFrameStats;
int tb_read_frame_pixels(unsigned char *rgba,size_t capacity,int *width,int *height);
int tb_capture_frame(const char *ppm_filename,TBFrameStats *stats);
int tb_poll_event(TBEvent *event);
void tb_input_read(TBInput *input,int reset_relative);
int tb_cursor(int show);
uint64_t tb_counter(void);
uint64_t tb_frequency(void);
uint32_t tb_milliseconds(void);
void tb_sleep(uint32_t milliseconds);
/* PCM formats used by Audiere. Queue owns no sample memory: headers remain
 * alive until marked DONE, exactly as waveOut's contract requires. */
TBAudio *tb_audio_open(unsigned rate,unsigned channels,unsigned bits);
int tb_audio_queue(TBAudio *audio,TBWaveHeader *header);
void tb_audio_reset(TBAudio *audio);
void tb_audio_close(TBAudio *audio);
int tb_audio_volume(TBAudio *audio,unsigned volume);
int tb_audio_devices(void);
unsigned tb_audio_master_get(void);
int tb_audio_master_set(unsigned volume);
int tb_audio_mute_get(void);
int tb_audio_mute_set(int mute);
int tb_resolve_path(const char *input,char *output,size_t capacity,int existing);
#ifdef __cplusplus
}
#endif
#endif
