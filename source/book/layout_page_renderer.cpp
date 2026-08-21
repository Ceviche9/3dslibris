/*
    3dslibris - layout_page_renderer.cpp
    Layout page renderer implementation
*/

#include "book/layout_page_renderer.h"
#include "book/book.h"
#include "ui/text.h"
#include "shared/orientation_utils.h"
#include "shared/debug_log.h"

namespace layout_page_renderer {

void RenderPage(
  const layout_engine::LayoutPage& page,
  Text* text,
  Book* book
) {
  if (!text || !book) return;
  IStatusReporter *r = book->GetStatusReporter();

  if (page.lines.empty()) {
    DBG_LOGF_CAT(r, DBG_LEVEL_WARN, DBG_CAT_RENDER,
                 "RENDER: RenderPage called with 0 lines book=%s",
                 book->GetFileName() ? book->GetFileName() : "");
  }

  // Initialize screen similar to Page::Draw
  text->InitPen();
  text->SetAutoWrapEnabled(false);
  text->SetClipToContentEnabled(true);

  const unsigned char orientation = book->GetOrientation();
  const bool first_screen_is_left = orientation_utils::FirstScreenIsLeft(orientation);
  u16 *first_screen = first_screen_is_left ? text->screenleft : text->screenright;

  // Clear both screens
  text->SetScreen(text->screenleft);
  book->DrawTopGradientBackground();
  text->MarkScreenDirty(text->screenleft);

  text->SetScreen(text->screenright);
  book->DrawBottomGradientBackground();
  text->MarkScreenDirty(text->screenright);

  // Start rendering on first screen
  text->SetScreen(first_screen);
  text->InitPen();

  // Render all lines
  for (const auto& line : page.lines) {
    RenderLine(line, text, book);
  }
}

void RenderLine(
  const layout_engine::LayoutLine& line,
  Text* text,
  Book* book
) {
  if (!text) return;

  for (const auto& fragment : line.fragments) {
    RenderFragment(fragment, text, book);
  }
}

void RenderFragment(
  const layout_engine::LineFragment& fragment,
  Text* text,
  Book* book
) {
  if (!text || !fragment.source_node) return;
  if (fragment.glyphs.empty()) return;

  // Set pen position to fragment start
  text->SetPen(fragment.x, fragment.y);

  // Set text color
  text->fgcolor = fragment.color;
  text->usefgcolor = (fragment.color != 0);

  // Render each glyph in the fragment
  // Note: PrintChar advances the pen automatically
  for (const auto& glyph : fragment.glyphs) {
    text->PrintChar(glyph.text.codepoint);
  }
}

} // namespace layout_page_renderer
