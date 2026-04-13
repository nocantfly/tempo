#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C"
{
#endif

    lv_obj_t *screen_loading_create(void);

    /**
     * @brief Cập nhật % loading (0–100)
     */
    void screen_loading_set_progress(int percent);

    /**
     * @brief Cập nhật dòng trạng thái ở màn loading
     */
    void screen_loading_set_status(const char *text);

    /**
     * @brief Giữ tương thích tên cũ
     */
    void screen_loading_set(int percent);

#ifdef __cplusplus
}
#endif
