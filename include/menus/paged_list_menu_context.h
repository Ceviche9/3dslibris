#pragma once

#include <3ds.h>

#include "app/frame_input.h"
#include "app/key_map.h"
#include "menus/menu_context.h"

class Book;

struct PagedListMenuContext {
  typedef Book *(*GetCurrentBookFn)(void *userdata);
  typedef void (*ShowCurrentBookViewFn)(void *userdata);
  typedef void (*ShowSettingsViewFn)(void *userdata, bool from_book);
  typedef void (*RequestStatusRedrawFn)(void *userdata);
  typedef bool (*IsBookSettingsContextFn)(void *userdata);
  typedef touchPosition (*MapTouchFn)(void *userdata, const FrameInput &input);

  MenuContext base;
  void *userdata;
  KeyMap keys;
  GetCurrentBookFn get_current_book;
  ShowCurrentBookViewFn show_current_book_view;
  ShowSettingsViewFn show_settings_view;
  RequestStatusRedrawFn request_status_redraw;
  IsBookSettingsContextFn is_book_settings_context;
  MapTouchFn map_touch;

  Book *GetCurrentBook() const {
    return get_current_book ? get_current_book(userdata) : nullptr;
  }

  void ShowCurrentBookView() const {
    if (show_current_book_view)
      show_current_book_view(userdata);
  }

  void ShowSettingsView(bool from_book) const {
    if (show_settings_view)
      show_settings_view(userdata, from_book);
  }

  void RequestStatusRedraw() const {
    if (request_status_redraw)
      request_status_redraw(userdata);
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
};
