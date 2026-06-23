/*
    3dslibris - fixed_layout_reader_input.h
    New 3DS reader module by Rigle.

    Summary:
    - Input handling for fixed-layout books (PDF/CBZ/XPS) in reading mode.
    - Extracted from reader/app_book.cpp HandleEventInBook.
*/

#pragma once

#include <stdint.h>

#include "reader/reader_controls.h"

class App;
class Book;
class Text;
struct FrameInput;

namespace fixed_layout_input {

// Handles all fixed-layout input for one event frame.
// Returns true if the display needs a status redraw.
// Early-returns true also means the caller should NOT continue to reflowable input.
bool HandleInBook(App &app, Book *book, Text *ts, const FrameInput &input,
                  uint16_t *pagecurrent, uint16_t pagecount,
                  const ReaderControls &ctrl);

} // namespace fixed_layout_input
