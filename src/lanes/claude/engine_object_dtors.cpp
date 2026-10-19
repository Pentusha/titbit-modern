// Trivial base destructors, kept in their own unit: when the compiler can see
// an empty destructor it drops the EH state tracking its callers have.
#include "lane_engine.h"
Frame_0040b090::~Frame_0040b090() {}
Component_0040b0f0::~Component_0040b0f0() {}
