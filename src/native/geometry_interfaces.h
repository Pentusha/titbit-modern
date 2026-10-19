#ifndef TITBIT_OBSERVED_GEOMETRY_INTERFACES_H
#define TITBIT_OBSERVED_GEOMETRY_INTERFACES_H
/* Observed interfaces, not full native object layouts. Never allocate original
 * global storage or infer native sizeof from these method views. */
/* This return view records the 12-byte value ABI and inline construction only;
 * it does not assert the original C++ class identity. */
struct ObservedMatrixPoint {
 float x, y, z;
 ObservedMatrixPoint(float a, float b, float c) : x(a), y(b), z(c) { }
};
struct ObservedMatrix4 {
 float v[16];
 ObservedMatrix4() { Identity(); }
 void Identity();
 ObservedMatrixPoint CameraOrigin() const;
};
struct ObservedVector3 {
 float x, y, z;
 ObservedVector3(float a, float b, float c);
 ObservedVector3(const ObservedVector3 &v);
 float operator*(const ObservedVector3 &v) const { return x*v.x + y*v.y + z*v.z; }
};
struct ObservedPointTarget { unsigned reserved; float x, y; };
struct ObservedPointNode {
 unsigned char reserved[0x38];
 ObservedPointTarget *point;
 unsigned reserved3c, changed;
};
#include "scene_owner_view.h"
class ObservedFrustumInterface {
 unsigned char observed_prefix[0x38];
 float planes[6][4];
public:
 void BuildPlanes();
 float RayPlane(ObservedVector3 a, ObservedVector3 b, ObservedVector3 c, float d);
};
#endif
