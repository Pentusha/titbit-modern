#include "engine_camera.h"
void ObservedCameraContext057::SetCamera(const ObservedCamera057& source) {
 ObservedCamera057& target=*(ObservedCamera057*)((char*)this+0xebfc4);
 target.matrix=source.matrix;
 target.zoom=source.zoom;
 target.fov=source.fov;
 if(*(const bool*)((const char*)&cameraOverride057+72)) {
  const ObservedCamera057& overrideCamera=*(const ObservedCamera057*)&cameraOverride057;
  target.matrix=overrideCamera.matrix;
  *(float*)((char*)this+0xec004)=overrideCamera.zoom;
  *(float*)((char*)this+0xec008)=overrideCamera.fov;
 }
}
