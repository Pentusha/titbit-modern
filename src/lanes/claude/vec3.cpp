// 3D vector helpers (0x406f20-0x407030).
#include <math.h>
struct Vec3_00406f20 {
 float x, y, z;
 Vec3_00406f20() {}
 Vec3_00406f20(float ax, float ay, float az) { x = ax; y = ay; z = az; }
 Vec3_00406f20 Sub_00406f20(const Vec3_00406f20 &o) const;
 float Length_00406f50() const;
 void Normalize_00406f80();
 Vec3_00406f20 Cross_00406fd0(const Vec3_00406f20 &o) const;
 float Dot_00407010(const Vec3_00406f20 &o) const;
 void Set_00407030(float ax, float ay, float az);
};

Vec3_00406f20 Vec3_00406f20::Sub_00406f20(const Vec3_00406f20 &o) const {
 return Vec3_00406f20(x - o.x, y - o.y, z - o.z);
}
float Vec3_00406f20::Length_00406f50() const { return (float)sqrt(x * x + y * y + z * z); }
void Vec3_00406f20::Normalize_00406f80() {
 float length = (float)sqrt(x * x + y * y + z * z);
 if (length != 0.0f) {
  float inv = 1.0f / length;
  x = inv * x; y = inv * y; z = inv * z;
 }
}
// NONMATCHING 0x406fd0: one commutative fmul operand order differs (4 bytes)
Vec3_00406f20 Vec3_00406f20::Cross_00406fd0(const Vec3_00406f20 &o) const {
 return Vec3_00406f20(y * o.z - z * o.y, z * o.x - o.z * x, x * o.y - y * o.x);
}
float Vec3_00406f20::Dot_00407010(const Vec3_00406f20 &o) const { return o.x * x + o.y * y + o.z * z; }
void Vec3_00406f20::Set_00407030(float ax, float ay, float az) { x = ax; y = ay; z = az; }
