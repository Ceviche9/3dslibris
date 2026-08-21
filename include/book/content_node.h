/*
    3dslibris - content_node.h
    New layout architecture - Document Tree structures

    Implements a DOM-like tree structure that preserves document content
    separately from layout decisions, enabling:
    - CSS context preservation across page boundaries
    - Layout recalculation without re-parsing
    - Stable bookmarks (node pointers instead of buffer offsets)
*/

#pragma once

#include <vector>
#include <string>
#include <cstddef>
#include <algorithm>
#include <3ds.h>

namespace content_tree {

// Computed style after CSS cascade
struct ComputedStyle {
  // Font
  int font_size;
  int font_weight;         // 400=normal, 700=bold
  int font_style;          // 0=normal, 1=italic

  // Box model (pixels)
  int margin_top;
  int margin_bottom;
  int margin_left;
  int margin_right;

  int padding_top;
  int padding_bottom;
  int padding_left;
  int padding_right;

  // Text
  int text_indent;         // first-line
  int line_height;

  // Colors (RGB565)
  u16 text_color;
  u16 bg_color;
  u16 underline_color;

  // Decoration
  u8 text_decoration;      // 0=none, 1=underline, 2=line-through
  u8 underline_style;

  // Alignment
  u8 text_align;           // 0=left, 1=center, 2=right, 3=justify
  u8 vertical_align;       // 0=baseline, 1=top, 2=middle, 3=bottom

  // Transform
  u8 text_transform;       // 0=none, 1=uppercase, 2=lowercase, 3=capitalize

  // Display
  u8 display;              // 0=inline, 1=block, 2=none, 3=inline-block
  u8 white_space;          // 0=normal, 1=pre, 2=nowrap, 3=pre-wrap

  // Direction
  bool is_rtl;

  ComputedStyle()
    : font_size(16),
      font_weight(400),
      font_style(0),
      margin_top(0), margin_bottom(0), margin_left(0), margin_right(0),
      padding_top(0), padding_bottom(0), padding_left(0), padding_right(0),
      text_indent(0),
      line_height(20),
      text_color(0x0000),      // black
      bg_color(0xFFFF),        // white
      underline_color(0x0000),
      text_decoration(0),
      underline_style(0),
      text_align(0),
      vertical_align(0),
      text_transform(0),
      display(1),              // block by default
      white_space(0),
      is_rtl(false)
  {}
};

// Content node in the document tree
struct ContentNode {
  enum Type {
    TEXT,
    BLOCK,
    INLINE,
    IMAGE,
    LIST_ITEM,
    TABLE_CELL,
    HEADING,
    PREFORMATTED
  };

  Type type;

  // UTF-8 text (for TEXT nodes)
  std::string text_utf8;

  // Computed style
  ComputedStyle style;

  // Hierarchy
  std::vector<ContentNode*> children;
  ContentNode* parent;

  // Link (for INLINE <a> nodes)
  u16 href_id;
  bool is_link;

  // Image (for IMAGE nodes)
  u16 image_id;

  // Original HTML tag (for debugging)
  std::string tag_name;

  ContentNode(Type t = TEXT)
    : type(t),
      parent(nullptr),
      href_id(0),
      is_link(false),
      image_id(0)
  {}

  // Does not delete children: DocumentTree::all_nodes flat-owns every node
  // and deletes them all in its own destructor. Deleting here too would
  // double-free descendants when the tree tears down.
  ~ContentNode() {}

  void AddChild(ContentNode* child) {
    if (!child) return;
    child->parent = this;
    children.push_back(child);
  }

  void RemoveChild(ContentNode* child) {
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end()) {
      (*it)->parent = nullptr;
      children.erase(it);
    }
  }

  bool IsBlock() const {
    return type == BLOCK || type == LIST_ITEM ||
           type == TABLE_CELL || type == HEADING ||
           type == PREFORMATTED;
  }

  bool IsInline() const {
    return type == TEXT || type == INLINE || type == IMAGE;
  }
};

// Document tree
struct DocumentTree {
  ContentNode* root;
  std::vector<ContentNode*> all_nodes;

  struct Metadata {
    std::string title;
    std::string author;
    std::string language;
  } metadata;

  struct Bookmark {
    ContentNode* node;
    size_t char_offset_in_node;
    std::string label;

    bool IsValid() const {
      return node != nullptr;
    }
  };
  std::vector<Bookmark> bookmarks;

  DocumentTree()
    : root(nullptr)
  {
    root = CreateNode(ContentNode::BLOCK);
    root->tag_name = "body";
  }

  ~DocumentTree() {
    for (auto* node : all_nodes) {
      delete node;
    }
  }

  ContentNode* CreateNode(ContentNode::Type type) {
    auto* node = new ContentNode(type);
    all_nodes.push_back(node);
    return node;
  }

  Bookmark FindNodeByGlobalOffset(size_t global_offset) const {
    size_t current_offset = 0;
    return FindNodeRecursive(root, global_offset, &current_offset);
  }

private:
  Bookmark FindNodeRecursive(ContentNode* node, size_t target_offset,
                             size_t* current_offset) const {
    if (!node) return {nullptr, 0};

    if (node->type == ContentNode::TEXT) {
      size_t node_end = *current_offset + node->text_utf8.size();
      if (target_offset < node_end) {
        return {node, target_offset - *current_offset};
      }
      *current_offset = node_end;
    }

    for (auto* child : node->children) {
      auto result = FindNodeRecursive(child, target_offset, current_offset);
      if (result.IsValid()) {
        return result;
      }
    }

    return {nullptr, 0};
  }
};

// Debug utilities
inline std::string DebugPrintTree(const ContentNode* node, int indent = 0) {
  if (!node) return "";

  std::string result;
  std::string indent_str(indent * 2, ' ');

  result += indent_str;

  const char* type_names[] = {
    "TEXT", "BLOCK", "INLINE", "IMAGE", "LIST_ITEM",
    "TABLE_CELL", "HEADING", "PREFORMATTED"
  };
  result += type_names[node->type];

  if (!node->tag_name.empty()) {
    result += " <" + node->tag_name + ">";
  }

  if (node->style.margin_left > 0) {
    result += " margin-left=" + std::to_string(node->style.margin_left);
  }
  if (node->style.text_indent > 0) {
    result += " text-indent=" + std::to_string(node->style.text_indent);
  }

  if (node->type == ContentNode::TEXT) {
    std::string text_preview = node->text_utf8;
    if (text_preview.size() > 40) {
      text_preview = text_preview.substr(0, 40) + "...";
    }
    size_t pos = 0;
    while ((pos = text_preview.find('\n', pos)) != std::string::npos) {
      text_preview.replace(pos, 1, "\\n");
      pos += 2;
    }
    result += " \"" + text_preview + "\"";
  }

  result += "\n";

  for (auto* child : node->children) {
    result += DebugPrintTree(child, indent + 1);
  }

  return result;
}

inline size_t CountTextChars(const ContentNode* node) {
  if (!node) return 0;

  size_t count = 0;
  if (node->type == ContentNode::TEXT) {
    count += node->text_utf8.size();
  }

  for (auto* child : node->children) {
    count += CountTextChars(child);
  }

  return count;
}

} // namespace content_tree
