#!@shell@
set -eu

run_engine() {
    # Desktop sessions export native GTK/GIO plugins. The i386 engine (also
    # under QEMU) must not load these 64-bit shared objects. Keep the native
    # setup wizard's environment intact by cleaning only the engine launch.
    unset GIO_EXTRA_MODULES GIO_MODULE_DIR GDK_PIXBUF_MODULE_FILE GDK_PIXBUF_MODULEDIR
    unset GTK_PATH GTK_MODULES GTK_IM_MODULE_FILE

    # SDL2 compatibility uses SDL3's Wayland-first selection. Prefer X11 for
    # this legacy OpenGL engine when XWayland is available; honor overrides
    # and leave Wayland-only sessions to SDL's normal backend selection.
    if [ -n "${DISPLAY:-}" ] && [ -z "${SDL_VIDEODRIVER:-}" ]; then
        export SDL_VIDEODRIVER=x11
    fi
    exec @engine@ "$@"
}

# Explicit arguments retain the engine's own --data and --help behaviour.
if [ "$#" -gt 0 ]; then
    run_engine "$@"
fi

data=${TITBIT_GAME:-${XDG_DATA_HOME:-$HOME/.local/share}/titbit/game}
if [ ! -f "$data/QUEST.ezd" ] && [ ! -f "$data/quest.ezd" ]; then
    printf '%s\n' "Game data not found in $data. Opening the import wizard." >&2
    exec @setup@
fi

run_engine --data "$data"
