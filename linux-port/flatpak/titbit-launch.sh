#!/bin/sh
# Flatpak entry point: imports the game data on first start, then runs the
# 32-bit game through the i386 compat runtime. --import / --studio open the tools.
set -eu
data=${TITBIT_GAME:-${XDG_DATA_HOME:-$HOME/.local/share}/titbit/game}
export TITBIT_GAME="$data"
# The import wizard's "Запустить игру" button starts the game through this script.
export TITBIT_LINUX_GAME=/app/bin/titbit-game
case "${1:-}" in
    --import) shift; exec /app/bin/titbit-tools setup "$@" ;;
    --studio) shift; exec /app/bin/titbit-tools studio --archive "$data/QUEST.ezd" "$@" ;;
esac
if [ ! -f "$data/QUEST.ezd" ] && [ ! -f "$data/quest.ezd" ]; then
    exec /app/bin/titbit-tools setup
fi
exec /app/bin/titbit-game "$@"
