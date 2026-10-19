/* Shared helpers of the compat layer (not used by game sources). */
#ifndef TITBIT_COMPAT_INTERNAL_H
#define TITBIT_COMPAT_INTERNAL_H
const char *CompatSetting(const char *key, const char *fallback, char *buffer, unsigned size);
/* log=1: one line per event to the debugger and to titbit_compat.log in the current folder. */
void CompatLog(const char *module, const char *what, long value);
bool CompatSettingIs(const char *key, const char *fallback, const char *value);
/* No C runtime: the compat units must also link into the patched original
 * image (tools/build_exe.py --compat), which has no CRT import for them. */
inline void *CompatAlloc(unsigned size) { return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size); }
inline void CompatFree(void *p) { if (p) HeapFree(GetProcessHeap(), 0, p); }
#endif
