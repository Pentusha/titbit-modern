#define OBSERVED_STRING_INLINE_COPY  // CString copy ctor expanded inline (WTL)
#define OBSERVED_STRING_WTL_LOADSTRING  // WTL CString layout (see engine_string.h)
#define OBSERVED_STRING_RELEASE_EXTERNAL  // CString::Release out of line (0x402180)
#define OBSERVED_STRING_LOADSTRING_INLINE  // this unit expands LoadString inline (original)
// Streamed sounds (music): in-memory audiere file, stream open/play, script API and
// the active sound list with end-of-stream sync messages (0x429a30-0x42a3a0).
#include "../../native/game_audio.h"
#include "../../native/engine_string.h"
#include "../../native/picking_control_interfaces.h"
#include "lane_xml.h"
void __cdecl Log_00403650(const char *text);

struct DomNode42 {
 virtual void s0();
 virtual void Release(int);
 virtual void s2(); virtual void s3(); virtual void s4();
 virtual const std::string *Name();
 virtual const std::string *Value();                 // +0x18
 virtual void s7(); virtual void s8(); virtual void s9(); virtual void s10();
 virtual DomNode42 *FirstChild();
 virtual void s12(); virtual void s13();
 virtual DomNode42 *NextSibling();
 virtual struct DomAttributes42 *Attributes();       // +0x3c
 int refs;
};
struct DomAttributes42 {
 virtual void s0();
 virtual void Release(int);
 virtual DomNode42 *Named(const std::string &name);  // +0x08
};

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

struct ScenePost42 { bool Send_00424780(ObservedPickingMessage *message); };
extern ScenePost42 g_005011c0;
extern audiere::AudioDevice *g_005ee6fc;

// A whole sound file kept in memory, exposed to audiere as a File.
class MemoryFile : public audiere::RefImplementation<audiere::File> {  // 0x429c70 (was MemoryFile_00429c70)
public:
 char *data;
 int position;
 int size;
 MemoryFile(char *buffer, int length) { data = buffer; position = 0; size = length; }
 ~MemoryFile();
 int ADR_CALL read(void *buffer, int count);
 bool ADR_CALL seek(int where, SeekMode mode);
 int ADR_CALL tell();
};

MemoryFile::~MemoryFile() {
 if (data) ::operator delete(data);
}

// NONMATCHING: faithful translation
int ADR_CALL MemoryFile::read(void *buffer, int count) {
 int remaining = size - position;
 if (count <= remaining) {
  memcpy(buffer, data + position, count);
  position += count;
  return count;
 }
 memcpy(buffer, data + position, remaining);
 position += remaining;
 return remaining;
}

// NONMATCHING: faithful translation
bool ADR_CALL MemoryFile::seek(int where, SeekMode mode) {
 int old = position;
 switch (mode) {
 case BEGIN: position = where; break;
 case CURRENT: position += where; break;
 case END: position = size - where; break;
 }
 if (position < 0 || position > size) {
  position = old;
  return false;
 }
 return true;
}

int ADR_CALL MemoryFile::tell() { return position; }

// One entry of the active stream list (0x5ee71c).
struct StreamEntry42 {
 SoundFocusPrefix *sound;
 int pauseDepth;
 void *sync;                        // object notified when the stream ends
 StreamEntry42() { sound = 0; pauseDepth = 0; sync = 0; }
};
struct StreamList42 {
 StreamEntry42 *data;
 int size, capacity, grow_by;
 void SetSize_0042a260(int newSize, int growBy);
};
extern StreamList42 g_005ee71c;

struct StreamSound42 : SoundFocusPrefix {
 bool Open_00429a30(ObservedString path);
 void Play_00429ca0(bool loop);
 void Load_00429d00(DomNode42 *node);
 void SetSync_0042a070(int unused, void *sync);
 void SetVolume_0042a140(int volume);
};

// NONMATCHING: faithful translation
bool StreamSound42::Open_00429a30(ObservedString path) {
 VfsFile file;
 file.Open_004206b0(path);
 unsigned length = file.GetSize_00413ca0();
 char *buffer = (char *)::operator new(length);
 file.Read_004209b0(buffer, length);
 file.Close_00420a60();
 MemoryFile *memory = new MemoryFile(buffer, length);
 Log_00403650("start with sound");
 stream = audiere::OpenSound(g_005ee6fc, audiere::hidden::AdrOpenSampleSourceFromFile(memory), true);
 Log_00403650("read done");
 int index = g_005ee71c.size;
 g_005ee71c.SetSize_0042a260(index + 1, -1);
 StreamEntry42 &entry = g_005ee71c.data[index];
 entry.sound = this;
 entry.pauseDepth = 0;
 entry.sync = 0;
 return true;
}

// NONMATCHING: faithful translation
void StreamSound42::Play_00429ca0(bool loop) {
 if (loop) {
  stream->setRepeat(true);
  stream->play();
  return;
 }
 stream->setRepeat(false);
 stream->play();
}

// <stream_sound stream_name="music.ogg"/>
// NONMATCHING: faithful translation
void StreamSound42::Load_00429d00(DomNode42 *node) {
 ObservedString name;
 DomAttributes42 *attributes = node->Attributes();
 DomNode42 *attribute = attributes->Named(std::string("stream_name"));
 if (attribute) name = attribute->Value()->c_str();
 Open_00429a30(name);
}

// Script CreateStream(name)
// NONMATCHING: faithful translation
SoundFocusPrefix *__cdecl CreateStream_00429ee0(const char *name) {
 SoundFocusPrefix *sound = new SoundFocusPrefix;
 ((StreamSound42 *)sound)->Open_00429a30(ObservedString(name));
 return sound;
}

// Script DeleteStream(stream)
void __cdecl DeleteStream_0042a060(SoundFocusPrefix *sound) {
 // CreateStream allocates one object. VC6 called its virtual deleting
 // destructor with flags=3; that does not establish an array allocation.
 // Clang's delete[] would instead read a nonexistent cookie at sound-4.
 delete sound;
}

// Script STREAM.SetSync(object)
void StreamSound42::SetSync_0042a070(int unused, void *sync) {
 int n = g_005ee71c.size;
 for (int i = 0; i < n; i++)
  if (g_005ee71c.data[i].sound == this) g_005ee71c.data[i].sync = sync;
}

// Posts message 12 for every stopped stream that has a sync object.
// NONMATCHING: faithful translation
void __cdecl UpdateSounds_0042a0b0() {
 int n = g_005ee71c.size;
 for (int i = 0; i < n; i++) {
  if (!g_005ee71c.data[i].sound->stream->isPlaying() && g_005ee71c.data[i].sync && g_005ee71c.data[i].pauseDepth == 0) {
   ObservedPickingMessage message;
   memset(&message, 0, sizeof(message));
   message.event = 0xc;
   message.selected = g_005ee71c.data[i].sync;
   ((int *)message.reserved28)[0] = 1;
   g_005011c0.Send_00424780(&message);
  }
 }
}

// Script STREAM.SetVolume(0..100)
void StreamSound42::SetVolume_0042a140(int volume) {
 if (volume < 0) volume = 0;
 else if (volume > 100) volume = 100;
 stream->setVolume((float)volume * 0.01f);
}


// NONMATCHING: faithful translation
static void __stdcall ConstructStreamEntries_0042a3a0(StreamEntry42 *elements, int count) {
 if (elements && count) {
  memset(elements, 0, count * sizeof(StreamEntry42));
  for (; count--; elements++) ::new((void *)elements) StreamEntry42;
 }
}

// NONMATCHING: faithful translation
void StreamList42::SetSize_0042a260(int newSize, int growBy) {
 if (newSize < 0) return;
 if (growBy != -1) grow_by = growBy;
 if (newSize == 0) {
  if (data) { ::operator delete(data); data = 0; }
  size = capacity = 0;
 } else if (data == 0) {
  data = (StreamEntry42 *)::operator new(newSize * sizeof(StreamEntry42));
  ConstructStreamEntries_0042a3a0(data, newSize);
  size = capacity = newSize;
 } else if (newSize <= capacity) {
  if (newSize > size) ConstructStreamEntries_0042a3a0(&data[size], newSize - size);
  size = newSize;
 } else {
  int grow = grow_by;
  if (grow == 0) {
   grow = size / 8;
   grow = (grow < 4) ? 4 : ((grow > 1024) ? 1024 : grow);
  }
  int newMax = capacity + grow;
  if (newMax <= newSize) newMax = newSize;
  StreamEntry42 *newData = (StreamEntry42 *)::operator new(newMax * sizeof(StreamEntry42));
  memcpy(newData, data, size * sizeof(StreamEntry42));
  ConstructStreamEntries_0042a3a0(&newData[size], newSize - size);
  ::operator delete(data);
  data = newData;
  size = newSize;
  capacity = newMax;
 }
}
