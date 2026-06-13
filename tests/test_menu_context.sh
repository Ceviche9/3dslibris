set -eu
source "$(dirname "$0")/test_build.sh"
build_test test_menu_context \
  "$TEST_ROOT/tests/test_menu_context.cpp" \
  "$TEST_ROOT/source/menus/menu.cpp" \
  "-I$TEST_ROOT/tests/stubs"
