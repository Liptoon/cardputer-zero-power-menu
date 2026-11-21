#pragma once

#include <string>

namespace keypad {

enum class Key { Up, Down, Left, Right, Select, Back, Quit, Num1, Num2, Num3 };

using Handler = void (*)(Key);

/* Starts polling an evdev keyboard. Returns false when the device cannot be
 * opened; the UI stays usable through the API for tests. */
bool start(const std::string & device, Handler handler);

void stop();

}  // namespace keypad
