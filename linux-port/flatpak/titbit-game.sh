#!/bin/sh
# Runs the ELF32 game with the 32-bit loader and libraries of the
# org.freedesktop.Platform.Compat.i386 and GL32 extensions.
set -eu
i386=/app/lib/i386-linux-gnu
gl=
for dir in "$i386"/GL/*/lib; do [ -d "$dir" ] && gl="${gl:+$gl:}$dir"; done
export LIBGL_DRIVERS_PATH="${gl:+$(printf '%s' "$gl" | sed 's#/lib\(:\|$\)#/lib/dri\1#g')}"
data=${TITBIT_GAME:-${XDG_DATA_HOME:-$HOME/.local/share}/titbit/game}
[ "$#" -gt 0 ] || set -- --data "$data"
exec "$i386/ld-linux.so.2" --library-path "${gl:+$gl:}$i386" /app/lib/titbit/titbit "$@"
