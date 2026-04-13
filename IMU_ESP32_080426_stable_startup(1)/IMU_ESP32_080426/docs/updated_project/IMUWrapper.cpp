#include "IMUWrapper.h"

IMUWrapper::IMUWrapper(SensorQMI8658 *qmi)
    : _qmi(qmi),
      _kAx(KALMAN_R, KALMAN_R, KALMAN_Q),
      _kAy(KALMAN_R, KALMAN_R, KALMAN_Q),
      _kAz(KALMAN_R, KALMAN_R, KALMAN_Q) {
}

void IMUWrapper::setup() {
    _qmi->configAccelerometer(SensorQMI8658::ACC_RANGE_4G,
                              SensorQMI8658::ACC_ODR_500Hz,
                              SensorQMI8658::LPF_MODE_3);
    delay(50);
    _qmi->configGyroscope(SensorQMI8658::GYR_RANGE_1024DPS,
                          (SensorQMI8658::GyroODR)SensorQMI8658::ACC_ODR_500Hz,
                          SensorQMI8658::LPF_MODE_3);
    delay(50);
    _qmi->enableAccelerometer();
    _qmi->enableGyroscope();
    delay(50);

    computeCalibrationFactors();
    resetFilters();
}

void IMUWrapper::resetFilters() {
    _kAx.reset(0.0f);
    _kAy.reset(0.0f);
    _kAz.reset(0.0f);
}

void IMUWrapper::computeCalibrationFactors() {
    for (int i = 0; i < 3; i++) {
        accBias[i] = (user_AccMax[i] + user_AccMin[i]) / 2.0f;
        float range = user_AccMax[i] - user_AccMin[i];
        accScale[i] = (range != 0.0f) ? (2.0f / range) : 1.0f;
    }
}

bool IMUWrapper::calibrateStatic(uint16_t sampleCount) {
    float sumG[3] = {0.0f, 0.0f, 0.0f};
    uint16_t validSamples = 0;
    unsigned long tStart = millis();

    resetFilters();

    while (validSamples < sampleCount && (millis() - tStart) < STATIC_CALIB_TIMEOUT_MS) {
        if (_qmi->getDataReady()) {
            float g[3];
            if (_qmi->getGyroscope(g[0], g[1], g[2])) {
                sumG[0] += g[0];
                sumG[1] += g[1];
                sumG[2] += g[2];
                validSamples++;
            }
        }
        delay(1);
    }

    if (validSamples == 0) {
        return false;
    }

    gyroBias[0] = sumG[0] / validSamples;
    gyroBias[1] = sumG[1] / validSamples;
    gyroBias[2] = sumG[2] / validSamples;
    resetFilters();
    return true;
}

bool IMUWrapper::read(IMUData &data) {
    if (!_qmi->getDataReady()) return false;

    float acc[3], gyr[3];
    if (!_qmi->getAccelerometer(acc[0], acc[1], acc[2]) ||
        !_qmi->getGyroscope(gyr[0], gyr[1], gyr[2])) {
        return false;
    }

    // Gyro: deg/s -> rad/s sau khi trừ bias
    data.gx = (gyr[0] - gyroBias[0]) * PI / 180.0f;
    data.gy = (gyr[1] - gyroBias[1]) * PI / 180.0f;
    data.gz = (gyr[2] - gyroBias[2]) * PI / 180.0f;

    // Gyro micro-deadzone cho jitter rất nhỏ
    if (fabsf(data.gx) < 0.02f) data.gx = 0.0f;
    if (fabsf(data.gy) < 0.02f) data.gy = 0.0f;
    if (fabsf(data.gz) < 0.02f) data.gz = 0.0f;

    // Accel: scale & bias correction -> pre-filter ngay ở sensor frame
    data.ax = (acc[0] - accBias[0]) * accScale[0];
    data.ay = (acc[1] - accBias[1]) * accScale[1];
    data.az = (acc[2] - accBias[2]) * accScale[2];

    data.ax = _kAx.updateEstimate(data.ax);
    data.ay = _kAy.updateEstimate(data.ay);
    data.az = _kAz.updateEstimate(data.az);

    data.accMag = sqrtf(data.ax * data.ax + data.ay * data.ay + data.az * data.az);
    data.gyroMag = sqrtf(data.gx * data.gx + data.gy * data.gy + data.gz * data.gz);

    return true;
}
