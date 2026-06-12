#include "settings/prefs_action_utils.h"

#include "test_assert.h"

int main() {
  test::ExpectFalse("toggle true", settings::ToggleSetting(true));
  test::ExpectTrue("toggle false", settings::ToggleSetting(false));

  test::ExpectEq("cycle advances", settings::NextCyclicSetting(2, 6), 3);
  test::ExpectEq("cycle wraps", settings::NextCyclicSetting(5, 6), 0);
  test::ExpectEq("cycle repairs negative", settings::NextCyclicSetting(-1, 6),
                 0);
  test::ExpectEq("cycle repairs high value",
                 settings::NextCyclicSetting(8, 6), 0);

  test::ExpectEq("override inherit to off",
                 settings::NextTriStateOverride(-1), 0);
  test::ExpectEq("override off to on", settings::NextTriStateOverride(0), 1);
  test::ExpectEq("override on to inherit",
                 settings::NextTriStateOverride(1), -1);
  test::ExpectEq("override repairs invalid",
                 settings::NextTriStateOverride(4), -1);

  return 0;
}
