/* Authored x87 plane normalization; native zero/NaN/Inf behavior preserved. */
#include "geometry_interfaces.h"
extern "C" double __cdecl sqrt(double);
#pragma intrinsic(sqrt)
void ObservedSceneInterface::NormalizePlane(float planes[][4], int index) {
    float *v = planes[index];
    float t = 1.0f / (float)sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    v[0] *= t; v[1] *= t; v[2] *= t; v[3] *= t;
}

