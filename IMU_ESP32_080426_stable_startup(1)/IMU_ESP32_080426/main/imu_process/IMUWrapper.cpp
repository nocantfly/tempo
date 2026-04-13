#include "IMUWrapper.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "math.h"

#define delay vTaskDelay
#define PI M_PI
#define millis() (esp_timer_get_time() / 1000ULL)

IMUWrapper::IMUWrapper(SensorQMI8658 *qmi)
    : _qmi(qmi),
      _kAx(KALMAN_R, KALMAN_R, KALMAN_Q),
      _kAy(KALMAN_R, KALMAN_R, KALMAN_Q),
      _kAz(KALMAN_R, KALMAN_R, KALMAN_Q)
{
}

void IMUWrapper::setup()
{
    _qmi->configAccelerometer(SensorQMI8658::ACC_RANGE_2G,
                              SensorQMI8658::ACC_ODR_500Hz,
                              SensorQMI8658::LPF_MODE_3);
    delay(pdMS_TO_TICKS(50));

    _qmi->configGyroscope(SensorQMI8658::GYR_RANGE_1024DPS,
                          (SensorQMI8658::GyroODR)SensorQMI8658::ACC_ODR_500Hz,
                          SensorQMI8658::LPF_MODE_3);
    delay(pdMS_TO_TICKS(50));

    _qmi->enableAccelerometer();
    _qmi->enableGyroscope();
    delay(pdMS_TO_TICKS(50));

    computeCalibrationFactors();
    gyroBias[0] = gyroBias[1] = gyroBias[2] = 0.0f;
    resetFilters();
}

void IMUWrapper::resetFilters()
{
    _kAx.reset(0.0f);
    _kAy.reset(0.0f);
    _kAz.reset(0.0f);
    filtersSeeded = false;
}

void IMUWrapper::resetFilters(float ax, float ay, float az)
{
    _kAx.reset(ax);
    _kAy.reset(ay);
    _kAz.reset(az);
    filtersSeeded = true;
}

void IMUWrapper::computeCalibrationFactors()
{
    for (int i = 0; i < 3; i++)
    {
        accBias[i] = (user_AccMax[i] + user_AccMin[i]) / 2.0f;
        float range = user_AccMax[i] - user_AccMin[i];
        accScale[i] = (range != 0.0f) ? (2.0f / range) : 1.0f;
    }
}

bool IMUWrapper::readRaw(float &ax, float &ay, float &az,
                         float &gx, float &gy, float &gz)
{
    if (!_qmi->getDataReady())
    {
        return false;
    }

    return _qmi->getAll(ax, ay, az, gx, gy, gz);
}

void IMUWrapper::applyAccelCorrection(float &ax, float &ay, float &az) const
{
    ax = (ax - accBias[0]) * accScale[0];
    ay = (ay - accBias[1]) * accScale[1];
    az = (az - accBias[2]) * accScale[2];
}

bool IMUWrapper::isStationary(float ax, float ay, float az,
                              float gxDps, float gyDps, float gzDps) const
{
    const float accMag = sqrtf(ax * ax + ay * ay + az * az);
    const float gyroMagDps = sqrtf(gxDps * gxDps + gyDps * gyDps + gzDps * gzDps);

    return (fabsf(accMag - 1.0f) <= IMU_STATIONARY_ACC_TOL_G) &&
           (gyroMagDps <= IMU_STATIONARY_GYRO_DPS);
}

void IMUWrapper::updateGyroBiasIfStationary(float ax, float ay, float az,
                                            float gxDps, float gyDps, float gzDps)
{
    if (!isStationary(ax, ay, az, gxDps, gyDps, gzDps))
    {
        return;
    }

    gyroBias[0] += IMU_GYRO_BIAS_ADAPT_ALPHA * (gxDps - gyroBias[0]);
    gyroBias[1] += IMU_GYRO_BIAS_ADAPT_ALPHA * (gyDps - gyroBias[1]);
    gyroBias[2] += IMU_GYRO_BIAS_ADAPT_ALPHA * (gzDps - gyroBias[2]);
}

bool IMUWrapper::runStartupCalibration(IMUStartupReport &report,
                                       uint16_t sampleCount,
                                       uint32_t timeoutMs)
{
    float ax = 0.0f, ay = 0.0f, az = 0.0f;
    float gx = 0.0f, gy = 0.0f, gz = 0.0f;

    resetFilters();

    uint16_t flushed = 0;
    unsigned long flushStart = millis();
    while (flushed < IMU_STARTUP_FLUSH_SAMPLES && (millis() - flushStart) < timeoutMs)
    {
        if (readRaw(ax, ay, az, gx, gy, gz))
        {
            flushed++;
        }
        delay(pdMS_TO_TICKS(1));
    }

    float sumAcc[3] = {0.0f, 0.0f, 0.0f};
    float sumGyr[3] = {0.0f, 0.0f, 0.0f};
    uint16_t stableSamples = 0;
    unsigned long tStart = millis();

    while ((millis() - tStart) < timeoutMs)
    {
        if (!readRaw(ax, ay, az, gx, gy, gz))
        {
            delay(pdMS_TO_TICKS(1));
            continue;
        }

        applyAccelCorrection(ax, ay, az);

        if (!isStationary(ax, ay, az, gx, gy, gz))
        {
            stableSamples = 0;
            sumAcc[0] = sumAcc[1] = sumAcc[2] = 0.0f;
            sumGyr[0] = sumGyr[1] = sumGyr[2] = 0.0f;
            delay(pdMS_TO_TICKS(1));
            continue;
        }

        sumAcc[0] += ax;
        sumAcc[1] += ay;
        sumAcc[2] += az;

        sumGyr[0] += gx;
        sumGyr[1] += gy;
        sumGyr[2] += gz;

        stableSamples++;
        if (stableSamples >= sampleCount)
        {
            break;
        }

        delay(pdMS_TO_TICKS(1));
    }

    if (stableSamples == 0 || stableSamples < sampleCount)
    {
        return false;
    }

    report.avgAx = sumAcc[0] / stableSamples;
    report.avgAy = sumAcc[1] / stableSamples;
    report.avgAz = sumAcc[2] / stableSamples;
    report.avgGx = sumGyr[0] / stableSamples;
    report.avgGy = sumGyr[1] / stableSamples;
    report.avgGz = sumGyr[2] / stableSamples;
    report.accMag = sqrtf(report.avgAx * report.avgAx +
                          report.avgAy * report.avgAy +
                          report.avgAz * report.avgAz);
    report.gyroMag = sqrtf(report.avgGx * report.avgGx +
                           report.avgGy * report.avgGy +
                           report.avgGz * report.avgGz) * PI / 180.0f;
    report.stableSamples = stableSamples;

    gyroBias[0] = report.avgGx;
    gyroBias[1] = report.avgGy;
    gyroBias[2] = report.avgGz;

    resetFilters(report.avgAx, report.avgAy, report.avgAz);
    return true;
}

bool IMUWrapper::calibrateStatic(uint16_t sampleCount)
{
    IMUStartupReport report;
    return runStartupCalibration(report, sampleCount, STATIC_CALIB_TIMEOUT_MS);
}

bool IMUWrapper::read(IMUData &data)
{
    float rawAx = 0.0f;
    float rawAy = 0.0f;
    float rawAz = 0.0f;
    float rawGx = 0.0f;
    float rawGy = 0.0f;
    float rawGz = 0.0f;

    if (!readRaw(rawAx, rawAy, rawAz, rawGx, rawGy, rawGz))
    {
        return false;
    }

    applyAccelCorrection(rawAx, rawAy, rawAz);
    updateGyroBiasIfStationary(rawAx, rawAy, rawAz, rawGx, rawGy, rawGz);

    if (!filtersSeeded)
    {
        resetFilters(rawAx, rawAy, rawAz);
    }

    data.ax = _kAx.updateEstimate(rawAx);
    data.ay = _kAy.updateEstimate(rawAy);
    data.az = _kAz.updateEstimate(rawAz);

    data.gx = (rawGx - gyroBias[0]) * PI / 180.0f;
    data.gy = (rawGy - gyroBias[1]) * PI / 180.0f;
    data.gz = (rawGz - gyroBias[2]) * PI / 180.0f;

    if (fabsf(data.gx) < GYRO_DEADZONE_RAD)
        data.gx = 0.0f;
    if (fabsf(data.gy) < GYRO_DEADZONE_RAD)
        data.gy = 0.0f;
    if (fabsf(data.gz) < GYRO_DEADZONE_RAD)
        data.gz = 0.0f;

    data.accMag = sqrtf(data.ax * data.ax + data.ay * data.ay + data.az * data.az);
    data.gyroMag = sqrtf(data.gx * data.gx + data.gy * data.gy + data.gz * data.gz);

    return true;
}
