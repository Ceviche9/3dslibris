#pragma once

#include <3ds.h>
#include <string>

#include "app/frame_input.h"
#include "app/key_map.h"
#include "menus/menu_context.h"

struct FontMenuContext {
  typedef void (*ShowSettingsViewFn)(void *userdata, bool from_book);
  typedef bool (*IsBookSettingsContextFn)(void *userdata);
  typedef touchPosition (*MapTouchFn)(void *userdata, const FrameInput &input);
  typedef void (*PrintStatusFn)(void *userdata, const char *message);
  typedef void (*MarkBookLayoutDirtyFn)(void *userdata);
  typedef void (*RefreshPrefsButtonFn)(void *userdata, int button_id);
  typedef int (*WritePrefsFn)(void *userdata);

  MenuContext base;
  void *userdata;
  const std::string *font_dir;
  KeyMap keys;
  ShowSettingsViewFn show_settings_view;
  IsBookSettingsContextFn is_book_settings_context;
  MapTouchFn map_touch;
  PrintStatusFn print_status;
  MarkBookLayoutDirtyFn mark_book_layout_dirty;
  RefreshPrefsButtonFn refresh_prefs_button;
  WritePrefsFn write_prefs;

  std::string FontDir() const {
    return font_dir ? *font_dir : std::string();
  }

  void ShowSettingsView(bool from_book) const {
    if (show_settings_view)
      show_settings_view(userdata, from_book);
  }

  bool IsBookSettingsContext() const {
    return is_book_settings_context ? is_book_settings_context(userdata) : false;
  }

  touchPosition MapTouch(const FrameInput &input) const {
    touchPosition pos = {};
    if (map_touch)
      pos = map_touch(userdata, input);
    return pos;
  }

  void PrintStatus(const char *message) const {
    if (print_status)
      print_status(userdata, message);
  }

  void MarkBookLayoutDirty() const {
    if (mark_book_layout_dirty)
      mark_book_layout_dirty(userdata);
  }

  void RefreshPrefsButton(int button_id) const {
    if (refresh_prefs_button)
      refresh_prefs_button(userdata, button_id);
  }

  int WritePrefs() const {
    return write_prefs ? write_prefs(userdata) : -1;
  }
};
