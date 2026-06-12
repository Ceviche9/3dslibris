#!/bin/bash
set -eu

ROOT="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
OUT="${TMPDIR:-/tmp}/3dslibris-tests/test_library_controller_header"
mkdir -p "$(dirname "$OUT")"

"${CXX:-c++}" -std=c++11 \
  -I"$ROOT/tests/stubs" \
  -I"$ROOT/include" \
  "$ROOT/tests/test_library_controller_header.cpp" \
  -o "$OUT"

"$OUT"
