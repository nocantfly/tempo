#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include "driver/i2c_master.h"

    typedef enum
    {
        HAND_TYPE_LEFT,
        HAND_TYPE_RIGHT
    } hand_type_t;

    typedef struct
    {
        float roll;
        float pitch;
        float yaw;
        float distance_m;
    } app_pose_t;

    /**
     * @brief Start application
     *
     * @param bus I2C master bus handle
     */
    void application_start(i2c_master_bus_handle_t bus);

    /**
     * @brief Get saved BEFORE pose
     */
    bool application_get_pose_before(app_pose_t *out_pose);

    /**
     * @brief Get saved AFTER pose
     */
    bool application_get_pose_after(app_pose_t *out_pose);
    void application_set_hand_type(hand_type_t hand);
    void application_prepare_session(void);

#ifdef __cplusplus
}
#endif