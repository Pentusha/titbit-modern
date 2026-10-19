/* Native scene/frame state, separate four-byte variables in static storage.
 * Frame41b40 writes the OS tick, computes a scaled delta, then advances scene
 * time. PickingControl reads the SAME current OS tick, not a second clock. */
int scene_elapsed_ms = 0;                   /* 0x5026bc */
int scene_frame_delta_ms = 0;               /* 0x5026c0 */
unsigned long previous_frame_tick = 0;      /* 0x5026c4 */
unsigned long current_frame_tick = 0;       /* 0x5026c8 */
