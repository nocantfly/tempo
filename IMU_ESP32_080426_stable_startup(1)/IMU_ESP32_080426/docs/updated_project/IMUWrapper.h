#ifndef IMU_WRAPPER_H
#define IMU_WRAPPER_H

#include <SensorQMI8658.hpp>
#include <Arduino.h>
#include "SimpleKalmanFilter.h"
#include "TrackingConfig.h"

struct IMUData {
    float ax, ay, az; // g, đã scale/bias correction và pre-filter
    float gx, gy, gz; // rad/s, đã trừ bias
    float accMag;     // Độ lớn gia tốc
    float gyroMag;    // Độ lớn vận tốc góc
};

class IMUWrapper {
public:
    IMUWrapper(SensorQMI8658 *qmi);
    void setup();
    bool calibrateStatic(uint16_t sampleCount = STATIC_CALIB_SAMPLES);
    bool read(IMUData &data);
    void resetFilters();

private:
    SensorQMI8658 *_qmi;
    float gyroBias[3] = {0};
    float accBias[3] = {0};
    float accScale[3] = {1, 1, 1};

    SimpleKalmanFilter _kAx;
    SimpleKalmanFilter _kAy;
    SimpleKalmanFilter _kAz;

    // Hardcoded calibration values (min/max 6 mặt) - vẫn giữ như hiện tại.
    float user_AccMin[3] = {-1.00f, -0.93f, -0.97f};
    float user_AccMax[3] = { 0.99f,  1.07f,  1.02f};

    void computeCalibrationFactors();
};
#endif
