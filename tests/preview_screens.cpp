/* Headless preview renderer: draws the UI into an offscreen buffer and writes
 * PPM snapshots. Used to review the 320x170 layout without a device. */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "lvgl.h"
#include "ui.h"

namespace {

constexpr int kWidth = 320;
constexpr int kHeight = 170;

uint32_t * g_pixels = nullptr;

void flush_cb(lv_display_t * display, const lv_area_t * area, uint8_t * px_map) {
    const int width = area->x2 - area->x1 + 1;
    const int height = area->y2 - area->y1 + 1;
    const uint32_t stride = lv_draw_buf_width_to_stride(width, LV_COLOR_FORMAT_XRGB8888);
    for (int y = 0; y < height; ++y) {
        std::memcpy(g_pixels + static_cast<size_t>(area->y1 + y) * kWidth + area->x1,
                    px_map + static_cast<size_t>(y) * stride,
                    static_cast<size_t>(width) * sizeof(uint32_t));
    }
    lv_display_flush_ready(display);
}

void save_pnm(const std::string & path) {
    FILE * file = std::fopen(path.c_str(), "wb");
    if (file == nullptr) {
        std::perror(path.c_str());
        return;
    }
    std::fprintf(file, "P6\n%d %d\n255\n", kWidth, kHeight);
    for (int i = 0; i < kWidth * kHeight; ++i) {
        const uint8_t * pixel = reinterpret_cast<const uint8_t *>(&g_pixels[i]);
        std::fwrite(pixel + 2, 1, 1, file);
        std::fwrite(pixel + 1, 1, 1, file);
        std::fwrite(pixel + 0, 1, 1, file);
    }
    std::fclose(file);
    std::printf("wrote %s\n", path.c_str());
}

void render(const std::string & directory, const char * name) {
    lv_tick_inc(20);
    lv_refr_now(lv_display_get_default());
    save_pnm(directory + "/" + name + ".ppm");
}

}  // namespace

int main(int argc, char ** argv) {
    const std::string directory = argc > 1 ? argv[1] : ".";

    lv_init();

    lv_display_t * display = lv_display_create(kWidth, kHeight);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_XRGB8888);

    g_pixels = static_cast<uint32_t *>(std::calloc(kWidth * kHeight, sizeof(uint32_t)));
    lv_display_set_buffers(display, g_pixels, nullptr, kWidth * kHeight * sizeof(uint32_t),
                           LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_flush_cb(display, flush_cb);
    lv_display_set_default(display);

    ui::start(false);
    render(directory, "01-menu-restart");

    ui::on_key(keypad::Key::Down);
    render(directory, "02-menu-shutdown");

    ui::on_key(keypad::Key::Down);
    render(directory, "03-menu-exit");

    ui::start(false);
    ui::on_key(keypad::Key::Select);
    render(directory, "04-confirm-restart");

    ui::start(false);
    ui::on_key(keypad::Key::Down);
    ui::on_key(keypad::Key::Select);
    render(directory, "05-confirm-shutdown");

    ui::on_key(keypad::Key::Right);
    render(directory, "06-confirm-shutdown-action");

    ui::start(true);
    ui::on_key(keypad::Key::Down);
    ui::on_key(keypad::Key::Select);
    ui::on_key(keypad::Key::Right);
    ui::on_key(keypad::Key::Select);
    render(directory, "07-busy-shutdown");

    for (int i = 0; i < 120; ++i) {
        lv_tick_inc(10);
        lv_timer_handler();
    }
    render(directory, "08-dry-run-result");

    lv_deinit();
    std::free(g_pixels);
    return 0;
}
