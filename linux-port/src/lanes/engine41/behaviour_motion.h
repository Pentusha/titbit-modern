// Observed motion/collision ABI used by Behaviour::Render, native 0x415e60.
// Views below allocate no game globals and do not claim complete owner types.
#include <math.h>

struct MotionVec {
 float x,y,z;
 MotionVec(){x=y=z=0;}
 MotionVec(float,float,float);                 // 0x406ee0
 MotionVec(const MotionVec&);                  // 0x406f00
 MotionVec &operator=(const MotionVec&);       // same folded copy body
 float Length_00406f50() const;
 MotionVec Scaled_00416780(float) const;
 MotionVec &Add_0040cbc0(const MotionVec&);
};
struct MotionMatrix {
 float m[16];
 void Identity_0040cc50();
 MotionVec Rotate_004167b0(float,float,float) const;
 MotionVec InverseRotate_00416810(float,float,float) const;
};
struct MotionBone {
 unsigned flags;
 MotionVec position;                          // +4
 unsigned char middle[0x94-0x10];
 MotionMatrix world;                          // +0x94
 // Explicit hidden-return-buffer view: native caller consumes returned EAX.
 MotionVec *GetRotation_0040dc40(MotionVec*) const;
 void SetRotation_0040ddc0(float,float,float);
};
struct MotionCollider {
 virtual void slot0();
 virtual int GetType();
 virtual void slot2();
 virtual void slot3();
 virtual void slot4();
 virtual void slot5();
 virtual void slot6();
 virtual float Collide(MotionVec,MotionVec,float,float,MotionVec*,float*); // +0x1c
 MotionCollider *owner;
 int field8;
 MotionCollider *next;                        // +0x0c
};
struct MotionOwner {
 unsigned char head[0x0c];
 MotionCollider *links;
 unsigned char beforeBones[0x38-0x10];
 MotionBone *bones;
 unsigned field3c;
 int dirty;                                  // +0x40
};
struct MotionSceneView {
 unsigned char head[0x14];
 MotionOwner **objects;                       // Scene+0x14 / native 0x5011d4
 int count;                                  // Scene+0x18 / native 0x5011d8
};
extern MotionSceneView g_005011c0;
extern int g_005026c0;
extern unsigned char rawKeys[256];            // native 0x500ef8
extern long mouseDeltaX;                      // native 0x501000
extern bool mouseRight;                       // native 0x500ffa

static inline float MotionLength(const MotionVec &v) {
 return (float)sqrt(v.z*v.z+v.y*v.y+v.x*v.x);
}
static inline void NormalizeMotion(MotionVec &v) {
 float length=v.Length_00406f50();
 if(length!=0.0f){float inverse=1.0f/length;v.x*=inverse;v.y*=inverse;v.z*=inverse;}
}
static inline MotionVec MotionPointIn(const MotionVec &p,const MotionMatrix &m) {
 float x=p.x-m.m[12],y=p.y-m.m[13],z=p.z-m.m[14];
 return MotionVec(z*m.m[2]+y*m.m[1]+x*m.m[0],
                  z*m.m[6]+y*m.m[5]+x*m.m[4],
                  z*m.m[10]+y*m.m[9]+x*m.m[8]);
}
static inline MotionVec MotionVectorIn(const MotionVec &p,const MotionMatrix &m) {
 return MotionVec(p.z*m.m[2]+p.y*m.m[1]+p.x*m.m[0],
                  p.z*m.m[6]+p.y*m.m[5]+p.x*m.m[4],
                  p.z*m.m[10]+p.y*m.m[9]+p.x*m.m[8]);
}
static inline MotionVec MotionVectorOut(const MotionVec &p,const MotionMatrix &m) {
 return MotionVec(p.z*m.m[8]+p.y*m.m[4]+p.x*m.m[0],
                  p.z*m.m[9]+p.x*m.m[1]+p.y*m.m[5],
                  p.z*m.m[10]+p.x*m.m[2]+p.y*m.m[6]);
}

// Actual source restoration: keyboard/mouse yaw, swept-sphere sliding and
// downward sweep. The original loop has no invented iteration cap.
void Behaviour::Render(int pass) {
 MotionVec velocity;
 MotionMatrix orientation,unused;
 orientation.Identity_0040cc50();
 unused.Identity_0040cc50();
 MotionOwner *object=(MotionOwner*)owner;
 float elapsed=(float)(unsigned int)g_005026c0;
 if((rawKeys[203]&0x80)!=0 || mouseDeltaX!=0){
  MotionVec buffer;
  MotionVec *angles=object->bones->GetRotation_0040dc40(&buffer);
  float x=angles->x,y=angles->y,z=angles->z;
  if(rawKeys[203]&0x80)z+=elapsed*0.125f;
  else if(mouseRight)z-=(float)(mouseDeltaX/5);
  object->bones->SetRotation_0040ddc0(x,y,z);
 }
 if(rawKeys[205]&0x80){
  MotionVec buffer;
  MotionVec *angles=object->bones->GetRotation_0040dc40(&buffer);
  float x=angles->x,y=angles->y,z=angles->z;
  z-=elapsed*0.125f;
  object->bones->SetRotation_0040ddc0(x,y,z);
 }
 if(rawKeys[200]&0x80)velocity.x=elapsed*-0.025f;
 if(rawKeys[208]&0x80)velocity.x=elapsed*0.025f;
 float distance=MotionLength(velocity);
 for(int k=0;k<16;++k)orientation.m[k]=object->bones->world.m[k];
 orientation.m[12]=orientation.m[13]=orientation.m[14]=0;
 MotionVec transformed=MotionVectorOut(velocity,orientation);
 velocity.x=transformed.x;velocity.y=transformed.y;velocity.z=transformed.z;
 NormalizeMotion(velocity);
 velocity.x=distance*velocity.x;velocity.y=velocity.y*distance;velocity.z=velocity.z*distance;
 distance=MotionLength(velocity);
 NormalizeMotion(velocity);
 fieldC=0;
 while(distance>0.001f){
  float travel=distance,remaining=0;
  MotionVec slide,localSlide;
  float residual;
  for(int i=0;i<g_005011c0.count;++i){
   MotionOwner *other=g_005011c0.objects[i];
   for(MotionCollider *c=other->links;c;c=c->next){
    if(c->GetType()!=4)continue;
    MotionVec origin=MotionPointIn(object->bones->position,other->bones->world);
    MotionVec direction=MotionVectorIn(velocity,other->bones->world);
    float hit=c->Collide(origin,direction,distance,1.0f,&localSlide,&residual);
    if(hit<travel){
     travel=hit;remaining=residual;
     MotionVec worldSlide=MotionVectorOut(localSlide,other->bones->world);
     slide.x=worldSlide.x;slide.y=worldSlide.y;slide.z=worldSlide.z;
    }
    break;
   }
  }
  MotionVec step(travel*velocity.x,velocity.y*travel,velocity.z*travel);
  object->bones->position.x=step.x+object->bones->position.x;
  object->bones->position.y=step.y+object->bones->position.y;
  object->bones->position.z=step.z+object->bones->position.z;
  if(remaining==distance && travel==0.0f)break;
  velocity.x=slide.x;velocity.y=slide.y;velocity.z=slide.z;
  NormalizeMotion(velocity);
  distance=remaining;
 }
 float fall=elapsed*0.025f,travel=fall;
 velocity.x=velocity.y=0;velocity.z=-1.0f;
 MotionVec localSlide,worldSlide;
 float residual;
 for(int i=0;i<g_005011c0.count;++i){
  MotionOwner *other=g_005011c0.objects[i];
  for(MotionCollider *c=other->links;c;c=c->next){
   if(c->GetType()!=4)continue;
   MotionVec origin=MotionPointIn(object->bones->position,other->bones->world);
   MotionVec direction=other->bones->world.InverseRotate_00416810(velocity.x,velocity.y,velocity.z);
   float hit=c->Collide(origin,direction,fall,1.0f,&localSlide,&residual);
   if(hit<travel){
    travel=hit;
    worldSlide=other->bones->world.Rotate_004167b0(localSlide.x,localSlide.y,localSlide.z);
   }
   break;
  }
 }
 object->bones->position.Add_0040cbc0(velocity.Scaled_00416780(travel));
 if(object->bones->position.z<1.3)object->bones->position.z=1.3f;
 ((MotionOwner*)owner)->dirty=1;
 if(next)next->Render(pass);
}
