// Box-shaped model collision (0x405320): the twelve triangles of the box with
// half extents (x, y, z) around the centre of the owner's first bone.
// Loader 0x4057f0: lanes/claude/model_xml.cpp; triangles: lanes/engine42/collision.cpp.
#include "lane_engine.h"

struct CVec3 {
 float x, y, z;
 CVec3() {}
 CVec3(const CVec3 &o);                              // 0x406f00
};

class Triangle {  // 0x428ab0 (was Triangle_00428ab0)
public:
 float a[3], b[3], c[3], normal[3], d;
 virtual ~Triangle();
 void Set_00428af0(CVec3 a, CVec3 b, CVec3 c);
};

struct BoxBone { char head[0x2c]; float center[3]; char tail[0xd8 - 0x38]; };
struct BoxOwner { char head[0x38]; BoxBone *bones; };

class BoxedModel_004057f0 : public Node_00405160 {
public:
 Triangle faces[12];         // 0x14
 float sx, sy, sz;                    // 0x2b4
 void SetBoxSize_00405320(float x, float y, float z);
};

void BoxedModel_004057f0::SetBoxSize_00405320(float x, float y, float z) {
 const float *center = ((BoxOwner *)owner)->bones[0].center;
 float cx = center[0], cy = center[1], cz = center[2];
 CVec3 ppp, nnn, npp, ppn, npn, pnp, nnp, pnn;
 ppp.x = cx + x; ppp.y = cy + y; ppp.z = cz + z;
 nnn.x = cx - x; nnn.y = cy - y; nnn.z = cz - z;
 npp.x = cx - x; npp.y = cy + y; npp.z = cz + z;
 ppn.x = cx + x; ppn.y = cy + y; ppn.z = cz - z;
 npn.x = cx - x; npn.y = cy + y; npn.z = cz - z;
 pnp.x = cx + x; pnp.y = cy - y; pnp.z = cz + z;
 nnp.x = cx - x; nnp.y = cy - y; nnp.z = cz + z;
 pnn.x = cx + x; pnn.y = cy - y; pnn.z = cz - z;
 faces[0].Set_00428af0(nnn, npn, ppn);
 faces[1].Set_00428af0(nnn, ppn, pnn);
 faces[2].Set_00428af0(nnn, npp, npn);
 faces[3].Set_00428af0(nnn, nnp, npp);
 faces[4].Set_00428af0(pnn, ppn, ppp);
 faces[5].Set_00428af0(pnn, ppp, pnp);
 faces[6].Set_00428af0(nnp, ppp, npp);
 faces[7].Set_00428af0(nnp, pnp, ppp);
 faces[8].Set_00428af0(npn, npp, ppp);
 faces[9].Set_00428af0(npn, ppp, ppn);
 faces[10].Set_00428af0(nnn, pnp, nnp);
 faces[11].Set_00428af0(nnn, pnn, pnp);
}
