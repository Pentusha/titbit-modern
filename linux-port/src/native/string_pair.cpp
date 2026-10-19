/* Observed two-handle cleanup; this unit has no C++ exception-unwind wrapper. */
#include "engine_string.h"
ObservedStringPair::~ObservedStringPair() {}
