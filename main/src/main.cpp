#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>

#include "keypad.h"
#include "lvgl.h"
#include "platform.h"
#include "ui.h"

namespace {

volatile std::sig_atomic_t g_stop_requested = 0;

void request_stop(int) {
    g_stop_requested = 1;
}

void print_usage(const char * argv0) {
    std::printf(
        "Usage: %s [--dry-run]\n"
        "\n"
        "CardputerZero power menu: restart or shut down the system.\n"
        "\n"
        "Options:\n"
        "  --dry-run   Print the system command instead of executing it.\n"
        "  -h, --help  Show this help.\n"
        "\n"
        "Environment:\n"
        "  POWER_MENU_FB_DEVICE    Framebuffer device (default: fb_st7789v from /proc/fb).\n"
        "  POWER_MENU_KEY_DEVICE   evdev keypad (default: platform-3f804000.i2c-event).\n"
        "  POWER_MENU_DRY_RUN      Same as --dry-run when set to 1/true/yes.\n",
        argv0);
}

}  // namespace

int main(int argc, char ** argv) {
    bool dry_run = platform::env_flag("POWER_MENU_DRY_RUN");

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--dry-run") == 0) {
            dry_run = true;
        }
        else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        else {
            std::fprintf(stderr, "%s: unknown option '%s'\n", argv[0], argv[i]);
            print_usage(argv[0]);
            return 2;
        }
    }

    lv_init();

    std::signal(SIGINT, request_stop);
    std::signal(SIGTERM, request_stop);

    lv_display_t * display = lv_linux_fbdev_create();
    if (display == nullptr) {
        std::fprintf(stderr, "power-menu: cannot create the fbdev display\n");
        return 1;
    }

    const std::string framebuffer = platform::find_framebuffer();
    if (framebuffer.empty() || lv_linux_fbdev_set_file(display, framebuffer.c_str()) != LV_RESULT_OK) {
        std::fprintf(stderr, "power-menu: cannot open framebuffer '%s'\n", framebuffer.c_str());
        return 1;
    }

    ui::start(dry_run);

    const std::string keyboard = platform::find_key_device();
    if (!keypad::start(keyboard, ui::on_key)) {
        std::fprintf(stderr, "power-menu: no keyboard device found; UI is read-only\n");
    }

    while (!ui::should_quit() && !g_stop_requested) {
        uint32_t wait_ms = lv_timer_handler();
        if (wait_ms == 0) {
            wait_ms = 1;
        }
        if (wait_ms > 20) {
            wait_ms = 20;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
    }

    keypad::stop();
    lv_deinit();
    return 0;
}
