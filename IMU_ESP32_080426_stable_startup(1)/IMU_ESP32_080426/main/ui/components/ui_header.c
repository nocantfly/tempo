#include "ui_header.h"
#include <stdio.h>

static lv_obj_t *label_time;
static lv_obj_t *label_battery;
static lv_obj_t *icon_battery;
static lv_obj_t *icon_wifi;
static lv_obj_t *icon_clock;
static lv_obj_t *label_wifi_text;
static lv_obj_t *wifi_capsule;

static void style_capsule(lv_obj_t *obj)
{
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_set_style_radius(obj, 999, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x1B3A2F), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_70, 0);

    lv_obj_set_style_border_width(obj, 2, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x4BE38A), 0);
    lv_obj_set_style_border_opa(obj, LV_OPA_70, 0);

    lv_obj_set_style_shadow_width(obj, 10, 0);
    lv_obj_set_style_shadow_color(obj, lv_color_hex(0x39D67A), 0);
    lv_obj_set_style_shadow_opa(obj, LV_OPA_20, 0);
    lv_obj_set_style_shadow_spread(obj, 1, 0);

    lv_obj_set_style_pad_left(obj, 14, 0);
    lv_obj_set_style_pad_right(obj, 14, 0);
    lv_obj_set_style_pad_top(obj, 6, 0);
    lv_obj_set_style_pad_bottom(obj, 6, 0);
}

static void style_icon(lv_obj_t *obj, int font_size)
{
    lv_obj_set_style_text_color(obj, lv_color_hex(0xFFFFFF), 0);

    if (font_size >= 20)
        lv_obj_set_style_text_font(obj, &lv_font_montserrat_20, 0);
    else if (font_size >= 18)
        lv_obj_set_style_text_font(obj, &lv_font_montserrat_18, 0);
    else
        lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, 0);
}

void ui_header_create(lv_obj_t *parent)
{
    lv_obj_t *top = lv_obj_create(parent);

    // chỉnh số 84 này để cả header lùi vào / lùi ra
    // số càng lớn -> càng lùi vào
    lv_obj_set_size(top, 410 - 84, LV_SIZE_CONTENT);
    lv_obj_align(top, LV_ALIGN_TOP_MID, 0, 24);

    lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(top, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(top, 0, 0);
    lv_obj_set_style_pad_all(top, 0, 0);

    lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    // ===== Khối thời gian =====
    lv_obj_t *time_capsule = lv_obj_create(top);
    style_capsule(time_capsule);

    lv_obj_set_height(time_capsule, LV_SIZE_CONTENT);
    lv_obj_set_width(time_capsule, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(time_capsule, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(time_capsule,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(time_capsule, 8, 0);

    // chỉnh số 4 này để khối clock lùi vào / lùi ra
    // số càng lớn -> càng lùi vào
    lv_obj_set_style_margin_left(time_capsule, 4, 0);

    icon_clock = lv_label_create(time_capsule);
    lv_label_set_text(icon_clock, LV_SYMBOL_REFRESH);
    style_icon(icon_clock, 16);

    label_time = lv_label_create(time_capsule);
    lv_label_set_text(label_time, "10:30");
    lv_obj_set_style_text_color(label_time, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_time, &lv_font_montserrat_20, 0);

    // ===== Khối wifi / IMU ở giữa =====
    wifi_capsule = lv_obj_create(top);
    style_capsule(wifi_capsule);

    lv_obj_set_height(wifi_capsule, LV_SIZE_CONTENT);
    lv_obj_set_width(wifi_capsule, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(wifi_capsule, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(wifi_capsule,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    lv_obj_set_style_pad_left(wifi_capsule, 12, 0);
    lv_obj_set_style_pad_right(wifi_capsule, 12, 0);
    lv_obj_set_style_pad_column(wifi_capsule, 6, 0);

    icon_wifi = lv_label_create(wifi_capsule);
    lv_label_set_text(icon_wifi, LV_SYMBOL_USB);
    style_icon(icon_wifi, 18);

    label_wifi_text = lv_label_create(wifi_capsule);
    lv_label_set_text(label_wifi_text, "IMU");
    lv_obj_set_style_text_color(label_wifi_text, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_wifi_text, &lv_font_montserrat_18, 0);

    // ===== Khối pin =====
    lv_obj_t *bat_capsule = lv_obj_create(top);
    style_capsule(bat_capsule);

    lv_obj_set_height(bat_capsule, LV_SIZE_CONTENT);
    lv_obj_set_width(bat_capsule, LV_SIZE_CONTENT);

    lv_obj_set_flex_flow(bat_capsule, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bat_capsule,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);

    lv_obj_set_style_pad_column(bat_capsule, 8, 0);

    // chỉnh số 4 này để khối pin lùi vào / lùi ra
    // số càng lớn -> càng lùi vào
    lv_obj_set_style_margin_right(bat_capsule, 4, 0);

    icon_battery = lv_label_create(bat_capsule);
    lv_label_set_text(icon_battery, LV_SYMBOL_BATTERY_3);
    style_icon(icon_battery, 18);

    label_battery = lv_label_create(bat_capsule);
    lv_label_set_text(label_battery, "78%");
    lv_obj_set_style_text_color(label_battery, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(label_battery, &lv_font_montserrat_18, 0);
}

void ui_header_set_time(const char *t)
{
    if (!label_time || !t) return;
    lv_label_set_text(label_time, t);
}

void ui_header_set_battery(int p)
{
    if (!label_battery || !icon_battery) return;

    if (p < 0) p = 0;
    if (p > 100) p = 100;

    static char buf[8];
    sprintf(buf, "%d%%", p);
    lv_label_set_text(label_battery, buf);

    if (p >= 80)
        lv_label_set_text(icon_battery, LV_SYMBOL_BATTERY_FULL);
    else if (p >= 60)
        lv_label_set_text(icon_battery, LV_SYMBOL_BATTERY_3);
    else if (p >= 40)
        lv_label_set_text(icon_battery, LV_SYMBOL_BATTERY_2);
    else if (p >= 20)
        lv_label_set_text(icon_battery, LV_SYMBOL_BATTERY_1);
    else
        lv_label_set_text(icon_battery, LV_SYMBOL_BATTERY_EMPTY);
}

void ui_header_set_imu_border(bool is_green)
{
    if (!wifi_capsule) return;

    if (is_green)
    {
        lv_obj_set_style_border_color(wifi_capsule, lv_color_hex(0x4BE38A), 0);
        lv_obj_set_style_border_opa(wifi_capsule, LV_OPA_70, 0);

        lv_obj_set_style_shadow_color(wifi_capsule, lv_color_hex(0x39D67A), 0);
        lv_obj_set_style_shadow_opa(wifi_capsule, LV_OPA_20, 0);
    }
    else
    {
        lv_obj_set_style_border_color(wifi_capsule, lv_color_hex(0x7A7A7A), 0);
        lv_obj_set_style_border_opa(wifi_capsule, LV_OPA_70, 0);

        lv_obj_set_style_shadow_color(wifi_capsule, lv_color_hex(0x7A7A7A), 0);
        lv_obj_set_style_shadow_opa(wifi_capsule, LV_OPA_10, 0);
    }
}