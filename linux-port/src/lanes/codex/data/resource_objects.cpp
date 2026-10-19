/* Source-owned global storage; the recovered native constructors/destructors
 * retain lifecycle ownership. No additional C++ startup actions are emitted.
 * Layouts: scene_cleanup_resources.h, caches.cpp, texture.cpp and sound.cpp.
 * Resource cache5ee6dc ends at texture cache5ee6f0; texture ends at audio5ee6fc.
 */
struct ResourceCacheStorage {
 void *vtable;
 void *data;
 int count, used, step;
};
struct TextureCacheStorage { void *vtable; int count; void *records; };
struct SoundBankStorage { void *vtable; int grow_by, count; void *slots; };
typedef char ResourceCacheIs20[(sizeof(ResourceCacheStorage) == 20) ? 1 : -1];
typedef char TextureCacheIs12[(sizeof(TextureCacheStorage) == 12) ? 1 : -1];
typedef char SoundBankIs16[(sizeof(SoundBankStorage) == 16) ? 1 : -1];
extern "C" {
ResourceCacheStorage resource_cache_storage;
TextureCacheStorage texture_cache_storage;
SoundBankStorage sound_bank_storage;
}
