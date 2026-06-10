#pragma once

#include "shared/orientation_utils.h"

// Logical pixel geometry of the two software screen buffers.
//
// Buffers are square (kBufferStridePx x kBufferStridePx) and rows are indexed
// buf[y * kBufferStridePx + x] in every orientation; only the logical
// width/height of the populated region changes:
//   portrait (turned left/right): both screens 240 wide; left 400, right 320 tall
//   landscape:                    both screens 240 tall; left 400, right 320 wide
// Pure header, host-testable.
namespace text_screen_geometry {

static const int kBufferStridePx = 400;

struct ScreenGeometry {
  int width;
  int height;
};

inline ScreenGeometry ResolveTextScreenGeometry(unsigned char orientation,
                                                bool is_left_buffer) {
  ScreenGeometry geometry;
  const int long_axis = is_left_buffer ? 400 : 320;
  if (orientation_utils::IsLandscape(orientation)) {
    geometry.width = long_axis;
    geometry.height = 240;
  } else {
    geometry.width = 240;
    geometry.height = long_axis;
  }
  return geometry;
}

} // namespace text_screen_geometry
