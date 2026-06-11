#include "formats/common/fixed_layout_viewport_utils.h"

#include <cstdio>
#include <cstdlib>

int main() {
  fixed_layout_viewport_utils::ViewportCenter center =
      fixed_layout_viewport_utils::DefaultPageTurnViewportCenter();
  if (center.x != 0.0f || center.y != 0.0f) {
    fprintf(stderr, "expected default viewport center to be 0,0\n");
    return 1;
  }
  fixed_layout_viewport_utils::ViewportState state;
  state.zoom_index = 5;
  state.center_x = 0.2f;
  state.center_y = 0.8f;
  state.interaction_active = true;
  fixed_layout_viewport_utils::ResetViewportForTargetChange(&state, 2);
  if (state.zoom_index != 2 || state.center_x != 0.5f ||
      state.center_y != 0.5f || state.interaction_active) {
    fprintf(stderr, "target change must reset viewport to centered fit zoom\n");
    return 1;
  }
  return 0;
}
