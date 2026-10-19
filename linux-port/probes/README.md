# Native ELF32 compiler bridge

The restored game contains four-byte pointers, explicit field offsets, Microsoft
vtable slots, and explicit `__thiscall` callbacks. A normal Linux C++ compilation
does not preserve all those contracts.

The working compiler path is:

1. Clang 18 compiles C/C++ source to Microsoft-ABI LLVM IR.
2. `ms_eh_to_dwarf.py` translates C++ EH funclets to DWARF landing pads, retaining
   typed catches, destructors, nested catches, and rethrows.
3. `retarget_ir.py` encodes Microsoft symbol names and removes Windows linker
   directives. The final linker must select largest original COMDAT providers
   using the build report, since ELF supports only `Any`/`NoDeduplicate` groups.
4. Clang's `i386-pc-windows-msvc-elf` backend generates **ELF32** directly while
   retaining Microsoft record layout. The native Linux exception adapter is
   compiled separately with the GNU C++ ABI and linked against `libstdc++`.

Two important traps are covered by the probes:

- `i386-linux-gnu` backend changes alignment of `double` fields. A Microsoft
  `struct { char; double; int; }` would silently change its field offsets.
- Microsoft `?method@Class@@...` symbols cannot be passed unchanged to GNU `ld`:
  `@` denotes ELF symbol versions. Without reversible encoding, Base/Derived
  virtual methods can silently resolve to the same implementation.

This does not use the original EXE or run PE files through Wine. It also does not
make Windows system calls portable: those calls still need native implementations.

## Verify without game files or a Windows SDK

Requires Clang/LLVM 18 and a native 32-bit C/C++ development runtime:

```sh
sh linux-port/probes/run_verified.sh
OPT_LEVEL=2 sh linux-port/probes/run_verified.sh
```

The tests execute genuine ELF32 binaries. They check record offsets, vtable
overrides, `__stdcall`, `__thiscall`, integer exceptions, nested RAII cleanup,
rethrow, propagation past an unmatched catch in another function, catch by value,
and throwing an lvalue. Outputs and compiler intermediates stay under `out/`.

The RTTI probe checks downcasts, multiple-inheritance crosscasts, repeated and
virtual bases, nonzero virtual-vfptr adjustment, private and ambiguous failures,
reference-cast `bad_cast` handling, `dynamic_cast<void*>`, and dynamic `typeid`.

The mixed-call probe checks the actual stack at native cdecl, stdcall, and
thiscall helper entry: `(ESP + 4) % 16` must be zero. It repeats all checks after
an assembly wrapper deliberately misaligns entry into the Microsoft-ABI caller.
The compiler bridge injects Clang's `override-stack-alignment=16` module flag
and `stackrealign` function attributes while retaining Microsoft record layout.

With the same external VC98 SDK used by the game build, verify legacy stdarg
adaptation and native formatting too:

```sh
python3 linux-port/probes/test_sdk_varargs.py --vc98 /path/to/VC98
```

This checks LLVM `va_start`, mixed integer/double/pointer/64-bit arguments,
native `vsnprintf`, and SeeR's address-of-`va_arg` requirement. The generated SDK
header copies stay in the ignored build directory; the SDK is not redistributed.

The by-value probe also verifies Microsoft `inalloca` calls and hidden return
storage for nontrivial classes: copy/destruction counts, `__thiscall`, small and
12-byte results, and mixed object/double arguments crossing native stack checks.

## Native game regression

After building, with an imported copy of the game's data:

```sh
cd linux-port
python3 tools/headless_probe.py --data /path/to/game-data --frames 9000 \
  --timeout 90 --out build/headless-street --actions probes/street-actions.json --gdb
```

Actions in `street-actions.json` wait for the engine's `Scene loaded` marker
and match the actual XML scene (`whenScene`), rather than assuming a wall-clock
deadline identifies the current screen. `--require-scene sumerki_entrance.XML`
adds an explicit scene requirement to the final report. A cold startup that
has not reached the street fails rather than passing on exit status alone.

The test creates its own private Xvfb display, skips publisher videos, selects
New Game, and skips the introductory cutscene. Check the final frame for the
rendered `sumerki_entrance.XML` street. The first verified run reached this scene
and exited normally after 9,000 game frames (67.42 seconds).

One source correction is required for Clang: `CreateStream` allocates a single
56-byte sound object, so `DeleteStream` uses scalar `delete`. Original function
`0042a060` passes flags=3 to deleting destructor `00429990`, which checks only
bit 0 and destroys one object. Translating this into Clang's `delete[]` reads a
nonexistent array cookie and caused the first scene-transition crash.

Mouse actions deliberately wait after motion before pressing the button. The
game selects the hovered object during its frame update; batching motion and
press into one event iteration can click the previously hovered menu item.

For the longer movement/save/load regression, use a test data directory without
existing saves; this scenario writes slot 1:

```sh
python3 tools/headless_probe.py --data /path/to/test-game-data --frames 20000 \
  --timeout 240 --out build/headless-gameplay --actions probes/gameplay-actions.json --gdb
```

Verified in one native process: rendered skeletal walking on the street, Escape
menu, Save Game, first slot, the slot's timestamp/location thumbnail, Load Game,
and the restored street's scripted dialogue. The run completed 20,000 frames in
167.93 seconds and exited with status 0. The 408-byte `slot1.sav` retains the
original QuestState layout, including location 17 and previous location 9.
Street movement is controlled by the original scene scripts; an arbitrary
ground click is not a general navigation command in this location.
