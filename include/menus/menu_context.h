#pragma once

#include <3ds/types.h>

class App;
class Button;
class IStatusReporter;
class Text;

struct MenuContext {
  App *app;
  Text *text;
  Button *previous_button;
  Button *next_button;
  Button *preferences_button;
  const u8 *color_mode;
  IStatusReporter *status_reporter;
};
