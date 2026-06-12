#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
. tests/test_build.sh
build_test test_prefs_style_value_utils \
  "$TEST_ROOT/tests/test_prefs_style_value_utils.cpp" \
  "$TEST_ROOT/source/settings/prefs_style_value_utils.cpp"
