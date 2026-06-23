#include "menus/paged_list_menu.h"

#include "app/app.h"

namespace {

Book *GetCurrentBook(void *userdata) {
  App *app = static_cast<App *>(userdata);
  return app ? app->GetCurrentBook() : nullptr;
}

void ShowCurrentBookView(void *userdata) {
  App *app = static_cast<App *>(userdata);
  if (app)
    app->ShowCurrentBookView();
}

void ShowSettingsView(void *userdata, bool from_book) {
  App *app = static_cast<App *>(userdata);
  if (app)
    app->ShowSettingsView(from_book);
}

void RequestStatusRedraw(void *userdata) {
  App *app = static_cast<App *>(userdata);
  if (app)
    app->RequestStatusRedraw();
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

PagedListMenuContext BuildPagedListMenuContext(App *requested_app) {
  App *app = requested_app ? requested_app : App::GetInstance();
  PagedListMenuContext context = {};
  context.base.app = app;
  context.userdata = app;
  context.get_current_book = GetCurrentBook;
  context.show_current_book_view = ShowCurrentBookView;
  context.show_settings_view = ShowSettingsView;
  context.request_status_redraw = RequestStatusRedraw;
  context.is_book_settings_context = IsBookSettingsContext;
  context.map_touch = MapTouch;
  if (app) {
    context.base.text = app->ts.get();
    context.base.previous_button = &app->buttonprev;
    context.base.next_button = &app->buttonnext;
    context.base.preferences_button = &app->buttonprefs;
    context.base.color_mode = &app->colorMode;
    context.base.status_reporter = app;
    context.keys = app->key;
  }
  return context;
}

} // namespace

PagedListMenu::PagedListMenu(App *app, const char *header_title)
    : PagedListMenu(BuildPagedListMenuContext(app), header_title) {}
