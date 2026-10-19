/* Native nonvirtual {grow_by,count,data} cache owners, three DWORDs each.
 * Walk callbacks406970/4069b0 and mesh callbacks413fa0/413fd0 own lifecycle;
 * this source provides their shared storage without adding startup actions. */
struct RecordCacheStorage { int grow_by, count; void *data; };
typedef char CacheIs12Bytes[(sizeof(RecordCacheStorage) == 12) ? 1 : -1];
extern "C" {
static RecordCacheStorage walk_area_cache;
static RecordCacheStorage mesh_record_cache;
unsigned char walk_cache_cleanup_guard;  /* 0x50247c */
unsigned char mesh_cache_cleanup_guard;  /* 0x50249c */

extern const unsigned source_size_cache = sizeof(RecordCacheStorage);
extern const unsigned source_size_cache_count = sizeof(walk_area_cache.count);
extern const unsigned source_size_cache_data = sizeof(walk_area_cache.data);
extern const void *const source_location_walk_count = &walk_area_cache.count;
extern const void *const source_location_walk_data = &walk_area_cache.data;
extern const void *const source_location_mesh_count = &mesh_record_cache.count;
extern const void *const source_location_mesh_data = &mesh_record_cache.data;
}
