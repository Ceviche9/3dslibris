#include "shared/text_screen_geometry.h"

#include "shared/orientation_utils.h"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

[[noreturn]] void Fail(const std::string &message) {
  std::fprintf(stderr, "%s\n", message.c_str());
  std::exit(1);
}

void ExpectEq(const char *label, int actual, int expected) {
  if (actual != expected) {
    Fail(std::string(label) + ": expected " + std::to_string(expected) +
         ", got " + std::to_string(actual));
  }
}

void ExpectGeometry(const char *label, unsigned char orientation,
                    bool is_left_buffer, int width, int height) {
  const text_screen_geometry::ScreenGeometry g =
      text_screen_geometry::ResolveTextScreenGeometry(orientation,
                                                      is_left_buffer);
  ExpectEq((std::string(label) + " width").c_str(), g.width, width);
  ExpectEq((std::string(label) + " height").c_str(), g.height, height);
}

void TestPortraitGeometry() {
  using namespace orientation_utils;
  ExpectGeometry("turned-left left screen", ORIENT_TURNED_LEFT, true, 240, 400);
  ExpectGeometry("turned-left right screen", ORIENT_TURNED_LEFT, false, 240, 320);
  ExpectGeometry("turned-right left screen", ORIENT_TURNED_RIGHT, true, 240, 400);
  ExpectGeometry("turned-right right screen", ORIENT_TURNED_RIGHT, false, 240, 320);
}

void TestLandscapeGeometry() {
  using namespace orientation_utils;
  ExpectGeometry("landscape top screen", ORIENT_LANDSCAPE, true, 400, 240);
  ExpectGeometry("landscape bottom screen", ORIENT_LANDSCAPE, false, 320, 240);
}

void TestGeometryFitsBufferStride() {
  using namespace orientation_utils;
  const unsigned char orientations[] = {ORIENT_TURNED_LEFT, ORIENT_TURNED_RIGHT,
                                        ORIENT_LANDSCAPE};
  for (unsigned char o : orientations) {
    for (int left = 0; left < 2; ++left) {
      const text_screen_geometry::ScreenGeometry g =
          text_screen_geometry::ResolveTextScreenGeometry(o, left != 0);
      const int max_index =
          (g.height - 1) * text_screen_geometry::kBufferStridePx +
          (g.width - 1);
      if (max_index >=
          text_screen_geometry::kBufferStridePx *
              text_screen_geometry::kBufferStridePx)
        Fail("geometry exceeds square buffer");
      if (g.width > text_screen_geometry::kBufferStridePx)
        Fail("logical width exceeds buffer stride");
    }
  }
}

void TestOrientationPredicates() {
  using namespace orientation_utils;
  if (IsTurnedRight(ORIENT_TURNED_LEFT) || !IsTurnedRight(ORIENT_TURNED_RIGHT) ||
      IsTurnedRight(ORIENT_LANDSCAPE))
    Fail("IsTurnedRight must be true only for ORIENT_TURNED_RIGHT");
  if (IsLandscape(ORIENT_TURNED_LEFT) || IsLandscape(ORIENT_TURNED_RIGHT) ||
      !IsLandscape(ORIENT_LANDSCAPE))
    Fail("IsLandscape must be true only for ORIENT_LANDSCAPE");
  if (!FirstScreenIsLeft(ORIENT_TURNED_LEFT) ||
      FirstScreenIsLeft(ORIENT_TURNED_RIGHT) ||
      !FirstScreenIsLeft(ORIENT_LANDSCAPE))
    Fail("FirstScreenIsLeft must be false only for ORIENT_TURNED_RIGHT");
}

} // namespace

int main() {
  TestPortraitGeometry();
  TestLandscapeGeometry();
  TestGeometryFitsBufferStride();
  TestOrientationPredicates();
  std::printf("All text_screen_geometry tests passed.\n");
  return 0;
}
