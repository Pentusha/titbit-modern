#include "scene_spatial.h"
void ObservedSpatialNode::Clear(){first=0;second=0;if(split){for(int i=0;i<8;++i){ObservedSpatialNode *p=children[i];if(p){p->Clear();delete p;}children[i]=0;}}split=false;}
ObservedSceneSpatial::ObservedSceneSpatial(){}
void ObservedSpatialBase::Bounds(void *,ObservedSpatialVector *,ObservedSpatialVector *){}
void ObservedSceneSpatial::Bounds(void *element,ObservedSpatialVector *maximum,ObservedSpatialVector *minimum){
 ObservedSpatialObject *&object=*(ObservedSpatialObject**)element;
 if(object->shape){
  ObservedSpatialMesh *p=object->mesh;
  float x=p->x*p->matrix[0]+(p->y*p->matrix[4]+p->z*p->matrix[8])+p->matrix[12];
  float y=(p->y*p->matrix[5]+p->x*p->matrix[1])+p->z*p->matrix[9]+p->matrix[13];
  float z=(p->y*p->matrix[6]+p->x*p->matrix[2])+p->z*p->matrix[10]+p->matrix[14];
  ObservedSpatialVector position(x,y,z);
  *maximum=position+object->mesh->extent;
  *minimum=position-object->mesh->extent;
 }
}
