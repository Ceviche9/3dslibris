#pragma once

namespace settings {

bool ToggleSetting(bool current);
int NextCyclicSetting(int current, int count);
int NextTriStateOverride(int current);

} // namespace settings
