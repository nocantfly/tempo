#ifndef MADGWICK_FILTER_H
#define MADGWICK_FILTER_H
#include <Arduino.h>
#include "TrackingConfig.h"

class MadgwickFilter {
public:
    MadgwickFilter();
    void update(float gx, float gy, float gz, float ax, float ay, float az, float dt, float accMag, float gyroMag);
    void getQuaternion(float &q0_out, float &q1_out, float &q2_out, float &q3_out);
    void reset();

private:
    float q0, q1, q2, q3;
    float beta;
};
#endif
