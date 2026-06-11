#pragma once

#include "settings/prefs_button_ids.h"

namespace settings {

unsigned char VisiblePrefsButtonCount(bool from_book,
                                      bool include_line_wrap_fix);
int PrefsButtonForVisibleSlot(bool from_book, bool include_line_wrap_fix,
                              unsigned char slot);
unsigned char ExtraPrefsButtonCount();
int ExtraPrefsButtonForSlot(unsigned char slot);
unsigned char BookPrefsPage2ButtonCount(bool fixed_layout);
int BookPrefsPage2ButtonForSlot(bool fixed_layout, unsigned char slot);

} // namespace settings
