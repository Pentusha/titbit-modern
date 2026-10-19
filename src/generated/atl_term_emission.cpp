// Build-only emission helper; excluded from reconstruction and LINK roots.
#include "../../src/native/app_module.h"
extern "C" void* AtlTermEmissionOnly(){return (void*)&ATL::AtlModuleTerm;}
