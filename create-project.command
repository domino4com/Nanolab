#!/bin/bash
# Create a complete student handoff folder without Python or Git dependencies.
set -euo pipefail
repo=$(cd -- "$(dirname -- "$0")" && pwd)
name=${1:-}
if [[ ! "$name" =~ ^[A-Za-z][A-Za-z0-9_]*$ ]]; then
    echo "Usage: $0 MyExperiment [parent-directory]" >&2
    echo 'Use letters, digits and underscores, beginning with a letter.' >&2
    exit 2
fi
parent=${2:-$PWD}
mkdir -p -- "$parent"
parent=$(cd -- "$parent" && pwd)
target="$parent/$name"
if [ -e "$target" ]; then echo "Already exists; nothing changed: $target" >&2; exit 1; fi
mkdir -p "$target/libraries/CanStatusClient"
cp -R "$repo/src" "$target/libraries/CanStatusClient/"
cp "$repo/library.properties" "$repo/LICENSE" "$target/libraries/CanStatusClient/"
cp "$repo/templates/Project.ino" "$target/$name.ino"
cp "$repo/templates/PROJECT.md" "$target/README.md"
cp "$repo/examples/TestClient/"*.command "$repo/examples/TestClient/"*.ps1 "$target/"
sed 's@dir: ../..$@dir: libraries/CanStatusClient@; s@# This example uses the library at the root of the Nanolab repository.@# This exact local library source is included in the submission.@' \
    "$repo/examples/TestClient/sketch.yaml" > "$target/sketch.yaml"
printf 'build/\n.DS_Store\n' > "$target/.gitignore"
printf 'https://github.com/domino4com/Nanolab\nBundled library version: ' > "$target/libraries/CanStatusClient/ORIGIN.txt"
sed -n 's/^version=//p' "$repo/library.properties" >> "$target/libraries/CanStatusClient/ORIGIN.txt"
chmod +x "$target/"*.command
printf 'Created %s\nOpen %s/%s.ino, merge your existing code, then run compile.command.\n' "$target" "$target" "$name"
