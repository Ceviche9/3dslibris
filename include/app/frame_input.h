#pragma once

#include <stdint.h>

#include "shared/touch_map_utils.h"

struct FrameInput {
  uint32_t keys_down;
  uint32_t keys_held;
  bool touch_active;
  int touch_raw_x;
  int touch_raw_y;
  uint64_t timestamp_ms;

  FrameInput()
      : keys_down(0), keys_held(0), touch_active(false), touch_raw_x(0),
        touch_raw_y(0), timestamp_ms(0) {}

  FrameInput(uint32_t down, uint32_t held, bool active, int raw_x, int raw_y,
             uint64_t timestamp)
      : keys_down(down), keys_held(held), touch_active(active),
        touch_raw_x(raw_x), touch_raw_y(raw_y), timestamp_ms(timestamp) {}

  touch_map_utils::TouchPoint MapTouch(unsigned char orientation) const {
    return touch_map_utils::MapRawTouch(orientation, touch_raw_x, touch_raw_y);
  }
};
