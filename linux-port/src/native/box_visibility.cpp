/* Authored method/data view; no scene owner allocation or complete sizeof. */
#include "geometry_interfaces.h"
bool ObservedSceneInterface::BoxVisible(const ObservedMatrixPoint &center,ObservedMatrixPoint halfExtents) {
 int i=0;
 float (*planes)[4]=(float (*)[4])((char*)this+0x1068);
 for(;i<6;++i) {
  if((center.x-halfExtents.x)*planes[i][0] + (center.y-halfExtents.y)*planes[i][1] + (center.z-halfExtents.z)*planes[i][2]+planes[i][3]>0.0f)continue;
  if((center.x+halfExtents.x)*planes[i][0] + (center.y-halfExtents.y)*planes[i][1] + (center.z-halfExtents.z)*planes[i][2]+planes[i][3]>0.0f)continue;
  if((center.x-halfExtents.x)*planes[i][0] + (center.y+halfExtents.y)*planes[i][1] + (center.z-halfExtents.z)*planes[i][2]+planes[i][3]>0.0f)continue;
  if((center.x+halfExtents.x)*planes[i][0] + (center.y+halfExtents.y)*planes[i][1] + (center.z-halfExtents.z)*planes[i][2]+planes[i][3]>0.0f)continue;
  if((center.x-halfExtents.x)*planes[i][0] + (center.y-halfExtents.y)*planes[i][1] + (center.z+halfExtents.z)*planes[i][2]+planes[i][3]>0.0f)continue;
  if((center.x+halfExtents.x)*planes[i][0] + (center.y-halfExtents.y)*planes[i][1] + (center.z+halfExtents.z)*planes[i][2]+planes[i][3]>0.0f)continue;
  if((center.x-halfExtents.x)*planes[i][0] + (center.y+halfExtents.y)*planes[i][1] + (center.z+halfExtents.z)*planes[i][2]+planes[i][3]>0.0f)continue;
  if((center.x+halfExtents.x)*planes[i][0] + (center.y+halfExtents.y)*planes[i][1] + (center.z+halfExtents.z)*planes[i][2]+planes[i][3]>0.0f)continue;
  return false;
 }
 return true;
}
