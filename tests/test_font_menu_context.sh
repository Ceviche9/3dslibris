set -eu
source "$(dirname "$0")/test_build.sh"
build_test test_font_menu_context \
  "$TEST_ROOT/tests/test_font_menu_context.cpp" \
  "-I$TEST_ROOT/tests/stubs"
