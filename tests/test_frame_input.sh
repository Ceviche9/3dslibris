set -eu
source "$(dirname "$0")/test_build.sh"
build_test test_frame_input \
  "$TEST_ROOT/tests/test_frame_input.cpp"
