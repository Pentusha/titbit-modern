#define OBSERVED_STRING_WTL_BUFFER  // GetBuffer/ReleaseBuffer expanded inline (WTL)
#define OBSERVED_STRING_INLINE_COPY  // CString copy ctor expanded inline (WTL)
#define OBSERVED_STRING_WTL_LOADSTRING  // WTL CString layout (see engine_string.h)
#define OBSERVED_STRING_RELEASE_EXTERNAL  // CString::Release out of line (0x402180)
// Texture cache (global at 0x5ee6f0): TGA / JPG / JPA (JPEG + alpha plane) loaders,
// OpenGL upload and name lookup (0x42ac70-0x42c730).
#include "../../native/engine_string.h"
#include <stdlib.h>
extern "C" {
 __declspec(dllimport) char *__stdcall CharNextA(const char *);
 __declspec(dllimport) int __stdcall lstrcmpA(const char *, const char *);
 int __cdecl memcmp(const void *, const void *, unsigned);
 __declspec(dllimport) void __stdcall glPixelStorei(unsigned, int);
 __declspec(dllimport) void __stdcall glBindTexture(unsigned, unsigned);
 __declspec(dllimport) void __stdcall glTexParameterf(unsigned, unsigned, float);
 __declspec(dllimport) void __stdcall glTexParameteri(unsigned, unsigned, int);
 __declspec(dllimport) void __stdcall glTexEnvi(unsigned, unsigned, int);
 __declspec(dllimport) void __stdcall glTexImage2D(unsigned, int, int, int, int, int, unsigned, unsigned, const void *);
 __declspec(dllimport) int __stdcall gluBuild2DMipmaps(unsigned, int, int, int, unsigned, unsigned, const void *);
}
#pragma intrinsic(memcmp)
extern "C" char *__cdecl strcpy(char *, const char *);
extern "C" void *__cdecl memset(void *, int, unsigned);
#pragma intrinsic(strcpy, memset)
void __cdecl Log_00403650(const char *text);
void __cdecl FormatArchiveString(ObservedString *, const char *, ...);
const char *__cdecl ReverseFind_00407b50(const char *text, char ch);
int __cdecl DecodeJpeg_0049bbb0(const void *data, int size, void *pixels, int *width, int *height,
                                int *components, int a, int b);

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
 unsigned Read_004209b0(char *buffer, unsigned count);
 void Close_00420a60();
 unsigned GetSize_00413ca0();
};

struct EditString42 : ObservedString {
 ObservedString Mid_00407b80(int first) const;
 ObservedString Left_00407bb0(int first, int count) const;
 int Replace_0040eff0(const char *from, const char *to);
 static char *__cdecl _cstrstr(const char *text, const char *find);  // 0x401fb0
 int Find(const char *sub) const {                                    // WTL CString::Find
  const char *text = *this;
  char *hit = _cstrstr(text, sub);
  return hit == 0 ? -1 : (int)(hit - text);
 }
};

// One cache slot (0x40 bytes).
struct TextureRecord42 {
 char name[0x20];
 int index;                         // 0x20
 int refs;                          // 0x24
 int reserved28;
 char extension[4];                 // 0x2c
 int bpp;                           // 0x30
 int width, height;                 // 0x34
 unsigned texture;                  // 0x3c GL texture name
 void Clear_0042ac20();
 void Reset_0042ac40();
 bool LoadTga_0042ac70(ObservedString path, unsigned char **pixels);
 bool LoadJpg_0042b030(ObservedString path, unsigned char **pixels);
 bool LoadJpa_0042b4c0(ObservedString path, unsigned char **pixels);
 bool LoadFile_0042b840(ObservedString path, int flags);
 void Upload_0042bef0(unsigned char flags, const void *pixels);
};

class TextureCache {  // 0x42c080 (was TextureCache_0042c080)
public:
 int count;
 TextureRecord42 *records;
 TextureCache();
 virtual ~TextureCache();
 int Find_0042c0c0(ObservedString name);
 int Load_0042c2b0(ObservedString path, int flags);
 int FindFree_0042c5a0();
 bool Grow_0042c5f0(int newCount);
 TextureRecord42 *Get_0042c730(unsigned id);
 int Create_0042c760(ObservedString path, int flags, int width, int height, int bpp);
};

// Copies the base file name (no folder, no extension) into the record name.
static inline void CopyBaseName(char *out, ObservedString &path) {
 const char *slash = 0, *p;
 for (p = path; *p; p = CharNextA(p)) if (*p == '\\') slash = p;
 int first = (slash && slash - (const char *)path != -1) ? slash - (const char *)path + 1 : 0;
 const char *dot = 0;
 for (p = path; *p; p = CharNextA(p)) if (*p == '.') dot = p;
 int last = dot ? dot - (const char *)path : -1;
 char *text = path.GetBuffer(0x20);
 memcpy(out, text + first, last - first);
}

// NONMATCHING: faithful translation
bool TextureRecord42::LoadTga_0042ac70(ObservedString path, unsigned char **pixels) {
 CopyBaseName(name, path);
 unsigned char tgaHeader[12] = { 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
 unsigned char tgaCompare[12];
 unsigned char header[6];
 VfsFile file;
 if (!file.Locate_00420420(path)) {
  Log_00403650("tga file not open");
  file.Close_00420a60();
  return false;
 }
 file.Open_004206b0(path);
 if (file.Read_004209b0((char *)tgaCompare, 12) != (unsigned)-1 && memcmp(tgaHeader, tgaCompare, 12) == 0 &&
     file.Read_004209b0((char *)header, 6) != (unsigned)-1) {
  width = header[1] * 256 + header[0];
  height = header[3] * 256 + header[2];
  if (width != 0 && height != 0 && (header[4] == 24 || header[4] == 32)) {
   bpp = header[4];
   unsigned bytesPerPixel = header[4] >> 3;
   unsigned imageSize = height * width * bytesPerPixel;
   *pixels = (unsigned char *)malloc(imageSize);
   if (*pixels) {
    if (file.Read_004209b0((char *)*pixels, imageSize) != (unsigned)-1) {
     for (unsigned i = 0; i < imageSize; i += bytesPerPixel) {
      unsigned char t = (*pixels)[i];
      (*pixels)[i] = (*pixels)[i + 2];
      (*pixels)[i + 2] = t;
     }
     file.Close_00420a60();
     return true;
    }
    if (*pixels) free(*pixels);
   }
  }
  file.Close_00420a60();
 }
 return false;
}

// NONMATCHING: faithful translation
bool TextureRecord42::LoadJpg_0042b030(ObservedString path, unsigned char **pixels) {
 CopyBaseName(name, path);
 VfsFile file;
 if (!file.Locate_00420420(path)) {
  Log_00403650("jpeg file not open");
  file.Close_00420a60();
  return false;
 }
 file.Open_004206b0(path);
 unsigned size = file.GetSize_00413ca0();
 char *data = (char *)malloc(size);
 unsigned read = file.Read_004209b0(data, size);
 file.Close_00420a60();
 if (read != size) {
  Log_00403650("wrong size !!!");
  return false;
 }
 int w, h, components;
 DecodeJpeg_0049bbb0(data, size, 0, &w, &h, &components, 0, 1);
 unsigned char *rgba = (unsigned char *)malloc(w * h * 4);
 unsigned char *rgb = (unsigned char *)malloc(w * h * 3);
 DecodeJpeg_0049bbb0(data, size, rgba, &w, &h, &components, 0, 1);
 free(data);
 width = w;
 unsigned total = w * h * 4;
 height = h;
 bpp = 32;
 unsigned i;
 for (i = 0; i < total; i += 4) {
  unsigned char t = rgba[i];
  rgba[i] = rgba[i + 2];
  rgba[i + 2] = t;
 }
 unsigned char *flipped = (unsigned char *)malloc(total);
 for (i = 0; i < total; i += 4) {
  const unsigned char *src = rgba + total - 4 - i;
  for (unsigned k = 0; k < 4; k++) flipped[i + k] = src[k];
 }
 for (unsigned y = 0; y < (unsigned)height; y++) {
  for (unsigned x = 0; x < (unsigned)width; x++) {
   rgba[(width * y + x) * 4] = flipped[((y + 1) * width - x) * 4 - 4];
   rgba[(width * y + x) * 4 + 1] = flipped[((y + 1) * width - x) * 4 - 3];
   rgba[(width * y + x) * 4 + 2] = flipped[((y + 1) * width - x) * 4 - 2];
   rgba[(width * y + x) * 4 + 3] = flipped[((y + 1) * width - x) * 4 - 1];
  }
 }
 unsigned char *out = rgb;
 for (i = 0; i < total; i += 4) {
  out[0] = rgba[i];
  out[1] = rgba[i + 1];
  out[2] = rgba[i + 2];
  out += 3;
 }
 bpp = 24;
 *pixels = rgb;
 free(flipped);
 free(rgba);
 return true;
}

// JPA: [int jpegSize][JPEG data][alpha plane, one byte per pixel].
// NONMATCHING: faithful translation (the RGBA buffer is freed right after being returned, as in the game)
bool TextureRecord42::LoadJpa_0042b4c0(ObservedString path, unsigned char **pixels) {
 CopyBaseName(name, path);
 VfsFile file;
 if (!file.Locate_00420420(path)) {
  Log_00403650("jpeg-alpha file not open");
  file.Close_00420a60();
  return false;
 }
 file.Open_004206b0(path);
 unsigned size = file.GetSize_00413ca0();
 int *data = (int *)malloc(size);
 unsigned read = file.Read_004209b0((char *)data, size);
 int jpegSize = data[0];
 file.Close_00420a60();
 if (read != size) {
  Log_00403650("wrong size !!!");
  return false;
 }
 int w, h, components;
 DecodeJpeg_0049bbb0(data + 1, size, 0, &w, &h, &components, 0, 1);
 unsigned planeSize = w * h;
 unsigned char *alpha = (unsigned char *)malloc(planeSize);
 memcpy(alpha, (char *)(data + 1) + jpegSize, planeSize);
 unsigned char *rgbx = (unsigned char *)malloc(w * h * 4);
 DecodeJpeg_0049bbb0(data + 1, size, rgbx, &w, &h, &components, 0, 1);
 width = w;
 int total = w * h * 4;
 height = h;
 bpp = 32;
 unsigned char *rgba = (unsigned char *)malloc(total);
 const unsigned char *src = rgbx, *a = alpha;
 for (unsigned char *dst = rgba; dst - rgba < total; dst += 4, src += 4) {
  dst[0] = src[0];
  dst[1] = src[1];
  dst[2] = src[2];
  dst[3] = *a++;
 }
 *pixels = rgba;
 free(data);
 free(rgba);
 free(rgbx);
 free(alpha);
 return true;
}

// Tries name.tga, then name.jpa, then name.jpg.
// NONMATCHING: faithful translation
bool TextureRecord42::LoadFile_0042b840(ObservedString path, int flags) {
 unsigned char *pixels = 0;
 path.MakeLower();
 VfsFile file;
 EditString42 &name42 = (EditString42 &)path;
 // Missing file: try .jpa, then .jpg (the last lookup is not checked).
 if (!file.Locate_00420420(path)) {
  name42.Replace_0040eff0("tga", "jpa");
  Log_00403650("Cant find tga try jpa");
  if (!file.Locate_00420420(path)) {
   Log_00403650("Cant find jpa try jpg");
   name42.Replace_0040eff0("jpa", "jpg");
  }
 }
 if (name42.Find("tga") != -1) {
  Log_00403650("Start loading TGA");
  if (!LoadTga_0042ac70(path, &pixels)) return false;
  Upload_0042bef0((unsigned char)flags, pixels);
  if (pixels) free(pixels);
  strcpy(extension, "tga");
  name42.Replace_0040eff0(".tga", "");
  strcpy(name, path);
 } else if (name42.Find("jpg") != -1) {
  Log_00403650("Start loading JPG");
  if (!LoadJpg_0042b030(path, &pixels)) {
   Log_00403650("Load failed");
   return false;
  }
  Upload_0042bef0((unsigned char)flags, pixels);
  if (pixels) free(pixels);
  strcpy(extension, "jpg");
  name42.Replace_0040eff0(".jpg", "");
  strcpy(name, path);
 } else if (name42.Find("jpa") != -1) {
  if (!LoadJpa_0042b4c0(path, &pixels)) return false;
  Upload_0042bef0((unsigned char)flags, pixels);
  if (pixels) free(pixels);
  strcpy(extension, "jpa");
  name42.Replace_0040eff0(".jpa", "");
  strcpy(name, path);
  return true;                                       // (no log for .jpa)
 } else {
  return false;
 }
 Log_00403650("Loaded and registered");
 return true;
}

// flags: 1 = mipmaps, 2 = repeat (else clamp), 4 = 16-bit internal format.
// NONMATCHING: faithful translation
void TextureRecord42::Upload_0042bef0(unsigned char flags, const void *pixels) {
 unsigned format = (bpp != 24) + 0x1907;                       // GL_RGB / GL_RGBA
 int internal = (flags & 4) ? ((bpp != 24) ? 0x8057 : 0x8050)  // GL_RGB5_A1 / GL_RGB5
                            : ((bpp != 24) ? 0x8058 : 0x8051); // GL_RGBA8 / GL_RGB8
 glPixelStorei(0xcf5, 1);
 glBindTexture(0xde1, texture);
 if (flags & 2) {
  if (flags & 1) {
   glTexParameterf(0xde1, 0x2801, 9985.0f);                    // GL_LINEAR_MIPMAP_NEAREST
   glTexParameterf(0xde1, 0x2800, 9985.0f);                    // (original also sets it as MAG filter)
  } else {
   glTexParameterf(0xde1, 0x2801, 9729.0f);                    // GL_LINEAR
   glTexParameterf(0xde1, 0x2800, 9729.0f);
  }
  glTexParameteri(0xde1, 0x2802, 0x2901);                      // GL_REPEAT
  glTexParameteri(0xde1, 0x2803, 0x2901);
 } else {
  glTexParameterf(0xde1, 0x2801, 9729.0f);
  glTexParameterf(0xde1, 0x2800, 9729.0f);
  glTexParameteri(0xde1, 0x2802, 0x2900);                      // GL_CLAMP
  glTexParameteri(0xde1, 0x2803, 0x2900);
 }
 glTexEnvi(0x2300, 0x2200, 0x2100);                             // GL_MODULATE
 if (flags & 1) gluBuild2DMipmaps(0xde1, internal, width, height, format, 0x1401, pixels);
 else glTexImage2D(0xde1, 0, internal, width, height, 0, format, 0x1401, pixels);
}

// NONMATCHING: faithful translation
int TextureCache::Find_0042c0c0(ObservedString name) {
 name.MakeLower();
 for (int i = count - 1; i >= 0; i--)
  if (lstrcmpA(name, records[i].name) == 0) return records[i].texture;
 return -1;
}

// NONMATCHING: faithful translation
int TextureCache::Load_0042c2b0(ObservedString path, int flags) {
 ObservedString name;
 const char *slash = 0, *p;
 for (p = path; *p; p = CharNextA(p)) if (*p == '\\') slash = p;
 name = ((EditString42 &)path).Mid_00407b80((slash ? slash - (const char *)path : -1) + 1);
 const char *dot = ReverseFind_00407b50(name, '.');
 if (dot && dot - (const char *)name != -1)
  name = ((EditString42 &)name).Left_00407bb0(0, dot - (const char *)name);
 int id = Find_0042c0c0(name);
 if (id == -1) {
  id = FindFree_0042c5a0();
  if (id == -1) {
   int n = count;
   id = n ? n : 1;
   Grow_0042c5f0(n + 10);
  }
  records[id].texture = id;
  if (!records[id].LoadFile_0042b840(path, flags)) {
   id = 0;
   records[0].texture = 0;
  }
  records[id].refs = 1;
 } else {
  records[id].refs++;
 }
 return id;
}

// NONMATCHING: faithful translation
int TextureCache::FindFree_0042c5a0() {
 int i = 1;
 if (count <= 1) return -1;
 while (records[i].texture != 0) {
  if (records[i].refs == 0) {
   records[i].Clear_0042ac20();
   return i;
  }
  if (++i >= count) return -1;
 }
 return i;
}

// NONMATCHING: faithful translation
bool TextureCache::Grow_0042c5f0(int newCount) {
 if (count >= newCount) return false;
 TextureRecord42 *data = (TextureRecord42 *)::operator new(newCount * sizeof(TextureRecord42));
 memset(&data[count], 0, (newCount - count) * sizeof(TextureRecord42));
 memcpy(data, records, count * sizeof(TextureRecord42));
 if (records) ::operator delete(records);
 records = data;
 for (int i = count; i < newCount; i++) records[i].Reset_0042ac40();
 count = newCount;
 ObservedString message;
 FormatArchiveString(&message, "Texture hostsize is %d", newCount);
 Log_00403650(message);
 return true;
}

TextureRecord42 *TextureCache::Get_0042c730(unsigned id) {
 if (id < (unsigned)count) return &records[id];
 return 0;
}

// Registers an empty (uninitialized) texture of the given size under a file name.
// NONMATCHING: faithful translation
int TextureCache::Create_0042c760(ObservedString path, int flags, int width, int height, int bpp) {
 ObservedString name;
 const char *slash = 0, *p;
 for (p = path; *p; p = CharNextA(p)) if (*p == '\\') slash = p;
 name = ((EditString42 &)path).Mid_00407b80((slash ? slash - (const char *)path : -1) + 1);
 const char *dot = 0;
 for (p = name; *p; p = CharNextA(p)) if (*p == '.') dot = p;
 if (dot && dot - (const char *)name != -1)
  name = ((EditString42 &)name).Left_00407bb0(0, dot - (const char *)name);
 int id = Find_0042c0c0(name);
 if (id == -1) {
  id = FindFree_0042c5a0();
  if (id == -1) {
   int n = count;
   id = n ? n : 1;
   Grow_0042c5f0(n + 10);
  }
  TextureRecord42 &record = records[id];
  record.texture = id;
  record.bpp = bpp;
  record.width = width;
  unsigned size = width * height;
  record.height = height;
  if (bpp == 16) size *= 2;
  else if (bpp == 24) size *= 3;
  else if (bpp == 32) size *= 4;
  void *pixels = malloc(size);
  records[id].Upload_0042bef0((unsigned char)flags, pixels);
  free(pixels);
  records[id].refs = 1;
 } else {
  records[id].refs++;
 }
 return id;
}
