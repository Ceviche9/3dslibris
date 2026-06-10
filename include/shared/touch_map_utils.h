#pragma once

#include <algorithm>

#include "shared/orientation_utils.h"

// Maps raw 3DS touch panel coordinates (320x240, native landscape) to logical
// screen-buffer coordinates for the active reading orientation. Must stay the
// inverse of the blit transform in framebuffer_blit_utils.h. Pure header,
// host-testable.
namespace touch_map_utils {

struct TouchPoint {
  int x;
  int y;
};

inline TouchPoint MapRawTouch(unsigned char orientation, int raw_x, int raw_y) {
  TouchPoint mapped;
  if (orientation_utils::IsLandscape(orientation)) {
    // Logical space is the native panel orientation.
    mapped.x = std::max(0, std::min(319, raw_x));
    mapped.y = std::max(0, std::min(239, raw_y));
    return mapped;
  }
  if (orientation_utils::IsTurnedRight(orientation)) {
    mapped.x = 239 - raw_y;
    mapped.y = raw_x;
  } else {
    // Default "Turned Left" orientation (historical mapping).
    mapped.x = raw_y;
    mapped.y = 319 - raw_x;
  }
  mapped.x = std::max(0, std::min(239, mapped.x));
  mapped.y = std::max(0, std::min(319, mapped.y));
  return mapped;
}

} // namespace touch_map_utils
