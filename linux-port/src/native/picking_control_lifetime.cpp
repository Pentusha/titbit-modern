/* Source-proved polymorphic lifetime. The shared controller layout includes
 * the actual six-plane frustum used by Pick; its default float storage is
 * intentionally not initialized by this constructor, as in the native code. */
#include "picking_control_state.h"
#include "picking_control_interfaces.h"
ObservedPickingControl::ObservedPickingControl() {
 picking_scene.SelectedObjectSlot()=0;
 last_click=0;
}
ObservedPickingControl::~ObservedPickingControl() { }
