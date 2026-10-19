#ifndef TITBIT_OBSERVED_SCENE_OWNER_VIEW_H
#define TITBIT_OBSERVED_SCENE_OWNER_VIEW_H
#include "game_audio.h"
#include "scene_pair_array.h"
#include "scene_spatial.h"
#include "pointer_array.h"
/* Authored coherent owner view, not original type identity or complete size.
 * Opaque gaps are not source global storage or reconstructed child objects. */
class ObservedPickingNode;
struct ObservedPickingMessage;
struct ObservedPointNode;
struct ObservedVector3;
struct ObservedMatrixPoint;
class ObservedString;
class ObservedScene {
 unsigned reserved0;
 typedef ObservedPointerArray<ObservedPickingNode *> ListView;
 ListView lists[3];
 unsigned char reserved34[0xfe8-0x34];
 ScenePairArray pair_array;
 ObservedPickingMessage *message_data;
 int message_size,message_capacity,message_grow_by;
 ObservedSceneSpatial spatial_child;
 unsigned char reserved1060[0x1114-0x1060];
 void *selected_object;
 // Native child ctor/dtor and independent56-byte heap allocation corroborate this member.
 SoundFocusPrefix sound_child;
 ObservedPointNode *point_node;
public:
 void *&SelectedObjectSlot(){return selected_object;}
 bool Send(ObservedPickingMessage *);
 ObservedPickingNode *Resolve(ObservedPickingMessage *);
 ObservedPickingNode *Find(ObservedPickingNode *,const char *);
 void Drain();
 void Remove(void *);
 void RemoveAll(unsigned);
 void Load(ObservedString);
 void NormalizePlane(float planes[][4],int index);
 void SetPoint(ObservedVector3);
 bool SphereVisible(float,float,float,float);
 bool BoxVisible(const ObservedMatrixPoint &,ObservedMatrixPoint);
};
typedef ObservedScene ObservedPickingScene;
typedef ObservedScene ObservedSceneInterface;
extern ObservedScene picking_scene;
#endif
