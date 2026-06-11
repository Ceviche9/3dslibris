#pragma once

struct InlineImageScreenLayout {
  int current_screen_width;
  int current_screen_height;
  int current_margin_bottom;
  int next_screen_width;
  int next_screen_height;
  int next_margin_bottom;
};

InlineImageScreenLayout ResolveInlineImageScreenLayout(
    bool current_screen_is_left, int full_bottom_margin, int line_height);

int ResolveReadingScreenIndexForPhysicalScreen(bool turned_right,
                                               bool current_screen_is_left);

InlineImageScreenLayout ResolveInlineImageScreenLayoutForReadingScreen(
    bool turned_right, int current_screen, int full_bottom_margin,
    int line_height);

InlineImageScreenLayout ResolveInlineImageScreenLayoutForOrientation(
    unsigned char orientation, int current_screen, int full_bottom_margin,
    int line_height);
