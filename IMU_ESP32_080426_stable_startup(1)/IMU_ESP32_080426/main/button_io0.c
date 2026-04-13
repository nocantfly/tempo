#include "button_io0.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <stdbool.h>

#define BTN_IO GPIO_NUM_0
#define BTN_ACTIVE_LEVEL 0

#define POLL_MS 10
#define DEBOUNCE_MS 30
#define CLICK_MIN_MS 30
#define CLICK_MAX_MS 800

static const char *TAG = "BTN_IO0";

static button_click_cb_t s_click_cb = NULL;

static inline uint32_t millis(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static void button_io0_task(void *arg)
{
    bool raw_last = gpio_get_level(BTN_IO);
    bool stable_state = raw_last;
    uint32_t last_change_ms = millis();
    uint32_t press_start_ms = 0;

    ESP_LOGI(TAG, "button task started");

    while (1)
    {
        bool raw_now = gpio_get_level(BTN_IO);
        uint32_t now = millis();

        if (raw_now != raw_last)
        {
            raw_last = raw_now;
            last_change_ms = now;
        }

        if ((now - last_change_ms) >= DEBOUNCE_MS)
        {
            if (stable_state != raw_now)
            {
                stable_state = raw_now;

                if (stable_state == BTN_ACTIVE_LEVEL)
                {
                    press_start_ms = now;
                    ESP_LOGI(TAG, "button pressed");
                }
                else
                {
                    uint32_t press_time = now - press_start_ms;
                    ESP_LOGI(TAG, "button released, press_time=%lu ms", (unsigned long)press_time);

                    if (press_time >= CLICK_MIN_MS && press_time <= CLICK_MAX_MS)
                    {
                        ESP_LOGI(TAG, "button click detected");

                        if (s_click_cb)
                        {
                            s_click_cb();
                        }
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(POLL_MS));
    }
}

void button_io0_start(button_click_cb_t cb)
{
    s_click_cb = cb;

    gpio_config_t io_conf = {0};
    io_conf.pin_bit_mask = (1ULL << BTN_IO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    xTaskCreatePinnedToCore(
        button_io0_task,
        "button_io0_task",
        4096,
        NULL,
        5,
        NULL,
        1);
}