#!@shell@
set -eu

data=${TITBIT_GAME:-${XDG_DATA_HOME:-$HOME/.local/share}/titbit/game}
args=()
while [ "$#" -gt 0 ]; do
  case "$1" in
    -h|--help) exec @game@ --help ;;
    --data)
      if [ "$#" -lt 2 ]; then echo '--data requires a directory' >&2; exit 2; fi
      data=$2
      shift 2
      ;;
    --data=*) data=${1#--data=}; shift ;;
    *) args+=("$1"); shift ;;
  esac
done

if [ -z "$data" ]; then echo '--data requires a nonempty directory' >&2; exit 2; fi

if [ ! -f "$data/QUEST.ezd" ] && [ ! -f "$data/quest.ezd" ]; then
  # The importer stages and verifies every file before committing it. Saves
  # and unrelated files survive; a changed existing archive is never replaced.
  @setup@ --import @data@ --dest "$data" > /dev/null
fi
exec @game@ --data "$data" "${args[@]}"
