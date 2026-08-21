// FASE 1: Estruturas de Dados para DocumentTree
// Arquivo: include/book/content_node.h

#pragma once

#include <vector>
#include <string>
#include <cstddef>
#include <3ds.h>

namespace content_tree {

// ============================================================================
// ESTILO COMPUTADO (após cascata CSS)
// ============================================================================

struct ComputedStyle {
  // Fonte
  int font_size;           // pixels
  int font_weight;         // 400=normal, 700=bold
  int font_style;          // 0=normal, 1=italic, 2=bold-italic

  // Box model (margens em pixels)
  int margin_top;
  int margin_bottom;
  int margin_left;
  int margin_right;

  int padding_top;
  int padding_bottom;
  int padding_left;
  int padding_right;

  // Texto
  int text_indent;         // primeira linha
  int line_height;         // altura da linha

  // Cores
  u16 text_color;          // RGB565
  u16 bg_color;
  u16 underline_color;

  // Decoração
  u8 text_decoration;      // 0=none, 1=underline, 2=overline, 3=line-through
  u8 underline_style;      // UNDERLINE_STYLE_* (from book_xml_css_style_utils.h)

  // Alinhamento
  u8 text_align;           // 0=left, 1=center, 2=right, 3=justify
  u8 vertical_align;       // 0=baseline, 1=top, 2=middle, 3=bottom

  // Transform
  u8 text_transform;       // 0=none, 1=uppercase, 2=lowercase, 3=capitalize

  // Display
  u8 display;              // 0=inline, 1=block, 2=none, 3=inline-block
  u8 white_space;          // 0=normal, 1=pre, 2=nowrap, 3=pre-wrap

  // RTL/LTR
  bool is_rtl;

  // Default constructor com valores sensatos
  ComputedStyle()
    : font_size(16),
      font_weight(400),
      font_style(0),
      margin_top(0), margin_bottom(0), margin_left(0), margin_right(0),
      padding_top(0), padding_bottom(0), padding_left(0), padding_right(0),
      text_indent(0),
      line_height(20),
      text_color(0x0000),  // preto
      bg_color(0xFFFF),    // branco
      underline_color(0x0000),
      text_decoration(0),
      underline_style(0),
      text_align(0),
      vertical_align(0),
      text_transform(0),
      display(1),  // block por padrão
      white_space(0),
      is_rtl(false)
  {}
};

// ============================================================================
// NÓ DE CONTEÚDO (elemento da árvore)
// ============================================================================

struct ContentNode {
  enum Type {
    TEXT,          // Nó de texto puro
    BLOCK,         // <p>, <div>, <section>, <blockquote>
    INLINE,        // <span>, <em>, <strong>, <a>
    IMAGE,         // <img>
    LIST_ITEM,     // <li>
    TABLE_CELL,    // <td>, <th>
    HEADING,       // <h1>-<h6>
    PREFORMATTED   // <pre>, <code>
  };

  Type type;

  // Texto UTF-8 original (para nós TEXT)
  // Para nós não-TEXT, este campo fica vazio
  std::string text_utf8;

  // Estilo computado para este nó
  ComputedStyle style;

  // Hierarquia
  std::vector<ContentNode*> children;
  ContentNode* parent;

  // Link (para nós INLINE do tipo <a>)
  u16 href_id;
  bool is_link;

  // Imagem (para nós IMAGE)
  u16 image_id;

  // Tag HTML original (para debugging)
  std::string tag_name;

  // Constructor
  ContentNode(Type t = TEXT)
    : type(t),
      parent(nullptr),
      href_id(0),
      is_link(false),
      image_id(0)
  {}

  ~ContentNode() {
    // Limpa children recursivamente
    for (auto* child : children) {
      delete child;
    }
  }

  // Adiciona child
  void AddChild(ContentNode* child) {
    if (!child) return;
    child->parent = this;
    children.push_back(child);
  }

  // Remove child (sem deletar)
  void RemoveChild(ContentNode* child) {
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end()) {
      (*it)->parent = nullptr;
      children.erase(it);
    }
  }

  // Verifica se é block-level
  bool IsBlock() const {
    return type == BLOCK || type == LIST_ITEM ||
           type == TABLE_CELL || type == HEADING ||
           type == PREFORMATTED;
  }

  // Verifica se é inline
  bool IsInline() const {
    return type == TEXT || type == INLINE || type == IMAGE;
  }
};

// ============================================================================
// ÁRVORE DE DOCUMENTO
// ============================================================================

struct DocumentTree {
  ContentNode* root;
  std::vector<ContentNode*> all_nodes;  // ownership, para cleanup fácil

  // Metadados do livro
  struct Metadata {
    std::string title;
    std::string author;
    std::string language;
  } metadata;

  // Bookmarks apontam para posições na árvore
  struct Bookmark {
    ContentNode* node;
    size_t char_offset_in_node;  // offset UTF-8 dentro do text_utf8

    std::string label;  // "Chapter 3", etc.

    bool IsValid() const {
      return node != nullptr;
    }
  };
  std::vector<Bookmark> bookmarks;

  // Constructor
  DocumentTree()
    : root(nullptr)
  {
    root = CreateNode(ContentNode::BLOCK);
    root->tag_name = "body";
  }

  ~DocumentTree() {
    // all_nodes possui ownership, deleta todos
    for (auto* node : all_nodes) {
      delete node;
    }
  }

  // Cria novo nó e adiciona ao registry
  ContentNode* CreateNode(ContentNode::Type type) {
    auto* node = new ContentNode(type);
    all_nodes.push_back(node);
    return node;
  }

  // Encontra nó por offset absoluto (para compatibilidade com sistema antigo)
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

// ============================================================================
// UTILITÁRIOS
// ============================================================================

// Serializa árvore para debug (formato indentado)
inline std::string DebugPrintTree(const ContentNode* node, int indent = 0) {
  if (!node) return "";

  std::string result;
  std::string indent_str(indent * 2, ' ');

  result += indent_str;

  // Tipo do nó
  const char* type_names[] = {
    "TEXT", "BLOCK", "INLINE", "IMAGE", "LIST_ITEM",
    "TABLE_CELL", "HEADING", "PREFORMATTED"
  };
  result += type_names[node->type];

  if (!node->tag_name.empty()) {
    result += " <" + node->tag_name + ">";
  }

  // Estilo relevante
  if (node->style.margin_left > 0) {
    result += " margin-left=" + std::to_string(node->style.margin_left);
  }
  if (node->style.text_indent > 0) {
    result += " text-indent=" + std::to_string(node->style.text_indent);
  }

  // Texto (truncado)
  if (node->type == ContentNode::TEXT) {
    std::string text_preview = node->text_utf8;
    if (text_preview.size() > 40) {
      text_preview = text_preview.substr(0, 40) + "...";
    }
    // Escapa newlines
    size_t pos = 0;
    while ((pos = text_preview.find('\n', pos)) != std::string::npos) {
      text_preview.replace(pos, 1, "\\n");
      pos += 2;
    }
    result += " \"" + text_preview + "\"";
  }

  result += "\n";

  // Children recursivo
  for (auto* child : node->children) {
    result += DebugPrintTree(child, indent + 1);
  }

  return result;
}

// Conta total de caracteres de texto na árvore
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
