#!/bin/sh
# Minimal static 32-bit FFmpeg for the game's videos: AVI demuxer, MP42 (msmpeg4v2)
# and CRAM (msvideo1) decoders, swscale. Lets build_linux.py --ffmpeg link it in,
# instead of the system FFmpeg and its ~140 dependent libraries.
#   sh tools/build_ffmpeg_min.sh [prefix]     (default: ~/.cache/titbit/ffmpeg-min)
set -e
PREFIX=$(realpath -m "${1:-${XDG_CACHE_HOME:-$HOME/.cache}/titbit/ffmpeg-min}")
SRC=$PREFIX/src
mkdir -p "$PREFIX"
[ -d "$SRC" ] || git clone -q --depth 1 --branch n6.1.1 https://git.ffmpeg.org/ffmpeg.git "$SRC"
cd "$SRC"
./configure --prefix="$PREFIX" --arch=x86 --target-os=linux --cc="gcc -m32" --extra-cflags="-O2 -fPIC" \
  --enable-static --disable-shared --disable-everything --disable-programs --disable-doc --disable-network \
  --disable-autodetect --disable-x86asm --disable-avdevice --disable-avfilter --disable-postproc \
  --enable-demuxer=avi --enable-decoder=msmpeg4v2,msvideo1 --enable-protocol=file \
  --enable-swscale --enable-swresample --enable-pic
make -j"$(nproc)"
make install
echo "Minimal FFmpeg: $PREFIX  ->  python3 tools/build_linux.py --ffmpeg $PREFIX"
