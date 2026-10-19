// Skin/font record loader (0x40e3e0): texture "<file>.tga" plus 256 character
// widths from "<file>.dat" (20 for every character when the file is missing).
#include "e40_string.h"

ObservedString __stdcall operator+(const ObservedString &text, const char *tail);   // 0x40c540

class VfsFile {  // 0x4203d0 (was VfsFile_004203d0)
public:
 ObservedString name;
 unsigned position;
 void *handle;
 unsigned size;
 int source, kind;
 VfsFile();
 ~VfsFile();
 int Locate_00420420(const char *path);
 void Open_004206b0(const char *path);
 unsigned Read_004209b0(void *buffer, unsigned count);
 void Close_00420a60();
};
struct TextureCache { int Load_0042c2b0(ObservedString name, int flags); }; // original global 0x5ee6f0  // 0x42c080 (was TextureCache_0042c080)
extern TextureCache g_005ee6f0;

class SoundHolder_0040e380 {
public:
 char name[0x20];
 int used, refs, age;                 // +0x24
 int widths[256];                     // +0x30
 int texture;                         // +0x430
 virtual ~SoundHolder_0040e380();
 bool Load_0040e3e0(ObservedString file);
};

bool SoundHolder_0040e380::Load_0040e3e0(ObservedString file) {
 ObservedString path = file + ".tga";
 texture = g_005ee6f0.Load_0042c2b0(path, 0);
 path = file + ".dat";
 VfsFile in;
 unsigned char buffer[256];
 if (in.Locate_00420420(path)) {
  in.Open_004206b0(path);
  in.Read_004209b0(buffer, 256);
  in.Close_00420a60();
  for (int i = 0; i < 256; i++) widths[i] = buffer[i];
 } else {
  for (int i = 0; i < 256; i++) widths[i] = 20;
 }
 return true;
}
