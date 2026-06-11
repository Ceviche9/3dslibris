#include "settings/prefs_button_context_utils.h"

namespace settings {

namespace {

static const int kGeneralPrefsButtons[] = {
    PREFS_BUTTON_STYLE_CUSTOMIZATION,
    PREFS_BUTTON_TIME24H,
    PREFS_BUTTON_TIME_REMAINING,
    PREFS_BUTTON_COLORMODE,
    PREFS_BUTTON_LIBRARY_VIEW,
    PREFS_BUTTON_LIBRARY_SORT,
};

static const int kGeneralExtraButtons[] = {
    PREFS_BUTTON_ORIENTATION,
    PREFS_BUTTON_HANDEDNESS,
    PREFS_BUTTON_REOPEN_LAST_BOOK,
    PREFS_BUTTON_CIRCLE_PAD_PAGE_TURN,
    PREFS_BUTTON_RESET_DEFAULTS,
    PREFS_BUTTON_CLEAR_CACHE,
};

static const int kBookPrefsButtons[] = {
    PREFS_BUTTON_STYLE_CUSTOMIZATION,
    PREFS_BUTTON_TIME24H,
    PREFS_BUTTON_TIME_REMAINING,
    PREFS_BUTTON_BOOK_INFO,
    PREFS_BUTTON_INDEX,
    PREFS_BUTTON_BOOKMARKS,
};

static const int kBookPrefsButtonsWithBookOption[] = {
    PREFS_BUTTON_STYLE_CUSTOMIZATION,
    PREFS_BUTTON_LIBRARY_VIEW,
    PREFS_BUTTON_TIME24H,
    PREFS_BUTTON_TIME_REMAINING,
    PREFS_BUTTON_BOOK_INFO,
    PREFS_BUTTON_INDEX,
    PREFS_BUTTON_BOOKMARKS,
};

static const int kReflowBookPrefsPage2Buttons[] = {
    PREFS_BUTTON_ORIENTATION,
    PREFS_BUTTON_HANDEDNESS,
    PREFS_BUTTON_FONTSIZE,
    PREFS_BUTTON_LINE_SPACING,
    PREFS_BUTTON_PARASPACING,
    PREFS_BUTTON_PUBLISHER_TEXT_INDENT,
    PREFS_BUTTON_PUBLISHER_BLOCK_MARGINS,
};

static const int kFixedLayoutBookPrefsPage2Buttons[] = {
    PREFS_BUTTON_ORIENTATION,
    PREFS_BUTTON_HANDEDNESS,
    PREFS_BUTTON_LIBRARY_VIEW,
};

} // namespace

unsigned char VisiblePrefsButtonCount(bool from_book,
                                      bool include_line_wrap_fix) {
  if (!from_book)
    return (unsigned char)(sizeof(kGeneralPrefsButtons) / sizeof(kGeneralPrefsButtons[0]));
  if (include_line_wrap_fix)
    return (unsigned char)(sizeof(kBookPrefsButtonsWithBookOption) /
                           sizeof(kBookPrefsButtonsWithBookOption[0]));
  return (unsigned char)(sizeof(kBookPrefsButtons) / sizeof(kBookPrefsButtons[0]));
}

int PrefsButtonForVisibleSlot(bool from_book, bool include_line_wrap_fix,
                              unsigned char slot) {
  if (!from_book)
    return kGeneralPrefsButtons[slot];
  if (include_line_wrap_fix)
    return kBookPrefsButtonsWithBookOption[slot];
  return kBookPrefsButtons[slot];
}

unsigned char ExtraPrefsButtonCount() {
  return (unsigned char)(sizeof(kGeneralExtraButtons) / sizeof(kGeneralExtraButtons[0]));
}

int ExtraPrefsButtonForSlot(unsigned char slot) {
  return kGeneralExtraButtons[slot];
}

unsigned char BookPrefsPage2ButtonCount(bool fixed_layout) {
  if (fixed_layout) {
    return (unsigned char)(sizeof(kFixedLayoutBookPrefsPage2Buttons) /
                           sizeof(kFixedLayoutBookPrefsPage2Buttons[0]));
  }
  return (unsigned char)(sizeof(kReflowBookPrefsPage2Buttons) /
                         sizeof(kReflowBookPrefsPage2Buttons[0]));
}

int BookPrefsPage2ButtonForSlot(bool fixed_layout, unsigned char slot) {
  return fixed_layout ? kFixedLayoutBookPrefsPage2Buttons[slot]
                      : kReflowBookPrefsPage2Buttons[slot];
}

} // namespace settings
