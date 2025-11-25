#include <math.h>
#include <stdint.h>
#include "angle.h"

void ICM20948_UpdateAngle(ICM_Angle_t *imu, float dt)
{
    // --- Tính góc từ accel ---
    float roll_acc  = atan2f(imu->Ay, imu->Az) * 180.0f / M_PI;
    float pitch_acc = atan2f(-imu->Ax, sqrtf(imu->Ay * imu->Ay + imu->Az * imu->Az)) * 180.0f / M_PI;

    // --- Cập nhật góc bằng lọc bổ sung ---
    const float alpha = 0.98f;
    imu->roll  = alpha * (imu->roll  + imu->Gx * dt) + (1.0f - alpha) * roll_acc;
    imu->pitch = alpha * (imu->pitch + imu->Gy * dt) + (1.0f - alpha) * pitch_acc;
}
