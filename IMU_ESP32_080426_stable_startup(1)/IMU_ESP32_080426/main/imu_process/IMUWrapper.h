#ifndef IMU_WRAPPER_H
#define IMU_WRAPPER_H

#include <SensorQMI8658.h>
#include <stdint.h>
#include "SimpleKalmanFilter.h"
#include "TrackingConfig.h"

struct IMUData
{
    float ax, ay, az; // g, đã scale/bias correction và pre-filter
    float gx, gy, gz; // rad/s, đã trừ bias
    float accMag;     // Độ lớn gia tốc
    float gyroMag;    // Độ lớn vận tốc góc
};

struct IMUStartupReport
{
    float avgAx = 0.0f;
    float avgAy = 0.0f;
    float avgAz = 1.0f;
    float avgGx = 0.0f;
    float avgGy = 0.0f;
    float avgGz = 0.0f;
    float accMag = 1.0f;
    float gyroMag = 0.0f;
    uint16_t stableSamples = 0;
};

class IMUWrapper
{
public:
    explicit IMUWrapper(SensorQMI8658 *qmi);
    void setup();
    bool calibrateStatic(uint16_t sampleCount = STATIC_CALIB_SAMPLES);
    bool runStartupCalibration(IMUStartupReport &report,
                               uint16_t sampleCount = IMU_STARTUP_STABLE_SAMPLES,
                               uint32_t timeoutMs = IMU_STARTUP_TIMEOUT_MS);
    bool read(IMUData &data);
    void resetFilters();
    void resetFilters(float ax, float ay, float az);

private:
    SensorQMI8658 *_qmi;
    float gyroBias[3] = {0};
    float accBias[3] = {0};
    float accScale[3] = {1, 1, 1};
    bool filtersSeeded = false;

    SimpleKalmanFilter _kAx;
    SimpleKalmanFilter _kAy;
    SimpleKalmanFilter _kAz;

    // Hardcoded calibration values (min/max 6 mặt) - vẫn giữ như hiện tại.
    float user_AccMin[3] = {-1.00f, -0.93f, -0.97f};
    float user_AccMax[3] = {0.99f, 1.07f, 1.02f};

    void computeCalibrationFactors();
    bool readRaw(float &ax, float &ay, float &az,
                 float &gx, float &gy, float &gz);
    void applyAccelCorrection(float &ax, float &ay, float &az) const;
    bool isStationary(float ax, float ay, float az,
                      float gxDps, float gyDps, float gzDps) const;
    void updateGyroBiasIfStationary(float ax, float ay, float az,
                                    float gxDps, float gyDps, float gzDps);
};
#endif
