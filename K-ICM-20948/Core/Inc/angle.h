/*
 * angle.h
 *
 *  Created on: Nov 8, 2025
 *      Author: tranh
 */

#ifndef INC_ANGLE_H_
#define INC_ANGLE_H_

typedef struct {
    float Ax, Ay, Az;
    float Gx, Gy, Gz;
    float roll;
    float pitch;
} ICM_Angle_t;
void ICM20948_UpdateAngle(ICM_Angle_t *imu, float dt);
#endif /* INC_ANGLE_H_ */
