/* Layouts reconstructed from x86 memory accesses, with names corroborated by
 * the SeeR registration table and src/script_headers/scener. Reserved fields
 * describe unknown portions of the native layout, not recovered member names.
 * Functions retain address names so compiled sections can be audited separately.
 */
typedef unsigned int u32;
struct EzdEntry {
    char *name;
    char *secondaryName;
    u32 compression, offset, decodedSize, packedSize;
    void *cached;
};
class EzdArchive {
    unsigned char reserved[0x18];
    EzdEntry *entries;
public:
    u32 FUN_00401b80(int index) const;
};
u32 EzdArchive::FUN_00401b80(int index) const { return entries[index].decodedSize; }

class SkeletonAnimation {
    unsigned char reserved[0x0c];
    float lastTime;
    int frames;
    float frameTime;
public:
    float FUN_00427750() const;
    float FUN_00427760() const;
    float FUN_00427770() const;
    float FUN_00427780() const;
    unsigned char FUN_00427790() const;
};
float SkeletonAnimation::FUN_00427750() const { return (float)frames; }
float SkeletonAnimation::FUN_00427760() const { return lastTime / frameTime; }
float SkeletonAnimation::FUN_00427770() const { return lastTime; }
float SkeletonAnimation::FUN_00427780() const { return frameTime; }
/* Native return is AL; the SeeR-facing declaration calls it bool. */
unsigned char SkeletonAnimation::FUN_00427790() const { return lastTime / frameTime < frames / 2; }

class Sound {
    unsigned char reserved[8];
    bool loop;
    unsigned char alignment[3];
    int rate, volume, panning;
public:
    void FUN_00428060(int frequency);
    void FUN_00428070(int pan);
    void FUN_00428080(int frequency, int vol, int pan, bool looping);
    void FUN_0044e460(int frequency, int vol, int pan);
};
void Sound::FUN_00428060(int frequency) { rate = frequency; }
void Sound::FUN_00428070(int pan) { panning = pan; }
void Sound::FUN_00428080(int frequency, int vol, int pan, bool looping) {
    rate = frequency; volume = vol; panning = pan; loop = looping;
}
void Sound::FUN_0044e460(int, int, int) { }

class GameObject {
    unsigned char reserved[0x88];
    bool selectable;
public:
    void FUN_0040f350(bool enabled);
};
void GameObject::FUN_0040f350(bool enabled) { selectable = enabled; }

class Camera {
    unsigned char reserved[0xebffc];
    float forwardDistance;
public:
    void FUN_0041fc20(float distance);
};
void Camera::FUN_0041fc20(float distance) { forwardDistance += distance; }

typedef char assert_x86_ezd_entry_stride[(sizeof(EzdEntry) == 0x1c) ? 1 : -1];
