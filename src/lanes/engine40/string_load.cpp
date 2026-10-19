// WTL CString::LoadString (0x406520), out-of-line copy with GetBuffer and
// ReleaseBuffer expanded inline.
#include "e40_string_buf.h"

int ObservedString::LoadString(unsigned id) {
 char temporary[256];
 int count = sizeof(temporary) / sizeof(temporary[0]);
 int length = LoadText(id, temporary, count);
 if (count - length > 2) {
  AssignCopy(lstrlenA(temporary), temporary);
  return length > 0;
 }
 int size = 256;
 do {
  size += 256;
  length = LoadText(id, GetBufferExpanded(size - 1), size);
 } while (size - length <= 2);
 ReleaseBufferExpanded();
 return length > 0;
}
