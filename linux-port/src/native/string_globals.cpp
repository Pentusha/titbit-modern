/* Independently bound source storage for the shared empty string only.
 * The trailing three bytes are compiler struct padding, not a larger string. */
#include "engine_string.h"
struct ObservedEmptyStringStorage {ObservedStringHeader header;char text[1];};
extern ObservedEmptyStringStorage observed_nil_storage;
char *observed_empty_string=observed_nil_storage.text;
ObservedStringHeader *observed_empty_header=&observed_nil_storage.header;
#pragma data_seg(".strnil")
ObservedEmptyStringStorage observed_nil_storage={{-1,0,0},{0}};
#pragma data_seg()
