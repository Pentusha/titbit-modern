// Test for compat_video.cpp: decodes frames of the game's AVI files through
// CompatAVIStreamGetFrame* and writes them as BMP files.
//   avi_decode_test <out_dir> <file.avi>...      (set TITBIT_COMPAT_VIDEO=internal
//   to bypass installed VfW codecs)
#include <windows.h>
#include <vfw.h>
#include <stdio.h>
#include "../compat.h"

static void WriteBmp(const char *path, BITMAPINFOHEADER *dib) {
 int stride = (dib->biWidth * 3 + 3) & ~3;
 BITMAPFILEHEADER fh = {0};
 fh.bfType = 0x4d42;
 fh.bfOffBits = sizeof(fh) + sizeof(BITMAPINFOHEADER);
 fh.bfSize = fh.bfOffBits + stride * dib->biHeight;
 FILE *f = fopen(path, "wb");
 if (!f) return;
 fwrite(&fh, sizeof(fh), 1, f);
 fwrite(dib, sizeof(BITMAPINFOHEADER), 1, f);
 fwrite(dib + 1, stride * dib->biHeight, 1, f);
 fclose(f);
}

int main(int argc, char **argv) {
 CoInitialize(0);
 AVIFileInit();
 for (int a = 2; a < argc; a++) {
  PAVISTREAM stream = 0;
  if (AVIStreamOpenFromFileA(&stream, argv[a], streamtypeVIDEO, 0, OF_READ, 0) != 0) { printf("%s: open failed\n", argv[a]); continue; }
  void *frames = AVIStreamGetFrameOpen(stream, 0);
  long length = AVIStreamLength(stream);
  printf("%s: frames=%p length=%ld\n", argv[a], frames, length);
  if (!frames) { AVIStreamRelease(stream); continue; }
  long picks[3] = { 0, length / 2, length - 1 };
  for (int i = 0; i < 3; i++) {
   BITMAPINFOHEADER *dib = (BITMAPINFOHEADER *)AVIStreamGetFrame(frames, picks[i]);
   if (!dib) { printf("  frame %ld: null\n", picks[i]); continue; }
   const char *name = strrchr(argv[a], '\\'); name = name ? name + 1 : argv[a];
   char out[MAX_PATH];
   sprintf(out, "%s\\%s_%ld.bmp", argv[1], name, picks[i]);
   printf("  frame %ld: %ldx%ld bpp=%d -> %s\n", picks[i], dib->biWidth, dib->biHeight, dib->biBitCount, out);
   if (dib->biBitCount == 24) WriteBmp(out, dib);
  }
  AVIStreamGetFrameClose(frames);
  AVIStreamRelease(stream);
 }
 AVIFileExit();
 CoUninitialize();
 return 0;
}
