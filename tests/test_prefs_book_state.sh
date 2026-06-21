#!/bin/bash
source "$(dirname "$0")/test_build.sh"

build_test test_prefs_book_state \
  "$TEST_ROOT/tests/test_prefs_book_state.cpp" \
  "$TEST_ROOT/source/settings/prefs_book_state.cpp"
