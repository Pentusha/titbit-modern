#!@shell@
set -eu
export PATH=@coreutils@:$PATH

data=${TITBIT_GAME:-${XDG_DATA_HOME:-$HOME/.local/share}/titbit/game}
args=()
while [ "$#" -gt 0 ]; do
  case "$1" in
    -h|--help)
      cat <<'HELP'
Usage: titbit [--data DIRECTORY] [ENGINE OPTIONS...]

Run the x86 Linux game in QEMU. Import game data with titbit-setup first.
TITBIT_GAME selects the writable data directory. QEMU_OPTS adds QEMU options.
TITBIT_VM_AUDIO selects the QEMU audio backend (coreaudio on macOS, pa on Linux;
use none to disable audio). Studio and the importer run natively on the host.
HELP
      exit 0
      ;;
    --data)
      if [ "$#" -lt 2 ]; then echo '--data requires a directory' >&2; exit 2; fi
      data=$2
      shift 2
      ;;
    --data=*) data=${1#--data=}; shift ;;
    *) args+=("$1"); shift ;;
  esac
done

if [ ! -f "$data/QUEST.ezd" ] && [ ! -f "$data/quest.ezd" ]; then
  printf 'Game data not found in %s; opening the import wizard.\n' "$data" >&2
  exec @setup@ --dest "$data"
fi

data=$(realpath "$data")
# QEMU's key=value option parser uses doubled commas for a literal comma.
export TITBIT_GAME=${data//,/,,}
state=$(mktemp -d "${TMPDIR:-/tmp}/titbit-vm.XXXXXXXX")
trap 'rm -rf -- "$state"' EXIT
chmod 755 "$state"
if [ "${#args[@]}" -gt 0 ]; then
  printf '%s\0' "${args[@]}" > "$state/args"
else
  : > "$state/args"
fi
chmod 644 "$state/args"
export SHARED_DIR=$state
export TMPDIR=$state
export USE_TMPDIR=1
@vm@
