#!/bin/sh
set -eu
source "$(dirname "$0")/test_build.sh"

build_test test_stb_image_formats \
  "$TEST_ROOT/tests/test_stb_image_formats.cpp" \
  "$TEST_ROOT/source/core/stb_image_impl.cpp" \
  -I"$TEST_ROOT/third_party/stb"
