#!@shell@
set -eu

# Explicit arguments retain the engine's own --data and --help behaviour.
if [ "$#" -gt 0 ]; then
    exec @engine@ "$@"
fi

data=${TITBIT_GAME:-${XDG_DATA_HOME:-$HOME/.local/share}/titbit/game}
if [ ! -f "$data/QUEST.ezd" ] && [ ! -f "$data/quest.ezd" ]; then
    printf '%s\n' "Game data not found in $data. Opening the import wizard." >&2
    exec @setup@
fi

exec @engine@ --data "$data"
