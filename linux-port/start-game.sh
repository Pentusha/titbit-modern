#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
# Use the WSLg X11 path exercised by the native renderer checks. A caller can
# still select another SDL driver explicitly; ordinary Linux uses SDL defaults.
if [ -n "${WSL_DISTRO_NAME:-}" ] && [ -n "${DISPLAY:-}" ] && [ -z "${SDL_VIDEODRIVER:-}" ]; then
    SDL_VIDEODRIVER=x11
    export SDL_VIDEODRIVER
fi
executable="$root/titbit"
if [ ! -x "$executable" ]; then
    executable="$root/build/titbit"
fi
if [ ! -x "$executable" ]; then
    printf '%s\n' 'Build the Linux executable first: see docs/BUILD-LINUX.md.' >&2
    exit 1
fi
if [ "$#" -eq 0 ]; then
    data=${TITBIT_GAME:-${XDG_DATA_HOME:-$HOME/.local/share}/titbit/game}
    if [ -z "${TITBIT_GAME:-}" ] && [ -d "$root/game" ]; then
        data="$root/game"
    fi
    if [ ! -f "$data/QUEST.ezd" ] && [ ! -f "$data/quest.ezd" ]; then
        printf '%s\n' "Game data not found in $data - opening the import wizard." >&2
        exec "$root/tools/import_assets.sh"
    fi
    set -- --data "$data"
fi
exec "$executable" "$@"
