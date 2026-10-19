/* Single authoritative buffer bodies, copied without semantic changes from
 * canonical engine_string.cpp. ARCHIVE_BUFFER_INLINE is an AFX_INLINE-style
 * build policy: empty in the external provider TU, inline in FormatV TU.
 * This controls historical compiler emission only; it does not change bodies.
 */
#ifndef ARCHIVE_BUFFER_INLINE
#error Define ARCHIVE_BUFFER_INLINE before including buffer implementation
#endif
ARCHIVE_BUFFER_INLINE char *ObservedString::GetBuffer(int minimum) {
 if (Header()->refs>1 || minimum>Header()->capacity) {
  ObservedStringHeader *old_header=Header();
  int old_length=Header()->length;
  if (minimum<old_length) minimum=old_length;
  if (!Allocate(minimum)) return data;
  memcpy(data,(char *)(old_header+1),old_length+1);
  Header()->length=old_length;
  if (old_header!=observed_empty_header && InterlockedDecrement(&old_header->refs)<=0)
   ::operator delete(old_header);
 }
 return data;
}

ARCHIVE_BUFFER_INLINE void ObservedString::ReleaseBuffer(int length) {
 if (Header()->refs>1) {
  ObservedStringHeader *old_header=Header();
  Release();
  int old_length=old_header->length;
  if (!old_length) data=observed_empty_string;
  else {
   ObservedStringHeader *p=(ObservedStringHeader *)::operator new(old_length+13);
   if (!p) goto finish;
   p->refs=1;((char *)(p+1))[old_length]=0;
   p->length=old_length;p->capacity=old_length;data=(char *)(p+1);
  }
  memcpy(data,(char *)(old_header+1),old_header->length+1);
 }
finish:
 if (length==-1) length=lstrlenA(data);
 Header()->length=length;
 data[length]=0;
}
