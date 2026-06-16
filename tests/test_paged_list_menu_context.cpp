#include "menus/paged_list_menu_context.h"

#include <cstdio>
#include <cstdlib>

static int g_calls = 0;

static Book *GetBook(void *) {
  g_calls |= 1;
  return reinterpret_cast<Book *>(0x10);
}

static void ShowCurrent(void *) { g_calls |= 2; }
static void ShowSettings(void *, bool from_book) {
  if (from_book)
    g_calls |= 4;
}
static void RequestStatus(void *) { g_calls |= 8; }
static bool IsBookSettings(void *) {
  g_calls |= 16;
  return true;
}
static touchPosition MapTouch(void *, const FrameInput &) {
  g_calls |= 32;
  touchPosition pos = {12, 34};
  return pos;
}

static void Expect(bool condition, const char *message) {
  if (!condition) {
    std::fprintf(stderr, "%s\n", message);
    std::exit(1);
  }
}

int main() {
  PagedListMenuContext context = {};
  context.userdata = reinterpret_cast<void *>(0x99);
  context.get_current_book = GetBook;
  context.show_current_book_view = ShowCurrent;
  context.show_settings_view = ShowSettings;
  context.request_status_redraw = RequestStatus;
  context.is_book_settings_context = IsBookSettings;
  context.map_touch = MapTouch;
  context.keys.a = 1;
  context.keys.b = 2;

  Expect(context.GetCurrentBook() == reinterpret_cast<Book *>(0x10),
         "current book");
  context.ShowCurrentBookView();
  context.ShowSettingsView(true);
  context.RequestStatusRedraw();
  Expect(context.IsBookSettingsContext(), "book settings context");
  const touchPosition pos = context.MapTouch(FrameInput());
  Expect(pos.px == 12 && pos.py == 34, "mapped touch");
  Expect(context.keys.a == 1 && context.keys.b == 2, "key map");
  Expect(g_calls == 63, "all callbacks called");

  PagedListMenuContext empty = {};
  Expect(empty.GetCurrentBook() == nullptr, "null current book");
  empty.ShowCurrentBookView();
  empty.ShowSettingsView(false);
  empty.RequestStatusRedraw();
  Expect(!empty.IsBookSettingsContext(), "default book settings context");
  const touchPosition empty_pos = empty.MapTouch(FrameInput());
  Expect(empty_pos.px == 0 && empty_pos.py == 0, "default mapped touch");

  std::printf("All paged_list_menu_context tests passed.\n");
  return 0;
}
