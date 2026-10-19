#!/bin/sh
# Minimal static 32-bit FFmpeg for the game's videos: AVI demuxer, MP42 (msmpeg4v2)
# and CRAM (msvideo1) decoders, swscale. Lets build_linux.py --ffmpeg link it in,
# instead of the system FFmpeg and its ~140 dependent libraries.
#   sh tools/build_ffmpeg_min.sh [prefix]     (default: ~/.cache/titbit/ffmpeg-min)
set -e
PREFIX=$(realpath -m "${1:-${XDG_CACHE_HOME:-$HOME/.cache}/titbit/ffmpeg-min}")
SRC=$PREFIX/src
mkdir -p "$PREFIX"
fetch() {  # url sha256 dir: download once, check the pinned checksum, unpack
  [ -d "$3" ] && return 0
  file=$PREFIX/$(basename "$1")
  if [ ! -f "$file" ]; then
    curl -fL --retry 3 -o "$file.part" "$1"
    mv "$file.part" "$file"
  fi
  if ! echo "$2  $file" | sha256sum -c --status -; then
    echo "SHA-256 mismatch, deleted: $file" >&2; rm -f "$file"; exit 1
  fi
  rm -rf "$3.part" && mkdir -p "$3.part"
  tar -xf "$file" -C "$3.part" --strip-components=1
  mv "$3.part" "$3"
}
fetch https://ffmpeg.org/releases/ffmpeg-6.1.1.tar.xz \
  8684f4b00f94b85461884c3719382f1261f0d9eb3d59640a1f4ac0873616f968 "$SRC"
cd "$SRC"
./configure --prefix="$PREFIX" --arch=x86 --target-os=linux --cc="gcc -m32" --extra-cflags="-O2 -fPIC" \
  --enable-static --disable-shared --disable-everything --disable-programs --disable-doc --disable-network \
  --disable-autodetect --disable-x86asm --disable-avdevice --disable-avfilter --disable-postproc \
  --enable-demuxer=avi --enable-decoder=msmpeg4v2,msvideo1 --enable-protocol=file \
  --enable-swscale --enable-swresample --enable-pic
make -j"$(nproc)"
make install
echo "Minimal FFmpeg: $PREFIX  ->  python3 tools/build_linux.py --ffmpeg $PREFIX"
