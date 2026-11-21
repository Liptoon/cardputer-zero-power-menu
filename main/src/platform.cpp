#include "platform.h"

#include <dirent.h>
#include <fcntl.h>
#include <glob.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

namespace platform {

namespace {

constexpr const char * kCardputerKeyDevice = "/dev/input/by-path/platform-3f804000.i2c-event";
constexpr const char * kFallbackFramebuffer = "/dev/fb0";

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool has_key(int fd, unsigned int code) {
    unsigned long bits[(KEY_MAX / (8 * sizeof(unsigned long))) + 1] = {0};
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(bits)), bits) < 0) {
        return false;
    }
    return (bits[code / (8 * sizeof(unsigned long))] >> (code % (8 * sizeof(unsigned long)))) & 1UL;
}

std::string event_name(int fd) {
    char name[256] = {0};
    if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), name) < 0) {
        return "";
    }
    return name;
}

bool can_type(int fd) {
    return has_key(fd, KEY_ENTER) || has_key(fd, KEY_KPENTER) || has_key(fd, KEY_ESC);
}

}  // namespace

std::string env(const char * name) {
    const char * value = std::getenv(name);
    return value ? value : "";
}

bool env_flag(const char * name) {
    const std::string value = lower(env(name));
    return value == "1" || value == "true" || value == "yes" || value == "on";
}

std::string find_framebuffer() {
    const std::string from_env = env("POWER_MENU_FB_DEVICE");
    if (!from_env.empty()) {
        return from_env;
    }
    const std::string from_lvgl = env("LV_LINUX_FBDEV_DEVICE");
    if (!from_lvgl.empty()) {
        return from_lvgl;
    }

    std::ifstream proc_fb("/proc/fb");
    std::string line;
    while (std::getline(proc_fb, line)) {
        if (line.find("fb_st7789v") == std::string::npos) {
            continue;
        }
        const int index = std::atoi(line.c_str());
        char path[32];
        std::snprintf(path, sizeof(path), "/dev/fb%d", index);
        if (access(path, R_OK | W_OK) == 0) {
            return path;
        }
    }

    return kFallbackFramebuffer;
}

std::string find_key_device() {
    const std::string from_env = env("POWER_MENU_KEY_DEVICE");
    if (!from_env.empty()) {
        return from_env;
    }
    if (access(kCardputerKeyDevice, R_OK) == 0) {
        return kCardputerKeyDevice;
    }

    std::string best;
    int best_score = 0;

    glob_t matches;
    if (glob("/dev/input/event*", 0, nullptr, &matches) == 0) {
        for (std::size_t i = 0; i < matches.gl_pathc; ++i) {
            const char * path = matches.gl_pathv[i];
            const int fd = open(path, O_RDONLY | O_NONBLOCK);
            if (fd < 0) {
                continue;
            }
            int score = 0;
            if (can_type(fd)) {
                score = 1;
                const std::string name = lower(event_name(fd));
                if (name.find("keyboard") != std::string::npos ||
                    name.find("tca8418") != std::string::npos ||
                    name.find("keypad") != std::string::npos) {
                    score = 2;
                }
            }
            close(fd);
            if (score > best_score) {
                best_score = score;
                best = path;
            }
        }
        globfree(&matches);
    }

    return best;
}

}  // namespace platform
