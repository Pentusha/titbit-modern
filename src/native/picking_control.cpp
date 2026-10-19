/* Authored views of observed control fields and message offsets, not complete
 * original class identities or sizes. External globals/callees are unbuilt. */
#include "picking_control_interfaces.h"
#include "picking_control_state.h"
extern unsigned picking_clock;
extern void __cdecl PickingTrace(const char *, ...);
void ObservedPickingControl::Update() {
 Pick(mouse_x, mouse_y);
 Select();
}
void ObservedPickingControl::Click(unsigned event) {
 if (last_click > picking_clock - 300u) {
  PickingTrace("Prevent double click");
  return;
 }
 last_click = picking_clock;
 if (picking_scene.SelectedObjectSlot()) {
  ObservedPickingMessage message;
  memset(&message, 0, sizeof(message));
  message.event = event;
  message.selected = picking_scene.SelectedObjectSlot();
  picking_scene.Send(&message);
 }
}
void ObservedPickingControl::Key(int key) {
 if (key == 27) {
  ObservedPickingMessage message;
  memset(&message, 0, sizeof(message));
  message.event = 12;
  message.reserved28[0] = 0;
  unsigned i = 0;
  do { ((char *)message.reserved0)[i] = "mainmenu"[i]; }
  while ("mainmenu"[i++] != 0);
  picking_scene.Send(&message);
  return;
 }
 if (key == 2) {
  ObservedPickingMessage message;
  memset(&message, 0, sizeof(message));
  message.event = 12;
  message.reserved28[0] = 1;
  unsigned i = 0;
  do { ((char *)message.reserved0)[i] = "mainmenu"[i]; }
  while ("mainmenu"[i++] != 0);
  picking_scene.Send(&message);
 }
}
