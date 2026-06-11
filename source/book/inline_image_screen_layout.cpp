#include "book/inline_image_screen_layout.h"

#include "app/status_layout_utils.h"
#include "shared/screen_dimensions.h"
#include "shared/text_render_layout_utils.h"
#include "shared/text_screen_geometry.h"

namespace {

int ResolveFullReadingImageBottomMargin(int screen_height, int line_height,
                                        int footer_reserved_height) {
  const status_layout_utils::BookStatusHudLayout hud_layout =
      status_layout_utils::ComputeBookStatusHudLayout(
          screen_height, line_height, footer_reserved_height);
  return std::max(0, screen_height - hud_layout.clear_top);
}

} // namespace

InlineImageScreenLayout ResolveInlineImageScreenLayout(
    bool current_screen_is_left, int full_bottom_margin, int line_height) {
  const int compact_bottom_margin =
      text_render_layout_utils::ResolveCompactReadingBottomMargin(
          full_bottom_margin);
  const int content_full_bottom_margin =
      ResolveFullReadingImageBottomMargin(screen_dims::kTopScreenHeightPx, line_height, full_bottom_margin);

  InlineImageScreenLayout layout{};
  layout.current_screen_width = screen_dims::kTopScreenWidthPx;
  layout.next_screen_width = screen_dims::kTopScreenWidthPx;
  if (current_screen_is_left) {
    layout.current_screen_height = screen_dims::kTopScreenHeightPx;
    layout.current_margin_bottom = content_full_bottom_margin;
    layout.next_screen_height = screen_dims::kBottomScreenHeightPx;
    layout.next_margin_bottom = compact_bottom_margin;
  } else {
    layout.current_screen_height = screen_dims::kBottomScreenHeightPx;
    layout.current_margin_bottom = compact_bottom_margin;
    layout.next_screen_height = screen_dims::kTopScreenHeightPx;
    layout.next_margin_bottom = content_full_bottom_margin;
  }
  return layout;
}

InlineImageScreenLayout ResolveInlineImageScreenLayoutForOrientation(
    unsigned char orientation, int current_screen, int full_bottom_margin,
    int line_height) {
  if (!orientation_utils::IsLandscape(orientation)) {
    return ResolveInlineImageScreenLayoutForReadingScreen(
        orientation_utils::IsTurnedRight(orientation), current_screen,
        full_bottom_margin, line_height);
  }

  InlineImageScreenLayout layout{};
  const text_screen_geometry::ScreenGeometry current_geometry =
      text_screen_geometry::ResolveReadingScreenGeometry(orientation,
                                                         current_screen);
  const text_screen_geometry::ScreenGeometry next_geometry =
      text_screen_geometry::ResolveReadingScreenGeometry(
          orientation, current_screen == 0 ? 1 : 0);
  layout.current_screen_width = current_geometry.width;
  layout.current_screen_height = current_geometry.height;
  layout.current_margin_bottom =
      text_render_layout_utils::
          ResolveReadingScreenRenderBottomMarginForOrientation(
              orientation, current_screen, full_bottom_margin,
              text_render_layout_utils::ResolveCompactReadingBottomMargin(
                  full_bottom_margin));
  layout.next_screen_width = next_geometry.width;
  layout.next_screen_height = next_geometry.height;
  layout.next_margin_bottom =
      text_render_layout_utils::
          ResolveReadingScreenRenderBottomMarginForOrientation(
              orientation, current_screen == 0 ? 1 : 0, full_bottom_margin,
              text_render_layout_utils::ResolveCompactReadingBottomMargin(
                  full_bottom_margin));
  return layout;
}

int ResolveReadingScreenIndexForPhysicalScreen(bool turned_right,
                                               bool current_screen_is_left) {
  if (!turned_right)
    return current_screen_is_left ? 0 : 1;
  return current_screen_is_left ? 1 : 0;
}

InlineImageScreenLayout ResolveInlineImageScreenLayoutForReadingScreen(
    bool turned_right, int current_screen, int full_bottom_margin,
    int line_height) {
  const bool current_screen_is_left =
      turned_right ? (current_screen != 0) : (current_screen == 0);
  return ResolveInlineImageScreenLayout(current_screen_is_left,
                                        full_bottom_margin, line_height);
}
