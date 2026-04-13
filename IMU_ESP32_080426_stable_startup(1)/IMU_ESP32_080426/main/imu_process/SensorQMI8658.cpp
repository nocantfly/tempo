#include "SensorQMI8658.h"
#include "esp_log.h"
#include "esp_timer.h"
static const char *TAG = "QMI_WRAPPER";

SensorQMI8658::SensorQMI8658(qmi8658_dev_t *dev)
{
    _dev = dev;
}

void SensorQMI8658::configAccelerometer(AccRange range, AccODR odr, LPFMode lpf)
{
    qmi8658_accel_range_t r = QMI8658_ACCEL_RANGE_4G;
    if (range == ACC_RANGE_2G)
        r = QMI8658_ACCEL_RANGE_2G;
    else if (range == ACC_RANGE_8G)
        r = QMI8658_ACCEL_RANGE_8G;

    qmi8658_accel_odr_t o = QMI8658_ACCEL_ODR_500HZ;
    if (odr == ACC_ODR_125Hz)
        o = QMI8658_ACCEL_ODR_125HZ;
    else if (odr == ACC_ODR_250Hz)
        o = QMI8658_ACCEL_ODR_250HZ;

    qmi8658_set_accel_range(_dev, r);
    qmi8658_set_accel_odr(_dev, o);
    qmi8658_set_accel_unit_mps2(_dev, false); // dùng g cho IMUWrapper
}

void SensorQMI8658::configGyroscope(GyroRange range, GyroODR odr, LPFMode lpf)
{
    qmi8658_gyro_range_t r = QMI8658_GYRO_RANGE_1024DPS;
    if (range == GYR_RANGE_256DPS)
        r = QMI8658_GYRO_RANGE_256DPS;
    else if (range == GYR_RANGE_512DPS)
        r = QMI8658_GYRO_RANGE_512DPS;

    qmi8658_gyro_odr_t o = QMI8658_GYRO_ODR_500HZ;
    if (odr == GYR_ODR_125Hz)
        o = QMI8658_GYRO_ODR_125HZ;
    else if (odr == GYR_ODR_250Hz)
        o = QMI8658_GYRO_ODR_250HZ;

    qmi8658_set_gyro_range(_dev, r);
    qmi8658_set_gyro_odr(_dev, o);
}

void SensorQMI8658::enableAccelerometer()
{
    // thường accel auto enable khi config
}

void SensorQMI8658::enableGyroscope()
{
    // tương tự
}

bool SensorQMI8658::getDataReady()
{
    uint8_t status = 0;
    qmi8658_read_register(_dev, QMI8658_STATUS0, &status, 1);
    return (status & 0x01);
}

bool SensorQMI8658::getAccelerometer(float &x, float &y, float &z)
{
    qmi8658_data_t data;
    if (qmi8658_read_sensor_data(_dev, &data) != ESP_OK)
        return false;

    x = data.accelX;
    y = data.accelY;
    z = data.accelZ;
    static uint64_t last_log = 0;
    uint64_t now = esp_timer_get_time(); // us

    // 100ms = 100000 us
    // if (now - last_log < 100000)
    //     return true;
    // last_log = now;
    // ESP_LOGI("QMI8658",
    //          "ACC[g]: %.3f %.3f %.3f ",
    //          x, y, z);
    return true;
}

bool SensorQMI8658::getGyroscope(float &x, float &y, float &z)
{
    qmi8658_data_t data;
    if (qmi8658_read_sensor_data(_dev, &data) != ESP_OK)
        return false;

    x = data.gyroX;
    y = data.gyroY;
    z = data.gyroZ;

    static uint64_t last_log = 0;
    uint64_t now = esp_timer_get_time(); // us

    // 100ms = 100000 us
    // if (now - last_log < 500000)
    //     return true;
    // last_log = now;
    // ESP_LOGI("QMI8658",
    //          "GYR[dps]: %.3f %.3f %.3f",
    //          x, y, z);

    return true;
}

bool SensorQMI8658::getAll(float &ax, float &ay, float &az,
                           float &gx, float &gy, float &gz)
{
    qmi8658_data_t data;

    if (qmi8658_read_sensor_data(_dev, &data) != ESP_OK)
        return false;

    ax = data.accelX;
    ay = data.accelY;
    az = data.accelZ;

    gx = data.gyroX;
    gy = data.gyroY;
    gz = data.gyroZ;

    static uint64_t last_log = 0;
    uint64_t now = esp_timer_get_time(); // us

    // 100ms = 100000 us
    // if (now - last_log < 500000)
    //     return true;
    // last_log = now;
    // ESP_LOGI("QMI8658",
    //          "ACC[g]: %.3f %.3f %.3f | GYR[dps]: %.3f %.3f %.3f",
    //          ax, ay, az,
    //          gx, gy, gz);
    return true;
}
