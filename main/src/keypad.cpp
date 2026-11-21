#include "keypad.h"

#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>

#include "lvgl.h"

namespace keypad {

namespace {

constexpr uint32_t kQuitHoldMs = 700;
constexpr uint32_t kPollPeriodMs = 10;

int g_fd = -1;
Handler g_handler = nullptr;
lv_timer_t * g_timer = nullptr;
bool g_back_down = false;
bool g_back_quit = false;
uint32_t g_back_start = 0;

void emit(Key key) {
    if (g_handler != nullptr) {
        g_handler(key);
    }
}

bool is_back_key(int code) {
    return code == KEY_ESC || code == KEY_BACKSPACE || code == KEY_Q;
}

void handle_event(const struct input_event & event) {
    if (event.type != EV_KEY) {
        return;
    }

    const int code = event.code;
    const int value = event.value;

    if (is_back_key(code)) {
        if (value == 1) {
            g_back_down = true;
            g_back_quit = false;
            g_back_start = lv_tick_get();
        }
        else if (value == 0 && g_back_down) {
            g_back_down = false;
            if (!g_back_quit) {
                emit(Key::Back);
            }
        }
        return;
    }

    if (value != 1) {
        return;
    }

    switch (code) {
        case KEY_UP:
        case KEY_F:
        case KEY_K:
        case KEY_W:
            emit(Key::Up);
            break;
        case KEY_DOWN:
        case KEY_X:
        case KEY_J:
        case KEY_S:
        case KEY_TAB:
            emit(Key::Down);
            break;
        case KEY_LEFT:
        case KEY_Z:
        case KEY_H:
        case KEY_A:
            emit(Key::Left);
            break;
        case KEY_RIGHT:
        case KEY_C:
        case KEY_L:
        case KEY_D:
            emit(Key::Right);
            break;
        case KEY_ENTER:
        case KEY_KPENTER:
        case KEY_SPACE:
            emit(Key::Select);
            break;
        case KEY_1:
            emit(Key::Num1);
            break;
        case KEY_2:
            emit(Key::Num2);
            break;
        case KEY_3:
            emit(Key::Num3);
            break;
        default:
            break;
    }
}

void close_device() {
    if (g_timer != nullptr) {
        lv_timer_delete(g_timer);
        g_timer = nullptr;
    }
    if (g_fd >= 0) {
        close(g_fd);
        g_fd = -1;
    }
}

void poll_cb(lv_timer_t *) {
    if (g_fd < 0) {
        return;
    }

    struct input_event event;
    while (true) {
        const ssize_t got = read(g_fd, &event, sizeof(event));
        if (got == static_cast<ssize_t>(sizeof(event))) {
            handle_event(event);
            continue;
        }
        if (got < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
            break;
        }
        if (got <= 0) {
            std::fprintf(stderr, "power-menu: keyboard device closed\n");
            close_device();
            return;
        }
    }

    if (g_back_down && !g_back_quit && lv_tick_elaps(g_back_start) >= kQuitHoldMs) {
        g_back_quit = true;
        emit(Key::Quit);
    }
}

}  // namespace

bool start(const std::string & device, Handler handler) {
    if (device.empty()) {
        return false;
    }

    g_fd = open(device.c_str(), O_RDONLY | O_NONBLOCK);
    if (g_fd < 0) {
        std::fprintf(stderr, "power-menu: cannot open %s: %s\n", device.c_str(), std::strerror(errno));
        return false;
    }

    g_handler = handler;
    g_timer = lv_timer_create(poll_cb, kPollPeriodMs, nullptr);
    return g_timer != nullptr;
}

void stop() {
    close_device();
    g_handler = nullptr;
}

}  // namespace keypad
