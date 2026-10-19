/* Authored vec3/ray-plane methods. Inline dot operand order is preserved;
 * mathematically commuted expansions can change native operand scheduling.
 */
#include "geometry_interfaces.h"
ObservedVector3::ObservedVector3(float a, float b, float c) : x(a), y(b), z(c) { }
ObservedVector3::ObservedVector3(const ObservedVector3 &v) : x(v.x), y(v.y), z(v.z) { }
float ObservedFrustumInterface::RayPlane(ObservedVector3 a, ObservedVector3 b,
                                     ObservedVector3 c, float d) {
    float t = c*b;
    if (t == 0) return -1.0f;
    return -(c*a + d)/t;
}
void ObservedSceneInterface::SetPoint(ObservedVector3 v) {
    if (point_node) {
        point_node->point->x = v.x;
        point_node->point->y = v.y;
        point_node->changed = 1;
    }
}
