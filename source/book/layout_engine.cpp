/*
    3dslibris - layout_engine.cpp
    Layout engine implementation
*/

#include "book/layout_engine.h"
#include "shared/debug_log.h"
#include <algorithm>
#include <cstring>

namespace layout_engine {

namespace {

// Next node after `node` (and everything under it) in document order:
// node's next sibling, or - if it has none - the next sibling of the
// nearest ancestor that has one. Returns nullptr once there is nothing
// left in the whole tree (true end of document).
content_tree::ContentNode* NextInDocumentOrder(content_tree::ContentNode* node) {
  while (node && node->parent) {
    std::vector<content_tree::ContentNode*>& siblings = node->parent->children;
    auto it = std::find(siblings.begin(), siblings.end(), node);
    if (it != siblings.end()) {
      ++it;
      if (it != siblings.end()) return *it;
    }
    node = node->parent;
  }
  return nullptr;
}

} // namespace

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
  ctx.reporter = reporter;
  ctx.global_chars_consumed = start.global_offset;
  ctx.pen_y = metrics.base_margin_top;
  ctx.current_line_height = 0;
  ctx.current_line_baseline = 0;

  // Walk forward in document order starting at (start.node, start.char_offset)
  // until the page fills up or there is nothing left. LayoutNode() only lays
  // out one node (and its own descendants); it never continues on to that
  // node's siblings by itself, so that has to happen here - otherwise
  // resuming mid-document (which is the normal case for every page after
  // the first) stops after a single node even when the page still has room.
  content_tree::ContentNode* current = start.node;
  size_t offset = start.char_offset;
  bool page_full = false;
  while (current) {
    // Rebuild the ancestor block/margin stack for THIS node every time,
    // not just once at the top of the function. EnterBlock()/ExitBlock()
    // stay balanced across a single LayoutNode() call's own recursion, but
    // NextInDocumentOrder() can jump sideways to a sibling (or an ancestor's
    // sibling) outside that recursion - reusing the previous node's stack
    // there applied a stale ancestor's margin to every following block,
    // shrinking available width until lines broke after almost every word.
    ctx.block_stack = BuildBlockStackForNode(current, metrics);
    if (ctx.current_line.empty()) {
      ctx.pen_x = ctx.block_stack.empty()
                      ? metrics.base_margin_left
                      : ctx.block_stack.back().effective_margin_left;
    }
    page_full = LayoutNode(current, ctx, page, offset);
    offset = 0;
    if (page_full) break;
    current = NextInDocumentOrder(current);
  }

  // Commit any pending line
  if (!ctx.current_line.empty()) {
    CommitLine(ctx, page);
  }

  if (!current) {
    // Walked off the end of the whole tree - this really is the last page,
    // regardless of whatever the last node's own bookkeeping left behind.
    page.end_position = PageStart();
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

bool LayoutEngine::LayoutNode(
  content_tree::ContentNode* node,
  LayoutContext& ctx,
  LayoutPage& page,
  size_t start_offset
) {
  if (!node) return false;

  // Skip if display:none
  if (node->style.display == 2) return false;

  // Skip non-visual elements
  if (!node->tag_name.empty()) {
    if (node->tag_name == "head" || node->tag_name == "script" ||
        node->tag_name == "style" || node->tag_name == "meta" ||
        node->tag_name == "link" || node->tag_name == "title") {
      return false;
    }
  }

  // Handle TEXT nodes
  if (node->type == content_tree::ContentNode::TEXT) {
    return LayoutTextNode(node, ctx, page, start_offset);
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
      if (LayoutNode(child, ctx, page, 0)) {
        // Child already set page.end_position itself; just propagate.
        return true;
      }

      // If page is full, stop
      if (!ctx.current_line.empty() && !LineWouldFit(ctx.pen_y, ctx.current_line_height, ctx)) {
        CommitLine(ctx, page);
        page.end_position = PageStart(child, 0);
        page.end_position.global_offset = ctx.global_chars_consumed;
        return true;
      }
    }

    // Commit pending line before exiting block
    if (!ctx.current_line.empty()) {
      CommitLine(ctx, page);
    }

    ExitBlock(node->style, ctx);
    return false;
  }

  // Handle INLINE nodes - layout children inline
  for (auto* child : node->children) {
    if (LayoutNode(child, ctx, page, 0))
      return true;
  }
  return false;
}

bool LayoutEngine::LayoutTextNode(
  content_tree::ContentNode* node,
  LayoutContext& ctx,
  LayoutPage& page,
  size_t start_offset
) {
  if (!node || node->text_utf8.empty()) return false;

  // Global offset at (node, start_offset), i.e. before any glyph in this
  // call has been consumed. Deltas of glyph_idx below apply equally to the
  // node-relative offset (start_offset + glyph_idx) and this baseline.
  const size_t node_start_global = ctx.global_chars_consumed;

  const char* text = node->text_utf8.c_str() + start_offset;
  size_t remaining_len = node->text_utf8.size() - start_offset;

  if (remaining_len == 0) return false;

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
    return false;
  }

  if (glyphs.empty()) return false;

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

  DBG_LOGF_CAT(ctx.reporter, DBG_LEVEL_DEBUG, DBG_CAT_LAYOUT,
               "LAYOUT: width-calc parent_tag=%s screen_w=%d margin_l=%d "
               "margin_r=%d avail_w=%d font_sz=%d line_h=%d stack_depth=%u "
               "glyph0_adv=%d glyph0_cp=%u",
               (node->parent && !node->parent->tag_name.empty())
                   ? node->parent->tag_name.c_str() : "?",
               ctx.metrics.screen_width, block_ctx.effective_margin_left,
               block_ctx.effective_margin_right, available_width,
               node->style.font_size, node->style.line_height,
               (unsigned)ctx.block_stack.size(), glyphs[0].advance,
               (unsigned)glyphs[0].text.codepoint);

  // Apply text-indent on first line of block
  if (block_ctx.first_line && node->style.text_indent > 0) {
    ctx.pen_x += node->style.text_indent;
    available_width -= node->style.text_indent;

    // Mark first line as used
    if (!ctx.block_stack.empty()) {
      ctx.block_stack.back().first_line = false;
    }
  }

  const bool is_preformatted =
      node->style.white_space == 1 || node->style.white_space == 3;

  // Process glyphs
  size_t glyph_idx = 0;
  while (glyph_idx < glyphs.size()) {
    // FindLineBreakAndMeasure() stops AT the whitespace glyph right after
    // each word (end_index == that glyph's own index, width == 0 for that
    // call) rather than past it - it measures one word run per call and
    // expects the caller to consume the separator before asking for the
    // next word. Skipping it here (not for preformatted text, which needs
    // to preserve spacing exactly) avoids re-querying with glyph_idx sitting
    // on whitespace, which always looks like "nothing fits" (0 width) and
    // previously forced a line break after every single word.
    if (!is_preformatted && glyphs[glyph_idx].text.whitespace) {
      ctx.pen_x += glyphs[glyph_idx].advance;
      glyph_idx++;
      continue;
    }

    // Calculate how many glyphs fit on current line
    int remaining_width = available_width - (ctx.pen_x - block_ctx.effective_margin_left);

    // Find line break
    text_layout_utils::LineBreakMeasureResult break_result;
    if (is_preformatted) {
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
          page.end_position.global_offset = node_start_global + glyph_idx;
          return true;
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
        page.end_position.global_offset = node_start_global + glyph_idx;
        return true;
      }

      ctx.pen_x = block_ctx.effective_margin_left;
    }
    // Check if line is full
    else if (ctx.pen_x >= block_ctx.effective_margin_left + available_width) {
      CommitLine(ctx, page);

      if (!LineWouldFit(ctx.pen_y, ctx.current_line_height, ctx)) {
        page.end_position = PageStart(node, start_offset + glyph_idx);
        page.end_position.global_offset = node_start_global + glyph_idx;
        return true;
      }

      ctx.pen_x = block_ctx.effective_margin_left;
    }
  }

  // Mark end of this text node, and advance the running global offset so
  // the next node (sibling/parent's next child) starts counting from here.
  ctx.global_chars_consumed = node_start_global + glyph_idx;
  if (glyph_idx >= glyphs.size()) {
    page.end_position = PageStart(node, node->text_utf8.size());
    page.end_position.global_offset = ctx.global_chars_consumed;
  }
  return false;
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

std::vector<BlockContext> LayoutEngine::BuildBlockStackForNode(
  content_tree::ContentNode* node,
  const LayoutMetrics& metrics
) const {
  std::vector<content_tree::ContentNode*> ancestors;
  for (content_tree::ContentNode* p = node ? node->parent : nullptr; p;
       p = p->parent) {
    if (p->IsBlock()) ancestors.push_back(p);
  }

  std::vector<BlockContext> stack;
  int margin_left = metrics.base_margin_left;
  int margin_right = metrics.base_margin_right;
  // ancestors is innermost-first; walk it in reverse for outermost-first,
  // matching the accumulation order EnterBlock() uses when descending.
  for (auto it = ancestors.rbegin(); it != ancestors.rend(); ++it) {
    content_tree::ContentNode* blk = *it;
    BlockContext block_ctx;
    block_ctx.effective_margin_left = margin_left + blk->style.margin_left;
    block_ctx.effective_margin_right = margin_right + blk->style.margin_right;
    block_ctx.text_indent = blk->style.text_indent;
    block_ctx.first_line = false;
    stack.push_back(block_ctx);
    margin_left = block_ctx.effective_margin_left;
    margin_right = block_ctx.effective_margin_right;
  }
  return stack;
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
