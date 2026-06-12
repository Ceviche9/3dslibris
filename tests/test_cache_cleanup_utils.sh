#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
. tests/test_build.sh
build_test test_cache_cleanup_utils \
  "$TEST_ROOT/tests/test_cache_cleanup_utils.cpp" \
  "$TEST_ROOT/source/settings/cache_cleanup_utils.cpp"
