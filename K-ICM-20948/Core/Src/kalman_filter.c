#include "kalman_filter.h"

void Kalman_Init(KalmanFilter_t *kf, float q, float r, float p, float x)
{
    kf->q = q;
    kf->r = r;
    kf->p = p;
    kf->x = x;
    kf->k = 0;
}

float Kalman_Update(KalmanFilter_t *kf, float measurement)
{
    // Prediction update
    kf->p = kf->p + kf->q;

    // Measurement update
    kf->k = kf->p / (kf->p + kf->r);
    kf->x = kf->x + kf->k * (measurement - kf->x);
    kf->p = (1 - kf->k) * kf->p;

    return kf->x;
}
