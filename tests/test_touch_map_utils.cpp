#include "shared/touch_map_utils.h"

#include "shared/orientation_utils.h"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

[[noreturn]] void Fail(const std::string &message) {
  std::fprintf(stderr, "%s\n", message.c_str());
  std::exit(1);
}

void ExpectPoint(const char *label, unsigned char orientation, int raw_x,
                 int raw_y, int expected_x, int expected_y) {
  const touch_map_utils::TouchPoint p =
      touch_map_utils::MapRawTouch(orientation, raw_x, raw_y);
  if (p.x != expected_x || p.y != expected_y) {
    Fail(std::string(label) + ": expected (" + std::to_string(expected_x) +
         "," + std::to_string(expected_y) + "), got (" + std::to_string(p.x) +
         "," + std::to_string(p.y) + ")");
  }
}

void TestTurnedLeftMapping() {
  using namespace orientation_utils;
  // Historical mapping: x = raw_y, y = 319 - raw_x.
  ExpectPoint("turned-left origin", ORIENT_TURNED_LEFT, 0, 0, 0, 319);
  ExpectPoint("turned-left far corner", ORIENT_TURNED_LEFT, 319, 239, 239, 0);
  ExpectPoint("turned-left mid", ORIENT_TURNED_LEFT, 100, 50, 50, 219);
}

void TestTurnedRightMapping() {
  using namespace orientation_utils;
  // x = 239 - raw_y, y = raw_x.
  ExpectPoint("turned-right origin", ORIENT_TURNED_RIGHT, 0, 0, 239, 0);
  ExpectPoint("turned-right far corner", ORIENT_TURNED_RIGHT, 319, 239, 0, 319);
  ExpectPoint("turned-right mid", ORIENT_TURNED_RIGHT, 100, 50, 189, 100);
}

void TestLandscapeMappingIsIdentity() {
  using namespace orientation_utils;
  ExpectPoint("landscape origin", ORIENT_LANDSCAPE, 0, 0, 0, 0);
  ExpectPoint("landscape far corner", ORIENT_LANDSCAPE, 319, 239, 319, 239);
  ExpectPoint("landscape mid", ORIENT_LANDSCAPE, 100, 50, 100, 50);
}

void TestClamping() {
  using namespace orientation_utils;
  // Portrait logical space is 240x320; landscape is 320x240.
  ExpectPoint("turned-left clamps", ORIENT_TURNED_LEFT, 1000, 1000, 239, 0);
  ExpectPoint("turned-right clamps", ORIENT_TURNED_RIGHT, 1000, 1000, 0, 319);
  ExpectPoint("landscape clamps", ORIENT_LANDSCAPE, 1000, 1000, 319, 239);
}

} // namespace

int main() {
  TestTurnedLeftMapping();
  TestTurnedRightMapping();
  TestLandscapeMappingIsIdentity();
  TestClamping();
  std::printf("All touch_map_utils tests passed.\n");
  return 0;
}
