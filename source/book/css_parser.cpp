/*
    3dslibris - css_parser.cpp
    CSS parser implementation
*/

#include "book/css_parser.h"
#include <cctype>
#include <cstdlib>

namespace css_parser {

std::string Trim(const std::string& str) {
  size_t start = 0;
  while (start < str.size() && isspace(str[start])) {
    start++;
  }

  size_t end = str.size();
  while (end > start && isspace(str[end - 1])) {
    end--;
  }

  return str.substr(start, end - start);
}

int ParsePixels(const char* value) {
  if (!value || !*value) return 0;

  std::string val_str = Trim(value);

  // Remove "px" suffix
  if (val_str.size() >= 2 && val_str.substr(val_str.size() - 2) == "px") {
    val_str = val_str.substr(0, val_str.size() - 2);
  }

  // TODO: Support em, rem, % etc.
  // For now, just parse as integer
  return atoi(val_str.c_str());
}

int ParseFontSize(const char* value) {
  if (!value || !*value) return 16;

  std::string val_str = Trim(value);

  // Handle pt (points) - rough conversion: 1pt ≈ 1.33px
  if (val_str.size() >= 2 && val_str.substr(val_str.size() - 2) == "pt") {
    val_str = val_str.substr(0, val_str.size() - 2);
    int pt = atoi(val_str.c_str());
    return (pt * 4) / 3;  // pt to px conversion
  }

  // Handle px
  return ParsePixels(value);
}

u16 ParseColor(const char* value) {
  if (!value || !*value) return 0x0000;

  std::string val_str = Trim(value);

  // Hex color: #RGB or #RRGGBB
  if (val_str[0] == '#') {
    const char* hex = val_str.c_str() + 1;
    unsigned int rgb = 0;

    if (strlen(hex) == 3) {
      // #RGB -> expand to #RRGGBB
      sscanf(hex, "%3x", &rgb);
      int r = ((rgb >> 8) & 0xF) * 17;
      int g = ((rgb >> 4) & 0xF) * 17;
      int b = (rgb & 0xF) * 17;
      // Convert RGB888 to RGB565
      return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    } else if (strlen(hex) == 6) {
      // #RRGGBB
      sscanf(hex, "%6x", &rgb);
      int r = (rgb >> 16) & 0xFF;
      int g = (rgb >> 8) & 0xFF;
      int b = rgb & 0xFF;
      // Convert RGB888 to RGB565
      return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    }
  }

  // Named colors
  if (strcmp(val_str.c_str(), "black") == 0) return 0x0000;
  if (strcmp(val_str.c_str(), "white") == 0) return 0xFFFF;
  if (strcmp(val_str.c_str(), "red") == 0) return 0xF800;
  if (strcmp(val_str.c_str(), "green") == 0) return 0x07E0;
  if (strcmp(val_str.c_str(), "blue") == 0) return 0x001F;
  if (strcmp(val_str.c_str(), "gray") == 0 || strcmp(val_str.c_str(), "grey") == 0) return 0x8410;

  return 0x0000; // default to black
}

void ParseInlineCSS(const char* css, content_tree::ComputedStyle* style) {
  if (!css || !*css || !style) return;

  std::string css_str(css);
  size_t pos = 0;

  while (pos < css_str.size()) {
    // Find next declaration: property: value;
    size_t colon = css_str.find(':', pos);
    if (colon == std::string::npos) break;

    size_t semicolon = css_str.find(';', colon);
    if (semicolon == std::string::npos) {
      semicolon = css_str.size();
    }

    std::string property = Trim(css_str.substr(pos, colon - pos));
    std::string value = Trim(css_str.substr(colon + 1, semicolon - colon - 1));

    // Apply property
    if (property == "margin-left") {
      style->margin_left = ParsePixels(value.c_str());
    } else if (property == "margin-right") {
      style->margin_right = ParsePixels(value.c_str());
    } else if (property == "margin-top") {
      style->margin_top = ParsePixels(value.c_str());
    } else if (property == "margin-bottom") {
      style->margin_bottom = ParsePixels(value.c_str());
    } else if (property == "padding-left") {
      style->padding_left = ParsePixels(value.c_str());
    } else if (property == "padding-right") {
      style->padding_right = ParsePixels(value.c_str());
    } else if (property == "padding-top") {
      style->padding_top = ParsePixels(value.c_str());
    } else if (property == "padding-bottom") {
      style->padding_bottom = ParsePixels(value.c_str());
    } else if (property == "text-indent") {
      style->text_indent = ParsePixels(value.c_str());
    } else if (property == "font-size") {
      style->font_size = ParseFontSize(value.c_str());
    } else if (property == "line-height") {
      style->line_height = ParsePixels(value.c_str());
    } else if (property == "color") {
      style->text_color = ParseColor(value.c_str());
    } else if (property == "background-color") {
      style->bg_color = ParseColor(value.c_str());
    } else if (property == "text-align") {
      if (value == "left") style->text_align = 0;
      else if (value == "center") style->text_align = 1;
      else if (value == "right") style->text_align = 2;
      else if (value == "justify") style->text_align = 3;
    } else if (property == "font-weight") {
      if (value == "bold" || value == "700" || value == "800" || value == "900") {
        style->font_weight = 700;
      } else if (value == "normal" || value == "400") {
        style->font_weight = 400;
      }
    } else if (property == "font-style") {
      if (value == "italic" || value == "oblique") {
        style->font_style = 1;
      } else {
        style->font_style = 0;
      }
    } else if (property == "text-decoration") {
      if (value.find("underline") != std::string::npos) {
        style->text_decoration = 1;
      } else if (value.find("line-through") != std::string::npos) {
        style->text_decoration = 2;
      } else {
        style->text_decoration = 0;
      }
    } else if (property == "text-transform") {
      if (value == "uppercase") style->text_transform = 1;
      else if (value == "lowercase") style->text_transform = 2;
      else if (value == "capitalize") style->text_transform = 3;
      else style->text_transform = 0;
    } else if (property == "display") {
      if (value == "inline") style->display = 0;
      else if (value == "block") style->display = 1;
      else if (value == "none") style->display = 2;
      else if (value == "inline-block") style->display = 3;
    } else if (property == "white-space") {
      if (value == "pre") style->white_space = 1;
      else if (value == "nowrap") style->white_space = 2;
      else if (value == "pre-wrap") style->white_space = 3;
      else style->white_space = 0;
    }

    pos = semicolon + 1;
  }
}

void ApplyDefaultTagStyles(const char* tag, content_tree::ComputedStyle* style) {
  if (!tag || !style) return;

  // Block elements
  if (strcmp(tag, "p") == 0) {
    style->margin_bottom = 12;
    style->display = 1;
  } else if (strcmp(tag, "div") == 0 || strcmp(tag, "section") == 0 || strcmp(tag, "article") == 0) {
    style->display = 1;
  } else if (strcmp(tag, "blockquote") == 0) {
    style->margin_left = 30;
    style->margin_right = 30;
    style->margin_top = 12;
    style->margin_bottom = 12;
    style->display = 1;
  }

  // Headings
  else if (strcmp(tag, "h1") == 0) {
    style->font_size = 32;
    style->font_weight = 700;
    style->margin_top = 20;
    style->margin_bottom = 16;
    style->display = 1;
  } else if (strcmp(tag, "h2") == 0) {
    style->font_size = 28;
    style->font_weight = 700;
    style->margin_top = 18;
    style->margin_bottom = 14;
    style->display = 1;
  } else if (strcmp(tag, "h3") == 0) {
    style->font_size = 24;
    style->font_weight = 700;
    style->margin_top = 16;
    style->margin_bottom = 12;
    style->display = 1;
  } else if (strcmp(tag, "h4") == 0) {
    style->font_size = 20;
    style->font_weight = 700;
    style->margin_top = 14;
    style->margin_bottom = 10;
    style->display = 1;
  } else if (strcmp(tag, "h5") == 0) {
    style->font_size = 18;
    style->font_weight = 700;
    style->margin_top = 12;
    style->margin_bottom = 8;
    style->display = 1;
  } else if (strcmp(tag, "h6") == 0) {
    style->font_size = 16;
    style->font_weight = 700;
    style->margin_top = 10;
    style->margin_bottom = 6;
    style->display = 1;
  }

  // Inline styles
  else if (strcmp(tag, "em") == 0 || strcmp(tag, "i") == 0) {
    style->font_style = 1;
    style->display = 0;
  } else if (strcmp(tag, "strong") == 0 || strcmp(tag, "b") == 0) {
    style->font_weight = 700;
    style->display = 0;
  } else if (strcmp(tag, "u") == 0) {
    style->text_decoration = 1;
    style->display = 0;
  } else if (strcmp(tag, "a") == 0) {
    style->text_decoration = 1;
    style->text_color = 0x001F; // blue
    style->display = 0;
  }

  // Preformatted
  else if (strcmp(tag, "pre") == 0 || strcmp(tag, "code") == 0) {
    style->white_space = 1; // pre
    style->display = 1;
  }

  // List item
  else if (strcmp(tag, "li") == 0) {
    style->margin_left = 20;
    style->margin_bottom = 6;
    style->display = 1;
  }
}

} // namespace css_parser
