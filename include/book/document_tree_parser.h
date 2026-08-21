/*
    3dslibris - document_tree_parser.h
    Expat XML parser integration for building DocumentTree
*/

#pragma once

#include "book/content_node.h"
#include "book/css_parser.h"
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

  TreeParserState()
    : doc_tree(nullptr),
      current_node(nullptr),
      lang("en")
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
// DSLIBRIS_DEBUG logging.
bool ParseDocumentToTree(
  const char* html_content,
  size_t html_len,
  content_tree::DocumentTree* out_tree,
  IStatusReporter* reporter = nullptr
);

} // namespace document_tree_parser
