#pragma once

#include "keypad.h"

namespace ui {

void start(bool dry_run);

void on_key(keypad::Key key);

bool should_quit();

}  // namespace ui
