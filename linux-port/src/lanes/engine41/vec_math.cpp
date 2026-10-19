// Small vector/matrix helpers returning vectors by value (0x416780-0x416860).
struct Vec3R {
 float x, y, z;
 Vec3R(float a, float b, float c) { x = a; y = b; z = c; }
 Vec3R Scale_00416780(float s) const;
};

struct Mat4V {
 float m[16];
 Vec3R Rotate_004167b0(float x, float y, float z) const;
 Vec3R InverseRotate_00416810(float x, float y, float z) const;
};

Vec3R Vec3R::Scale_00416780(float s) const { return Vec3R(s * x, s * y, s * z); }

Vec3R Mat4V::Rotate_004167b0(float x, float y, float z) const {
 return Vec3R(z * m[8] + y * m[4] + x * m[0], z * m[9] + y * m[5] + x * m[1], z * m[10] + y * m[6] + x * m[2]);
}

Vec3R Mat4V::InverseRotate_00416810(float x, float y, float z) const {
 return Vec3R(z * m[2] + y * m[1] + x * m[0], z * m[6] + y * m[5] + x * m[4], z * m[10] + y * m[9] + x * m[8]);
}
