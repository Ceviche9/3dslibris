#pragma once

#include "shared/screen_dimensions.h"
#include "shared/text_screen_geometry.h"

namespace fixed_layout_screen {

static const int kTopScreenWidth    = screen_dims::kTopScreenWidthPx;
static const int kTopScreenHeight   = screen_dims::kTopScreenHeightPx;
static const int kBottomScreenWidth  = screen_dims::kBottomScreenWidthPx;
static const int kBottomScreenHeight = screen_dims::kBottomScreenHeightPx;

struct TargetDimensions {
  int width;
  int height;
};

inline TargetDimensions TargetDims(unsigned char orientation, bool is_top) {
  const text_screen_geometry::ScreenGeometry geometry =
      text_screen_geometry::ResolveTextScreenGeometry(orientation, is_top);
  TargetDimensions dims = {geometry.width, geometry.height};
  return dims;
}

} // namespace fixed_layout_screen
