#include "app/status_layout_utils.h"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

[[noreturn]] void Fail(const std::string &message) {
  fprintf(stderr, "%s\n", message.c_str());
  std::exit(1);
}

void ExpectTrue(const char *label, bool value) {
  if (!value)
    Fail(std::string(label) + ": expected true");
}

void ExpectEq(const char *label, int actual, int expected) {
  if (actual != expected) {
    Fail(std::string(label) + ": expected " + std::to_string(expected) +
         ", got " + std::to_string(actual));
  }
}

void TestTopScreenLeavesBottomPadding() {
  status_layout_utils::BookStatusHudLayout layout =
      status_layout_utils::ComputeBookStatusHudLayout(400, 12, 36);
  ExpectTrue("text baseline inside screen", layout.text_y < 400);
  ExpectTrue("progress bar bottom padded",
             layout.progress_bar_y + layout.progress_bar_height <= 387);
  ExpectEq("clear band starts at reserved footer", layout.clear_top, 364);
  ExpectEq("clear band ends at screen bottom", layout.clear_bottom, 400);
}

void TestShorterScreenStillFits() {
  status_layout_utils::BookStatusHudLayout layout =
      status_layout_utils::ComputeBookStatusHudLayout(320, 12, 16);
  ExpectTrue("progress bar fits shorter screen",
             layout.progress_bar_y + layout.progress_bar_height <= 307);
  ExpectEq("clear band respects footer reserve", layout.clear_top, 304);
}

void TestFixedLayoutBottomOverlayFits() {
  status_layout_utils::FixedLayoutBottomHudLayout layout =
      status_layout_utils::ComputeFixedLayoutBottomHudLayout(320, 12);
  ExpectEq("fixed layout top text y", layout.time_y, 10);
  ExpectEq("fixed layout top clear start", layout.time_clear_top, 0);
  ExpectEq("fixed layout top clear end", layout.time_clear_bottom, 18);
  ExpectEq("fixed layout bottom text y", layout.page_y, 298);
  ExpectEq("fixed layout bottom clear start", layout.page_clear_top, 289);
  ExpectEq("fixed layout bottom clear end", layout.page_clear_bottom, 306);
  ExpectEq("fixed layout right margin", layout.right_margin, 8);
}

void TestLandscapeBookHudUsesSlimBottomStrip() {
  status_layout_utils::LandscapeBookStatusHudLayout layout =
      status_layout_utils::ComputeLandscapeBookStatusHudLayout(320, 240, 12);
  ExpectEq("landscape clear top", layout.clear_top, 218);
  ExpectEq("landscape clear bottom", layout.clear_bottom, 240);
  ExpectEq("landscape text baseline", layout.text_y, 235);
  ExpectEq("landscape left margin", layout.left_margin, 6);
  ExpectEq("landscape right margin", layout.right_margin, 6);
  ExpectTrue("landscape progress bar fits strip",
             layout.progress_bar_y >= layout.clear_top &&
                 layout.progress_bar_y + layout.progress_bar_height <=
                     layout.clear_bottom);
}

} // namespace

int main() {
  TestTopScreenLeavesBottomPadding();
  TestShorterScreenStillFits();
  TestFixedLayoutBottomOverlayFits();
  TestLandscapeBookHudUsesSlimBottomStrip();
  return 0;
}
