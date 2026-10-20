# Portable source ABI

Common compatibility code for the Windows and Linux source builds. VC6 headers,
runtime libraries and compiler binaries are not normal build inputs.

| Directory | Purpose |
|---|---|
| `include/crt` | C declarations and the fixed 32-bit source-facing file interface |
| `include/cxx` | The string/container/exception/stream subset used by the restored sources |
| `include/win32` | Small ATL implementation and adapters around open mingw-w64 declarations |
| `src/windows_runtime.cpp` | Allocation and exception support backed by current UCRT/VCRuntime |
| `src/container_smoke.cpp` | Ownership, aliasing, nested containers and layout checks |
| `src/win32_abi.cpp` | Win32 structure sizes and field offsets used by the game |

This is a project-specific compatibility surface, not a complete STL or ATL SDK.
Its implementations are independently authored; Microsoft SDK implementation
sources are not embedded. System Win32 declarations come from mingw-w64.

The engine still uses four-byte pointers and Microsoft object layout. In particular,
the source-facing string and vector owners are 16 bytes, deque is 48 bytes, the
XML attribute view is 84 bytes, the application module is 144 bytes and the legacy
DEVMODEA view is 148 bytes. These contracts must not silently change with the host
compiler's standard library. Native platform code uses the host runtime separately.

Linux compiles Microsoft-layout LLVM IR and translates exception handling to DWARF
before emitting ELF32. Windows emits COFF directly and links current MSVC/UCRT
libraries. The Linux export carries this directory inside its own root; a full
Git checkout keeps one shared copy here.

See [Windows build](../docs/BUILD.md) and
[Linux build](../linux-port/docs/BUILD-LINUX.md). Runtime checks require no game
assets: `python3 tools/check_cpp_runtime.py` from `linux-port/`, and
`python tools/check_windows_runtime.py` from the repository root on Windows.

An explicit `--vc98` Linux comparison mode and the Windows legacy build scripts
remain available for historical verification. Normal builds ignore `TITBIT_VC6`.
