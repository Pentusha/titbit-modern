// Walkable area: up to 1000 floor/ceiling vertices and 1000 boundary edges,
// default 80x60 rectangle (0x407ff0 and neighbours). Record size 0x138b8.
struct ObservedSoundState {          // 0x2c-byte named header (ctor 0x402ce0, Reset 0x402cf0)
 char name[32]; int a, b, c;
 ObservedSoundState();
 void Reset();
};

struct AreaVec { float x, y, z; AreaVec() { x = y = z = 0; } };

struct AreaEdge_00406770 {           // 0x38 bytes
 int index;
 AreaVec *floor0, *floor1, *ceil0, *ceil1;
 AreaVec normal0, normal1;
 int flags;
 AreaEdge_00406770 *next, *prev;
 AreaEdge_00406770();
 ~AreaEdge_00406770();
};

struct WalkArea_00407ff0 : ObservedSoundState {
 AreaVec floor[1000];                // 0x2c
 AreaVec ceiling[1000];              // 0x2f0c
 int edgeCount;                      // 0x5dec
 AreaEdge_00406770 edges[1000];      // 0x5df0
 int vertexCount;                    // 0x138b0
 AreaEdge_00406770 *firstEdge;       // 0x138b4
 WalkArea_00407ff0();
};

static inline void SetVec(AreaVec &v, float x, float y, float z) { v.x = x; v.y = y; v.z = z; }

AreaEdge_00406770::AreaEdge_00406770() { index = 0; floor0 = 0; floor1 = 0; flags = 0; next = 0; prev = 0; }

WalkArea_00407ff0::WalkArea_00407ff0() {
 SetVec(floor[0], 40, 30, 0);   SetVec(ceiling[0], 40, 30, 1);
 SetVec(floor[1], 40, -30, 0);  SetVec(ceiling[1], 40, -30, 1);
 SetVec(floor[2], -40, -30, 0); SetVec(ceiling[2], -40, -30, 1);
 SetVec(floor[3], -40, 30, 0);  SetVec(ceiling[3], -40, 30, 1);
 edges[0].floor0 = &floor[0]; edges[0].floor1 = &floor[1]; edges[0].ceil0 = &ceiling[0]; edges[0].ceil1 = &ceiling[1];
 edges[1].floor0 = &floor[1]; edges[1].floor1 = &floor[2]; edges[1].ceil0 = &ceiling[1]; edges[1].ceil1 = &ceiling[2];
 edges[2].floor0 = &floor[2]; edges[2].floor1 = &floor[3]; edges[2].ceil0 = &ceiling[2]; edges[2].ceil1 = &ceiling[3];
 edges[3].floor0 = &floor[3]; edges[3].floor1 = &floor[0]; edges[3].ceil0 = &ceiling[3]; edges[3].ceil1 = &ceiling[0];
 edgeCount = 4;
 SetVec(edges[0].normal0, 60, 20, 0);   SetVec(edges[0].normal1, 60, -20, 0);
 SetVec(edges[1].normal0, 20, -40, 0);  SetVec(edges[1].normal1, -20, -40, 0);
 SetVec(edges[2].normal0, -60, -20, 0); SetVec(edges[2].normal1, -60, 20, 0);
 SetVec(edges[3].normal0, -20, 40, 0);  SetVec(edges[3].normal1, 20, 40, 0);
 edges[3].flags = 16; edges[2].flags = 16; edges[1].flags = 16; edges[0].flags = 16;
 edges[1].next = &edges[2]; edges[1].prev = &edges[0];
 edges[3].next = &edges[0]; edges[3].prev = &edges[2];
 firstEdge = &edges[0];
 edges[0].next = &edges[1]; edges[0].prev = &edges[3];
 edges[2].next = &edges[3]; edges[2].prev = &edges[1];
 vertexCount = 4;
}
