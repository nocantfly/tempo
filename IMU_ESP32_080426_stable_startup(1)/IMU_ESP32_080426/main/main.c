#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_err.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "driver/gpio.h"
#include "qmi8658.h"
#include "application.h"
#include "ui.h"

i2c_master_bus_handle_t bus_handle;
static const char *TAG = "main";
qmi8658_dev_t *dev;

void qmi_task(void *arg)
{
    while (1)
    {
        qmi8658_data_t data;

        if (qmi8658_read_sensor_data(dev, &data) == ESP_OK)
        {
            ESP_LOGI(TAG, "accelX X: %.4f m/s², accelX Y: %.4f m/s²m, accelZ: %.4f m/s²"
                          "gyroX:%.4f, gyroY:%.4f, gyroZ:%.4f",
                     data.accelX, data.accelY, data.accelZ,
                     data.gyroX, data.gyroY, data.gyroZ);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    lv_display_t *disp = bsp_display_start();
    if (disp)
    {
        bsp_display_backlight_on();
        int display_width = lv_disp_get_hor_res(disp);
        int display_height = lv_disp_get_ver_res(disp);
        ESP_LOGI(TAG, "display_width: %d, display_height: %d", display_width, display_height);
    }
    // ui_create_calib_button();
    bsp_display_lock(0);
    ui_init();
    bsp_display_unlock();
    bus_handle = bsp_i2c_get_handle();

    // qmi8658_dev_t *dev = (qmi8658_dev_t *)malloc(sizeof(qmi8658_dev_t));
    // ESP_ERROR_CHECK(qmi8658_init(dev, bus_handle, QMI8658_ADDRESS_HIGH));
    // qmi8658_set_accel_range(dev, QMI8658_ACCEL_RANGE_8G);
    // qmi8658_set_accel_odr(dev, QMI8658_ACCEL_ODR_500HZ);
    // qmi8658_set_accel_unit_mps2(dev, true);
    // qmi8658_write_register(dev, QMI8658_CTRL5, 0x03);

    application_start(bus_handle);
}