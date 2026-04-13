#pragma once

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void ui_header_create(lv_obj_t *parent);

    void ui_header_set_time(const char *time);
    void ui_header_set_battery(int percent);
    void ui_header_set_imu_border(bool is_green);

#ifdef __cplusplus
}
#endif