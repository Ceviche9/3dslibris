#include "settings/font.h"

#include "app/app.h"
#include "settings/prefs.h"

namespace {

void ShowSettingsView(void *userdata, bool from_book) {
  App *app = static_cast<App *>(userdata);
  if (app)
    app->ShowSettingsView(from_book);
}

bool IsBookSettingsContext(void *userdata) {
  App *app = static_cast<App *>(userdata);
  return app ? app->IsBookSettingsContext() : false;
}

touchPosition MapTouch(void *userdata, const FrameInput &input) {
  App *app = static_cast<App *>(userdata);
  touchPosition pos = {};
  if (app)
    pos = app->MapTouch(input);
  return pos;
}

void PrintStatus(void *userdata, const char *message) {
  App *app = static_cast<App *>(userdata);
  if (app)
    app->PrintStatus(message);
}

void MarkBookLayoutDirty(void *userdata) {
  App *app = static_cast<App *>(userdata);
  if (app)
    app->MarkBookLayoutDirty();
}

void RefreshPrefsButton(void *userdata, int button_id) {
  App *app = static_cast<App *>(userdata);
  if (app)
    app->PrefsRefreshButton(button_id);
}

int WritePrefs(void *userdata) {
  App *app = static_cast<App *>(userdata);
  return (app && app->prefs) ? app->prefs->Write() : -1;
}

FontMenuContext BuildFontMenuContext(App *requested_app) {
  App *app = requested_app ? requested_app : App::GetInstance();
  FontMenuContext context = {};
  context.base.app = app;
  context.userdata = app;
  context.show_settings_view = ShowSettingsView;
  context.is_book_settings_context = IsBookSettingsContext;
  context.map_touch = MapTouch;
  context.print_status = PrintStatus;
  context.mark_book_layout_dirty = MarkBookLayoutDirty;
  context.refresh_prefs_button = RefreshPrefsButton;
  context.write_prefs = WritePrefs;
  if (app) {
    context.base.text = app->ts.get();
    context.base.previous_button = &app->buttonprev;
    context.base.next_button = &app->buttonnext;
    context.base.preferences_button = &app->buttonprefs;
    context.base.color_mode = &app->colorMode;
    context.base.status_reporter = app;
    context.font_dir = &app->fontdir;
    context.keys = app->key;
  }
  return context;
}

} // namespace

FontMenu::FontMenu(App *app) : FontMenu(BuildFontMenuContext(app)) {}
