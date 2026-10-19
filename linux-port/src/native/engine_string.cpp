#include "engine_string.h"
#define ARCHIVE_BUFFER_INLINE
#include "engine_string_buffer.inl"
#undef ARCHIVE_BUFFER_INLINE
ObservedString &ObservedString::Assign(const char *text) {
 AssignCopy(text==0?0:lstrlenA(text),text);
 return *this;
}
