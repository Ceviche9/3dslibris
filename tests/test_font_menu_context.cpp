#include "settings/font_menu_context.h"

#include <cstdio>
#include <cstdlib>
#include <string>

static int g_calls = 0;
static std::string g_font_dir = "/fonts";

static void ShowSettings(void *, bool from_book) {
  if (from_book)
    g_calls |= 1;
}
static bool IsBookSettings(void *) {
  g_calls |= 2;
  return true;
}
static touchPosition MapTouch(void *, const FrameInput &) {
  g_calls |= 4;
  touchPosition pos = {21, 43};
  return pos;
}
static void PrintStatus(void *, const char *) { g_calls |= 8; }
static void MarkLayout(void *) { g_calls |= 16; }
static void RefreshButton(void *, int id) {
  if (id == 7)
    g_calls |= 32;
}
static int WritePrefs(void *) {
  g_calls |= 64;
  return 123;
}

static void Expect(bool condition, const char *message) {
  if (!condition) {
    std::fprintf(stderr, "%s\n", message);
    std::exit(1);
  }
}

int main() {
  FontMenuContext context = {};
  context.userdata = reinterpret_cast<void *>(0x44);
  context.font_dir = &g_font_dir;
  context.show_settings_view = ShowSettings;
  context.is_book_settings_context = IsBookSettings;
  context.map_touch = MapTouch;
  context.print_status = PrintStatus;
  context.mark_book_layout_dirty = MarkLayout;
  context.refresh_prefs_button = RefreshButton;
  context.write_prefs = WritePrefs;
  context.keys.a = 9;

  Expect(context.FontDir() == "/fonts", "font directory");
  context.ShowSettingsView(true);
  Expect(context.IsBookSettingsContext(), "book settings context");
  const touchPosition pos = context.MapTouch(FrameInput());
  Expect(pos.px == 21 && pos.py == 43, "mapped touch");
  context.PrintStatus("status");
  context.MarkBookLayoutDirty();
  context.RefreshPrefsButton(7);
  Expect(context.WritePrefs() == 123, "write prefs result");
  Expect(context.keys.a == 9, "key map");
  Expect(g_calls == 127, "all callbacks called");

  FontMenuContext empty = {};
  Expect(empty.FontDir().empty(), "empty font directory");
  empty.ShowSettingsView(false);
  Expect(!empty.IsBookSettingsContext(), "empty book settings context");
  const touchPosition empty_pos = empty.MapTouch(FrameInput());
  Expect(empty_pos.px == 0 && empty_pos.py == 0, "empty mapped touch");
  empty.PrintStatus("ignored");
  empty.MarkBookLayoutDirty();
  empty.RefreshPrefsButton(7);
  Expect(empty.WritePrefs() == -1, "empty write prefs result");

  std::printf("All font_menu_context tests passed.\n");
  return 0;
}
