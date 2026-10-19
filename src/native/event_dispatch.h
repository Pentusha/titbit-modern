#ifndef TITBIT_EVENT_DISPATCH_H
#define TITBIT_EVENT_DISPATCH_H
#include "picking_control_interfaces.h"
/* Only the observed prefix and thiscall interface, not a complete class. */
class ObservedEventScript {
 unsigned prefix[2];
 void *instance;
 int handlers[13];
public:
 void Dispatch(ObservedPickingMessage *);
};
#endif
