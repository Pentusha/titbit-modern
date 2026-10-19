// Existing LaneString inline bodies exposed under their original callable ABI.
#include "../lanes/include/lane_string.h"
int LaneString::Allocate_00402120(int n) {return AllocInline(n);}
void LaneString::Release_00402180() {ReleaseInline();}
