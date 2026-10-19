// Animation curve segments: cubic Bezier (0x4067a0) and linear (0x4068c0).
struct CurveVec { float x, y, z; };

struct BezierSegment_004067a0 {
 int pad0;
 CurveVec *p0, *p1;          // +4, +8
 int padC, pad10;
 CurveVec c0, c1;            // +0x14, +0x20
 void Eval_004067a0(CurveVec *out, float t);
};
void BezierSegment_004067a0::Eval_004067a0(CurveVec *out, float t) {
 float f = t - (float)(int)t;
 float f2 = f * f, f3 = f2 * f;
 float h10 = (f3 - (f2 + f2)) + f;
 float h11 = f3 - f2;
 float h00 = ((f3 + f3) - f2 * 3.0f) + 1.0f;
 float h01 = f3 * -2.0f + f2 * 3.0f;
 out->x = p0->x * h00 + p1->x * h01 + (c0.x - p0->x) * 3.0f * h10 + (p1->x - c1.x) * 3.0f * h11;
 out->y = p0->y * h00 + p1->y * h01 + (c0.y - p0->y) * 3.0f * h10 + (p1->y - c1.y) * 3.0f * h11;
 out->z = p0->z * h00 + p1->z * h01 + (c0.z - p0->z) * 3.0f * h10 + (p1->z - c1.z) * 3.0f * h11;
}

struct LinearSegment_004068c0 {
 int pad0;
 CurveVec *a0, *b0, *a1, *b1;   // +4, +8, +0xc, +0x10
 void Eval_004068c0(CurveVec *out, float t);
};
void LinearSegment_004068c0::Eval_004068c0(CurveVec *out, float t) {
 float dx = a1->x - a0->x, dz = a1->z - a0->z;
 float f = t - (float)(int)t;
 out->x = ((b1->x - b0->x) - dx) * f + dx;
 float dy = a1->y - a0->y;
 out->y = ((b1->y - b0->y) - dy) * f + dy;
 out->z = ((b1->z - b0->z) - dz) * f + dz;
}
