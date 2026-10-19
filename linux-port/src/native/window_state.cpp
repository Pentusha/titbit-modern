/* Source-owned scalar/keyboard storage with independently bound addresses.
 * Picking and Scene objects remain external observed prefixes, not full objects. */
bool gameActive=true;
unsigned char keyStates[256]={0};
int displayWidth=0,displayHeight=0;
float gameSpeed=0;
extern "C" void __cdecl SetGameSpeed(float value){gameSpeed=value;}
