/* Native 0x5ef6a4: DWORD read/write by DecodeJpegMemory, static storage.
 * The frontend resets it to one for each call and returns zero on longjmp. */
int jpeg_decode_status = 0;

/* One-byte guard shared by the source-built library initializer prefixes.
 * This is observed byte storage, not a substitute for any constructor. */
/* The actual template COMDAT guard is emitted by the C++ units. The original
 * build binds this external view to that one shared byte, avoiding a duplicate. */
extern unsigned char iostream_setup_guard;
