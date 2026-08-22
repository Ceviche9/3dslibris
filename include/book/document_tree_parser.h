/*
    3dslibris - document_tree_parser.h
    Expat XML parser integration for building DocumentTree
*/

#pragma once

#include "book/content_node.h"
#include "book/css_parser.h"
#include "book/epub_css_class_map.h"
#include <vector>
#include <string>
#include <expat.h>

class IStatusReporter;

namespace document_tree_parser {

// Parser state for building DocumentTree
struct TreeParserState {
  content_tree::DocumentTree* doc_tree;
  content_tree::ContentNode* current_node;
  std::vector<content_tree::ContentNode*> node_stack;
  std::vector<content_tree::ComputedStyle> style_stack;
  std::string text_buffer;
  std::string lang;

  // Optional. Non-owning; parsed external stylesheet rules for the document
  // currently being parsed (from <link rel="stylesheet">), looked up by tag
  // name and class="" attribute in ApplyAttributesAndStyle(). Null when the
  // caller has none (e.g. no linked stylesheet, or a non-EPUB caller).
  const epub_css_class_map::CssClassMap* css_class_map;

  TreeParserState()
    : doc_tree(nullptr),
      current_node(nullptr),
      lang("en"),
      css_class_map(nullptr)
  {}
};

// Initialize parser state
void InitTreeParserState(TreeParserState* state, content_tree::DocumentTree* tree);

// Expat callbacks
void HandleStartElement(TreeParserState* state, const char* name, const char** attr);
void HandleEndElement(TreeParserState* state, const char* name);
void HandleCharacterData(TreeParserState* state, const char* txt, int len);

// Flush pending text buffer to tree
void FlushTextBuffer(TreeParserState* state);

// Determine node type from tag name
content_tree::ContentNode::Type DetermineNodeType(const char* tag);

// Apply attributes and styles to node
void ApplyAttributesAndStyle(
  content_tree::ContentNode* node,
  const char* tag,
  const char** attr,
  TreeParserState* state
);

// Parse full document. `reporter` is optional and only used for
// DSLIBRIS_DEBUG logging. `css_class_map` is optional: parsed rules from the
// document's linked external stylesheet(s), applied (tag then class) before
// any inline style="" attribute on each element.
bool ParseDocumentToTree(
  const char* html_content,
  size_t html_len,
  content_tree::DocumentTree* out_tree,
  IStatusReporter* reporter = nullptr,
  const epub_css_class_map::CssClassMap* css_class_map = nullptr
);

} // namespace document_tree_parser
