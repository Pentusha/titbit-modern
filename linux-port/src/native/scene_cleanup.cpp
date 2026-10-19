#include "picking_control_interfaces.h"
extern "C" int __cdecl strcmp(const char *,const char *);
#pragma intrinsic(strcmp)
#include "scene_cleanup_resources.h"
void ObservedScene::RemoveAll(unsigned flags){
 ObservedPickingMessage message;memset(&message,0,sizeof(message));message.event=4;
 if(flags&2){for(int i=0;i<lists[0].size;++i){message.selected=lists[0].data[i];Send(&message);}spatial_child.storage.SetSize(0);spatial_child.root.Clear();}
 if(flags&1){for(int i=0;i<lists[1].size;++i){message.selected=lists[1].data[i];Send(&message);}}
 if(flags&4){for(int i=0;i<lists[2].size;++i){if(strcmp(lists[2].data[i]->name,"mainmenu")!=0 &&
      strcmp(lists[2].data[i]->name,"cursor")!=0 &&
      strcmp(lists[2].data[i]->name,"sound_back")!=0 &&
      strcmp(lists[2].data[i]->name,"newgame")!=0 &&
      strcmp(lists[2].data[i]->name,"settings")!=0 &&
      strcmp(lists[2].data[i]->name,"savegame")!=0 &&
      strcmp(lists[2].data[i]->name,"loadgame")!=0 &&
      strcmp(lists[2].data[i]->name,"help")!=0 &&
      strcmp(lists[2].data[i]->name,"credits")!=0 &&
      strcmp(lists[2].data[i]->name,"quit")!=0 &&
      strcmp(lists[2].data[i]->name,"items")!=0){message.selected=lists[2].data[i];Send(&message);}}}
 if(flags==255){
  if(observed_animation_data){ObservedAnimationRecord *p=observed_animation_data;int n=observed_animation_count;while(n--){p->~ObservedAnimationRecord();++p;}::operator delete(observed_animation_data);observed_animation_data=0;}
  observed_animation_count=0;observed_resources.Clear();observed_textures.Clear();
  lists[2].SetSize(0);lists[1].SetSize(0);lists[0].RemoveAll();
  if(message_data){::operator delete(message_data);message_data=0;}message_size=message_capacity=0;point_node=0;
 }
}
