/* The variadic native wrapper logs its first string and legacy LF/CR ending. */
extern "C" __declspec(dllimport) void __stdcall OutputDebugStringA(const char *);
void __cdecl PickingTrace(const char *text,...) {
 OutputDebugStringA(text);
 OutputDebugStringA("\n\r");
}
