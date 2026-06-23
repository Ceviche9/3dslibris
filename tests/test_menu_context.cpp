#include "menus/menu.h"
#include "menus/menu_context.h"

#include <cstdio>
#include <cstdlib>

static void Expect(bool condition, const char *message) {
  if (!condition) {
    std::fprintf(stderr, "%s\n", message);
    std::exit(1);
  }
}

class TestMenu : public Menu {
public:
  explicit TestMenu(const MenuContext &context) : Menu(context) {}

  void Draw() override {}
  void HandleInput(const FrameInput &) override {}
};

int main() {
  App *app = reinterpret_cast<App *>(1);
  Text *text = reinterpret_cast<Text *>(2);
  Button *previous = reinterpret_cast<Button *>(3);
  Button *next = reinterpret_cast<Button *>(4);
  Button *preferences = reinterpret_cast<Button *>(5);
  IStatusReporter *reporter = reinterpret_cast<IStatusReporter *>(6);
  u8 color_mode = 2;

  const MenuContext populated = {app, text, previous, next, preferences,
                                 &color_mode, reporter};
  TestMenu menu(populated);
  Expect(menu.app == app, "app pointer");
  Expect(menu.ts == text, "text pointer");
  Expect(menu.buttonprev == previous, "previous button pointer");
  Expect(menu.buttonnext == next, "next button pointer");
  Expect(menu.buttonprefs == preferences, "preferences button pointer");
  Expect(menu.color_mode == &color_mode, "color mode pointer");
  Expect(menu.status_reporter == reporter, "status reporter pointer");
  Expect(menu.pagesize == 7, "default page size");
  Expect(menu.selected == 0, "default selection");
  Expect(menu.page == 0, "default page");
  Expect(menu.dirty, "default dirty state");

  const MenuContext empty = {};
  TestMenu null_menu(empty);
  Expect(null_menu.app == nullptr, "null app pointer");
  Expect(null_menu.ts == nullptr, "null text pointer");
  Expect(null_menu.buttonprev == nullptr, "null previous button pointer");
  Expect(null_menu.buttonnext == nullptr, "null next button pointer");
  Expect(null_menu.buttonprefs == nullptr, "null preferences button pointer");
  Expect(null_menu.color_mode == nullptr, "null color mode pointer");
  Expect(null_menu.status_reporter == nullptr, "null status reporter pointer");

  std::printf("All menu_context tests passed.\n");
  return 0;
}
