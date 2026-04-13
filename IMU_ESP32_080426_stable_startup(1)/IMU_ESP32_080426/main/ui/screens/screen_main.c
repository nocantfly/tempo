#include "ui.h"
#include "ui_header.h"
#include "application.h"
#include <stddef.h>

static void goto_personal(lv_event_t *e)
{
    LV_UNUSED(e);
    application_prepare_session();
    ui_switch_screen(UI_SCREEN_PERSONAL);
}

static void goto_team(lv_event_t *e)
{
    LV_UNUSED(e);
    application_prepare_session();
    ui_switch_screen(UI_SCREEN_TEAM);
}

lv_obj_t *screen_main_create(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(scr, lv_color_hex(0), 0);

    lv_obj_t *bg = lv_img_create(scr);
    lv_img_set_src(bg, &ui_img_bgr_png);
    lv_obj_center(bg);
    lv_obj_move_background(bg);

    ui_header_create(scr);

    // ===== Button 1 =====
    lv_obj_t *btn1 = lv_btn_create(scr);
    lv_obj_set_size(btn1, 320, 80);
    lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -60);
    lv_obj_add_event_cb(btn1, goto_personal, LV_EVENT_CLICKED, NULL);

    // style button
    lv_obj_set_style_radius(btn1, 25, 0);
    lv_obj_set_style_bg_color(btn1, lv_color_hex(0x1B5E20), 0);
    lv_obj_set_style_bg_opa(btn1, LV_OPA_90, 0);

    // label
    lv_obj_t *lbl1 = lv_label_create(btn1);
    lv_label_set_text(lbl1, "Luyện tập cá nhân");
    lv_obj_set_style_text_font(lbl1, &font_noto_basic_30_4, 0);
    lv_obj_set_style_text_color(lbl1, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(lbl1);

    // ===== Button 2 =====
    lv_obj_t *btn2 = lv_btn_create(scr);
    lv_obj_set_size(btn2, 320, 80);
    lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 60);
    lv_obj_add_event_cb(btn2, goto_team, LV_EVENT_CLICKED, NULL);

    // style button
    lv_obj_set_style_radius(btn2, 25, 0);
    lv_obj_set_style_bg_color(btn2, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_bg_opa(btn2, LV_OPA_90, 0);

    // label
    lv_obj_t *lbl2 = lv_label_create(btn2);
    lv_label_set_text(lbl2, "Luyện tập đội hình");
    lv_obj_set_style_text_font(lbl2, &font_noto_basic_30_4, 0);
    lv_obj_set_style_text_color(lbl2, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(lbl2);

    return scr;
}
