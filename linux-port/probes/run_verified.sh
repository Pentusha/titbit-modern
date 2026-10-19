#!/bin/sh
# Native source-only ABI/EH checks. No SDK, game files, Wine, or display needed.
set -eu
ulimit -c 0
cd "$(dirname "$0")"
mkdir -p out
CXX=${CXX:-clang++-18}
CC=${CC:-gcc}
OPT_LEVEL=${OPT_LEVEL:-0}
MSFLAGS="--target=i386-pc-windows-msvc -O$OPT_LEVEL -fms-extensions -fno-rtti -fexceptions"
ELFFLAGS='--target=i386-pc-windows-msvc-elf -Wno-override-module'

$CXX --target=i386-pc-windows-msvc -fms-extensions -fno-rtti -fno-exceptions \
    -S -emit-llvm msabi_probe.cpp -o out/verified-layout.ll
python3 retarget_ir.py out/verified-layout.ll out/verified-layout-safe.ll
$CXX $ELFFLAGS -c out/verified-layout-safe.ll -o out/verified-layout.o
as --32 probe_start.S -o out/verified-start.o
ld -m elf_i386 -o out/verified-layout out/verified-start.o out/verified-layout.o
timeout 10s ./out/verified-layout

$CXX -m32 -c ../platform/ms_exception.cpp -o out/verified-runtime.o
$CC -m32 -c eh_main.c -o out/verified-main.o
$CC -m32 -c nested_eh_main.c -o out/verified-nested-main.o
for name in msabi_eh msabi_nested_eh; do
    $CXX $MSFLAGS -S -emit-llvm "$name.cpp" -o "out/verified-$name.ll"
    python3 ms_eh_to_dwarf.py "out/verified-$name.ll" "out/verified-$name-dwarf.ll"
    opt-18 -passes=verify -disable-output "out/verified-$name-dwarf.ll"
    python3 retarget_ir.py "out/verified-$name-dwarf.ll" "out/verified-$name-safe.ll"
    $CXX $ELFFLAGS -c "out/verified-$name-safe.ll" -o "out/verified-$name.o"
done
TYPEINFO='-Wl,--defsym=titbit_ms_3f3f5f37747970655f696e666f4040364240=titbit_type_info_vtable'
$CXX -m32 -no-pie out/verified-main.o out/verified-msabi_eh.o out/verified-runtime.o \
    "$TYPEINFO" -o out/verified-eh
$CXX -m32 -no-pie out/verified-nested-main.o out/verified-msabi_nested_eh.o out/verified-runtime.o \
    "$TYPEINFO" -o out/verified-nested-eh
timeout 10s ./out/verified-eh
timeout 10s ./out/verified-nested-eh

$CXX --target=i386-pc-windows-msvc -O"$OPT_LEVEL" -fms-extensions -fexceptions \
    -S -emit-llvm msabi_rtti.cpp -o out/verified-rtti.ll
python3 ms_eh_to_dwarf.py out/verified-rtti.ll out/verified-rtti-dwarf.ll
opt-18 -passes=verify -disable-output out/verified-rtti-dwarf.ll
python3 retarget_ir.py out/verified-rtti-dwarf.ll out/verified-rtti-safe.ll
$CXX $ELFFLAGS -c out/verified-rtti-safe.ll -o out/verified-rtti.o
$CXX -m32 -c ../platform/ms_rtti.cpp -o out/verified-rtti-native.o
$CC -m32 -c rtti_main.c -o out/verified-rtti-main.o
$CXX -m32 -no-pie out/verified-rtti-main.o out/verified-rtti.o \
    out/verified-rtti-native.o out/verified-runtime.o "$TYPEINFO" -o out/verified-rtti
timeout 10s ./out/verified-rtti

$CXX $MSFLAGS -S -emit-llvm msabi_alignment.cpp -o out/verified-alignment.ll
python3 retarget_ir.py out/verified-alignment.ll out/verified-alignment-safe.ll
$CXX $ELFFLAGS -c out/verified-alignment-safe.ll -o out/verified-alignment.o
as --32 alignment_helpers.S -o out/verified-alignment-helpers.o
$CC -m32 -c alignment_main.c -o out/verified-alignment-main.o
$CXX -m32 -no-pie out/verified-alignment-main.o out/verified-alignment.o \
    out/verified-alignment-helpers.o \
    -Wl,--defsym=titbit_ms_5c30315f616c69676e5f73746463616c6c403132=native_align_stdcall \
    -Wl,--defsym=align_thiscall=native_align_thiscall -o out/verified-alignment
timeout 10s ./out/verified-alignment

$CXX $MSFLAGS -S -emit-llvm msabi_inalloca.cpp -o out/verified-inalloca.ll
python3 ms_eh_to_dwarf.py out/verified-inalloca.ll out/verified-inalloca-dwarf.ll
python3 retarget_ir.py out/verified-inalloca-dwarf.ll out/verified-inalloca-safe.ll
$CXX $ELFFLAGS -c out/verified-inalloca-safe.ll -o out/verified-inalloca.o
$CC -m32 -c inalloca_main.c -o out/verified-inalloca-main.o
$CXX -m32 -no-pie out/verified-inalloca-main.o out/verified-inalloca.o \
    out/verified-runtime.o "$TYPEINFO" -o out/verified-inalloca
timeout 10s ./out/verified-inalloca
