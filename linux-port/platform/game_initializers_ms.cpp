// The restored Windows source stores these objects as POD and supplies explicit
// constructor callbacks. ELF runs compiler-emitted C++ constructors itself, but
// does not consume the historical .CRT$XCU table. Call only its missing POD
// callbacks, in their original relative order. See docs/INITIALIZERS.md.
extern "C" {
void Script_CXX_Initializer_00406970();
void Script_CXX_Initializer_00413f90();
void FUN_00421580();
void FUN_004215d0();
void FUN_00427d70();
void FUN_004298d0();
}
void InitializeTextureCache();
void InitializeResourceCache();

extern "C" void titbit_linux_initialize_pod_globals() {
    static bool initialized;
    if (initialized) return;
    initialized = true;
    Script_CXX_Initializer_00406970();
    Script_CXX_Initializer_00413f90();
    InitializeTextureCache();
    InitializeResourceCache();
    FUN_00421580();
    FUN_004215d0();
    FUN_00427d70();
    FUN_004298d0();
}
