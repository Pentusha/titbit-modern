#ifndef TITBIT_PICKING_CONTROL_STATE_H
#define TITBIT_PICKING_CONTROL_STATE_H
/* Shared controller layout. Pick (0x40fe90) and BuildPlanes independently
 * address the six four-float frustum planes at +0x38. Global source storage
 * must include them: allocating only the earlier prefix overwrites its next
 * neighbor during the first frame. Original class/field names are unknown. */
struct ObservedPickingRayVector {
 float x,y,z;
 ObservedPickingRayVector(){x=y=z=0;}
};
class ObservedPickingControl {
public:
 ObservedPickingControl();
 virtual ~ObservedPickingControl();
 unsigned last_click;
 int left,top,right,bottom,mouse_x,mouse_y;
 ObservedPickingRayVector ray_origin,ray_direction;
 float planes[6][4];                     // +0x38, native BuildPlanes/Pick
 void Pick(int,int);
 void Select();
 void Update();
 void Click(unsigned);
 void Key(int);
};
typedef char PickingControllerSourceSize[(sizeof(ObservedPickingControl)==0x98)?1:-1];
extern ObservedPickingControl pickingControl;
#endif
