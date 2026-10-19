#!/bin/sh
set -eu
base=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
# Release package: one bundled executable, no Python needed.
if [ -x "$base/titbit-tools" ] && [ -z "${TITBIT_PYTHON:-}" ]; then
    exec "$base/titbit-tools" studio "$@"
fi
python=${TITBIT_PYTHON:-python3}
if [ -x "$base/.venv/bin/python" ] && [ -z "${TITBIT_PYTHON:-}" ]; then
    python="$base/.venv/bin/python"
fi
exec "$python" "$base/tools/start_studio.py" "$@"
