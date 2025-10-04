#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR=${BUILD_DIR:-build}
cmake -S . -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" -j

BIN="$BUILD_DIR/src/cmini"
if [[ ! -x "$BIN" ]]; then
  echo "Compiler binary not found: $BIN" >&2
  exit 1
fi

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <input.cmini> [more files...]" >&2
  exit 1
fi

for f in "$@"; do
  out="${f%.*}.ll"
  "$BIN" "$f" -o "$out"
  echo "OK: $f -> $out"
  if command -v llc >/dev/null 2>&1; then
    llc -filetype=obj "$out" -o "${f%.*}.o" || true
  fi
  if command -v clang >/dev/null 2>&1 && [[ -f "${f%.*}.o" ]]; then
    clang "${f%.*}.o" -o "${f%.*}" || true
  fi
done
