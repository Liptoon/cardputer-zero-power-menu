/**
 * LVGL 9.5 configuration for the CardputerZero power menu.
 *
 * Only options that differ from LVGL defaults are listed here; everything else
 * falls back to lv_conf_internal.h defaults.
 */

#ifndef LV_CONF_H
#define LV_CONF_H

/* 32-bit rendering pipeline; the fbdev driver picks the real framebuffer
 * format (RGB565/RGB888/XRGB8888) from the active /dev/fbN. */
#define LV_COLOR_DEPTH 32

/* Keep the UI responsive on the small 320x170 panel. */
#define LV_DEF_REFR_PERIOD 16

/* Diagnostics on stderr. */
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

/* CardputerZero backend: Linux framebuffer + evdev keypad. */
#define LV_USE_LINUX_FBDEV 1
#define LV_USE_EVDEV 1

/* Built-in fonts used by the UI. */
#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

#define LV_USE_SPINNER 1

#if defined(__aarch64__)
#define LV_USE_DRAW_SW_ASM LV_DRAW_SW_ASM_NEON
#endif

#endif /* LV_CONF_H */
