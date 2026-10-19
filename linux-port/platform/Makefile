CC ?= cc
PKG_CONFIG ?= pkg-config
ARCH_FLAGS ?=
CFLAGS ?= -O2 -g
SDL_CFLAGS := $(shell $(PKG_CONFIG) --cflags sdl2)
SDL_LIBS := $(shell $(PKG_CONFIG) --libs sdl2)
AV_CFLAGS := $(shell $(PKG_CONFIG) --cflags libavformat libavcodec libavutil libswscale)
AV_LIBS := $(shell $(PKG_CONFIG) --libs libavformat libavcodec libavutil libswscale)
ifneq ($(filter -m32,$(ARCH_FLAGS)),)
SEER_OBJECT := seer_call.o
endif
all: platform-smoke libtitbit-platform.a
sdl_platform.o: sdl_platform.c titbit_platform.h
	$(CC) $(CFLAGS) $(ARCH_FLAGS) -Wall -Wextra $(SDL_CFLAGS) -c $< -o $@
win32_sdl.o: win32_sdl.c titbit_platform.h
	$(CC) $(CFLAGS) $(ARCH_FLAGS) -Wall -Wextra $(SDL_CFLAGS) -c $< -o $@
gl_stdcall.o: gl_stdcall.c
	$(CC) $(CFLAGS) $(ARCH_FLAGS) -Wall -Wextra -c $< -o $@
win32_kernel.o: win32_kernel.c titbit_platform.h
	$(CC) $(CFLAGS) $(ARCH_FLAGS) -Wall -Wextra -c $< -o $@
win32_heap.o: win32_heap.c
	$(CC) $(CFLAGS) $(ARCH_FLAGS) -Wall -Wextra -c $< -o $@
win32_misc.o: win32_misc.c
	$(CC) $(CFLAGS) $(ARCH_FLAGS) -Wall -Wextra -c $< -o $@
win32_mixer.o: win32_mixer.c titbit_platform.h
	$(CC) $(CFLAGS) $(ARCH_FLAGS) -Wall -Wextra -c $< -o $@
win32_host.o: win32_host.c titbit_platform.h
	$(CC) $(CFLAGS) $(ARCH_FLAGS) -Wall -Wextra -c $< -o $@
seer_call.o: seer_call.S
	$(CC) $(ARCH_FLAGS) -c $< -o $@
avi_ffmpeg.o: avi_ffmpeg.c titbit_platform.h
	$(CC) $(CFLAGS) $(ARCH_FLAGS) -Wall -Wextra $(AV_CFLAGS) -c $< -o $@
libtitbit-platform.a: sdl_platform.o win32_sdl.o win32_kernel.o win32_heap.o win32_misc.o win32_mixer.o win32_host.o gl_stdcall.o avi_ffmpeg.o $(SEER_OBJECT)
	ar rcs $@ $^
platform-smoke: smoke.c sdl_platform.o win32_sdl.o win32_kernel.o win32_heap.o win32_misc.o win32_mixer.o gl_stdcall.o
	$(CC) $(CFLAGS) $(ARCH_FLAGS) -Wall -Wextra $(SDL_CFLAGS) $^ $(SDL_LIBS) -lGL -lm -lpthread -ldl -o $@
avi-smoke: avi_smoke.c avi_ffmpeg.o sdl_platform.o
	$(CC) $(CFLAGS) $(ARCH_FLAGS) -Wall -Wextra $(SDL_CFLAGS) $(AV_CFLAGS) $^ $(SDL_LIBS) $(AV_LIBS) -lGL -lm -o $@
seer-call-smoke: seer_call.S seer_call_smoke.c
	$(CC) $(CFLAGS) $(ARCH_FLAGS) -fno-omit-frame-pointer -Wall -Wextra $^ -lm -o $@
audio-capture-smoke: audio_capture_smoke.c sdl_platform.o
	$(CC) $(CFLAGS) $(ARCH_FLAGS) -Wall -Wextra $(SDL_CFLAGS) $^ $(SDL_LIBS) -lGL -lm -o $@
check: platform-smoke
	SDL_AUDIODRIVER=dummy LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a -s '-screen 0 1024x768x24' ./platform-smoke
clean:
	rm -f platform-smoke avi-smoke seer-call-smoke audio-capture-smoke *.o libtitbit-platform.a
.PHONY: all check clean
