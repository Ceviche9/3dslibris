#include "reader/reader_controls.h"

#include <cstdio>
#include <cstdlib>

namespace {

struct TestKeys {
  uint32_t a, b, l, r, up, down, left, right;
  uint32_t dup, ddown, dleft, dright, zl, zr, start, select;
};

void ExpectEq(const char *label, uint32_t actual, uint32_t expected) {
  if (actual != expected) {
    std::fprintf(stderr, "%s: expected %u, got %u\n", label,
                 (unsigned)expected, (unsigned)actual);
    std::exit(1);
  }
}

void TestLandscapeControlsUseNaturalAxes() {
  TestKeys key = {1u << 0,  1u << 1,  1u << 2,  1u << 3,  1u << 4,
                  1u << 5,  1u << 6,  1u << 7,  1u << 8,  1u << 9,
                  1u << 10, 1u << 11, 1u << 12, 1u << 13, 1u << 14,
                  1u << 15};
  const ReaderControls c = BuildLandscapeControls(key);
  ExpectEq("landscape next page",
           c.page_next, key.a | key.r | key.right | key.dright | key.zl);
  ExpectEq("landscape previous page",
           c.page_prev, key.b | key.l | key.left | key.dleft | key.zr);
  ExpectEq("landscape next bookmark", c.bookmark_next, key.down | key.ddown);
  ExpectEq("landscape previous bookmark", c.bookmark_prev, key.up | key.dup);
  ExpectEq("landscape next link", c.link_next, key.down | key.ddown);
  ExpectEq("landscape previous link", c.link_prev, key.up | key.dup);
  ExpectEq("landscape fixed next", c.fixed_page_next, key.dright);
  ExpectEq("landscape fixed previous", c.fixed_page_prev, key.dleft);
}

} // namespace

int main() {
  TestLandscapeControlsUseNaturalAxes();
  return 0;
}
