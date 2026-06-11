set -eu
source "$(dirname "$0")/test_build.sh"
build_test test_reader_controls \
  "$TEST_ROOT/tests/test_reader_controls.cpp"
