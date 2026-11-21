#!/usr/bin/env bash
# Fetches the LVGL checkout used by the build (shallow clone of a release tag).
set -euo pipefail

ref="${1:-v9.5.0}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
target="$root/third_party/lvgl"

if [ -d "$target/.git" ]; then
    echo "lvgl already present at $target"
    exit 0
fi

mkdir -p "$(dirname "$target")"
git clone --depth 1 --branch "$ref" https://github.com/lvgl/lvgl.git "$target"
echo "lvgl $ref ready at $target"
