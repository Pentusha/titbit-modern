#!/bin/sh
set -eu
base=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
python=${TITBIT_PYTHON:-python3}
if [ -x "$base/.venv/bin/python" ] && [ -z "${TITBIT_PYTHON:-}" ]; then
    python="$base/.venv/bin/python"
fi
exec "$python" "$base/tools/start_studio.py" "$@"
