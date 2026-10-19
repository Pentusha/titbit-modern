/* Native renderer at 0x5026d0. The independent constructor41e8f0 initializes
 * 10000 records at stride96 and32 stack entries at stride80, then its camera.
 * The source schema below accounts for every byte; no size is guessed from a
 * maximum operand. Native global initializer421590 owns actual construction.
 * POD storage intentionally does not register another constructor/destructor. */
#include <stddef.h>

struct FrameVector { float x, y, z; };
struct FrameMatrix { float value[16]; };
struct FrameDepthEntry {
    FrameVector vertex[3];
    float uv[6], color[4];
    int type;
    float depth;
    unsigned texture;
    FrameDepthEntry *next, *previous;
};
struct FrameStackEntry { unsigned head[4]; FrameMatrix matrix; };
struct FrameRendererState {
    void *graphics_context;
    unsigned reserved_header[3];
    int next_light, depth_count;
    FrameDepthEntry depth_entries[10000];
    FrameDepthEntry *first, *cursor;
    FrameStackEntry stack[32];
    void *lights[1000];
    int light_count;
    FrameMatrix view;
    float zoom, fov;
};
typedef char EntryStride96[(sizeof(FrameDepthEntry) == 96) ? 1 : -1];
typedef char StackStride80[(sizeof(FrameStackEntry) == 80) ? 1 : -1];
typedef char StackOffset[(offsetof(FrameRendererState, stack) == 0xea620) ? 1 : -1];
typedef char LightsOffset[(offsetof(FrameRendererState, lights) == 0xeb020) ? 1 : -1];
typedef char ViewOffset[(offsetof(FrameRendererState, view) == 0xebfc4) ? 1 : -1];
typedef char RendererSize[(sizeof(FrameRendererState) == 0xec00c) ? 1 : -1];

extern "C" {
static FrameRendererState frame_renderer_storage;
extern const unsigned source_size_frame_renderer = sizeof(frame_renderer_storage);
extern const unsigned source_size_frame_view = sizeof(frame_renderer_storage.view);
extern const unsigned source_size_frame_zoom = sizeof(frame_renderer_storage.zoom);
extern const void * const source_location_frame_view = &frame_renderer_storage.view;
extern const void * const source_location_frame_zoom = &frame_renderer_storage.zoom;
}
