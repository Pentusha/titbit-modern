#!/bin/sh
# Static 32-bit SDL2 that loads X11 / Wayland / PulseAudio / ALSA / PipeWire at run
# time (dlopen) instead of linking them: the game ELF then has no direct
# dependency on any of them and works with whatever the system provides.
#   sh tools/build_sdl2_min.sh [prefix]      (default: ~/.cache/titbit/sdl2-min; no spaces allowed)
set -e
PREFIX=$(realpath -m "${1:-${XDG_CACHE_HOME:-$HOME/.cache}/titbit/sdl2-min}")
case "$PREFIX" in *" "*) echo "SDL2 autotools cannot build in a path with spaces: $PREFIX" >&2; exit 1;; esac
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
fetch https://github.com/libsdl-org/SDL/releases/download/release-2.30.9/SDL2-2.30.9.tar.gz \
  24b574f71c87a763f50704bbb630cbe38298d544a1f890f099a4696b1d6beba4 "$SRC"
mkdir -p "$SRC/build32" && cd "$SRC/build32"
CC="gcc -m32" CFLAGS="-O2 -fPIC -ffile-prefix-map=$SRC=sdl2" PKG_CONFIG_LIBDIR=/usr/lib/i386-linux-gnu/pkgconfig:/usr/share/pkgconfig \
../configure --prefix="$PREFIX" --host=i686-pc-linux-gnu --enable-static --disable-shared \
  --enable-x11-shared --enable-wayland-shared --enable-pulseaudio-shared --enable-alsa-shared \
  --enable-pipewire-shared --enable-libdecor-shared --enable-video-opengl \
  --disable-video-vulkan --disable-joystick-mfi --disable-hidapi-libusb --disable-dbus --disable-ibus
make -j"$(nproc)"
make install
echo "Static SDL2: $PREFIX  ->  python3 tools/build_linux.py --sdl2 $PREFIX"
