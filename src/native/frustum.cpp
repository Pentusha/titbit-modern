/* Authored restoration; no original global storage or complete class claim. */
#include "geometry_interfaces.h"
extern "C" __declspec(dllimport) void __stdcall glGetFloatv(unsigned int, float *);
void ObservedMatrix4::Identity() {
    v[0] = v[5] = v[10] = v[15] = 1;
    v[1] = v[2] = v[3] = v[4] = v[6] = v[7] = v[8] = v[9] =
        v[11] = v[12] = v[13] = v[14] = 0;
}

void ObservedFrustumInterface::BuildPlanes() {
    ObservedMatrix4 m;
    glGetFloatv(0x0ba7, m.v); // GL_PROJECTION_MATRIX, evidenced native enum
    float (*f)[4] = planes;
    float *v = m.v;
    for (int i = 0; i < 6; i += 2) {
        planes[i][0] = m.v[3] - v[0];
        planes[i][1] = m.v[7] - v[4];
        planes[i][2] = m.v[11] - v[8];
        planes[i][3] = m.v[15] - v[12];
        picking_scene.NormalizePlane(f, i);
        planes[i+1][0] = m.v[3] + v[0];
        planes[i+1][1] = m.v[7] + v[4];
        planes[i+1][2] = m.v[11] + v[8];
        planes[i+1][3] = m.v[15] + v[12];
        picking_scene.NormalizePlane(f, i+1);
        ++v;
    }
}
