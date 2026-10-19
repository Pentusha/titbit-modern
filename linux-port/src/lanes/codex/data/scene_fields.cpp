/* Field labels share the existing source-owned Scene object. No duplicate
 * camera override or selected-pointer variable is allocated. */
#include "e42_scene.h"
#include <stddef.h>
extern Scene g_005011c0;
typedef char ProjectionAt10c8[(offsetof(Scene, projection) == 0x10c8) ? 1 : -1];
typedef char SelectedAt1114[(offsetof(Scene, selected) == 0x1114) ? 1 : -1];
typedef char PairsAtFe8[(offsetof(Scene, pairs) == 0xfe8) ? 1 : -1];
extern "C" {
extern const unsigned source_size_scene = sizeof(Scene);
extern const unsigned source_size_scene_camera = offsetof(Scene, selected) - offsetof(Scene, projection);
extern const unsigned source_size_scene_selected = sizeof(g_005011c0.selected);
extern const unsigned source_size_scene_pairs = sizeof(g_005011c0.pairs);
extern const void * const source_location_scene_camera = &g_005011c0.projection;
extern const void * const source_location_scene_selected = &g_005011c0.selected;
extern const void * const source_location_scene_pairs = &g_005011c0.pairs;
}
