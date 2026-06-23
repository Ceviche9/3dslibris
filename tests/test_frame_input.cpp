#include "app/frame_input.h"
#include "shared/orientation_utils.h"

#include <cstdio>
#include <cstdlib>

static void Expect(bool condition, const char *message) {
  if (!condition) {
    std::fprintf(stderr, "%s\n", message);
    std::exit(1);
  }
}

int main() {
  FrameInput empty;
  Expect(empty.keys_down == 0, "default keys_down");
  Expect(empty.keys_held == 0, "default keys_held");
  Expect(!empty.touch_active, "default touch inactive");
  Expect(empty.timestamp_ms == 0, "default timestamp");

  FrameInput input(4, 12, true, 100, 50, 1234);
  Expect(input.keys_down == 4, "captured keys_down");
  Expect(input.keys_held == 12, "captured keys_held");
  Expect(input.touch_active, "captured touch active");
  Expect(input.timestamp_ms == 1234, "captured timestamp");

  const touch_map_utils::TouchPoint mapped =
      input.MapTouch(orientation_utils::ORIENT_TURNED_LEFT);
  Expect(mapped.x == 50 && mapped.y == 219, "mapped captured touch");
  std::printf("All frame_input tests passed.\n");
  return 0;
}
