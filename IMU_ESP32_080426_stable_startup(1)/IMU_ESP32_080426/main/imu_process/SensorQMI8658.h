#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
#include "qmi8658.h"
#ifdef __cplusplus
}
#endif

class SensorQMI8658
{
public:
    enum AccRange
    {
        ACC_RANGE_2G,
        ACC_RANGE_4G,
        ACC_RANGE_8G
    };

    enum AccODR
    {
        ACC_ODR_125Hz,
        ACC_ODR_250Hz,
        ACC_ODR_500Hz
    };

    enum GyroRange
    {
        GYR_RANGE_256DPS,
        GYR_RANGE_512DPS,
        GYR_RANGE_1024DPS
    };

    enum GyroODR
    {
        GYR_ODR_125Hz,
        GYR_ODR_250Hz,
        GYR_ODR_500Hz
    };

    enum LPFMode
    {
        LPF_MODE_0,
        LPF_MODE_1,
        LPF_MODE_2,
        LPF_MODE_3
    };

public:
    SensorQMI8658(qmi8658_dev_t *dev);

    void configAccelerometer(AccRange range, AccODR odr, LPFMode lpf);
    void configGyroscope(GyroRange range, GyroODR odr, LPFMode lpf);

    void enableAccelerometer();
    void enableGyroscope();

    bool getDataReady();

    bool getAccelerometer(float &x, float &y, float &z);
    bool getGyroscope(float &x, float &y, float &z);
    bool getAll(float &ax, float &ay, float &az,
                float &gx, float &gy, float &gz);

private:
    qmi8658_dev_t *_dev;
};