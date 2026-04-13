#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

    lv_obj_t *screen_personal_create(void);

    void screen_personal_update_imu(float roll, float pitch, float yaw);
    void screen_personal_set_distance(float distance_m);
    void screen_personal_set_status(const char *text);

#ifdef __cplusplus
}
#endif