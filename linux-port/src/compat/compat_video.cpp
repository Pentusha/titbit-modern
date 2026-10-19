/* AVI frame decompression for modern Windows (TITBIT_COMPAT).
 *
 * The game plays its videos through Video for Windows: AVIStreamGetFrameOpen /
 * AVIStreamGetFrame / AVIStreamGetFrameClose, then DrawDibDraw of the returned
 * packed DIB. The videos are 'MP42' (MS-MPEG4 v2, 24-bit) and 'CRAM'
 * (Microsoft Video 1, 16-bit). Windows 10/11 ship no VfW (VCM) decoder for MP42,
 * so the original AVIStreamGetFrameOpen fails and the game crashes.
 *
 * CompatAVIStreamGetFrameOpen first asks VfW (unchanged behaviour wherever a
 * codec is installed). If VfW cannot decompress the stream it builds its own
 * frame source that reads the raw samples with AVIStreamRead and decodes them:
 *   - MP42/MPG4: the "Mpeg4 Decoder DMO" (mpg4decd.dll) and MP43: the
 *     "Mpeg43 Decoder DMO" (mp43decd.dll), both Media Foundation components
 *     that ship with Windows, used through IMediaObject;
 *   - CRAM/MSVC (16-bit): a small decoder in this file;
 *   - anything else, or no decoder available (Windows "N" editions without
 *     Media Foundation): black frames, so the game keeps running.
 * Frames are returned as bottom-up 24-bit packed DIBs (BITMAPINFOHEADER +
 * pixels), the format DrawDibDraw accepts. No third-party code is bundled.
 * Compiles with VC6 and current MSVC (DMO interfaces are declared here).
 */
#define TITBIT_COMPAT_IMPLEMENTATION
#include <windows.h>
#include <objbase.h>
#include <vfw.h>
#include <string.h>
#pragma intrinsic(memset, memcpy)
#include "compat.h"
#include "compat_internal.h"

/* ---- DirectX Media Object interfaces (mediaobj.h), declared locally ---- */
typedef LONGLONG CompatRefTime;
struct CompatMediaType {               /* DMO_MEDIA_TYPE == AM_MEDIA_TYPE */
 GUID majortype, subtype;
 BOOL bFixedSizeSamples, bTemporalCompression;
 ULONG lSampleSize;
 GUID formattype;
 IUnknown *pUnk;
 ULONG cbFormat;
 BYTE *pbFormat;
};
struct CompatVideoInfo {               /* VIDEOINFOHEADER */
 RECT rcSource, rcTarget;
 DWORD dwBitRate, dwBitErrorRate;
 LONGLONG AvgTimePerFrame;
 BITMAPINFOHEADER bmiHeader;
};
struct CompatMediaBuffer : public IUnknown {
 virtual HRESULT __stdcall SetLength(DWORD length) = 0;
 virtual HRESULT __stdcall GetMaxLength(DWORD *max_length) = 0;
 virtual HRESULT __stdcall GetBufferAndLength(BYTE **buffer, DWORD *length) = 0;
};
struct CompatOutputBuffer {            /* DMO_OUTPUT_DATA_BUFFER */
 CompatMediaBuffer *pBuffer;
 DWORD dwStatus;
 CompatRefTime rtTimestamp, rtTimelength;
};
struct CompatMediaObject : public IUnknown {
 virtual HRESULT __stdcall GetStreamCount(DWORD *inputs, DWORD *outputs) = 0;
 virtual HRESULT __stdcall GetInputStreamInfo(DWORD index, DWORD *flags) = 0;
 virtual HRESULT __stdcall GetOutputStreamInfo(DWORD index, DWORD *flags) = 0;
 virtual HRESULT __stdcall GetInputType(DWORD index, DWORD type_index, CompatMediaType *type) = 0;
 virtual HRESULT __stdcall GetOutputType(DWORD index, DWORD type_index, CompatMediaType *type) = 0;
 virtual HRESULT __stdcall SetInputType(DWORD index, const CompatMediaType *type, DWORD flags) = 0;
 virtual HRESULT __stdcall SetOutputType(DWORD index, const CompatMediaType *type, DWORD flags) = 0;
 virtual HRESULT __stdcall GetInputCurrentType(DWORD index, CompatMediaType *type) = 0;
 virtual HRESULT __stdcall GetOutputCurrentType(DWORD index, CompatMediaType *type) = 0;
 virtual HRESULT __stdcall GetInputSizeInfo(DWORD index, DWORD *size, DWORD *lookahead, DWORD *alignment) = 0;
 virtual HRESULT __stdcall GetOutputSizeInfo(DWORD index, DWORD *size, DWORD *alignment) = 0;
 virtual HRESULT __stdcall GetInputMaxLatency(DWORD index, CompatRefTime *latency) = 0;
 virtual HRESULT __stdcall SetInputMaxLatency(DWORD index, CompatRefTime latency) = 0;
 virtual HRESULT __stdcall Flush() = 0;
 virtual HRESULT __stdcall Discontinuity(DWORD index) = 0;
 virtual HRESULT __stdcall AllocateStreamingResources() = 0;
 virtual HRESULT __stdcall FreeStreamingResources() = 0;
 virtual HRESULT __stdcall GetInputStatus(DWORD index, DWORD *flags) = 0;
 virtual HRESULT __stdcall ProcessInput(DWORD index, CompatMediaBuffer *buffer, DWORD flags,
                                        CompatRefTime timestamp, CompatRefTime timelength) = 0;
 virtual HRESULT __stdcall ProcessOutput(DWORD flags, DWORD count, CompatOutputBuffer *buffers, DWORD *status) = 0;
 virtual HRESULT __stdcall Lock(LONG lock) = 0;
};

static const GUID IID_CompatMediaObject = {0xd8ad0f58, 0x5494, 0x4102, {0x97, 0xc5, 0xec, 0x79, 0x8e, 0x59, 0xbc, 0xf4}};
static const GUID IID_CompatUnknown = {0x00000000, 0x0000, 0x0000, {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
static const GUID IID_CompatMediaBuffer = {0x59eff8b9, 0x938c, 0x4a26, {0x82, 0xf2, 0x95, 0xcb, 0x84, 0xcd, 0xc8, 0x37}};
static const GUID CLSID_CompatMpeg43Decoder = {0xcba9e78b, 0x49a3, 0x49ea, {0x93, 0xd4, 0x6b, 0xcb, 0xa8, 0xc4, 0xde, 0x07}};  /* mp43decd.dll: MP43 */
static const GUID CLSID_CompatMpeg4Decoder = {0xf371728a, 0x6052, 0x4d47, {0x82, 0x7c, 0xd0, 0x39, 0x33, 0x5d, 0xfe, 0x0a}};   /* mpg4decd.dll: MP42, MPG4 */
static const GUID COMPAT_MEDIATYPE_Video = {0x73646976, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
static const GUID COMPAT_FORMAT_VideoInfo = {0x05589f80, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}};
static const GUID COMPAT_SUBTYPE_RGB24 = {0xe436eb7d, 0x524f, 0x11ce, {0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};
static const GUID COMPAT_SUBTYPE_RGB32 = {0xe436eb7e, 0x524f, 0x11ce, {0x9f, 0x53, 0x00, 0x20, 0xaf, 0x0b, 0xa7, 0x70}};

#define COMPAT_FOURCC(a, b, c, d) ((DWORD)(BYTE)(a) | ((DWORD)(BYTE)(b) << 8) | ((DWORD)(BYTE)(c) << 16) | ((DWORD)(BYTE)(d) << 24))
#define DMO_SYNCPOINT 0x1
#define DMO_INCOMPLETE 0x01000000

static void Log(const char *what, long value) { CompatLog("video", what, value); }

static GUID FourccSubtype(DWORD fourcc) {
 GUID g = COMPAT_MEDIATYPE_Video;     /* {XXXXXXXX-0000-0010-8000-00AA00389B71} */
 g.Data1 = fourcc;
 return g;
}

static DWORD UpperFourcc(DWORD f) {
 DWORD r = 0;
 for (int i = 0; i < 4; i++) {
  BYTE c = (BYTE)(f >> (i * 8));
  if (c >= 'a' && c <= 'z') c = (BYTE)(c - 32);
  r |= (DWORD)c << (i * 8);
 }
 return r;
}

static void FreeMediaType(CompatMediaType *t) {
 if (t->pbFormat) CoTaskMemFree(t->pbFormat);
 if (t->pUnk) t->pUnk->Release();
 t->pbFormat = 0;
 t->pUnk = 0;
}

/* A plain memory IMediaBuffer over caller-owned storage. */
class FixedBuffer : public CompatMediaBuffer {
 BYTE *data;
 DWORD length, capacity;
public:
 FixedBuffer(BYTE *storage, DWORD size, DWORD used) : data(storage), length(used), capacity(size) {}
 HRESULT __stdcall QueryInterface(REFIID iid, void **out) {
  if (IsEqualGUID(iid, IID_CompatUnknown) || IsEqualGUID(iid, IID_CompatMediaBuffer)) { *out = this; return S_OK; }
  *out = 0;
  return E_NOINTERFACE;
 }
 ULONG __stdcall AddRef() { return 2; }       /* stack object, never freed by COM */
 ULONG __stdcall Release() { return 1; }
 HRESULT __stdcall SetLength(DWORD n) { if (n > capacity) return E_INVALIDARG; length = n; return S_OK; }
 HRESULT __stdcall GetMaxLength(DWORD *n) { *n = capacity; return S_OK; }
 HRESULT __stdcall GetBufferAndLength(BYTE **p, DWORD *n) { if (p) *p = data; if (n) *n = length; return S_OK; }
 DWORD Length() const { return length; }
};

enum { KIND_BLACK, KIND_DMO, KIND_CRAM };
enum { OUT_RGB24, OUT_RGB32, OUT_YV12, OUT_I420, OUT_YUY2, OUT_NV12 };

struct CompatFrames {
 CompatFrames *next_open;
 PAVISTREAM stream;
 int kind;
 int width, height;
 long first, end;                     /* sample range */
 long last;                           /* last decoded sample, -1 = none */
 BITMAPINFOHEADER *dib;               /* returned frame: header + bottom-up BGR24 */
 BYTE *pixels;
 int stride;
 BYTE *sample;                        /* raw sample buffer */
 long sample_size;
 /* DMO path */
 CompatMediaObject *dmo;
 int out_kind;
 BYTE *out;
 DWORD out_size;
 int out_stride;
 bool out_topdown;
 /* CRAM path */
 WORD *cram;                          /* persistent 15-bit frame, bottom-up */
};

static CompatFrames *open_frames;     /* our objects; anything else came from VfW */

static bool IsOurs(void *p) {
 for (CompatFrames *f = open_frames; f; f = f->next_open)
  if (f == p) return true;
 return false;
}

/* ---- pixel conversion into the bottom-up BGR24 frame ---- */
static BYTE Clamp(int v) { return (BYTE)(v < 0 ? 0 : v > 255 ? 255 : v); }

static void PutYuv(BYTE *d, int y, int u, int v) {
 int c = (y - 16) * 298, du = u - 128, dv = v - 128;
 d[0] = Clamp((c + 516 * du + 128) >> 8);
 d[1] = Clamp((c - 100 * du - 208 * dv + 128) >> 8);
 d[2] = Clamp((c + 409 * dv + 128) >> 8);
}

static void ConvertDmoOutput(CompatFrames *f) {
 int w = f->width, h = f->height;
 for (int row = 0; row < h; row++) {
  int src_row = f->out_topdown ? (h - 1 - row) : row;   /* row 0 = bottom of the DIB */
  BYTE *d = f->pixels + row * f->stride;
  if (f->out_kind == OUT_RGB24) {
   memcpy(d, f->out + src_row * f->out_stride, w * 3);
  } else if (f->out_kind == OUT_RGB32) {
   const BYTE *s = f->out + src_row * f->out_stride;
   for (int x = 0; x < w; x++, s += 4, d += 3) { d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; }
  } else if (f->out_kind == OUT_YUY2) {
   const BYTE *s = f->out + src_row * f->out_stride;
   for (int x = 0; x < w; x += 2, s += 4, d += 6) {
    PutYuv(d, s[0], s[1], s[3]);
    if (x + 1 < w) PutYuv(d + 3, s[2], s[1], s[3]);
   }
  } else {                                              /* planar 4:2:0 */
   int ys = f->out_stride, cs = f->out_stride / 2;
   const BYTE *yp = f->out + src_row * ys;
   const BYTE *plane1 = f->out + ys * h, *plane2 = plane1 + cs * (h / 2);
   for (int x = 0; x < w; x++, d += 3) {
    int u, v;
    if (f->out_kind == OUT_NV12) {
     const BYTE *uv = f->out + ys * h + (src_row / 2) * ys + (x & ~1);
     u = uv[0]; v = uv[1];
    } else {
     int ci = (src_row / 2) * cs + x / 2;
     if (f->out_kind == OUT_YV12) { v = plane1[ci]; u = plane2[ci]; }
     else { u = plane1[ci]; v = plane2[ci]; }
    }
    PutYuv(d, yp[x], u, v);
   }
  }
 }
}

/* ---- Media Foundation "Mpeg43 Decoder DMO" ---- */
static bool OpenDmo(CompatFrames *f, const BITMAPINFOHEADER *in, const GUID &decoder) {
 HRESULT created = CoCreateInstance(decoder, 0, CLSCTX_INPROC_SERVER, IID_CompatMediaObject, (void **)&f->dmo);
 if (FAILED(created)) { Log("CoCreateInstance(Mpeg43 DMO)", created); f->dmo = 0; return false; }
 CompatVideoInfo info;
 memset(&info, 0, sizeof(info));
 info.bmiHeader = *in;
 info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
 info.bmiHeader.biCompression = UpperFourcc(in->biCompression);
 CompatMediaType type;
 memset(&type, 0, sizeof(type));
 type.majortype = COMPAT_MEDIATYPE_Video;
 type.subtype = FourccSubtype(info.bmiHeader.biCompression);
 type.bTemporalCompression = TRUE;
 type.formattype = COMPAT_FORMAT_VideoInfo;
 type.cbFormat = sizeof(info);
 type.pbFormat = (BYTE *)&info;
 HRESULT set = f->dmo->SetInputType(0, &type, 0);
 if (FAILED(set)) { Log("SetInputType", set); return false; }

 static const DWORD yuv[4] = { COMPAT_FOURCC('Y','V','1','2'), COMPAT_FOURCC('I','4','2','0'),
                               COMPAT_FOURCC('Y','U','Y','2'), COMPAT_FOURCC('N','V','1','2') };
 int best = -1, best_rank = 99;
 CompatMediaType offered;
 for (DWORD i = 0; ; i++) {
  memset(&offered, 0, sizeof(offered));
  if (f->dmo->GetOutputType(0, i, &offered) != S_OK) break;
  int rank = 99;
  if (IsEqualGUID(offered.subtype, COMPAT_SUBTYPE_RGB24)) rank = OUT_RGB24;
  else if (IsEqualGUID(offered.subtype, COMPAT_SUBTYPE_RGB32)) rank = OUT_RGB32;
  else if (IsEqualGUID(offered.subtype, FourccSubtype(yuv[0]))) rank = OUT_YV12;
  else if (IsEqualGUID(offered.subtype, FourccSubtype(yuv[1])) ||
           IsEqualGUID(offered.subtype, FourccSubtype(COMPAT_FOURCC('I','Y','U','V')))) rank = OUT_I420;
  else if (IsEqualGUID(offered.subtype, FourccSubtype(yuv[2]))) rank = OUT_YUY2;
  else if (IsEqualGUID(offered.subtype, FourccSubtype(yuv[3]))) rank = OUT_NV12;
  if (rank < best_rank && IsEqualGUID(offered.formattype, COMPAT_FORMAT_VideoInfo) && offered.cbFormat >= sizeof(CompatVideoInfo)) {
   best_rank = rank;
   best = (int)i;
  }
  FreeMediaType(&offered);
 }
 if (best < 0) { Log("no usable output type", 0); return false; }
 Log("output kind", best_rank);
 memset(&offered, 0, sizeof(offered));
 if (f->dmo->GetOutputType(0, best, &offered) != S_OK) return false;
 CompatVideoInfo *out = (CompatVideoInfo *)offered.pbFormat;
 f->out_kind = best_rank;
 f->out_topdown = out->bmiHeader.biHeight < 0;
 int bw = out->bmiHeader.biWidth > 0 ? out->bmiHeader.biWidth : f->width;
 if (best_rank == OUT_RGB24) f->out_stride = (bw * 3 + 3) & ~3;
 else if (best_rank == OUT_RGB32) f->out_stride = bw * 4;
 else if (best_rank == OUT_YUY2) f->out_stride = bw * 2;
 else f->out_stride = bw;
 HRESULT hr = f->dmo->SetOutputType(0, &offered, 0);
 FreeMediaType(&offered);
 if (FAILED(hr)) { Log("SetOutputType", hr); return false; }
 DWORD size = 0, alignment = 0;
 f->dmo->GetOutputSizeInfo(0, &size, &alignment);
 DWORD need = (DWORD)f->out_stride * f->height * 2;
 f->out_size = size > need ? size : need;
 f->out = (BYTE *)CompatAlloc(f->out_size);
 return f->out != 0;
}

static void DecodeDmo(CompatFrames *f, long position, DWORD bytes) {
 BOOL key = AVIStreamIsKeyFrame(f->stream, position);
 FixedBuffer input(f->sample, bytes, bytes);
 HRESULT hr = f->dmo->ProcessInput(0, &input, key ? DMO_SYNCPOINT : 0, 0, 0);
 for (int guard = 0; guard < 8; guard++) {
  FixedBuffer output(f->out, f->out_size, 0);
  CompatOutputBuffer buffer;
  memset(&buffer, 0, sizeof(buffer));
  buffer.pBuffer = &output;
  DWORD status = 0;
  HRESULT out = f->dmo->ProcessOutput(0, 1, &buffer, &status);
  if (out != S_OK) { if (FAILED(out)) Log("ProcessOutput", out); break; }
  if (output.Length()) ConvertDmoOutput(f);
  if (!(buffer.dwStatus & DMO_INCOMPLETE)) break;
 }
 if (FAILED(hr)) {                    /* decoder refused the input: resynchronise */
  Log("ProcessInput", hr);
  f->dmo->Flush();
 }
}

/* ---- Microsoft Video 1 (CRAM), 16-bit ---- */
static void DecodeCram(CompatFrames *f, DWORD bytes) {
 const BYTE *p = f->sample, *end = f->sample + bytes;
 int bw = f->width / 4, bh = f->height / 4, w = f->width;
 int skip = 0;
 for (int by = 0; by < bh; by++) {                    /* block rows, bottom first */
  for (int bx = 0; bx < bw; bx++) {
   if (skip) { skip--; continue; }
   if (p + 2 > end) goto done;
   int a = p[0], b = p[1];
   p += 2;
   WORD *block = f->cram + by * 4 * w + bx * 4;       /* lower left pixel */
   if (a == 0 && b == 0 && by == bh - 1 && bx == bw - 1) goto done;
   if ((b & 0xfc) == 0x84) { skip = ((b - 0x84) << 8) + a - 1; continue; }
   if (b >= 0x80) {                                  /* one colour */
    WORD c = (WORD)((b << 8) | a);
    for (int y = 0; y < 4; y++) for (int x = 0; x < 4; x++) block[y * w + x] = c;
    continue;
   }
   unsigned flags = (b << 8) | a;
   if (p + 4 > end) goto done;
   WORD colors[8];
   colors[0] = (WORD)(p[0] | (p[1] << 8));
   colors[1] = (WORD)(p[2] | (p[3] << 8));
   p += 4;
   if (colors[0] & 0x8000) {                          /* two colours per 2x2 quadrant */
    if (p + 12 > end) goto done;
    for (int i = 2; i < 8; i++, p += 2) colors[i] = (WORD)(p[0] | (p[1] << 8));
    for (int y = 0; y < 4; y++)
     for (int x = 0; x < 4; x++, flags >>= 1)
      block[y * w + x] = colors[((y & 2) << 1) + (x & 2) + ((flags & 1) ^ 1)];
   } else {                                           /* two colours per block */
    for (int y = 0; y < 4; y++)
     for (int x = 0; x < 4; x++, flags >>= 1)
      block[y * w + x] = colors[(flags & 1) ^ 1];
   }
  }
 }
done:
 for (int row = 0; row < f->height; row++) {
  const WORD *s = f->cram + row * w;
  BYTE *d = f->pixels + row * f->stride;
  for (int x = 0; x < w; x++, d += 3) {
   WORD c = s[x];
   int r = (c >> 10) & 31, g = (c >> 5) & 31, bl = c & 31;
   d[0] = (BYTE)((bl << 3) | (bl >> 2));
   d[1] = (BYTE)((g << 3) | (g >> 2));
   d[2] = (BYTE)((r << 3) | (r >> 2));
  }
 }
}

static void DecodeSample(CompatFrames *f, long position) {
 LONG bytes = 0, samples = 0;
 if (AVIStreamRead(f->stream, position, 1, 0, 0, &bytes, &samples) != 0) return;
 if (bytes <= 0) return;                              /* dropped frame: keep the previous image */
 if (bytes > f->sample_size) {
  CompatFree(f->sample);
  f->sample = (BYTE *)CompatAlloc(bytes);
  f->sample_size = f->sample ? bytes : 0;
  if (!f->sample) return;
 }
 if (AVIStreamRead(f->stream, position, 1, f->sample, bytes, &bytes, &samples) != 0) return;
 if (f->kind == KIND_DMO) DecodeDmo(f, position, (DWORD)bytes);
 else if (f->kind == KIND_CRAM) DecodeCram(f, (DWORD)bytes);
}

static void CloseOurs(CompatFrames *f) {
 CompatFrames **link = &open_frames;
 while (*link && *link != f) link = &(*link)->next_open;
 if (*link) *link = f->next_open;
 if (f->dmo) { f->dmo->FreeStreamingResources(); f->dmo->Release(); }
 CompatFree(f->out);
 CompatFree(f->cram);
 CompatFree(f->sample);
 CompatFree(f->dib);
 if (f->stream) AVIStreamRelease(f->stream);
 CompatFree(f);
}

extern "C" void *__stdcall CompatAVIStreamGetFrameOpen(void *stream, void *wanted_format) {
 /* video=internal skips the installed VfW codecs (testing/diagnosis). */
 if (!CompatSettingIs("video", "auto", "internal")) {
  PGETFRAME system = AVIStreamGetFrameOpen((PAVISTREAM)stream, (LPBITMAPINFOHEADER)wanted_format);
  if (system) return system;
 }
 if (!stream) return 0;

 LONG format_size = 0;
 AVIStreamReadFormat((PAVISTREAM)stream, AVIStreamStart((PAVISTREAM)stream), 0, &format_size);
 if (format_size < (LONG)sizeof(BITMAPINFOHEADER)) format_size = sizeof(BITMAPINFOHEADER);
 BITMAPINFOHEADER *in = (BITMAPINFOHEADER *)CompatAlloc((1) * (format_size));
 CompatFrames *f = (CompatFrames *)CompatAlloc((1) * (sizeof(CompatFrames)));
 if (!in || !f) { CompatFree(in); CompatFree(f); return 0; }
 AVIStreamReadFormat((PAVISTREAM)stream, AVIStreamStart((PAVISTREAM)stream), in, &format_size);

 f->stream = (PAVISTREAM)stream;
 AVIStreamAddRef(f->stream);
 f->width = in->biWidth;
 f->height = in->biHeight < 0 ? -in->biHeight : in->biHeight;
 f->first = AVIStreamStart(f->stream);
 f->end = AVIStreamEnd(f->stream);
 f->last = -1;
 f->stride = (f->width * 3 + 3) & ~3;
 f->dib = (BITMAPINFOHEADER *)CompatAlloc((1) * (sizeof(BITMAPINFOHEADER)) + f->stride * f->height);
 if (!f->dib) { CompatFree(in); CloseOurs(f); return 0; }
 f->dib->biSize = sizeof(BITMAPINFOHEADER);
 f->dib->biWidth = f->width;
 f->dib->biHeight = f->height;
 f->dib->biPlanes = 1;
 f->dib->biBitCount = 24;
 f->dib->biCompression = BI_RGB;
 f->dib->biSizeImage = f->stride * f->height;
 f->pixels = (BYTE *)(f->dib + 1);

 DWORD fourcc = UpperFourcc(in->biCompression);
 f->kind = KIND_BLACK;
 if (fourcc == COMPAT_FOURCC('M','P','4','2') || fourcc == COMPAT_FOURCC('M','P','4','3') ||
     fourcc == COMPAT_FOURCC('M','P','G','4')) {
  const GUID &first = fourcc == COMPAT_FOURCC('M','P','4','3') ? CLSID_CompatMpeg43Decoder : CLSID_CompatMpeg4Decoder;
  const GUID &second = fourcc == COMPAT_FOURCC('M','P','4','3') ? CLSID_CompatMpeg4Decoder : CLSID_CompatMpeg43Decoder;
  for (int attempt = 0; attempt < 2 && f->kind == KIND_BLACK; attempt++) {
   if (OpenDmo(f, in, attempt ? second : first)) f->kind = KIND_DMO;
   else {
    if (f->dmo) { f->dmo->Release(); f->dmo = 0; }
    CompatFree(f->out); f->out = 0;
   }
  }
 } else if ((fourcc == COMPAT_FOURCC('C','R','A','M') || fourcc == COMPAT_FOURCC('M','S','V','C') ||
             fourcc == COMPAT_FOURCC('W','H','A','M')) && in->biBitCount == 16) {
  f->cram = (WORD *)CompatAlloc((f->width * f->height + 16) * (sizeof(WORD)));
  if (f->cram) f->kind = KIND_CRAM;
 }
 CompatFree(in);
 f->next_open = open_frames;
 open_frames = f;
 return f;
}

extern "C" void *__stdcall CompatAVIStreamGetFrame(void *frames, long position) {
 if (!IsOurs(frames)) return AVIStreamGetFrame((PGETFRAME)frames, position);
 CompatFrames *f = (CompatFrames *)frames;
 if (position < f->first) position = f->first;
 if (position >= f->end) position = f->end - 1;
 if (f->kind != KIND_BLACK && position != f->last) {
  long from = f->last + 1;
  if (f->last < 0 || position < from) {
   from = AVIStreamFindSample(f->stream, position, FIND_PREV | FIND_KEY);
   if (from < f->first) from = f->first;
   if (f->dmo) f->dmo->Flush();
  } else {
   long key = AVIStreamFindSample(f->stream, position, FIND_PREV | FIND_KEY);
   if (key > from) { from = key; if (f->dmo) f->dmo->Flush(); }   /* skip ahead to the nearer key frame */
  }
  for (long i = from; i <= position; i++) DecodeSample(f, i);
  f->last = position;
 }
 return f->dib;
}

extern "C" long __stdcall CompatAVIStreamGetFrameClose(void *frames) {
 if (!IsOurs(frames)) return AVIStreamGetFrameClose((PGETFRAME)frames);
 CloseOurs((CompatFrames *)frames);
 return 0;
}
