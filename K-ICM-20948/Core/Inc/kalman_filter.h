 /*
 * kalman_filter.h
 *
 *  Created on: Nov 2, 2025
 *      Author: tranh
 */

#ifndef INC_KALMAN_FILTER_H_
#define INC_KALMAN_FILTER_H_

typedef struct {
    float q;   // Process noise covariance
    float r;   // Measurement noise covariance
    float x;   // Value
    float p;   // Estimation error covariance
    float k;   // Kalman gain
} KalmanFilter_t;

void Kalman_Init(KalmanFilter_t *kf, float q, float r, float p, float x);
float Kalman_Update(KalmanFilter_t *kf, float measurement);


#endif /* INC_KALMAN_FILTER_H_ */
