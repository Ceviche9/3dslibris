set -eu
source "$(dirname "$0")/test_build.sh"
build_test test_paged_list_menu_context \
  "$TEST_ROOT/tests/test_paged_list_menu_context.cpp" \
  "-I$TEST_ROOT/tests/stubs"
