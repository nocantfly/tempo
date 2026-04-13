#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        UI_SCREEN_LOADING = 0,
        UI_SCREEN_SELECT,
        UI_SCREEN_MAIN,
        UI_SCREEN_PERSONAL,
        UI_SCREEN_TEAM,
    } ui_screen_t;
    extern const lv_font_t font_noto_basic_20_4;
    extern const lv_font_t font_noto_basic_30_4;
    extern const lv_font_t ui_font_Font72Bold;
    extern const lv_font_t ui_font_Font100;
    extern const lv_font_t ui_font_Font130;

    extern const lv_image_dsc_t ui_img_bgr_png;
    // init
    void ui_init(void);

    // switch screen
    void ui_switch_screen(ui_screen_t screen);

    // global update (forward xuống header / screen)
    void ui_set_time(const char *time);
    void ui_set_battery(int percent);

    // personal
    void ui_set_temperature(int temp);
    void ui_set_description(const char *text);

    // team
    void ui_set_training_info(int duration, int steps);

#ifdef __cplusplus
}
#endif