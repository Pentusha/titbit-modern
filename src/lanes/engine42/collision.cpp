#define OBSERVED_STRING_WTL_BUFFER  // GetBuffer/ReleaseBuffer expanded inline (WTL)
#define OBSERVED_STRING_INLINE_COPY  // CString copy ctor expanded inline (WTL)
#define OBSERVED_STRING_WTL_LOADSTRING  // WTL CString layout (see engine_string.h)
#define OBSERVED_STRING_RELEASE_EXTERNAL  // CString::Release out of line (0x402180)
// Occluder triangles and sphere-vs-triangle sliding collision (Fauerby style),
// Moller-Trumbore ray/triangle test (0x428ab0-0x429640).
#include <math.h>

struct CVec3 {
 float x, y, z;
 CVec3() {}
 CVec3(float ax, float ay, float az);                // 0x406ee0
 CVec3(float ax, float ay, float az, int) { x = ax; y = ay; z = az; }
 CVec3(const CVec3 &o);                              // 0x406f00
 CVec3 &operator=(const CVec3 &o);                   // 0x406f00 (folded)
 float Length_00406f50() const;
 void Normalize_00406f80();
 CVec3 Cross_00406fd0(const CVec3 &o) const;
 float Dot_00407010(const CVec3 &o) const;
 CVec3 Scaled_00416780(float s) const;
 CVec3 &Add_0040cbc0(const CVec3 &o);
 CVec3 Negated_004295c0() const;
};

struct ZVec3 { float x, y, z; ZVec3() { x = y = z = 0; } };

class Triangle {  // 0x428ab0 (was Triangle_00428ab0)
public:
 ZVec3 a, b, c;                    // 0x04
 ZVec3 normal;                     // 0x28
 float d;                          // 0x34
 Triangle();
 virtual ~Triangle();
 void Set_00428af0(float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz);
 void ComputePlane_00428b40();
 CVec3 ClosestPoint_00428d70(float px, float py, float pz);
 int Classify_00428f10(float px, float py, float pz);
 float RayPlane_00428f70(float ox, float oy, float oz, float dx, float dy, float dz);
 float Collide_00429060(CVec3 position, CVec3 direction, float distance, float radius, CVec3 *slide, float *slideLength);
 bool RayHits_004295e0(CVec3 origin, CVec3 direction, float *t);
 bool Intersect_00429640(CVec3 origin, CVec3 direction, float *t, float *u, float *v);
};

enum { PLANE_BACKSIDE = 0x1, PLANE_FRONT = 0x10, ON_PLANE = 0x100 };

Triangle::Triangle() {}

void Triangle::Set_00428af0(float ax, float ay, float az, float bx, float by, float bz,
                                     float cx, float cy, float cz) {
 a.x = ax; a.y = ay; a.z = az;
 b.x = bx; b.y = by; b.z = bz;
 c.x = cx; c.y = cy; c.z = cz;
 ComputePlane_00428b40();
}

// NONMATCHING: faithful translation
void Triangle::ComputePlane_00428b40() {
 // Every step goes through the float members (the original re-reads them from
 // memory, so intermediate values are rounded to 32 bits).
 normal.x = b.x - a.x;
 normal.y = b.y - a.y;
 normal.z = b.z - a.z;
 float ex = c.x - a.x, ey = c.y - a.y, ez = c.z - a.z;
 float x = ez * normal.y - ey * normal.z;
 float y = ex * normal.z - ez * normal.x;
 float z = ey * normal.x - ex * normal.y;
 normal.x = x;
 normal.y = y;
 normal.z = z;
 float length = (float)sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
 if (length != 0.0f) {
  length = 1.0f / length;
  normal.x = length * normal.x;
  normal.y = length * normal.y;
  normal.z = length * normal.z;
 }
 d = -(normal.z * a.z + normal.y * a.y + normal.x * a.x);
}

// Closest point to p on the segment a-b.
// NONMATCHING: faithful translation
CVec3 __stdcall ClosestPointOnLine_00428c30(const ZVec3 &a, const ZVec3 &b, const CVec3 &p) {
 float vx = b.x - a.x, vy = b.y - a.y, vz = b.z - a.z;
 float d = (float)sqrt(vx * vx + vy * vy + vz * vz);
 if (d != 0.0f) {
  float inv = 1.0f / d;
  vx *= inv; vy *= inv; vz *= inv;
 }
 float t = vx * (p.x - a.x) + vy * (p.y - a.y) + vz * (p.z - a.z);
 if (t < 0.0f) return CVec3(a.x, a.y, a.z);
 if (t > d) return CVec3(b.x, b.y, b.z);
 return CVec3(vx * t + a.x, vy * t + a.y, vz * t + a.z);
}

// NONMATCHING: faithful translation
CVec3 Triangle::ClosestPoint_00428d70(float px, float py, float pz) {
 CVec3 p(px, py, pz);
 CVec3 result(0, 0, 0);
 CVec3 rab = ClosestPointOnLine_00428c30(a, b, p);
 CVec3 rbc = ClosestPointOnLine_00428c30(b, c, p);
 CVec3 rca = ClosestPointOnLine_00428c30(c, a, p);
 float dab = (float)sqrt((px - rab.x) * (px - rab.x) + (py - rab.y) * (py - rab.y) + (pz - rab.z) * (pz - rab.z));
 float dbc = (float)sqrt((px - rbc.x) * (px - rbc.x) + (py - rbc.y) * (py - rbc.y) + (pz - rbc.z) * (pz - rbc.z));
 CVec3 vca(px - rca.x, py - rca.y, pz - rca.z);
 float dca = (float)sqrt(vca.x * vca.x + vca.y * vca.y + vca.z * vca.z);
 result.x = rab.x; result.y = rab.y; result.z = rab.z;
 float best = dab;
 if (dbc < best) { best = dbc; result = rbc; }
 if (dca < best) result = rca;
 return result;
}

// NONMATCHING: faithful translation
int Triangle::Classify_00428f10(float px, float py, float pz) {
 float distance = (a.x - px) * normal.x + (a.y - py) * normal.y + (a.z - pz) * normal.z;
 if (distance < -0.001f) return PLANE_FRONT;
 if (distance > 0.001f) return PLANE_BACKSIDE;
 return ON_PLANE;
}

// NONMATCHING: faithful translation
float Triangle::RayPlane_00428f70(float ox, float oy, float oz, float dx, float dy, float dz) {
 float denom = dx * normal.x + dy * normal.y + dz * normal.z;
 if (denom == 0.0f) return -1.0f;
 return -((ox * normal.x + oy * normal.y + oz * normal.z + d) / denom);
}

// NONMATCHING: faithful translation
float __stdcall RaySphere_00428fd0(float ox, float oy, float oz, float dx, float dy, float dz,
                                 float sx, float sy, float sz, float radius) {
 float qx = sx - ox, qy = sy - oy, qz = sz - oz;
 float c = (float)sqrt(qx * qx + qy * qy + qz * qz);
 float v = qx * dx + dy * qy + dz * qz;
 float d = radius * radius - (c * c - v * v);
 if (d < 0.0f) return -1.0f;
 return v - (float)sqrt(d);
}

// Sphere (unit radius) moving along `direction` for `distance` against this triangle.
// Returns the distance that can be travelled; fills the slide vector and its length.
// NONMATCHING: faithful translation
float Triangle::Collide_00429060(CVec3 position, CVec3 direction, float distance, float radius,
                                          CVec3 *slide, float *slideLength) {
 CVec3 sphereIntersect(position.x - normal.x, position.y - normal.y, position.z - normal.z);
 CVec3 slideNormal;
 slideNormal.x = 0; slideNormal.y = 0; slideNormal.z = 0;
 if (direction.x * normal.x + direction.y * normal.y + direction.z * normal.z >= 0.0f) return distance;
 float t;
 CVec3 planeIntersect;
 if (Classify_00428f10(sphereIntersect.x, sphereIntersect.y, sphereIntersect.z) == PLANE_BACKSIDE) {
  CVec3 origin(sphereIntersect);
  t = RayPlane_00428f70(origin.x, origin.y, origin.z, normal.x, normal.y, normal.z);
  CVec3 p(t * normal.x, t * normal.y, t * normal.z);
  p = CVec3(p.x + sphereIntersect.x, p.y + sphereIntersect.y, p.z + sphereIntersect.z);
  planeIntersect = p;
 } else {
  CVec3 origin(sphereIntersect);
  t = RayPlane_00428f70(origin.x, origin.y, origin.z, direction.x, direction.y, direction.z);
  CVec3 p(direction.x * t, direction.y * t, direction.z * t);
  p = CVec3(p.x + sphereIntersect.x, p.y + sphereIntersect.y, p.z + sphereIntersect.z);
  planeIntersect = p;
 }
 if (t > distance) return distance;
 if (t == -1.0f) return distance;
 if (t < -radius) return distance;
 CVec3 polyIntersect(planeIntersect);
 float travel = t;
 if (!RayHits_004295e0(sphereIntersect, direction, &t)) {
  polyIntersect = ClosestPoint_00428d70(planeIntersect.x, planeIntersect.y, planeIntersect.z);
  CVec3 back = direction.Negated_004295c0();
  travel = RaySphere_00428fd0(polyIntersect.x, polyIntersect.y, polyIntersect.z, back.x, back.y, back.z,
                              position.x, position.y, position.z, 1.0f);
  if (travel > distance || travel == -1.0f) return distance;
  if (travel > 0.0f) {
   CVec3 step(direction.x * travel, direction.y * travel, direction.z * travel);
   sphereIntersect = CVec3(polyIntersect.x - step.x, polyIntersect.y - step.y, polyIntersect.z - step.z);
  }
  CVec3 step(direction.x * travel, direction.y * travel, direction.z * travel);
  CVec3 center(step.x + position.x, position.y + step.y, position.z + step.z);
  position = CVec3(center.x - sphereIntersect.x, center.y - sphereIntersect.y, center.z - sphereIntersect.z);
  slideNormal = position;
  slideNormal.Normalize_00406f80();
 } else {
  slideNormal.x = normal.x; slideNormal.y = normal.y; slideNormal.z = normal.z;
 }
 if (travel >= 0.001f) travel = travel - 0.001f;
 else travel = 0;
 CVec3 destination = direction.Scaled_00416780(distance - travel);
 float l = -slideNormal.Dot_00407010(destination);
 destination.Add_0040cbc0(slideNormal.Scaled_00416780(l));
 *slide = destination;
 slide->Normalize_00406f80();
 *slideLength = destination.Length_00406f50();
 return travel;
}

// NONMATCHING: faithful translation
bool Triangle::RayHits_004295e0(CVec3 origin, CVec3 direction, float *t) {
 float u, v;
 if (Intersect_00429640(origin, direction, t, &u, &v) == true && *t >= 0.0f) return true;
 return false;
}

// Moller-Trumbore ray/triangle intersection.
// NONMATCHING: faithful translation
bool Triangle::Intersect_00429640(CVec3 origin, CVec3 direction, float *t, float *u, float *v) {
 CVec3 tvec, qvec;
 tvec.x = tvec.y = tvec.z = 0;
 qvec.x = qvec.y = qvec.z = 0;
 CVec3 edge1(b.x - a.x, b.y - a.y, b.z - a.z);
 CVec3 edge2(c.x - a.x, c.y - a.y, c.z - a.z);
 CVec3 pvec(direction.y * edge2.z - direction.z * edge2.y,
            direction.z * edge2.x - edge2.z * direction.x,
            edge2.y * direction.x - direction.y * edge2.x);
 float det = pvec.x * edge1.x + pvec.y * edge1.y + pvec.z * edge1.z;
 if (det > -0.001f && det < 0.001f) return false;
 float inv = 1.0f / det;
 origin = CVec3(origin.x - a.x, origin.y - a.y, origin.z - a.z);
 tvec = origin;
 *u = tvec.Dot_00407010(pvec) * inv;
 qvec = tvec.Cross_00406fd0(edge1);
 *v = direction.Dot_00407010(qvec) * inv;
 *t = edge2.Dot_00407010(qvec) * inv;
 if (*u < 0.0f || *u > 1.0f) return false;
 if (*v < 0.0f || *u + *v > 1.0f) return false;
 return true;
}

CVec3 CVec3::Negated_004295c0() const {
 return CVec3(-x, -y, -z, 0);
}
