/* One canonical release body; consumers choose historical inline emission.
 * Shell sees the independently linked external provider, as in native calls. */
inline void ObservedString::Release() {
  if (Header()!=observed_empty_header) {
   if (InterlockedDecrement(&Header()->refs)<=0) ::operator delete(Header());
   Init();
  }
 }
