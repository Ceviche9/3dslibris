#pragma once

// Reading orientation model.
//
// Historically orientation was a single handedness bit (device held rotated
// like a book, turned left or right). Landscape is a third state where pages
// are laid out for the screens' native orientation. Pure header so host tests
// can exercise orientation predicates without the 3DS toolchain.
namespace orientation_utils {

enum Orientation : unsigned char {
  ORIENT_TURNED_LEFT = 0,
  ORIENT_TURNED_RIGHT = 1,
  ORIENT_LANDSCAPE = 2,
};

inline bool IsTurnedRight(unsigned char orientation) {
  return orientation == ORIENT_TURNED_RIGHT;
}

inline bool IsLandscape(unsigned char orientation) {
  return orientation == ORIENT_LANDSCAPE;
}

// Reading order: which physical screen shows reading screen 0. True for
// turned-left (top/left physical screen first) and landscape (top screen
// first); false only for turned-right.
inline bool FirstScreenIsLeft(unsigned char orientation) {
  return !IsTurnedRight(orientation);
}

} // namespace orientation_utils
