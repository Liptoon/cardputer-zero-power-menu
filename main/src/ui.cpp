#include "ui.h"

#include <string>

#include "lvgl.h"
#include "power_action.h"

#ifndef APP_TITLE
#define APP_TITLE "Power"
#endif

namespace ui {

namespace {

constexpr int kScreenW = 320;
constexpr int kScreenH = 170;

constexpr uint32_t kColBg = 0x0E1118;
constexpr uint32_t kColBar = 0x161B24;
constexpr uint32_t kColRow = 0x1B212D;
constexpr uint32_t kColBorder = 0x2A3240;
constexpr uint32_t kColText = 0xE8ECF4;
constexpr uint32_t kColMuted = 0x8D97A8;
constexpr uint32_t kColAccent = 0x2F6FEB;
constexpr uint32_t kColDanger = 0xF2555C;
constexpr uint32_t kColOk = 0x3FB950;

enum class View { Menu, Confirm, Busy, Message };

struct MenuItem {
    const char * symbol;
    const char * label;
    uint32_t icon_color;
    power::Action action;
};

const MenuItem kItems[] = {
    {LV_SYMBOL_REFRESH, "Restart", 0x6EA8FE, power::Action::Restart},
    {LV_SYMBOL_POWER, "Shut down", kColDanger, power::Action::Shutdown},
    {LV_SYMBOL_CLOSE, "Exit", kColMuted, power::Action::None},
};
constexpr int kItemCount = static_cast<int>(sizeof(kItems) / sizeof(kItems[0]));

View g_view = View::Menu;
int g_focus = 0;
int g_confirm_focus = 0;
power::Action g_pending = power::Action::None;
std::string g_message;
bool g_message_error = false;
bool g_dry_run = false;
bool g_quit = false;
lv_obj_t * g_root = nullptr;
lv_obj_t * g_busy_status = nullptr;
lv_timer_t * g_action_timer = nullptr;

lv_obj_t * make_label(lv_obj_t * parent, const char * text, const lv_font_t * font, uint32_t color) {
    lv_obj_t * label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    return label;
}

void reset_screen() {
    g_root = lv_screen_active();
    g_busy_status = nullptr;
    lv_obj_clean(g_root);
    lv_obj_remove_flag(g_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(g_root, 0, 0);
    lv_obj_set_style_border_width(g_root, 0, 0);
    lv_obj_set_style_bg_color(g_root, lv_color_hex(kColBg), 0);
    lv_obj_set_style_bg_opa(g_root, LV_OPA_COVER, 0);
}

void build_header() {
    lv_obj_t * bar = lv_obj_create(g_root);
    lv_obj_remove_style_all(bar);
    lv_obj_set_pos(bar, 0, 0);
    lv_obj_set_size(bar, kScreenW, 26);
    lv_obj_set_style_bg_color(bar, lv_color_hex(kColBar), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_remove_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * title = make_label(bar, APP_TITLE, &lv_font_montserrat_14, kColText);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);

    lv_obj_t * note = make_label(bar, g_dry_run ? "DRY RUN" : "CardputerZero",
                                 &lv_font_montserrat_10,
                                 g_dry_run ? kColOk : kColMuted);
    lv_obj_align(note, LV_ALIGN_RIGHT_MID, -10, 0);
}

void build_menu() {
    reset_screen();
    build_header();

    for (int i = 0; i < kItemCount; ++i) {
        const bool focused = i == g_focus;

        lv_obj_t * row = lv_obj_create(g_root);
        lv_obj_remove_style_all(row);
        lv_obj_set_pos(row, 8, 32 + i * 35);
        lv_obj_set_size(row, kScreenW - 16, 32);
        lv_obj_set_style_radius(row, 6, 0);
        lv_obj_set_style_bg_color(row, lv_color_hex(focused ? kColAccent : kColRow), 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t * icon = make_label(row, kItems[i].symbol, &lv_font_montserrat_14,
                                     focused ? kColText : kItems[i].icon_color);
        lv_obj_align(icon, LV_ALIGN_LEFT_MID, 10, 0);

        lv_obj_t * text = make_label(row, kItems[i].label, &lv_font_montserrat_12, kColText);
        lv_obj_align(text, LV_ALIGN_LEFT_MID, 34, 0);
    }

    lv_obj_t * hint = make_label(g_root, "Enter: select   Esc: back / hold to exit",
                                 &lv_font_montserrat_10, kColMuted);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -4);
}

void build_confirm() {
    reset_screen();
    build_header();

    const bool restart = g_pending == power::Action::Restart;

    lv_obj_t * card = lv_obj_create(g_root);
    lv_obj_remove_style_all(card);
    lv_obj_set_size(card, 288, 132);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 2);
    lv_obj_set_style_bg_color(card, lv_color_hex(kColBar), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, 8, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(kColBorder), 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * question = make_label(card, restart ? "Restart the device?" : "Shut down the device?",
                                     &lv_font_montserrat_14, kColText);
    lv_obj_align(question, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t * detail = make_label(card,
                                   restart ? "The system will reboot." : "The system will power off.",
                                   &lv_font_montserrat_10, kColMuted);
    lv_obj_align(detail, LV_ALIGN_TOP_MID, 0, 34);

    const char * labels[2] = {"Cancel", restart ? "Restart" : "Power off"};
    for (int i = 0; i < 2; ++i) {
        const bool focused = i == g_confirm_focus;

        lv_obj_t * button = lv_obj_create(card);
        lv_obj_remove_style_all(button);
        lv_obj_set_size(button, 110, 30);
        lv_obj_set_style_radius(button, 6, 0);
        lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(button,
                                  lv_color_hex(focused ? (i == 1 ? kColDanger : kColAccent) : kColRow), 0);
        lv_obj_set_style_border_width(button, focused ? 0 : 1, 0);
        lv_obj_set_style_border_color(button, lv_color_hex(kColBorder), 0);
        lv_obj_remove_flag(button, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t * text = make_label(button, labels[i], &lv_font_montserrat_12, kColText);
        lv_obj_center(text);

        if (i == 0) {
            lv_obj_align(button, LV_ALIGN_BOTTOM_LEFT, 12, -12);
        }
        else {
            lv_obj_align(button, LV_ALIGN_BOTTOM_RIGHT, -12, -12);
        }
    }
}

void build_busy() {
    reset_screen();
    build_header();

    const bool restart = g_pending == power::Action::Restart;
    const uint32_t accent = restart ? kColAccent : kColDanger;

    lv_obj_t * spinner = lv_spinner_create(g_root);
    lv_obj_set_size(spinner, 36, 36);
    lv_obj_align(spinner, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_arc_width(spinner, 4, LV_PART_MAIN);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(kColRow), LV_PART_MAIN);
    lv_obj_set_style_arc_width(spinner, 4, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(spinner, lv_color_hex(accent), LV_PART_INDICATOR);

    lv_obj_t * text = make_label(g_root,
                                 restart ? "Restarting the system" : "Shutting down the system",
                                 &lv_font_montserrat_14, kColText);
    lv_obj_align(text, LV_ALIGN_TOP_MID, 0, 88);

    g_busy_status = make_label(g_root,
                               restart ? "Sending restart request..." : "Sending shutdown request...",
                               &lv_font_montserrat_10, kColMuted);
    lv_label_set_long_mode(g_busy_status, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(g_busy_status, kScreenW - 32);
    lv_obj_set_style_text_align(g_busy_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(g_busy_status, LV_ALIGN_TOP_MID, 0, 112);
}

void build_message() {
    reset_screen();
    build_header();

    lv_obj_t * icon = make_label(g_root, g_message_error ? LV_SYMBOL_WARNING : LV_SYMBOL_OK,
                                 &lv_font_montserrat_16, g_message_error ? kColDanger : kColOk);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 36);

    lv_obj_t * text = make_label(g_root, g_message.c_str(), &lv_font_montserrat_12, kColText);
    lv_label_set_long_mode(text, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(text, kScreenW - 32);
    lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(text, LV_ALIGN_TOP_MID, 0, 64);

    lv_obj_t * hint = make_label(g_root, "Press any key to return", &lv_font_montserrat_10, kColMuted);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -4);
}

void rebuild() {
    switch (g_view) {
        case View::Menu:
            build_menu();
            break;
        case View::Confirm:
            build_confirm();
            break;
        case View::Busy:
            build_busy();
            break;
        case View::Message:
            build_message();
            break;
    }
}

void set_menu_focus(int index) {
    const int count = kItemCount;
    g_focus = ((index % count) + count) % count;
    rebuild();
}

void start_action() {
    g_view = View::Busy;
    rebuild();
    g_action_timer = lv_timer_create(
        [](lv_timer_t * timer) {
            lv_timer_delete(timer);
            g_action_timer = nullptr;
            const power::Result result = power::run(g_pending, g_dry_run);
            if (g_dry_run || !result.ok) {
                g_message_error = !result.ok;
                g_message = result.detail;
                g_view = View::Message;
                rebuild();
                return;
            }
            if (g_busy_status != nullptr) {
                lv_label_set_text(g_busy_status,
                                  g_pending == power::Action::Restart
                                      ? "Restart request sent - waiting for the reboot"
                                      : "Shutdown request sent - waiting for power off");
            }
        },
        250, nullptr);
}

void activate_menu_item() {
    if (g_focus < 0 || g_focus >= kItemCount) {
        return;
    }
    const power::Action action = kItems[g_focus].action;
    if (action == power::Action::None) {
        g_quit = true;
        return;
    }
    g_pending = action;
    g_confirm_focus = 0;
    g_view = View::Confirm;
    rebuild();
}

}  // namespace

void start(bool dry_run) {
    g_dry_run = dry_run;
    g_view = View::Menu;
    g_focus = 0;
    g_quit = false;
    rebuild();
}

void on_key(keypad::Key key) {
    if (key == keypad::Key::Quit && g_view != View::Busy) {
        g_quit = true;
        return;
    }

    switch (g_view) {
        case View::Menu:
            switch (key) {
                case keypad::Key::Up:
                    set_menu_focus(g_focus - 1);
                    break;
                case keypad::Key::Down:
                case keypad::Key::Right:
                    set_menu_focus(g_focus + 1);
                    break;
                case keypad::Key::Left:
                    set_menu_focus(g_focus - 1);
                    break;
                case keypad::Key::Select:
                    activate_menu_item();
                    break;
                case keypad::Key::Num1:
                    g_focus = 0;
                    activate_menu_item();
                    break;
                case keypad::Key::Num2:
                    g_focus = 1;
                    activate_menu_item();
                    break;
                case keypad::Key::Num3:
                    g_focus = 2;
                    activate_menu_item();
                    break;
                default:
                    break;
            }
            break;

        case View::Confirm:
            switch (key) {
                case keypad::Key::Left:
                case keypad::Key::Up:
                    g_confirm_focus = 0;
                    rebuild();
                    break;
                case keypad::Key::Right:
                case keypad::Key::Down:
                    g_confirm_focus = 1;
                    rebuild();
                    break;
                case keypad::Key::Select:
                    if (g_confirm_focus == 0) {
                        g_view = View::Menu;
                        rebuild();
                    }
                    else {
                        start_action();
                    }
                    break;
                case keypad::Key::Back:
                    g_view = View::Menu;
                    rebuild();
                    break;
                default:
                    break;
            }
            break;

        case View::Busy:
            break;

        case View::Message:
            g_view = View::Menu;
            rebuild();
            break;
    }
}

bool should_quit() {
    return g_quit;
}

}  // namespace ui
