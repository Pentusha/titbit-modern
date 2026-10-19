/* Deterministic record/replay for testing (TITBIT_COMPAT, off unless asked for).
 *
 * Setting replay = record | play (env TITBIT_COMPAT_REPLAY), file replay_file
 * (default titbit_replay.txt in the game folder), frame log framelog (default
 * titbit_frames.txt). When active, CompatStartup() (run before the program's
 * entry point, see tools/build_exe.py --compat) redirects these imports of the
 * main module (in its import address table) to the wrappers below:
 *   timeGetTime, QueryPerformanceCounter/Frequency, GetLocalTime
 *       -> a virtual clock that advances a fixed step (frame_ms, default 40 ms)
 *          at every SwapBuffers, so game time depends only on the frame count;
 *   SwapBuffers -> before presenting: hash the back buffer (FNV-1a of the RGBA
 *          pixels), write "frame hash" to the frame log, optionally dump
 *          frames listed in dump_frames as BMP (frame_<n>.bmp);
 *   PeekMessageA -> record: log mouse/keyboard messages with their frame number;
 *          play: drop real input messages and deliver the recorded ones at the
 *          same frame;
 *   DirectInputCreateA -> device proxies whose GetDeviceState is logged
 *          (record) or answered from the log (play);
*   waveOut* -> a silent device whose buffers finish by the virtual clock;
 *   Sleep -> on worker threads: wait for the next frame (lockstep).
 * rand() needs no seed: the CRT state starts at 1 and the game never calls
 * srand, so equal frame counts give equal random sequences.
 * Replay mode is silent (sound would follow real time).
 */
#define TITBIT_COMPAT_IMPLEMENTATION
#include <windows.h>
#include "compat.h"
#include "compat_internal.h"
#pragma intrinsic(memset, memcpy, memcmp)
inline void *__cdecl operator new(unsigned, void *place) { return place; }
inline void __cdecl operator delete(void *, void *) {}

enum { MODE_OFF, MODE_RECORD, MODE_PLAY };
static int mode;
static HANDLE replay_file, frame_log;
static unsigned long frame;               /* SwapBuffers count */
static unsigned long virtual_ms = 1000000;
static unsigned long frame_ms = 40;
static unsigned long dump_list[16];
static int dump_count;

/* originals */
static DWORD (WINAPI *real_timeGetTime)(void);
static BOOL (WINAPI *real_QueryPerformanceCounter)(LARGE_INTEGER *);
static BOOL (WINAPI *real_QueryPerformanceFrequency)(LARGE_INTEGER *);
static void (WINAPI *real_GetLocalTime)(SYSTEMTIME *);
static BOOL (WINAPI *real_SwapBuffers)(HDC);
static BOOL (WINAPI *real_PeekMessageA)(MSG *, HWND, UINT, UINT, UINT);
static HRESULT (WINAPI *real_DirectInputCreateA)(HINSTANCE, DWORD, void **, IUnknown *);

/* ---- replay file: text lines "frame kind data" ---- */
static char in_buf[1 << 16];
static DWORD in_len, in_pos;

static void WriteText(HANDLE f, const char *s) {
 DWORD n;
 if (f != INVALID_HANDLE_VALUE && f) WriteFile(f, s, lstrlenA(s), &n, 0);
}

static int ReadLine(char *line, int size) {      /* 1 = got a line */
 int n = 0;
 for (;;) {
  if (in_pos >= in_len) {
   if (!ReadFile(replay_file, in_buf, sizeof(in_buf), &in_len, 0) || in_len == 0) { line[n] = 0; return n > 0; }
   in_pos = 0;
  }
  char c = in_buf[in_pos++];
  if (c == '\n') { line[n] = 0; return 1; }
  if (c != '\r' && n < size - 1) line[n++] = c;
 }
}

static unsigned long ParseHex(const char **p) {
 unsigned long v = 0;
 while (**p == ' ') (*p)++;
 if ((*p)[0] == '0' && ((*p)[1] == 'x' || (*p)[1] == 'X')) *p += 2;
 for (;; (*p)++) {
  char c = **p;
  if (c >= '0' && c <= '9') v = v * 16 + (c - '0');
  else if (c >= 'a' && c <= 'f') v = v * 16 + (c - 'a' + 10);
  else if (c >= 'A' && c <= 'F') v = v * 16 + (c - 'A' + 10);
  else break;
 }
 return v;
}

static void HexBytes(char *out, const BYTE *data, int n) {
 static const char digits[] = "0123456789abcdef";
 for (int i = 0; i < n; i++) { out[i * 2] = digits[data[i] >> 4]; out[i * 2 + 1] = digits[data[i] & 15]; }
 out[n * 2] = 0;
}

static void UnhexBytes(BYTE *data, const char *s, int n) {
 for (int i = 0; i < n && s[i * 2] && s[i * 2 + 1]; i++) {
  char t[3] = { s[i * 2], s[i * 2 + 1], 0 };
  const char *p = t;
  data[i] = (BYTE)ParseHex(&p);
 }
}

/* play mode: the next pending record */
struct Pending { int valid; unsigned long frame; char kind; char data[600]; };
static Pending next_rec;
static BYTE play_keys[256], play_mouse[16];

static void LoadNext() {
 char line[700];
 next_rec.valid = 0;
 while (ReadLine(line, sizeof(line))) {
  const char *p = line;
  next_rec.frame = ParseHex(&p);
  while (*p == ' ') p++;
  next_rec.kind = *p ? *p++ : 0;
  while (*p == ' ') p++;
  lstrcpynA(next_rec.data, p, sizeof(next_rec.data));
  next_rec.valid = 1;
  return;
 }
}

/* apply all device records of frames <= current; returns a message record if one is due */
static int TakeDue(MSG *msg) {
 while (next_rec.valid && next_rec.frame <= frame) {
  const char *p = next_rec.data;
  if (next_rec.kind == 'K') UnhexBytes(play_keys, p, 256);
  else if (next_rec.kind == 'D') UnhexBytes(play_mouse, p, 16);
  else if (next_rec.kind == 'M' && msg) {
   msg->message = ParseHex(&p);
   msg->wParam = ParseHex(&p);
   msg->lParam = ParseHex(&p);
   LoadNext();
   return 1;
  }
  LoadNext();
 }
 return 0;
}

/* ---- virtual audio device (Audiere's WinMM output) ----
 * Audiere plays through waveOutWrite and polls WHDR_DONE from its own thread.
 * Here buffers are "played" by the virtual clock: a buffer is done once the
 * virtual time passes its end, so a sound lasts the same number of frames in
 * every run (scripts wait for sounds to end). Nothing reaches the sound card. */
struct FakeWave { WAVEHDR *queue[64]; unsigned long end_ms[64]; int count; unsigned long last_end; DWORD bytes_per_sec; };
static FakeWave fake_wave;
static CRITICAL_SECTION wave_lock;

static MMRESULT WINAPI VWaveOutOpen(HWAVEOUT *out, UINT, const WAVEFORMATEX *fmt, DWORD, DWORD, DWORD flags) {
 if (flags & WAVE_FORMAT_QUERY) return MMSYSERR_NOERROR;
 EnterCriticalSection(&wave_lock);
 fake_wave.count = 0;
 fake_wave.last_end = 0;
 fake_wave.bytes_per_sec = fmt && fmt->nAvgBytesPerSec ? fmt->nAvgBytesPerSec : 44100 * 4;
 LeaveCriticalSection(&wave_lock);
 if (out) *out = (HWAVEOUT)&fake_wave;
 return MMSYSERR_NOERROR;
}
static MMRESULT WINAPI VWaveOutPrepareHeader(HWAVEOUT, WAVEHDR *h, UINT) { h->dwFlags |= WHDR_PREPARED; return MMSYSERR_NOERROR; }
static MMRESULT WINAPI VWaveOutUnprepareHeader(HWAVEOUT, WAVEHDR *h, UINT) { h->dwFlags &= ~WHDR_PREPARED; return MMSYSERR_NOERROR; }
static MMRESULT WINAPI VWaveOutWrite(HWAVEOUT, WAVEHDR *h, UINT) {
 EnterCriticalSection(&wave_lock);
 h->dwFlags = (h->dwFlags & ~WHDR_DONE) | WHDR_INQUEUE;
 if (fake_wave.count < 64) {
  unsigned long start = fake_wave.last_end > virtual_ms ? fake_wave.last_end : virtual_ms;
  unsigned long end = start + (unsigned long)MulDiv((int)h->dwBufferLength, 1000, (int)fake_wave.bytes_per_sec);
  fake_wave.queue[fake_wave.count] = h;
  fake_wave.end_ms[fake_wave.count++] = end;
  fake_wave.last_end = end;
 } else {
  h->dwFlags = (h->dwFlags & ~WHDR_INQUEUE) | WHDR_DONE;
 }
 LeaveCriticalSection(&wave_lock);
 return MMSYSERR_NOERROR;
}
static void CompleteWave(unsigned long now, bool all) {
 EnterCriticalSection(&wave_lock);
 int kept = 0;
 for (int i = 0; i < fake_wave.count; i++) {
  if (all || fake_wave.end_ms[i] <= now) {
   fake_wave.queue[i]->dwFlags = (fake_wave.queue[i]->dwFlags & ~WHDR_INQUEUE) | WHDR_DONE;
  } else {
   fake_wave.queue[kept] = fake_wave.queue[i];
   fake_wave.end_ms[kept++] = fake_wave.end_ms[i];
  }
 }
 fake_wave.count = kept;
 if (all) fake_wave.last_end = 0;
 LeaveCriticalSection(&wave_lock);
}
static MMRESULT WINAPI VWaveOutReset(HWAVEOUT) { CompleteWave(0, true); return MMSYSERR_NOERROR; }
static MMRESULT WINAPI VWaveOutClose(HWAVEOUT) { CompleteWave(0, true); return MMSYSERR_NOERROR; }

/* ---- worker threads in lockstep with frames ----
 * Sleep from any thread but the main one waits for the next frame, and every
 * frame waits briefly until a worker has parked again: background loops (the
 * audio update) run one iteration per frame instead of per wall-clock interval. */
static DWORD main_thread;
static volatile LONG frame_generation;
static HANDLE tick_event, parked_event;
static void (WINAPI *real_Sleep)(DWORD);

static void WINAPI VSleep(DWORD ms) {
 if (GetCurrentThreadId() == main_thread) { real_Sleep(ms); return; }
 LONG g = frame_generation;
 SetEvent(parked_event);
 for (int guard = 0; frame_generation == g && guard < 400; guard++) WaitForSingleObject(tick_event, 5);
}

static void FrameTick() {
 CompleteWave(virtual_ms, false);
 InterlockedIncrement((LONG *)&frame_generation);
 PulseEvent(tick_event);
 WaitForSingleObject(parked_event, 30);
}

/* ---- virtual clock ---- */
static DWORD WINAPI VTimeGetTime(void) { return virtual_ms; }
static BOOL WINAPI VQueryPerformanceFrequency(LARGE_INTEGER *f) { f->QuadPart = 1000000; return TRUE; }
static BOOL WINAPI VQueryPerformanceCounter(LARGE_INTEGER *c) { c->QuadPart = (LONGLONG)virtual_ms * 1000; return TRUE; }
static void WINAPI VGetLocalTime(SYSTEMTIME *t) {
 memset(t, 0, sizeof(*t));
 t->wYear = 2003; t->wMonth = 1; t->wDay = 15; t->wHour = 12;
 t->wMinute = (WORD)(virtual_ms / 60000 % 60); t->wSecond = (WORD)(virtual_ms / 1000 % 60);
}

/* ---- frame hash ---- */
typedef void (WINAPI *ReadPixelsFn)(int, int, int, int, unsigned, unsigned, void *);
typedef void (WINAPI *GetIntegervFn)(unsigned, int *);
static ReadPixelsFn gl_read_pixels;
static GetIntegervFn gl_get_integerv;
static BYTE *pixels;
static unsigned pixels_size;

static char shot_path[MAX_PATH], shot_request[MAX_PATH + 8];   /* shot_file: <path>.req asks for one frame */
static bool pixels_swapped;                 /* R/B already swapped for a BMP this frame */

static void WriteBmp(const char *name, int w, int h) {
 HANDLE f = CreateFileA(name, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
 if (f == INVALID_HANDLE_VALUE) return;
 BITMAPFILEHEADER fh; BITMAPINFOHEADER ih;
 memset(&fh, 0, sizeof(fh)); memset(&ih, 0, sizeof(ih));
 fh.bfType = 0x4d42; fh.bfOffBits = sizeof(fh) + sizeof(ih); fh.bfSize = fh.bfOffBits + w * h * 4;
 ih.biSize = sizeof(ih); ih.biWidth = w; ih.biHeight = h; ih.biPlanes = 1; ih.biBitCount = 32;
 DWORD n2;
 WriteFile(f, &fh, sizeof(fh), &n2, 0); WriteFile(f, &ih, sizeof(ih), &n2, 0);
 if (!pixels_swapped)
  for (int i = 0; i < w * h; i++) { BYTE t = pixels[i * 4]; pixels[i * 4] = pixels[i * 4 + 2]; pixels[i * 4 + 2] = t; }
 pixels_swapped = true;
 WriteFile(f, pixels, w * h * 4, &n2, 0);
 CloseHandle(f);
}

static void DumpBmp(unsigned long n, int w, int h) {
 char name[64];
 wsprintfA(name, "frame_%lu.bmp", n);
 WriteBmp(name, w, h);
}

static void HashFrame() {
 if (!gl_read_pixels) {
  HMODULE gl = GetModuleHandleA("opengl32.dll");
  if (!gl) return;
  gl_read_pixels = (ReadPixelsFn)GetProcAddress(gl, "glReadPixels");
  gl_get_integerv = (GetIntegervFn)GetProcAddress(gl, "glGetIntegerv");
  if (!gl_read_pixels || !gl_get_integerv) { gl_read_pixels = 0; return; }
 }
 int vp[4] = { 0, 0, 0, 0 };
 gl_get_integerv(0x0BA2, vp);                 /* GL_VIEWPORT */
 int w = vp[2], h = vp[3];
 if (w <= 0 || h <= 0 || w > 4096 || h > 4096) return;
 unsigned need = (unsigned)w * h * 4;
 if (need > pixels_size) { CompatFree(pixels); pixels = (BYTE *)CompatAlloc(need); pixels_size = pixels ? need : 0; }
 if (!pixels) return;
 gl_read_pixels(vp[0], vp[1], w, h, 0x1908, 0x1401, pixels);   /* GL_RGBA, GL_UNSIGNED_BYTE */
 unsigned long hash = 2166136261UL;
 for (unsigned i = 0; i < need; i++) hash = (hash ^ pixels[i]) * 16777619UL;
 char line[80];
 wsprintfA(line, "%lu %08lx\r\n", frame, hash);
 WriteText(frame_log, line);
 pixels_swapped = false;
 for (int k = 0; k < dump_count; k++)
  if (dump_list[k] == frame) DumpBmp(frame, w, h);
 if (shot_path[0] && GetFileAttributesA(shot_request) != (DWORD)-1) {
  WriteBmp(shot_path, w, h);                  /* screenshots of runs on a hidden desktop (PrintWindow sees nothing there) */
  DeleteFileA(shot_request);
 }
}

static BOOL WINAPI VSwapBuffers(HDC dc) {
 frame++;
 HashFrame();
 BOOL r = real_SwapBuffers(dc);
 virtual_ms += frame_ms;
 FrameTick();
 return r;
}

/* ---- window messages ---- */
static bool IsInputMessage(UINT m) {
 return (m >= WM_KEYFIRST && m <= WM_KEYLAST) || (m >= WM_MOUSEFIRST && m <= 0x020E);
}

static HWND game_hwnd;

/* synth_input=1 (record mode): when the real DirectInput device delivers no
 * state (not acquired - e.g. the game runs on a hidden desktop without focus),
 * build it from the window input messages the game received: mouse buttons and
 * keys. The synthesized state is what gets recorded, so replays reproduce it. */
static bool synth_input;
static BYTE synth_keys[256], synth_buttons[4];
static long synth_x, synth_y, synth_dx, synth_dy;
static bool synth_have_pos;

static void SynthFromMessage(const MSG *m) {
 if (m->message >= WM_MOUSEFIRST && m->message <= WM_MBUTTONDBLCLK) {   /* client position -> deltas */
  long x = (short)LOWORD(m->lParam), y = (short)HIWORD(m->lParam);
  if (synth_have_pos) { synth_dx += x - synth_x; synth_dy += y - synth_y; }
  synth_x = x; synth_y = y; synth_have_pos = true;
 }
 switch (m->message) {
 case WM_LBUTTONDOWN: synth_buttons[0] = 0x80; break;
 case WM_LBUTTONUP: synth_buttons[0] = 0; break;
 case WM_RBUTTONDOWN: synth_buttons[1] = 0x80; break;
 case WM_RBUTTONUP: synth_buttons[1] = 0; break;
 case WM_MBUTTONDOWN: synth_buttons[2] = 0x80; break;
 case WM_MBUTTONUP: synth_buttons[2] = 0; break;
 case WM_KEYDOWN: case WM_KEYUP: case WM_SYSKEYDOWN: case WM_SYSKEYUP: {
  UINT scan = (UINT)((m->lParam >> 16) & 0xff);
  if (!scan) scan = MapVirtualKeyA((UINT)m->wParam, 0);
  if (m->lParam & 0x01000000) scan |= 0x80;          /* extended key -> DIK_ 0x80+ */
  if (scan < 256) synth_keys[scan] = (m->message == WM_KEYDOWN || m->message == WM_SYSKEYDOWN) ? 0x80 : 0;
  break;
 }
 }
}

static BOOL WINAPI VPeekMessageA(MSG *msg, HWND hwnd, UINT first, UINT last, UINT remove) {
 if (mode == MODE_PLAY) {
  MSG due;
  if ((remove & PM_REMOVE) && TakeDue(&due)) {
   memset(msg, 0, sizeof(*msg));
   msg->hwnd = game_hwnd ? game_hwnd : FindWindowA("OpenGL", 0);
   msg->message = due.message; msg->wParam = due.wParam; msg->lParam = due.lParam;
   msg->time = virtual_ms;
   return TRUE;
  }
  for (;;) {
   BOOL got = real_PeekMessageA(msg, hwnd, first, last, remove);
   if (!got) return FALSE;
   if (msg->hwnd && !game_hwnd) {
    char cls[16];
    if (GetClassNameA(msg->hwnd, cls, sizeof(cls)) && lstrcmpA(cls, "OpenGL") == 0) game_hwnd = msg->hwnd;
   }
   if (!(remove & PM_REMOVE) || !IsInputMessage(msg->message)) return TRUE;   /* drop real input */
  }
 }
 BOOL got = real_PeekMessageA(msg, hwnd, first, last, remove);
 if (got && (remove & PM_REMOVE) && IsInputMessage(msg->message)) {
  if (synth_input) SynthFromMessage(msg);
  char line[80];
  wsprintfA(line, "%lx M %lx %lx %lx\r\n", frame, (unsigned long)msg->message, (unsigned long)msg->wParam, (unsigned long)msg->lParam);
  WriteText(replay_file, line);
 }
 return got;
}

/* ---- DirectInput proxies (dinput 5 vtable order) ---- */
struct RealDevice : public IUnknown {
 virtual HRESULT __stdcall GetCapabilities(void *) = 0;
 virtual HRESULT __stdcall EnumObjects(void *, void *, DWORD) = 0;
 virtual HRESULT __stdcall GetProperty(const GUID &, void *) = 0;
 virtual HRESULT __stdcall SetProperty(const GUID &, const void *) = 0;
 virtual HRESULT __stdcall Acquire() = 0;
 virtual HRESULT __stdcall Unacquire() = 0;
 virtual HRESULT __stdcall GetDeviceState(DWORD, void *) = 0;
 virtual HRESULT __stdcall GetDeviceData(DWORD, void *, DWORD *, DWORD) = 0;
 virtual HRESULT __stdcall SetDataFormat(const void *) = 0;
 virtual HRESULT __stdcall SetEventNotification(HANDLE) = 0;
 virtual HRESULT __stdcall SetCooperativeLevel(HWND, DWORD) = 0;
 virtual HRESULT __stdcall GetObjectInfo(void *, DWORD, DWORD) = 0;
 virtual HRESULT __stdcall GetDeviceInfo(void *) = 0;
 virtual HRESULT __stdcall RunControlPanel(HWND, DWORD) = 0;
 virtual HRESULT __stdcall Initialize(HINSTANCE, DWORD, const GUID &) = 0;
};

class DeviceProxy : public RealDevice {
 RealDevice *real;
 char kind;                                 /* 'K' keyboard, 'D' mouse */
 BYTE last[256];
 int have_last;
public:
 DeviceProxy(RealDevice *r, char k) : real(r), kind(k), have_last(0) {}
 HRESULT __stdcall QueryInterface(REFIID iid, void **out) { return real->QueryInterface(iid, out); }
 ULONG __stdcall AddRef() { return real->AddRef(); }
 ULONG __stdcall Release() { ULONG n = real->Release(); if (!n) CompatFree(this); return n; }
 HRESULT __stdcall GetCapabilities(void *a) { return real->GetCapabilities(a); }
 HRESULT __stdcall EnumObjects(void *a, void *b, DWORD c) { return real->EnumObjects(a, b, c); }
 HRESULT __stdcall GetProperty(const GUID &a, void *b) { return real->GetProperty(a, b); }
 HRESULT __stdcall SetProperty(const GUID &a, const void *b) { return real->SetProperty(a, b); }
 HRESULT __stdcall Acquire() {
  if (mode == MODE_PLAY) return S_OK;
  HRESULT r = real->Acquire();
  return FAILED(r) && synth_input ? S_OK : r;   /* InitGameInput gives up on input when Acquire fails */
 }
 HRESULT __stdcall Unacquire() { return real->Unacquire(); }
 HRESULT __stdcall GetDeviceState(DWORD size, void *data) {
  if (size > 256) size = 256;
  if (mode == MODE_PLAY) {
   TakeDue(0);
   memcpy(data, kind == 'K' ? play_keys : play_mouse, size);
   if (kind == 'D' && size >= 8) memset(play_mouse, 0, 8);   /* deltas are per poll */
   return S_OK;
  }
  HRESULT r = real->GetDeviceState(size, data);
  if (r != S_OK) {
   memset(data, 0, size);
   if (synth_input) {
    if (kind == 'K') memcpy(data, synth_keys, size);
    else if (size >= 16) {                  /* DIMOUSESTATE: lX, lY (deltas since the last poll), lZ, rgbButtons */
     ((long *)data)[0] = synth_dx; ((long *)data)[1] = synth_dy;
     synth_dx = synth_dy = 0;
     memcpy((BYTE *)data + 12, synth_buttons, 4);
    }
   }
  }
  if (!have_last || memcmp(last, data, size) != 0 || (kind == 'D' && size >= 8 && (((long *)data)[0] || ((long *)data)[1]))) {
   char line[600];
   wsprintfA(line, "%lx %c ", frame, kind);
   HexBytes(line + lstrlenA(line), (const BYTE *)data, (int)size);
   lstrcatA(line, "\r\n");
   WriteText(replay_file, line);
   memcpy(last, data, size);
   have_last = 1;
  }
  return S_OK;
 }
 HRESULT __stdcall GetDeviceData(DWORD a, void *b, DWORD *c, DWORD d) { return real->GetDeviceData(a, b, c, d); }
 HRESULT __stdcall SetDataFormat(const void *a) { return real->SetDataFormat(a); }
 HRESULT __stdcall SetEventNotification(HANDLE a) { return real->SetEventNotification(a); }
 HRESULT __stdcall SetCooperativeLevel(HWND a, DWORD b) { return real->SetCooperativeLevel(a, b); }
 HRESULT __stdcall GetObjectInfo(void *a, DWORD b, DWORD c) { return real->GetObjectInfo(a, b, c); }
 HRESULT __stdcall GetDeviceInfo(void *a) { return real->GetDeviceInfo(a); }
 HRESULT __stdcall RunControlPanel(HWND a, DWORD b) { return real->RunControlPanel(a, b); }
 HRESULT __stdcall Initialize(HINSTANCE a, DWORD b, const GUID &c) { return real->Initialize(a, b, c); }
};

struct RealInput : public IUnknown {
 virtual HRESULT __stdcall CreateDevice(const GUID &, RealDevice **, IUnknown *) = 0;
 virtual HRESULT __stdcall EnumDevices(DWORD, void *, void *, DWORD) = 0;
 virtual HRESULT __stdcall GetDeviceStatus(const GUID &) = 0;
 virtual HRESULT __stdcall RunControlPanel(HWND, DWORD) = 0;
 virtual HRESULT __stdcall Initialize(HINSTANCE, DWORD) = 0;
};

static const GUID GUID_CompatSysKeyboard = {0x6F1D2B61, 0xD5A0, 0x11CF, {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};

class InputProxy : public RealInput {
 RealInput *real;
public:
 InputProxy(RealInput *r) : real(r) {}
 HRESULT __stdcall QueryInterface(REFIID iid, void **out) { return real->QueryInterface(iid, out); }
 ULONG __stdcall AddRef() { return real->AddRef(); }
 ULONG __stdcall Release() { ULONG n = real->Release(); if (!n) CompatFree(this); return n; }
 HRESULT __stdcall CreateDevice(const GUID &g, RealDevice **out, IUnknown *outer) {
  RealDevice *dev = 0;
  HRESULT r = real->CreateDevice(g, &dev, outer);
  if (FAILED(r) || !dev) { *out = dev; return r; }
  void *mem = CompatAlloc(sizeof(DeviceProxy));
  *out = mem ? new (mem) DeviceProxy(dev, IsEqualGUID(g, GUID_CompatSysKeyboard) ? 'K' : 'D') : dev;
  return r;
 }
 HRESULT __stdcall EnumDevices(DWORD a, void *b, void *c, DWORD d) { return real->EnumDevices(a, b, c, d); }
 HRESULT __stdcall GetDeviceStatus(const GUID &a) { return real->GetDeviceStatus(a); }
 HRESULT __stdcall RunControlPanel(HWND a, DWORD b) { return real->RunControlPanel(a, b); }
 HRESULT __stdcall Initialize(HINSTANCE a, DWORD b) { return real->Initialize(a, b); }
};

static HRESULT WINAPI VDirectInputCreateA(HINSTANCE instance, DWORD version, void **out, IUnknown *outer) {
 RealInput *in = 0;
 HRESULT r = real_DirectInputCreateA(instance, version, (void **)&in, outer);
 if (FAILED(r) || !in) { *out = in; return r; }
 void *mem = CompatAlloc(sizeof(InputProxy));
 *out = mem ? (void *)new (mem) InputProxy(in) : (void *)in;
 return r;
}

/* ---- import address table patching ---- */
static void *PatchImport(const char *dll, const char *name, void *wrapper) {
 BYTE *base = (BYTE *)GetModuleHandleA(0);
 IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
 IMAGE_DATA_DIRECTORY dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
 if (!dir.VirtualAddress) return 0;
 for (IMAGE_IMPORT_DESCRIPTOR *d = (IMAGE_IMPORT_DESCRIPTOR *)(base + dir.VirtualAddress); d->Name; d++) {
  if (lstrcmpiA((const char *)(base + d->Name), dll) != 0) continue;
  /* thunks as plain DWORDs: the VC6 SDK declares the IMAGE_THUNK_DATA fields as pointers */
  DWORD *names = (DWORD *)(base + (d->OriginalFirstThunk ? (DWORD)d->OriginalFirstThunk : d->FirstThunk));
  DWORD *slots = (DWORD *)(base + (DWORD)d->FirstThunk);
  for (; *names; names++, slots++) {
   if (*names & IMAGE_ORDINAL_FLAG) continue;
   IMAGE_IMPORT_BY_NAME *ibn = (IMAGE_IMPORT_BY_NAME *)(base + *names);
   if (lstrcmpA((const char *)ibn->Name, name) != 0) continue;
   DWORD old;
   void *original = (void *)*slots;
   VirtualProtect(slots, sizeof(DWORD), PAGE_EXECUTE_READWRITE, &old);
   *slots = (DWORD)wrapper;
   VirtualProtect(slots, sizeof(DWORD), old, &old);
   return original;
  }
 }
 CompatLog("replay", "import not found", 0);
 return 0;
}

extern "C" void __stdcall CompatStartup(void) {
 char value[MAX_PATH];
 CompatSetting("replay", "off", value, sizeof(value));
 if (lstrcmpiA(value, "record") == 0) mode = MODE_RECORD;
 else if (lstrcmpiA(value, "play") == 0) mode = MODE_PLAY;
 else return;
 CompatSetting("frame_ms", "40", value, sizeof(value));
 { const char *p = value; unsigned long v = 0; while (*p >= '0' && *p <= '9') v = v * 10 + (*p++ - '0'); if (v) frame_ms = v; }
 CompatSetting("dump_frames", "", value, sizeof(value));       /* "120,250,400" */
 for (const char *p = value; *p && dump_count < 16; ) {
  unsigned long v = 0;
  while (*p >= '0' && *p <= '9') v = v * 10 + (*p++ - '0');
  if (v) dump_list[dump_count++] = v;
  while (*p && (*p < '0' || *p > '9')) p++;
 }
 CompatSetting("synth_input", "0", value, sizeof(value));
 synth_input = value[0] == '1';
 CompatSetting("shot_file", "", shot_path, sizeof(shot_path));
 if (shot_path[0]) { lstrcpyA(shot_request, shot_path); lstrcatA(shot_request, ".req"); }
 CompatSetting("replay_file", "titbit_replay.txt", value, sizeof(value));
 replay_file = CreateFileA(value, mode == MODE_RECORD ? GENERIC_WRITE : GENERIC_READ, FILE_SHARE_READ, 0,
                           mode == MODE_RECORD ? CREATE_ALWAYS : OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
 CompatSetting("framelog", "titbit_frames.txt", value, sizeof(value));
 frame_log = CreateFileA(value, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
 if (mode == MODE_PLAY) LoadNext();
 main_thread = GetCurrentThreadId();
 InitializeCriticalSection(&wave_lock);
 tick_event = CreateEventA(0, TRUE, FALSE, 0);
 parked_event = CreateEventA(0, FALSE, FALSE, 0);

 *(void **)&real_timeGetTime = PatchImport("WINMM.dll", "timeGetTime", (void *)VTimeGetTime);
 *(void **)&real_QueryPerformanceCounter = PatchImport("KERNEL32.dll", "QueryPerformanceCounter", (void *)VQueryPerformanceCounter);
 *(void **)&real_QueryPerformanceFrequency = PatchImport("KERNEL32.dll", "QueryPerformanceFrequency", (void *)VQueryPerformanceFrequency);
 *(void **)&real_GetLocalTime = PatchImport("KERNEL32.dll", "GetLocalTime", (void *)VGetLocalTime);
 *(void **)&real_SwapBuffers = PatchImport("GDI32.dll", "SwapBuffers", (void *)VSwapBuffers);
 *(void **)&real_PeekMessageA = PatchImport("USER32.dll", "PeekMessageA", (void *)VPeekMessageA);
 *(void **)&real_DirectInputCreateA = PatchImport("DINPUT.dll", "DirectInputCreateA", (void *)VDirectInputCreateA);
 *(void **)&real_Sleep = PatchImport("KERNEL32.dll", "Sleep", (void *)VSleep);
 if (!real_Sleep) *(void **)&real_Sleep = GetProcAddress(GetModuleHandleA("kernel32.dll"), "Sleep");
 PatchImport("WINMM.dll", "waveOutOpen", (void *)VWaveOutOpen);
 PatchImport("WINMM.dll", "waveOutPrepareHeader", (void *)VWaveOutPrepareHeader);
 PatchImport("WINMM.dll", "waveOutUnprepareHeader", (void *)VWaveOutUnprepareHeader);
 PatchImport("WINMM.dll", "waveOutWrite", (void *)VWaveOutWrite);
 PatchImport("WINMM.dll", "waveOutReset", (void *)VWaveOutReset);
 PatchImport("WINMM.dll", "waveOutClose", (void *)VWaveOutClose);
 CompatLog("replay", mode == MODE_RECORD ? "recording, frame ms" : "replaying, frame ms", (long)frame_ms);
}
