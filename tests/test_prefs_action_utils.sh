#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
. tests/test_build.sh
build_test test_prefs_action_utils \
  "$TEST_ROOT/tests/test_prefs_action_utils.cpp" \
  "$TEST_ROOT/source/settings/prefs_action_utils.cpp"
