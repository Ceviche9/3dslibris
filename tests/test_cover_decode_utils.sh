#!/usr/bin/env bash
set -eu
source "$(dirname "$0")/test_build.sh"
build_test test_cover_decode_utils \
  "$TEST_ROOT/tests/test_cover_decode_utils.cpp" \
  "$TEST_ROOT/source/shared/image_scale_utils.cpp"
