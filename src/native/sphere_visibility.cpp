/* Proposed method-only addition to existing canonical scene interface.
 * Source identifiers authored; no global storage or complete sizeof claim. */
#include "geometry_interfaces.h"
bool ObservedSceneInterface::SphereVisible(float x,float y,float z,float radius) {
 radius=-radius;
 int i=0;
 float (*plane)[4]=(float (*)[4])((char*)this+0x1068);
 for(;i<6;++i,++plane) {
  if ((y*(*plane)[1]+z*(*plane)[2])+x*(*plane)[0]+(*plane)[3]<=radius)return false;
 }
 return true;
}
