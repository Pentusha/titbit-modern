#!/bin/sh
# Builds dist/Titbit-<version>-x86_64.flatpak from build/titbit and build/titbit-tools.
#   sh flatpak/build_flatpak.sh [version]
# Needs flatpak, flatpak-builder and, from Flathub (user install is enough):
#   org.freedesktop.Platform//25.08 org.freedesktop.Sdk//25.08
#   org.freedesktop.Platform.Compat.i386//25.08 org.freedesktop.Platform.GL32.default//25.08
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$(dirname "$here")
version=${1:-dev}
for f in "$root/build/titbit" "$root/build/titbit-tools"; do
    [ -x "$f" ] || { echo "Missing $f: run tools/build_linux.py --minimal and tools/build_tools.sh" >&2; exit 1; }
done
work=${XDG_CACHE_HOME:-$HOME/.cache}/titbit/flatpak
mkdir -p "$work" "$root/dist"
# same treatment as the tar package: drop build-time DWARF, keep EH tables
rm -rf "$here/stage" && mkdir -p "$here/stage"
strip --strip-debug --remove-section=.comment -o "$here/stage/titbit" "$root/build/titbit"
cp "$root/build/titbit-tools" "$here/stage/titbit-tools"
# engine notices plus the licences of everything packed into titbit-tools
mkdir -p "$here/stage/licenses/runtime"
cp -r "$root/docs/licenses/." "$here/stage/licenses/"
cp "$root/build/tools-licenses/"* "$here/stage/licenses/runtime/"
flatpak-builder --user --force-clean --disable-rofiles-fuse --state-dir="$work/state" \
    --repo="$work/repo" "$work/app" "$here/io.github.offx366.Titbit.yml"
bundle="$root/dist/Titbit-$version-x86_64.flatpak"
flatpak build-bundle --runtime-repo=https://dl.flathub.org/repo/flathub.flatpakrepo \
    "$work/repo" "$bundle" io.github.offx366.Titbit
sha256sum "$bundle" | sed 's#  .*/#  #' > "$bundle.sha256"
echo "OK: $bundle"
echo "Install: flatpak install --user $(basename "$bundle")"
