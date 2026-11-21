#pragma once

#include <string>

namespace platform {

/* Returns the value of an environment variable, or an empty string. */
std::string env(const char * name);

/* True when the variable is set to 1/true/yes (case-insensitive). */
bool env_flag(const char * name);

/* Resolves the ST7789V framebuffer device.
 * Order: POWER_MENU_FB_DEVICE, LV_LINUX_FBDEV_DEVICE, /proc/fb scan, /dev/fb0. */
std::string find_framebuffer();

/* Resolves the TCA8418 keypad event device.
 * Order: POWER_MENU_KEY_DEVICE, CardputerZero by-path node, /dev/input scan. */
std::string find_key_device();

}  // namespace platform
