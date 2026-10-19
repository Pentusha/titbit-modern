/* Native renderBounds501020 is pickingControl501018.left..bottom, not an
 * independently allocated rectangle. These compiler expressions prove the
 * shared storage without absolute implementation definitions or byte payload. */
#include "../../../native/picking_control_state.h"
#include <stddef.h>

typedef char PickingBoundsAt8[(offsetof(ObservedPickingControl,left)==8)?1:-1];
typedef char PickingBoundsEndAt18[(offsetof(ObservedPickingControl,mouse_x)==0x18)?1:-1];
extern "C" {
extern const unsigned source_size_picking_controller = sizeof(pickingControl);
extern const unsigned source_size_picking_bounds = offsetof(ObservedPickingControl,mouse_x)-offsetof(ObservedPickingControl,left);
extern const void * const source_location_picking_bounds = &pickingControl.left;
}
