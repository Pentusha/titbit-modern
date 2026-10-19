#ifndef TITBIT_SCENE_SPATIAL_H
#define TITBIT_SCENE_SPATIAL_H
/* Authored observed field/ABI views. Complete original class identity and
 * extent are unproved. This declaration allocates no Scene/global storage.
 * Destructor declarations stay unresolved until complete native EH proof. */
void __cdecl operator delete(void *);
struct ObservedSpatialVector {float x,y,z;ObservedSpatialVector(){x=y=z=0;}ObservedSpatialVector &operator=(const ObservedSpatialVector &v){x=v.x;y=v.y;z=v.z;return *this;}ObservedSpatialVector(float a,float b,float c):x(a),y(b),z(c){}
 ObservedSpatialVector operator+(const ObservedSpatialVector &v)const{ObservedSpatialVector r;r.x=x+v.x;r.y=y+v.y;r.z=z+v.z;return r;}
 ObservedSpatialVector operator-(const ObservedSpatialVector &v)const{ObservedSpatialVector r;r.x=x-v.x;r.y=y-v.y;r.z=z-v.z;return r;}};
struct ObservedSpatialNode {
 unsigned first,second;bool split;ObservedSpatialNode *children[8];ObservedSpatialVector center;
 ObservedSpatialNode(){split=false;first=0;second=0;for(int i=0;i<8;++i)children[i]=0;}
 void Clear();
};
struct ObservedSpatialObject;
#include "pointer_array.h"
class ObservedSpatialBase {
public:
 virtual void Bounds(void *,ObservedSpatialVector *,ObservedSpatialVector *);
 int count,depth;ObservedPointerArray<ObservedSpatialObject *> storage;ObservedSpatialNode root;
 ObservedSpatialBase(){count=0;depth=6;}
 ~ObservedSpatialBase();
};
class ObservedSceneSpatial:public ObservedSpatialBase {
public:ObservedSceneSpatial();~ObservedSceneSpatial();virtual void Bounds(void *,ObservedSpatialVector *,ObservedSpatialVector *);
};
struct ObservedSpatialMesh {
 unsigned char reserved0[0x2c];float x,y,z;unsigned char reserved38[0x44-0x38];
 ObservedSpatialVector extent;unsigned char reserved50[0x94-0x50];float matrix[16];
};
struct ObservedSpatialObject {unsigned char reserved0[0x34];void *shape;ObservedSpatialMesh *mesh;};
// Source view sizes only; these are not native allocation/complete-owner proofs.
typedef char SpatialOwnerViewMustBe84[(sizeof(ObservedSceneSpatial)==84)?1:-1];
typedef char SpatialNodeViewMustBe56[(sizeof(ObservedSpatialNode)==56)?1:-1];

#endif
