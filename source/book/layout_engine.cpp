/*
    3dslibris - layout_engine.cpp
    Layout engine implementation
*/

#include "book/layout_engine.h"
#include "shared/debug_log.h"
#include <algorithm>
#include <cstring>

namespace layout_engine {

LayoutEngine::LayoutEngine() {
}

LayoutPage LayoutEngine::ComputePage(
  const PageStart& start,
  const LayoutMetrics& metrics,
  IStatusReporter* reporter
) {
  LayoutPage page;
  page.page_number = 0;

  if (!start.node) {
    DBG_LOGF_CAT(reporter, DBG_LEVEL_WARN, DBG_CAT_LAYOUT,
                 "LAYOUT: ComputePage null start-node offset=%u",
                 (unsigned)start.char_offset);
    return page;
  }

  if (!metrics.measure_fn) {
    DBG_LOGF_CAT(reporter, DBG_LEVEL_ERROR, DBG_CAT_LAYOUT,
                 "LAYOUT: ComputePage no measure_fn - text will not shape");
  }

  LayoutContext ctx;
  ctx.metrics = metrics;
  ctx.pen_x = metrics.base_margin_left;
  ctx.pen_y = metrics.base_margin_top;
  ctx.current_line_height = 0;
  ctx.current_line_baseline = 0;
  ctx.reporter = reporter;

  // Start layout from given node and offset
  LayoutNode(start.node, ctx, page, start.char_offset);

  // Commit any pending line
  if (!ctx.current_line.empty()) {
    CommitLine(ctx, page);
  }

  if (page.lines.empty()) {
    DBG_LOGF_CAT(reporter, DBG_LEVEL_WARN, DBG_CAT_LAYOUT,
                 "LAYOUT: ComputePage produced 0 lines start-node=%p "
                 "start-type=%d screen=%dx%d",
                 (void*)start.node, (int)start.node->type,
                 metrics.screen_width, metrics.screen_height);
  }

  return page;
}

void LayoutEngine::LayoutNode(
  content_tree::ContentNode* node,
  LayoutContext& ctx,
  LayoutPage& page,
  size_t start_offset
) {
  if (!node) return;

  // Skip if display:none
  if (node->style.display == 2) return;

  // Skip non-visual elements
  if (!node->tag_name.empty()) {
    if (node->tag_name == "head" || node->tag_name == "script" ||
        node->tag_name == "style" || node->tag_name == "meta" ||
        node->tag_name == "link" || node->tag_name == "title") {
      return;
    }
  }

  // Handle TEXT nodes
  if (node->type == content_tree::ContentNode::TEXT) {
    LayoutTextNode(node, ctx, page, start_offset);
    return;
  }

  // Handle BLOCK nodes
  if (node->IsBlock()) {
    // Commit current line before block
    if (!ctx.current_line.empty()) {
      CommitLine(ctx, page);
    }

    EnterBlock(node->style, ctx);

    // Layout children
    for (auto* child : node->children) {
      LayoutNode(child, ctx, page, 0);

      // If page is full, stop
      if (!ctx.current_line.empty() && !LineWouldFit(ctx.pen_y, ctx.current_line_height, ctx)) {
        CommitLine(ctx, page);
        page.end_position = PageStart(child, 0);
        return;
      }
    }

    // Commit pending line before exiting block
    if (!ctx.current_line.empty()) {
      CommitLine(ctx, page);
    }

    ExitBlock(node->style, ctx);
    return;
  }

  // Handle INLINE nodes - layout children inline
  for (auto* child : node->children) {
    LayoutNode(child, ctx, page, 0);
  }
}

void LayoutEngine::LayoutTextNode(
  content_tree::ContentNode* node,
  LayoutContext& ctx,
  LayoutPage& page,
  size_t start_offset
) {
  if (!node || node->text_utf8.empty()) return;

  const char* text = node->text_utf8.c_str() + start_offset;
  size_t remaining_len = node->text_utf8.size() - start_offset;

  if (remaining_len == 0) return;

  // Shape the text
  std::vector<text_layout_utils::ShapedGlyph> glyphs;
  bool has_rtl = false;

  if (!text_layout_utils::ShapeTextRunUtf8(
        text, remaining_len, ctx.lang,
        ctx.metrics.measure_fn, ctx.metrics.measure_ctx,
        &glyphs, &has_rtl)) {
    DBG_LOGF_CAT(ctx.reporter, DBG_LEVEL_WARN, DBG_CAT_LAYOUT,
                 "LAYOUT: shape failed node=%p len=%u measure_fn=%p",
                 (void*)node, (unsigned)remaining_len,
                 (void*)ctx.metrics.measure_fn);
    return;
  }

  if (glyphs.empty()) return;

  // Get current block context
  BlockContext block_ctx;
  if (!ctx.block_stack.empty()) {
    block_ctx = ctx.block_stack.back();
  } else {
    block_ctx.effective_margin_left = ctx.metrics.base_margin_left;
    block_ctx.effective_margin_right = ctx.metrics.base_margin_right;
  }

  // Calculate available width
  int available_width = ctx.metrics.screen_width -
                        block_ctx.effective_margin_left -
                        block_ctx.effective_margin_right;

  // Apply text-indent on first line of block
  if (block_ctx.first_line && node->style.text_indent > 0) {
    ctx.pen_x += node->style.text_indent;
    available_width -= node->style.text_indent;

    // Mark first line as used
    if (!ctx.block_stack.empty()) {
      ctx.block_stack.back().first_line = false;
    }
  }

  // Process glyphs
  size_t glyph_idx = 0;
  while (glyph_idx < glyphs.size()) {
    // Calculate how many glyphs fit on current line
    int remaining_width = available_width - (ctx.pen_x - block_ctx.effective_margin_left);

    // Find line break
    text_layout_utils::LineBreakMeasureResult break_result;
    if (node->style.white_space == 1 || node->style.white_space == 3) {
      // Preformatted - break at newlines
      break_result = text_layout_utils::FindPreformattedLineBreakAndMeasure(
        glyphs, glyph_idx, remaining_width);
    } else {
      // Normal - break at word boundaries
      break_result = text_layout_utils::FindLineBreakAndMeasure(
        glyphs, glyph_idx, remaining_width);
    }

    if (break_result.end_index == glyph_idx) {
      // Nothing fits - commit current line and try again
      if (!ctx.current_line.empty()) {
        CommitLine(ctx, page);

        // Check if we're out of space
        if (!LineWouldFit(ctx.pen_y, node->style.line_height, ctx)) {
          // Page full - save position and return
          page.end_position = PageStart(node, start_offset + glyph_idx);
          return;
        }

        // Reset pen for new line
        ctx.pen_x = block_ctx.effective_margin_left;
        continue;
      } else {
        // Force at least one glyph
        break_result.end_index = glyph_idx + 1;
        break_result.width = glyphs[glyph_idx].advance;
      }
    }

    // Create fragment for this segment
    LineFragment frag;
    frag.source_node = node;
    frag.text_start = start_offset + glyph_idx;
    frag.text_length = break_result.end_index - glyph_idx;
    frag.glyphs.assign(glyphs.begin() + glyph_idx, glyphs.begin() + break_result.end_index);
    frag.x = ctx.pen_x;
    frag.y = ctx.pen_y;
    frag.width = break_result.width;
    frag.color = node->style.text_color;
    frag.is_link = node->is_link;
    frag.href_id = node->href_id;

    // Update line height
    int line_height = node->style.line_height;
    if (line_height < node->style.font_size) {
      line_height = node->style.font_size;
    }

    if (line_height > ctx.current_line_height) {
      ctx.current_line_height = line_height;
    }

    // Add fragment to current line
    ctx.current_line.push_back(frag);
    ctx.pen_x += break_result.width;

    // Advance glyph index
    glyph_idx = break_result.end_index;

    // Check if we hit a newline (for preformatted text)
    if (break_result.end_index < glyphs.size() &&
        glyphs[break_result.end_index].text.codepoint == '\n') {
      glyph_idx++; // skip the newline
      CommitLine(ctx, page);

      if (!LineWouldFit(ctx.pen_y, ctx.current_line_height, ctx)) {
        page.end_position = PageStart(node, start_offset + glyph_idx);
        return;
      }

      ctx.pen_x = block_ctx.effective_margin_left;
    }
    // Check if line is full
    else if (ctx.pen_x >= block_ctx.effective_margin_left + available_width) {
      CommitLine(ctx, page);

      if (!LineWouldFit(ctx.pen_y, ctx.current_line_height, ctx)) {
        page.end_position = PageStart(node, start_offset + glyph_idx);
        return;
      }

      ctx.pen_x = block_ctx.effective_margin_left;
    }
  }

  // Mark end of this text node
  if (glyph_idx >= glyphs.size()) {
    page.end_position = PageStart(node, node->text_utf8.size());
  }
}

void LayoutEngine::EnterBlock(
  const content_tree::ComputedStyle& style,
  LayoutContext& ctx
) {
  // Add top margin
  AddVerticalSpace(style.margin_top, ctx);

  // Create block context
  BlockContext block_ctx;

  // Calculate effective margins
  if (ctx.block_stack.empty()) {
    block_ctx.effective_margin_left = ctx.metrics.base_margin_left + style.margin_left;
    block_ctx.effective_margin_right = ctx.metrics.base_margin_right + style.margin_right;
  } else {
    const BlockContext& parent = ctx.block_stack.back();
    block_ctx.effective_margin_left = parent.effective_margin_left + style.margin_left;
    block_ctx.effective_margin_right = parent.effective_margin_right + style.margin_right;
  }

  block_ctx.text_indent = style.text_indent;
  block_ctx.first_line = true;

  // Push context
  ctx.block_stack.push_back(block_ctx);

  // Reset pen x to new margin
  ctx.pen_x = block_ctx.effective_margin_left;
}

void LayoutEngine::ExitBlock(
  const content_tree::ComputedStyle& style,
  LayoutContext& ctx
) {
  // Pop context
  if (!ctx.block_stack.empty()) {
    ctx.block_stack.pop_back();
  }

  // Add bottom margin
  AddVerticalSpace(style.margin_bottom, ctx);

  // Reset pen x
  if (!ctx.block_stack.empty()) {
    ctx.pen_x = ctx.block_stack.back().effective_margin_left;
  } else {
    ctx.pen_x = ctx.metrics.base_margin_left;
  }
}

void LayoutEngine::CommitLine(
  LayoutContext& ctx,
  LayoutPage& page
) {
  if (ctx.current_line.empty()) return;

  LayoutLine line;
  line.y = ctx.pen_y;
  line.height = ctx.current_line_height + ctx.metrics.line_spacing;
  line.baseline = ctx.current_line_baseline;
  line.fragments = ctx.current_line;

  page.lines.push_back(line);

  // Advance to next line
  ctx.pen_y += line.height;

  // Reset line state
  ctx.current_line.clear();
  ctx.current_line_height = 0;
  ctx.current_line_baseline = 0;
}

bool LayoutEngine::LineWouldFit(
  int pen_y,
  int line_height,
  const LayoutContext& ctx
) const {
  int bottom = pen_y + line_height + ctx.metrics.line_spacing;
  return bottom <= (ctx.metrics.screen_height - ctx.metrics.base_margin_bottom);
}

void LayoutEngine::AddVerticalSpace(
  int space,
  LayoutContext& ctx
) {
  if (space > 0) {
    ctx.pen_y += space;
  }
}

} // namespace layout_engine
