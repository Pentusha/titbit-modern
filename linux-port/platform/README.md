# Native Linux platform boundary

This directory is independent of the original source tree. It uses SDL2 and
Linux facilities directly; it does not load or execute PE files or use Wine.

`sdl_platform.c` implements a compatibility OpenGL context, window management,
Windows-style keyboard/mouse messages, DirectInput scan-code state, timers,
PCM sample queues and case-insensitive game-resource paths. `win32_sdl.c`
adapts the supported API subset to the engine's 32-bit Win32 declarations.
`game_input_native.cpp` replaces the DirectInput translation unit in a Linux
source build. It preserves the globals consumed by gameplay code.

Build and exercise the backend without touching the user's desktop:

```sh
sudo apt install build-essential pkg-config libsdl2-dev libgl1-mesa-dev libglu1-mesa-dev libavformat-dev libavcodec-dev libswscale-dev xvfb xauth
make -C linux-port/platform check
```

For the 32-bit engine build use the i386 SDL2/OpenGL/FFmpeg development packages:

```sh
make -C linux-port/platform clean
PKG_CONFIG_LIBDIR=/usr/lib/i386-linux-gnu/pkgconfig make -C linux-port/platform \
  ARCH_FLAGS=-m32 CFLAGS='-O2 -g -fno-omit-frame-pointer -Werror' all avi-smoke seer-call-smoke check
linux-port/platform/seer-call-smoke
```

The smoke check verifies framebuffer pixels after a real OpenGL draw, injected
key/click/mouse events, DirectInput state, PCM callback completion/reset, timers,
and Windows-style resource path resolution. Audio uses SDL's dummy driver in
automated tests; this proves queue consumption, not audible speaker output.

`gl_stdcall.c` exports 55 prefixed x86 stdcall trampolines for the current
source GL/GLU imports and six multitexture extension functions. These call ELF cdecl symbols
resolved from `libGL.so.1`/`libGLU.so.1`. `gl-import-map.json` supplies import
renames. On i386 the smoke test checks ESP after 40,000 GL/GLU calls.

`avi_ffmpeg.c` implements AviPlane's VIDEO-stream VfW interface and memory DIB
operations through FFmpeg. It preserves empty AVI samples as held frames and
supports arbitrary frame seeking. The original 18 tested AVI files contain no
audio streams; music and effects belong to the separate Audiere backend.

```sh
make -C linux-port/platform avi-smoke
python3 linux-port/platform/verify_avi.py /path/to/game --report /tmp/avi-check.json
```

This compares 72 frame/DIB operations and all 18 first-frame pixel buffers
against independent FFmpeg CLI output. The amd64 test matched all pixels;
i386 matched 16/18 frames exactly, with maximum channel difference 1 in the
other two frames due to decoder SIMD rounding. The test records pixel error
and rejects larger deviations. FFmpeg CLI is only a test dependency; the
application uses libavformat/libavcodec/libswscale directly.

`seer_call.S` preserves SeeR's 32-bit cdecl/Windows thiscall argument vectors,
EAX:EDX integer results and x87 double results, while aligning calls for ELF.
Its separate test checks 200,000 calls and exact ESP restoration.

Kernel adapters use pthread suspended-thread gates, recursive mutexes, native
TLS and tracked mmap reserve/commit/decommit/release. Their smoke tests verify
thread execution, per-thread isolation and zeroed memory after recommit.
Resource file operations use 64-bit Linux file offsets/inodes even in i386,
which is necessary when reading files from WSL's NTFS-mounted directories.

The WinMM mixer adapter controls this application's actual SDL PCM output.
It preserves the 0–65535 slider range, mute controls and window notifications;
it does not change another application's audio or the system speaker gain.
An additional disk-driver check validates the PCM samples emitted at full,
half and zero volume:

```sh
PKG_CONFIG_LIBDIR=/usr/lib/i386-linux-gnu/pkgconfig make -C linux-port/platform ARCH_FLAGS=-m32 audio-capture-smoke
python3 linux-port/platform/verify_audio.py --report /tmp/titbit-pcm.json
```

Screenshot capture reads the actual current GL backbuffer before swap:

```sh
TITBIT_SCREENSHOT_DIR=/tmp/titbit-frames TITBIT_SCREENSHOT_FRAMES=100,300,1000 ./titbit-linux
```

Only selected frames write files. Each frame produces a top-down binary PPM
and JSON metrics: dimensions, nonblack pixel count, mean RGB and occupied color
bins. `tb_read_frame_pixels` and `tb_capture_frame` also expose on-demand C APIs.
The reader restores GL read-buffer and pixel-pack state after capture.

Real game audio can be captured immediately before SDL delivery by setting
`TITBIT_AUDIO_CAPTURE_DIR` to a diagnostic directory. Each successfully opened
device gets `device-NNN.pcm` and a JSON description with driver, sample rate,
channel count, sample format, emitted bytes, nonzero samples, peak and RMS.
Files open before the callback starts and flush/close on device close or
`tb_shutdown`. Initial metadata has `closed: false`; a completed capture has
`closed: true`. Probe devices may legitimately contain no samples.

This records the actual mixed Audiere output, after application volume/mute.
It does not select or replace the audio driver. Without the environment
variable, no audio capture files or PCM-analysis work are performed. Capture
is diagnostic I/O and can affect timing; it is disabled in normal gameplay.
The headless runner's optional `--exe` argument selects an isolated native
binary while retaining the usual main-build default.

The source-owned editable RC string tables are translated to
`resource_strings.inc`; the window icon is the project's own T artwork.
CPU/page/time queries return native host information. DirectSound/COM is
explicitly unavailable, which selects Audiere's real SDL-backed waveOut path.
External-document launch currently reports an unsupported-operation error;
MessageBox errors go to stderr and never block a hidden desktop test.

This boundary is integrated into the native ELF32 game. The Microsoft object ABI,
DWARF exception bridge, source-owned CRT adapters and engine startup are built by
`tools/build_linux.py`; see the project README for gameplay validation.
Unsupported APIs report failure rather than fake success.

The optional SDL `disk` diagnostic driver is not a supported full-game output
path when writing into the tested Windows-mounted `/mnt/c` directory: SDL
rejects that output as "not a regular file", and the engine subsequently reaches
a null Audiere stream at frame 2. The direct device probe passes with its output
in `/tmp`; multiple simultaneous devices are supported.
Normal SDL driver selection uses PulseAudio on the tested WSLg system; the
32-bit device-open and queued-silence hardware probe passes, and actual game
PCM capture with `dummy` passes. Use `TITBIT_AUDIO_CAPTURE_DIR` for game audio
capture instead of selecting the `disk` driver. No driver fallback is silently
substituted. Evidence: `build/audio-game/audio-qa.json` and the final production
runtime report.

