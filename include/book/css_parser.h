/*
    3dslibris - css_parser.h
    CSS parser for inline styles and default element styles
*/

#pragma once

#include "book/content_node.h"
#include <string>
#include <cstring>

namespace css_parser {

// Parse inline CSS string and apply to style
// Example: "margin-left: 40px; text-indent: 20px; color: #FF0000"
void ParseInlineCSS(const char* css, content_tree::ComputedStyle* style);

// Apply default styles based on HTML tag
void ApplyDefaultTagStyles(const char* tag, content_tree::ComputedStyle* style);

// Parse color value (supports hex and named colors)
// Returns RGB565
u16 ParseColor(const char* value);

// Parse pixel value from "40px", "2em", "1.5rem", etc.
// Currently only supports px units
int ParsePixels(const char* value);

// Parse font size
int ParseFontSize(const char* value);

// Trim whitespace from string
std::string Trim(const std::string& str);

} // namespace css_parser
