/*
    3dslibris - layout_page_renderer.h
    Renders a computed LayoutPage to screen
*/

#pragma once

#include "book/layout_engine.h"

class Text;
class Book;

namespace layout_page_renderer {

// Render a layout page to screen using Text renderer
void RenderPage(
  const layout_engine::LayoutPage& page,
  Text* text,
  Book* book
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
