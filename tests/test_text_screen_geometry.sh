set -eu
source "$(dirname "$0")/test_build.sh"
build_test test_text_screen_geometry \
  "$TEST_ROOT/tests/test_text_screen_geometry.cpp"
