/* One native16-byte array owner at5ee71c. PauseGameAudio originally exposed
 * only its first two fields; StreamList42/SetSize42a260 requires all four
 * fields to be contiguous, irrespective of compiler global-variable order.
 * Initializer4298e0 and destructor429910 own lifecycle. */
#include "../../../native/game_audio.h"
#include <stddef.h>
struct ActiveStreamStorage {
 ActiveSoundEntry *data;
 int size, capacity, grow_by;
};
typedef char StreamListIs16[(sizeof(ActiveStreamStorage) == 16) ? 1 : -1];
typedef char StreamCountAt4[(offsetof(ActiveStreamStorage, size) == 4) ? 1 : -1];
extern "C" {
ActiveStreamStorage active_stream_list;
extern const unsigned source_size_stream_list = sizeof(active_stream_list);
extern const unsigned source_size_stream_pointer = sizeof(active_stream_list.data);
extern const unsigned source_size_stream_scalar = sizeof(active_stream_list.size);
extern const void *const source_location_stream_pointer = &active_stream_list.data;
extern const void *const source_location_stream_count = &active_stream_list.size;
extern const void *const source_location_stream_capacity = &active_stream_list.capacity;
extern const void *const source_location_stream_grow = &active_stream_list.grow_by;
}
