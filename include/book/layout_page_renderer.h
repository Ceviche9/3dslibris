/*
    3dslibris - layout_page_renderer.h
    Renders a computed LayoutPage to screen
*/

#pragma once

#include "book/layout_engine.h"

class Text;
class Book;

namespace layout_page_renderer {

// Renders the current reading spread (both physical 3DS screens - see
// Book::ComputeReflowSpread for why one LayoutPage is only ever one
// screen's worth of content) using the Text renderer.
void RenderPage(
  Book* book,
  Text* text
);

// Render a single line
void RenderLine(
  const layout_engine::LayoutLine& line,
  Text* text,
  Book* book
);

// Render a single fragment
void RenderFragment(
  const layout_engine::LineFragment& fragment,
  Text* text,
  Book* book
);

} // namespace layout_page_renderer
