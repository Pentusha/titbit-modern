// Spline animation component (vtable 0x4cd480, 0x1c bytes): moves its owner
// along a walk path. Constructor/destructor: lanes/claude/engine_objects.cpp
// (there Behaviour_00406a40); path stepping: walk_path.cpp.
#define E40_LOADSTRING_CALL
#include "e40_engine.h"
#include "lane_xml.h"
#include <math.h>
extern "C" __declspec(dllimport) char *__stdcall CharNextA(const char *);

/* Vector class of this unit (lanes/claude/vec3.cpp): the constructors are out
 * of line here (0x406ee0, 0x406f00). */
struct SVec {
 float x, y, z;
 SVec() { x = y = z = 0; }
 SVec(float ax, float ay, float az);                 // 0x406ee0
 SVec(const SVec &o);                                // 0x406f00
 SVec Sub_00406f20(const SVec &o) const;
 float Length_00406f50() const;
 void Normalize_00406f80();
 SVec Cross_00406fd0(const SVec &o) const;
 float Dot_00407010(const SVec &o) const;
 void Set_00407030(float ax, float ay, float az);
 // inline forms used by 0x406a80
 SVec CrossInline(const SVec &o) const { return SVec(y * o.z - z * o.y, z * o.x - o.z * x, o.y * x - y * o.x); }
 float DotInline(const SVec &o) const { return y * o.y + x * o.x + z * o.z; }
 void NormalizeInline() {
  float length = Length_00406f50();
  if (length != 0.0f) {
   float inv = 1.0f / length;
   x = x * inv; y = y * inv; z = z * inv;
  }
 }
};

class WalkArea_00407ff0 {
public:
 void Step_004082e0(float *t, SVec *position, SVec *ahead, SVec *direction, float *speed);
 SVec *PointAt_004089c0(SVec *out, float t);
};
class WalkAreaCache_00502480 {
public:
 int Load_00407880(ObservedString path, int age);
 WalkArea_00407ff0 *At_00407e30(int index);
};
extern WalkAreaCache_00502480 g_00502480;

struct SplineBone {                   // 0xd8 bytes (Bone_0040c6d0)
 int flags;
 SVec position;                       // +4
 void SetRotation_0040ddc0(SVec angles);
};
struct SplineOwner { char head[0x38]; SplineBone *bones; int pad3c; int dirty; };

class Behaviour_00406a40 {
public:
 EngineObject *owner;        // +0x04
 void *next;                          // +0x08
 int path;                            // +0x0c walk-area cache index
 int lastTime;                        // +0x10
 float speed;                         // +0x14
 float position;                      // +0x18
 Behaviour_00406a40(EngineObject *owner);
 virtual ObservedString Export();
 virtual void Update(int time);
 virtual void Render(int pass);
 virtual ~Behaviour_00406a40();
 void Load_00407280(XmlNode_v *node);
};

ObservedString Behaviour_00406a40::Export() {
 ObservedString number, xml;
 xml = "<splineanimation ";
 xml += " ";
 xml += "speedfactor=\"";
 number.Format("%.4f", (double)speed);
 xml += number;
 xml += "\" ";
 xml += "position=\"";
 number.Format("%.4f", (double)position);
 xml += number;
 xml += "\" ";
 xml += "splinename=\"";
 xml += number;                       // sic: the path name is not written
 xml += "\" ";
 xml += "/>";
 xml += "\n\r";
 return xml;
}

void Behaviour_00406a40::Load_00407280(XmlNode_v *node) {
 XmlElement_v *element = node->ToElement();
 if (!element) return;
 XmlAttr_v *a = element->Attribute(std::string("speedfactor"));
 if (a) speed = (float)atof(a->Value()->c_str());
 a = element->Attribute(std::string("position"));
 if (a) position = (float)atof(a->Value()->c_str());
 a = element->Attribute(std::string("splinename"));
 if (a) {
  ObservedString file = a->Value()->c_str();
  const char *dot = 0;
  for (const char *p = file; *p; p = CharNextA(p))
   if (*p == '.') dot = p;
  ObservedString name = file.Left(dot ? (int)(dot - (const char *)file) : -1);
  path = g_00502480.Load_00407880(file, -1);   // sic: the name without extension is unused
 }
 if (--element->refs <= 0) element->Destroy(1);
}

// Step along the path by the elapsed time, put the owner's first bone on the
// path and turn it towards the points 0.06 and 0.2 ahead (angles in degrees).
void Behaviour_00406a40::Render(int time) {
 if (time == 0) return;
 if (lastTime == 0) lastTime = time;
 int dt = time - lastTime;
 lastTime = time;
 float step = dt * speed;
 SVec point, ahead, direction, angles;
 WalkArea_00407ff0 *walk = g_00502480.At_00407e30(path);
 walk->Step_004082e0(&position, &point, &ahead, &direction, &step);
 SVec heading(ahead.x - point.x, ahead.y - point.y, ahead.z - point.z);
 direction.x = 0; direction.y = 0; direction.z = 1;
 SVec side = direction.CrossInline(heading);
 direction.NormalizeInline();
 ((SplineOwner *)owner)->bones->position = point;
 SVec flat(ahead.x - point.x, ahead.y - point.y, ahead.z - point.z);
 SVec axisX; axisX.x = 1; axisX.y = 0; axisX.z = 0;
 SVec axisY; axisY.x = 0; axisY.y = 1; axisY.z = 0;
 SVec up;
 flat.z = 0;
 flat.NormalizeInline();
 float cosY = axisY.DotInline(flat);
 float sign = cosY / fabs(cosY);
 up.x = 0; up.y = 0; up.z = 1;
 angles.z = (float)(acos(axisX.DotInline(flat)) * sign * 57.29577951289617);
 SVec *nearPoint = walk->PointAt_004089c0(&side, position + 0.06);
 SVec toNear(nearPoint->x - point.x, nearPoint->y - point.y, nearPoint->z - point.z);
 axisX = toNear;
 axisX = axisX.Cross_00406fd0(up);
 axisX.Normalize_00406f80();
 SVec toFar(walk->PointAt_004089c0(&toNear, position + 0.2)->Sub_00406f20(point));
 toFar.Normalize_00406f80();
 angles.x = (float)(asin(axisX.Dot_00407010(toFar)) * 150.0);
 axisX.Set_00407030(0, 0, 1);
 SVec forward(ahead.Sub_00406f20(point));
 forward.Normalize_00406f80();
 angles.y = (float)(acos(axisX.Dot_00407010(forward)) * 57.29577951289617 - 90.0);
 ((SplineOwner *)owner)->bones->SetRotation_0040ddc0(angles);
 ((SplineOwner *)owner)->dirty = 1;
}
