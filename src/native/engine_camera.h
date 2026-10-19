#ifndef TITBIT_ENGINE_CAMERA_H
#define TITBIT_ENGINE_CAMERA_H
#include "geometry_interfaces.h"
struct ObservedCamera057 {ObservedMatrix4 matrix;float zoom,fov;};
class ObservedCameraContext057 {public:void SetCamera(const ObservedCamera057&);};
class CameraOverrideOwner057;
extern CameraOverrideOwner057 cameraOverride057;
// A distinct observed method view avoids conflicting definitions of RenderContext.
// Owner size/initialization remain unknown; these declarations allocate nothing.
#endif
