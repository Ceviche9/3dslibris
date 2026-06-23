#!/usr/bin/env bash
set -eu
source "$(dirname "$0")/test_build.sh"
build_test test_cover_override_utils \
  "$TEST_ROOT/tests/test_cover_override_utils.cpp" \
  "$TEST_ROOT/source/library/cover_override_utils.cpp"
