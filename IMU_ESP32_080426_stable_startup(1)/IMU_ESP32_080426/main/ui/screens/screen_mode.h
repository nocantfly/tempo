#pragma once
#include "lvgl.h"

typedef enum
{
    SCREEN_MODE_PERSONAL = 0,
    SCREEN_MODE_TEAM
} screen_mode_type_t;

typedef enum
{
    SCREEN_MODE_SAVE_BEFORE = 0,
    SCREEN_MODE_SAVE_AFTER
} screen_mode_save_target_t;

typedef void (*screen_mode_save_cb_t)(screen_mode_save_target_t target);

lv_obj_t *screen_mode_create(void);

void screen_mode_set_mode(screen_mode_type_t mode);
void screen_mode_set_title(const char *title);

void screen_mode_set_status(const char *text);
void screen_mode_set_card_color(lv_color_t main, lv_color_t bg);
void screen_mode_update_pose(float roll, float pitch, float yaw, float distance_m);

/**
 * @brief Gọi khi click nút IO0 lúc đang ở screen mode
 */
void screen_mode_handle_button_click(void);

/**
 * @brief Đăng ký callback để application nhận sự kiện save
 */
void screen_mode_register_save_callback(screen_mode_save_cb_t cb);