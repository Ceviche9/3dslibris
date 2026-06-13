#include "menus/menu.h"

#include "app/app.h"

namespace {

MenuContext BuildMenuContext(App *requested_app) {
  App *app = requested_app ? requested_app : App::GetInstance();
  MenuContext context = {};
  context.app = app;
  if (app) {
    context.text = app->ts.get();
    context.previous_button = &app->buttonprev;
    context.next_button = &app->buttonnext;
    context.preferences_button = &app->buttonprefs;
    context.color_mode = &app->colorMode;
  }
  return context;
}

} // namespace

Menu::Menu(App *app) : Menu(BuildMenuContext(app)) {}
