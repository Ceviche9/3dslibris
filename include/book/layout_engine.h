/*
    3dslibris - layout_engine.h
    On-demand layout engine with preserved CSS context
*/

#pragma once

#include "book/content_node.h"
#include "shared/text_layout_utils.h"
#include <vector>
#include <string>

class IStatusReporter;

namespace layout_engine {

// Position in the document (for resuming pagination)
struct PageStart {
  content_tree::ContentNode* node;
  size_t char_offset;
  // Running count of text chars preceding this position in document order.
  // Used to persist/resume reading position and compute progress % without
  // a page index (the new engine has no fixed page count).
  size_t global_offset;

  PageStart() : node(nullptr), char_offset(0), global_offset(0) {}
  PageStart(content_tree::ContentNode* n, size_t offset)
    : node(n), char_offset(offset), global_offset(0) {}
};

// Layout metrics for page computation
struct LayoutMetrics {
  int screen_width;
  int screen_height;
  int base_margin_left;
  int base_margin_right;
  int base_margin_top;
  int base_margin_bottom;
  int line_spacing;       // extra spacing between lines
  int space_advance;      // width of space character
  // Text's current base pixel size. Not used for layout math directly (per-
  // node sizing already comes from ComputedStyle/measure_fn, both of which
  // read the live Text state), but PageCache keys on it so a font-size
  // change busts cached pages for the same (node, offset) instead of
  // silently reusing ones shaped at the old size.
  int base_font_size;

  text_layout_utils::MeasureCodepointFn measure_fn;
  void* measure_ctx;

  LayoutMetrics()
    : screen_width(400),
      screen_height(240),
      base_margin_left(16),
      base_margin_right(16),
      base_margin_top(12),
      base_margin_bottom(36),
      line_spacing(3),
      space_advance(6),
      base_font_size(16),
      measure_fn(nullptr),
      measure_ctx(nullptr)
  {}
};

// Fragment of text on a line
struct LineFragment {
  content_tree::ContentNode* source_node;
  size_t text_start;      // UTF-8 offset in source node
  size_t text_length;     // UTF-8 length
  std::vector<text_layout_utils::ShapedGlyph> glyphs;
  int x;
  int y;
  int width;
  int baseline;
  u16 color;
  u16 href_id;
  bool is_link;

  LineFragment()
    : source_node(nullptr),
      text_start(0),
      text_length(0),
      x(0), y(0),
      width(0),
      baseline(0),
      color(0x0000),
      href_id(0),
      is_link(false)
  {}
};

// Line in the layout
struct LayoutLine {
  std::vector<LineFragment> fragments;
  int y;
  int height;
  int baseline;

  LayoutLine() : y(0), height(0), baseline(0) {}
};

// Computed page layout
struct LayoutPage {
  int page_number;
  std::vector<LayoutLine> lines;
  PageStart end_position;
};

// Block context (for margin/indent stack)
struct BlockContext {
  int effective_margin_left;
  int effective_margin_right;
  int text_indent;
  bool first_line;

  BlockContext()
    : effective_margin_left(0),
      effective_margin_right(0),
      text_indent(0),
      first_line(true)
  {}
};

// Layout state during page computation
struct LayoutContext {
  // Viewport
  LayoutMetrics metrics;

  // Current pen position
  int pen_x;
  int pen_y;
  int current_line_height;
  int current_line_baseline;

  // Block stack (for nested blocks with margins)
  std::vector<BlockContext> block_stack;

  // Current line being built
  std::vector<LineFragment> current_line;

  // Language for text shaping
  const char* lang;

  // Optional, only used for DSLIBRIS_DEBUG logging.
  IStatusReporter* reporter;

  // Running count of text chars consumed so far in this ComputePage() call,
  // relative to the document start (mirrors char_offset/glyph_idx
  // bookkeeping but accumulated across nodes). Seeded from the PageStart
  // that began this page; stamped onto page.end_position.global_offset.
  size_t global_chars_consumed;

  LayoutContext()
    : pen_x(0),
      pen_y(0),
      current_line_height(0),
      current_line_baseline(0),
      lang("en"),
      reporter(nullptr),
      global_chars_consumed(0)
  {}
};

// Layout engine
class LayoutEngine {
public:
  LayoutEngine();

  // Compute a page starting from given position. `reporter` is optional and
  // only used for DSLIBRIS_DEBUG logging.
  LayoutPage ComputePage(
    const PageStart& start,
    const LayoutMetrics& metrics,
    IStatusReporter* reporter = nullptr
  );

private:
  // Layout a node and its children. Returns true if the page became full
  // while doing so (caller must stop); false if the node (and everything
  // under it) was fully laid out and the caller should move on to whatever
  // comes next in document order.
  bool LayoutNode(
    content_tree::ContentNode* node,
    LayoutContext& ctx,
    LayoutPage& page,
    size_t start_offset
  );

  // Layout text node. Same true/false contract as LayoutNode().
  bool LayoutTextNode(
    content_tree::ContentNode* node,
    LayoutContext& ctx,
    LayoutPage& page,
    size_t start_offset
  );

  // Add block margins and push context
  void EnterBlock(
    const content_tree::ComputedStyle& style,
    LayoutContext& ctx
  );

  // Reconstructs the BlockContext stack for every IsBlock() ancestor of
  // `node` (walking up via ContentNode::parent), so a page that starts
  // mid-document - not at tree root - still gets correct margins/indent.
  // Does not touch pen_y (ancestors' top margins were already consumed on
  // an earlier page) and forces first_line=false on every reconstructed
  // level (never re-apply text-indent on a resumed/continued block).
  std::vector<BlockContext> BuildBlockStackForNode(
    content_tree::ContentNode* node,
    const LayoutMetrics& metrics
  ) const;

  // Pop block context and add bottom margin
  void ExitBlock(
    const content_tree::ComputedStyle& style,
    LayoutContext& ctx
  );

  // Commit current line to page
  void CommitLine(
    LayoutContext& ctx,
    LayoutPage& page
  );

  // Check if line would fit on page
  bool LineWouldFit(
    int pen_y,
    int line_height,
    const LayoutContext& ctx
  ) const;

  // Add vertical space
  void AddVerticalSpace(
    int space,
    LayoutContext& ctx
  );
};

} // namespace layout_engine
