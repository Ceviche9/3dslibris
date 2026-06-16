#pragma once

#include <3ds/types.h>

struct KeyMap {
  u32 up, down, left, right;
  u32 zl, zr, l, r;
  u32 dup, ddown, dleft, dright;
  u32 a, b, x, y;
  u32 start, select;
  u32 downrepeat;
};
