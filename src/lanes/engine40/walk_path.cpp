// Walk path: a closed chain of cubic segments (0x38 bytes each, 1000 in a
// walk-area record) walked by a float parameter (integer part = segment).
// Segment evaluation (0x4067a0/0x4068c0; earlier void-returning versions in
// lanes/claude/curves.cpp) returns the vector by value.
struct PathVec {
 float x, y, z;
 PathVec() {}
 PathVec(float ax, float ay, float az) { x = ax; y = ay; z = az; }
 PathVec operator-(const PathVec &o) const { return PathVec(x - o.x, y - o.y, z - o.z); }
 PathVec operator+(const PathVec &o) const { return PathVec(x + o.x, y + o.y, z + o.z); }
 PathVec operator*(float f) const { return PathVec(x * f, y * f, z * f); }
};

struct PathSegment_00406770 {        // 0x38 bytes
 int index;
 PathVec *p0, *p1;                   // +4, +8
 PathVec *t0, *t1;                   // +0xc, +0x10
 PathVec c0, c1;                     // +0x14, +0x20
 int flags;                          // +0x2c
 PathSegment_00406770 *next, *prev;  // +0x30, +0x34
 PathVec Point_004067a0(float t);
 PathVec Tangent_004068c0(float t);
 ~PathSegment_00406770();
};

// Cubic Hermite through p0, p1 with tangents 3 (c0 - p0) and 3 (p1 - c1).
PathVec PathSegment_00406770::Point_004067a0(float t) {
 float f = t - (float)(int)t;
 float f2 = f * f, f3 = f2 * f;
 float h10 = (f3 - (f2 + f2)) + f;
 float h11 = f3 - f2;
 float h00 = ((f3 + f3) - f2 * 3.0f) + 1.0f;
 float h01 = f3 * -2.0f + f2 * 3.0f;
 PathVec out;
 out.x = p0->x * h00 + p1->x * h01 + (c0.x - p0->x) * 3.0f * h10 + (p1->x - c1.x) * 3.0f * h11;
 out.y = p0->y * h00 + p1->y * h01 + (c0.y - p0->y) * 3.0f * h10 + (p1->y - c1.y) * 3.0f * h11;
 out.z = p0->z * h00 + p1->z * h01 + (c0.z - p0->z) * 3.0f * h10 + (p1->z - c1.z) * 3.0f * h11;
 return out;
}

// Linear blend of the two edge directions (p1 - p0) and (t1 - t0).
PathVec PathSegment_00406770::Tangent_004068c0(float t) {
 PathVec a = *t0 - *p0;
 PathVec b = *t1 - *p1;
 float f = t - (float)(int)t;
 return (b - a) * f + a;
}

struct WalkArea_00407ff0 {
 char head[0x5dec];
 int edgeCount;                      // 0x5dec
 PathSegment_00406770 edges[1000];   // 0x5df0
 int segmentCount;                   // 0x138b0
 PathSegment_00406770 *cursor;       // 0x138b4
 void Next_004082c0();
 void Step_004082e0(float *t, PathVec *position, PathVec *ahead, PathVec *direction, float *speed);
 PathVec *PointAt_004089c0(PathVec *out, float t);   // returns out (EAX), callers may use it
};

void WalkArea_00407ff0::Next_004082c0() {
 PathSegment_00406770 *n = cursor->next;
 if (n) cursor = n;
}

// Move t back by 0.01 * speed (wrapping around the closed path) and return
// the position, the direction and the position 0.1 further on.
void WalkArea_00407ff0::Step_004082e0(float *t, PathVec *position, PathVec *ahead, PathVec *direction, float *speed) {
 float previous = *t;
 *t -= 0.01f;
 PathSegment_00406770 *saved = cursor;
 *t = (*t - previous) * *speed + previous;
 if (*t < 0) *t = segmentCount + *t;
 if (*t >= segmentCount) *t -= segmentCount;
 int segment = (int)*t;
 float f = *t - segment;
 int segment2 = (int)(*t + 0.1);
 float f2 = (*t + 0.1f) - segment;
 cursor = edges;
 int i;
 for (i = segment; i > 0; i--) Next_004082c0();
 *position = cursor->Point_004067a0(f);
 *direction = cursor->Tangent_004068c0(f);
 cursor = edges;
 for (i = segment2; i > 0; i--) Next_004082c0();
 *ahead = cursor->Point_004067a0(f2);
 cursor = saved;
}

PathVec *WalkArea_00407ff0::PointAt_004089c0(PathVec *out, float t) {
 PathSegment_00406770 *saved = cursor;
 int segment = (int)t;
 cursor = edges;
 t -= segment;
 for (int i = segment; i > 0; i--) Next_004082c0();
 PathVec p = cursor->Point_004067a0(t);
 cursor = saved;
 *out = p;
 return out;
}
