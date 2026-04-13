#include "ui.h"
#include "lvgl.h"
#include <stdio.h>

static lv_obj_t *label_progress;
static lv_obj_t *label_status;
static lv_obj_t *bar;
static int loading_percent = 0;

static int clamp_percent(int percent)
{
    if (percent < 0)
        return 0;
    if (percent > 100)
        return 100;
    return percent;
}

lv_obj_t *screen_loading_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

    // ===== Title =====
    lv_obj_t *label_title = lv_label_create(scr);
    lv_label_set_text(label_title,
                      "THIET BI HO TRO\n"
                      "HUAN LUYEN DIEU LENH");

    lv_obj_set_style_text_font(label_title, &font_noto_basic_30_4, 0);
    lv_obj_set_style_text_color(label_title, lv_color_hex(0xFFFF00), 0);
    lv_obj_set_style_text_align(label_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label_title, LV_ALIGN_BOTTOM_MID, 0, -150);

    // ===== Progress Bar =====
    bar = lv_bar_create(scr);
    lv_obj_set_size(bar, 250, 12);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -84);

    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x00FFAA), LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 10, 0);

    // ===== Progress Text =====
    label_progress = lv_label_create(scr);
    lv_label_set_text(label_progress, "Khoi dong 0%");
    lv_obj_set_style_text_font(label_progress, &font_noto_basic_20_4, 0);
    lv_obj_set_style_text_color(label_progress, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(label_progress, LV_ALIGN_BOTTOM_MID, 0, -42);

    // ===== Status Text =====
    label_status = lv_label_create(scr);
    lv_label_set_text(label_status, "Dang bat dau bo cam bien...");
    lv_obj_set_width(label_status, 320);
    lv_label_set_long_mode(label_status, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(label_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_status, &font_noto_basic_20_4, 0);
    lv_obj_set_style_text_color(label_status, lv_color_hex(0xA0FFA0), 0);
    lv_obj_align(label_status, LV_ALIGN_BOTTOM_MID, 0, -8);

    loading_percent = 0;
    return scr;
}

void screen_loading_set_progress(int percent)
{
    loading_percent = clamp_percent(percent);

    if (label_progress)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "Khoi dong %d%%", loading_percent);
        lv_label_set_text(label_progress, buf);
    }

    if (bar)
    {
        lv_bar_set_value(bar, loading_percent, LV_ANIM_ON);
    }
}

void screen_loading_set_status(const char *text)
{
    if (label_status && text)
    {
        lv_label_set_text(label_status, text);
    }
}

void screen_loading_set(int percent)
{
    screen_loading_set_progress(percent);
}
