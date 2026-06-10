set -eu
source "$(dirname "$0")/test_build.sh"
build_test test_touch_map_utils \
  "$TEST_ROOT/tests/test_touch_map_utils.cpp"
