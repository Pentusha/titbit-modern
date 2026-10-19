#!/bin/sh
# No installation or game launch. All generated artifacts stay under probes/out.
set -u
ulimit -c 0
cd "$(dirname "$0")"
mkdir -p out
CLANG=${CLANG:-clang++-18}

"$CLANG" -m32 -fms-extensions -fc++-abi=microsoft -fno-rtti -fno-exceptions \
    -c msabi_probe.cpp -o out/direct-msabi.o >out/direct-msabi.log 2>&1
echo "direct ELF Microsoft ABI compile: $?"

"$CLANG" --target=i386-pc-windows-msvc -fms-extensions -fno-rtti -fno-exceptions \
    -S -emit-llvm msabi_probe.cpp -o out/windows.ll >out/windows.log 2>&1
echo "MS-target IR compile: $?"
if test -f out/windows.ll; then
    "$CLANG" --target=i386-linux-gnu -Wno-override-module -c out/windows.ll \
        -o out/retarget.o >out/retarget.log 2>&1
    echo "IR to ELF retarget: $?"
    file out/retarget.o 2>/dev/null || true
fi

"$CLANG" -m32 -fms-extensions -fc++-abi=microsoft -fexceptions \
    -S -emit-llvm msabi_eh.cpp -o out/direct-eh.ll >out/direct-eh.log 2>&1
echo "direct ELF Microsoft EH compile: $?"
"$CLANG" --target=i386-pc-windows-msvc -fms-extensions -fexceptions \
    -S -emit-llvm msabi_eh.cpp -o out/windows-eh.ll >out/windows-eh.log 2>&1
echo "MS-target EH IR compile: $?"
if test -f out/windows-eh.ll; then
    "$CLANG" --target=i386-linux-gnu -Wno-override-module -c out/windows-eh.ll \
        -o out/retarget-eh.o >out/retarget-eh.log 2>&1
    echo "EH IR to ELF retarget: $?"
fi
"$CLANG" --target=i386-pc-windows-msvc -fms-extensions -fexceptions -fdwarf-exceptions \
    -S -emit-llvm msabi_eh.cpp -o out/windows-dwarf-eh.ll >out/windows-dwarf-eh.log 2>&1
echo "MS-target Dwarf EH IR compile: $?"
if test -f out/windows-dwarf-eh.ll; then
    "$CLANG" --target=i386-linux-gnu -Wno-override-module -c out/windows-dwarf-eh.ll \
        -o out/retarget-dwarf-eh.o >out/retarget-dwarf-eh.log 2>&1
    echo "Dwarf EH IR to ELF retarget: $?"
fi
"$CLANG" --target=i386-w64-windows-gnu -fc++-abi=microsoft -fms-extensions -fexceptions -fdwarf-exceptions \
    -S -emit-llvm msabi_eh.cpp -o out/mingw-dwarf-eh.ll >out/mingw-dwarf-eh.log 2>&1
echo "MinGW MSABI Dwarf EH IR compile: $?"
cat out/*.log
