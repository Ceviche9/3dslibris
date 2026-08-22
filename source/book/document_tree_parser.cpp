/*
    3dslibris - document_tree_parser.cpp
    Document tree parser implementation
*/

#include "book/document_tree_parser.h"
#include "book/book_xml_css_style_utils.h"
#include "shared/debug_log.h"
#include <cstring>

namespace document_tree_parser {

namespace {

// Fixed approximation of the display width external CSS percent/em lengths
// resolve against - the real per-book screen width isn't known yet at parse
// time (this runs once at book-open, long before any Text/LayoutMetrics
// exists). Percent-based margins are rare in EPUB body CSS, so this only
// affects a minority of books and only approximately.
const int kNominalContainerWidthPx = 240;

int ResolveLengthPx(const book_xml_css_style_utils::MarginTopResult& mtr,
                    int font_size_px) {
  return book_xml_css_style_utils::ResolveHorizontalMarginPx(
      mtr, kNominalContainerWidthPx, font_size_px);
}

// Maps parsed external-CSS rules (tag + class, already merged by the caller)
// onto a node's ComputedStyle. Only touches fields the rules actually set,
// so it composes correctly with the tag defaults already applied and the
// inline style="" that gets layered on top of this by the caller afterward.
void ApplyCssClassMargins(const epub_css_class_map::CssClassMargins& css,
                          content_tree::ComputedStyle* style) {
  using book_xml_css_style_utils::TextAlign;
  using book_xml_css_style_utils::WhiteSpaceMode;
  using book_xml_css_style_utils::TextTransform;
  using MTUnit = book_xml_css_style_utils::MarginTopResult::Unit;

  if (!style) return;

  if (css.margin_top.unit != MTUnit::None)
    style->margin_top = ResolveLengthPx(css.margin_top, style->font_size);
  if (css.margin_bottom.unit != MTUnit::None)
    style->margin_bottom = ResolveLengthPx(css.margin_bottom, style->font_size);
  if (css.margin_left.unit != MTUnit::None)
    style->margin_left = ResolveLengthPx(css.margin_left, style->font_size);
  if (css.margin_right.unit != MTUnit::None)
    style->margin_right = ResolveLengthPx(css.margin_right, style->font_size);
  if (css.text_indent.unit != MTUnit::None)
    style->text_indent = ResolveLengthPx(css.text_indent, style->font_size);

  if (css.font_size.unit != book_xml_css_style_utils::FontSizeSpec::Unit::None) {
    style->font_size = book_xml_css_style_utils::ResolveFontSizePx(
        css.font_size, style->font_size, 16);
  }

  if (css.has_text_align) {
    switch (css.text_align) {
      case TextAlign::Left:    style->text_align = 0; break;
      case TextAlign::Center:  style->text_align = 1; break;
      case TextAlign::Right:   style->text_align = 2; break;
      case TextAlign::Justify: style->text_align = 3; break;
    }
  }

  if (css.has_white_space) {
    switch (css.white_space) {
      case WhiteSpaceMode::Normal:  style->white_space = 0; break;
      case WhiteSpaceMode::Pre:     style->white_space = 1; break;
      case WhiteSpaceMode::Nowrap:  style->white_space = 2; break;
      case WhiteSpaceMode::PreWrap: style->white_space = 3; break;
      // No dedicated slot for pre-line (collapse spaces, keep newlines) -
      // pre-wrap is the closest available behavior in this engine.
      case WhiteSpaceMode::PreLine: style->white_space = 3; break;
    }
  }

  if (css.has_text_transform) {
    switch (css.text_transform) {
      case TextTransform::None:       style->text_transform = 0; break;
      case TextTransform::Uppercase:  style->text_transform = 1; break;
      case TextTransform::Lowercase:  style->text_transform = 2; break;
      case TextTransform::Capitalize: style->text_transform = 3; break;
    }
  }

  if (css.force_bold) style->font_weight = 700;
  if (css.reset_bold) style->font_weight = 400;
  if (css.force_italic) style->font_style = 1;
  if (css.reset_italic) style->font_style = 0;
  if (css.no_underline) style->text_decoration = 0;

  if (css.is_display_none) style->display = 2;
  else if (css.is_display_block) style->display = 1;
}

} // namespace

void InitTreeParserState(TreeParserState* state, content_tree::DocumentTree* tree) {
  state->doc_tree = tree;
  state->current_node = tree->root;
  state->node_stack.clear();
  state->style_stack.clear();
  state->text_buffer.clear();

  // Push root onto stack
  state->node_stack.push_back(tree->root);
  state->style_stack.push_back(tree->root->style);
}

content_tree::ContentNode::Type DetermineNodeType(const char* tag) {
  // Block elements - structural
  if (strcmp(tag, "html") == 0 || strcmp(tag, "head") == 0 ||
      strcmp(tag, "body") == 0) {
    return content_tree::ContentNode::BLOCK;
  }

  // Block elements - content
  if (strcmp(tag, "p") == 0 || strcmp(tag, "div") == 0 ||
      strcmp(tag, "section") == 0 || strcmp(tag, "article") == 0 ||
      strcmp(tag, "blockquote") == 0 || strcmp(tag, "header") == 0 ||
      strcmp(tag, "footer") == 0 || strcmp(tag, "aside") == 0 ||
      strcmp(tag, "nav") == 0 || strcmp(tag, "main") == 0 ||
      strcmp(tag, "ul") == 0 || strcmp(tag, "ol") == 0 ||
      strcmp(tag, "dl") == 0 || strcmp(tag, "table") == 0) {
    return content_tree::ContentNode::BLOCK;
  }

  // Headings
  if (strcmp(tag, "h1") == 0 || strcmp(tag, "h2") == 0 ||
      strcmp(tag, "h3") == 0 || strcmp(tag, "h4") == 0 ||
      strcmp(tag, "h5") == 0 || strcmp(tag, "h6") == 0) {
    return content_tree::ContentNode::HEADING;
  }

  // Preformatted
  if (strcmp(tag, "pre") == 0 || strcmp(tag, "code") == 0) {
    return content_tree::ContentNode::PREFORMATTED;
  }

  // Image
  if (strcmp(tag, "img") == 0) {
    return content_tree::ContentNode::IMAGE;
  }

  // List item
  if (strcmp(tag, "li") == 0) {
    return content_tree::ContentNode::LIST_ITEM;
  }

  // Table cell
  if (strcmp(tag, "td") == 0 || strcmp(tag, "th") == 0) {
    return content_tree::ContentNode::TABLE_CELL;
  }

  // Inline elements
  return content_tree::ContentNode::INLINE;
}

void ApplyAttributesAndStyle(
  content_tree::ContentNode* node,
  const char* tag,
  const char** attr,
  TreeParserState* state
) {
  // Apply default tag styles
  css_parser::ApplyDefaultTagStyles(tag, &node->style);

  // Apply external stylesheet rules (tag selector, then class selectors -
  // lower specificity than the inline style="" handled below in the same
  // attribute pass).
  if (state->css_class_map) {
    epub_css_class_map::CssClassMargins css;
    const bool has_tag_rule =
        epub_css_class_map::LookupAllForTag(tag, *state->css_class_map, &css);
    const char* class_attr = nullptr;
    for (int i = 0; attr && attr[i]; i += 2) {
      if (strcmp(attr[i], "class") == 0) {
        class_attr = attr[i + 1];
        break;
      }
    }
    bool has_class_rule = false;
    if (class_attr && class_attr[0]) {
      has_class_rule = epub_css_class_map::MergeClassRulesToStyle(
          class_attr, *state->css_class_map, &css);
    }
    if (has_tag_rule || has_class_rule) {
      ApplyCssClassMargins(css, &node->style);
    }
  }

  // Process attributes
  for (int i = 0; attr && attr[i]; i += 2) {
    const char* attr_name = attr[i];
    const char* attr_value = attr[i + 1];

    if (strcmp(attr_name, "style") == 0) {
      // Parse inline CSS
      css_parser::ParseInlineCSS(attr_value, &node->style);
    } else if (strcmp(attr_name, "href") == 0) {
      // Link - we'll set a placeholder href_id
      node->is_link = true;
      node->href_id = 1; // TODO: proper href registry
    } else if (strcmp(attr_name, "src") == 0 && strcmp(tag, "img") == 0) {
      // Image - placeholder image_id
      node->image_id = 1; // TODO: proper image registry
    } else if (strcmp(attr_name, "lang") == 0 || strcmp(attr_name, "xml:lang") == 0) {
      // Language
      state->lang = attr_value;
    }
  }
}

void HandleStartElement(TreeParserState* state, const char* name, const char** attr) {
  if (!state || !state->doc_tree) return;

  // Flush any pending text
  FlushTextBuffer(state);

  // Determine node type
  content_tree::ContentNode::Type node_type = DetermineNodeType(name);

  // Create new node
  auto* node = state->doc_tree->CreateNode(node_type);
  node->tag_name = name;

  // Inherit style from parent
  if (!state->style_stack.empty()) {
    node->style = state->style_stack.back();
  }

  // Apply attributes and CSS
  ApplyAttributesAndStyle(node, name, attr, state);

  // Add to parent
  if (!state->node_stack.empty()) {
    state->node_stack.back()->AddChild(node);
  } else {
    state->doc_tree->root->AddChild(node);
  }

  // Push onto stacks
  state->node_stack.push_back(node);
  state->style_stack.push_back(node->style);
  state->current_node = node;
}

void HandleEndElement(TreeParserState* state, const char* name) {
  if (!state || !state->doc_tree) return;

  // Flush any pending text
  FlushTextBuffer(state);

  // Pop from stacks
  if (!state->node_stack.empty()) {
    state->node_stack.pop_back();
  }
  if (!state->style_stack.empty()) {
    state->style_stack.pop_back();
  }

  // Update current node
  if (!state->node_stack.empty()) {
    state->current_node = state->node_stack.back();
  } else {
    state->current_node = nullptr;
  }
}

void HandleCharacterData(TreeParserState* state, const char* txt, int len) {
  if (!state || !state->current_node) return;

  // Accumulate text in buffer
  state->text_buffer.append(txt, len);
}

void FlushTextBuffer(TreeParserState* state) {
  if (!state || state->text_buffer.empty() || !state->current_node) return;

  // Trim whitespace for non-preformatted nodes
  std::string text = state->text_buffer;

  if (state->current_node->style.white_space == 0) {
    // Normal whitespace - collapse whitespace
    bool in_space = false;
    std::string collapsed;

    for (char c : text) {
      if (isspace(c)) {
        if (!in_space) {
          collapsed += ' ';
          in_space = true;
        }
      } else {
        collapsed += c;
        in_space = false;
      }
    }

    text = collapsed;
  }

  // Skip empty text nodes
  if (text.empty() || (text.size() == 1 && isspace(text[0]))) {
    state->text_buffer.clear();
    return;
  }

  // Create text node
  auto* text_node = state->doc_tree->CreateNode(content_tree::ContentNode::TEXT);
  text_node->text_utf8 = text;
  text_node->style = state->current_node->style;

  // Add to current node
  state->current_node->AddChild(text_node);

  // Clear buffer
  state->text_buffer.clear();
}

// Expat callback wrappers
static void ExpatStartElement(void* userData, const char* name, const char** atts) {
  TreeParserState* state = static_cast<TreeParserState*>(userData);
  HandleStartElement(state, name, atts);
}

static void ExpatEndElement(void* userData, const char* name) {
  TreeParserState* state = static_cast<TreeParserState*>(userData);
  HandleEndElement(state, name);
}

static void ExpatCharacterData(void* userData, const char* s, int len) {
  TreeParserState* state = static_cast<TreeParserState*>(userData);
  HandleCharacterData(state, s, len);
}

bool ParseDocumentToTree(
  const char* html_content,
  size_t html_len,
  content_tree::DocumentTree* out_tree,
  IStatusReporter* reporter,
  const epub_css_class_map::CssClassMap* css_class_map
) {
  if (!html_content || html_len == 0 || !out_tree) {
    DBG_LOGF_CAT(reporter, DBG_LEVEL_ERROR, DBG_CAT_EPUB,
                 "TREE-PARSER: bad args html=%p len=%u tree=%p",
                 (const void*)html_content, (unsigned)html_len,
                 (const void*)out_tree);
    return false;
  }

  const size_t nodes_before = out_tree->all_nodes.size();

  // Initialize parser state
  TreeParserState state;
  InitTreeParserState(&state, out_tree);
  if (css_class_map && !css_class_map->empty())
    state.css_class_map = css_class_map;

  // Create Expat parser
  XML_Parser parser = XML_ParserCreate(NULL);
  if (!parser) {
    DBG_LOGF_CAT(reporter, DBG_LEVEL_ERROR, DBG_CAT_EPUB,
                 "TREE-PARSER: XML_ParserCreate failed");
    return false;
  }

  XML_SetUserData(parser, &state);
  XML_SetElementHandler(parser, ExpatStartElement, ExpatEndElement);
  XML_SetCharacterDataHandler(parser, ExpatCharacterData);

  // Parse
  int status = XML_Parse(parser, html_content, html_len, 1);

  if (status == XML_STATUS_ERROR) {
    DBG_LOGF_CAT(reporter, DBG_LEVEL_WARN, DBG_CAT_EPUB,
                 "TREE-PARSER: xml-err=%s line=%lu col=%lu len=%u",
                 XML_ErrorString(XML_GetErrorCode(parser)),
                 (unsigned long)XML_GetCurrentLineNumber(parser),
                 (unsigned long)XML_GetCurrentColumnNumber(parser),
                 (unsigned)html_len);
  }

  // Flush any remaining text
  FlushTextBuffer(&state);

  XML_ParserFree(parser);

  const size_t nodes_added = out_tree->all_nodes.size() - nodes_before;
  DBG_LOGF_CAT(reporter, DBG_LEVEL_INFO, DBG_CAT_EPUB,
               "TREE-PARSER: done ok=%d nodes+=%u total_nodes=%u chars=%u",
               status != XML_STATUS_ERROR, (unsigned)nodes_added,
               (unsigned)out_tree->all_nodes.size(),
               (unsigned)content_tree::CountTextChars(out_tree->root));

  return status != XML_STATUS_ERROR;
}

} // namespace document_tree_parser
