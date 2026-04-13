#pragma once
#include "lvgl.h"

lv_obj_t *screen_team_create(void);

void screen_team_set_status(const char *text);
void screen_team_set_card_color(lv_color_t main, lv_color_t bg);
void screen_team_set_distance(float distance_m);
void screen_team_update_imu(float roll, float pitch, float yaw);