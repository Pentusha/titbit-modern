// Behaviour constructor (0x406a40). Its original translation unit was compiled
// with VC6.0 (build 8168), not 6.5 like its neighbours: byte-exact only there.
#include "lane_engine.h"
class Behaviour_00406a40 : public Component_0040b0f0 {
public:
 int fieldC, field10; float rate; int field18;
 Behaviour_00406a40(int ownerObject);
 virtual ~Behaviour_00406a40();
#ifdef TITBIT_SOURCE_LINK
 virtual ObservedString Export();
 virtual void Render(int);
#endif
};
Behaviour_00406a40::Behaviour_00406a40(int ownerObject) {
 owner = ownerObject; field18 = 0; field10 = 0; rate = 0.06f;
}
