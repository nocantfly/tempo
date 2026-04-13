#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_lvgl_port.h"
#include "esp_timer.h"

extern "C"
{
#include "button_io0.h"
#include "qmi8658.h"
#include "screen_loading.h"
#include "screen_mode.h"
#include "ui.h"
}

// ===== C++ =====
#include "AppTracking.h"
#include "SensorQMI8658.h"
#include <stdlib.h>
#include "application.h"
#include "string.h"

static const char *TAG = "APP";

// ===== HARDWARE =====
static SensorQMI8658 *qmi = nullptr;
static AppTracking *appTracking = nullptr;

// ===== latest live data =====
static app_pose_t s_live_pose = {0};
static app_pose_t s_pose_before = {0};
static app_pose_t s_pose_after = {0};
static bool s_has_pose_before = false;
static bool s_has_pose_after = false;

// ===== UI API =====
extern "C" void screen_mode_update_pose(float roll, float pitch, float yaw, float distance_m);
extern "C" void screen_mode_handle_button_click(void);

// ===== CONFIG =====
#define delay(ms) vTaskDelay(pdMS_TO_TICKS(ms))
#define millis() (esp_timer_get_time() / 1000ULL)

hand_type_t g_hand_type = HAND_TYPE_LEFT;

static void loading_ui_update(int percent, const char *status, bool switch_to_select)
{
    lvgl_port_lock(0);
    screen_loading_set_progress(percent);
    if (status)
    {
        screen_loading_set_status(status);
    }
    if (switch_to_select)
    {
        ui_switch_screen(UI_SCREEN_SELECT);
    }
    lvgl_port_unlock();
}

void application_set_hand_type(hand_type_t hand)
{
    g_hand_type = hand;
    ESP_LOGW(TAG, "Hand type set to: %s", (hand == HAND_TYPE_LEFT) ? "LEFT" : "RIGHT");
}

static void on_screen_mode_save(screen_mode_save_target_t target)
{
    if (target == SCREEN_MODE_SAVE_BEFORE)
    {
        s_pose_before = s_live_pose;
        s_has_pose_before = true;

        ESP_LOGW(TAG,
                 "SAVE BEFORE: roll=%.2f pitch=%.2f yaw=%.2f dist=%.2f",
                 s_pose_before.roll,
                 s_pose_before.pitch,
                 s_pose_before.yaw,
                 s_pose_before.distance_m);
    }
    else if (target == SCREEN_MODE_SAVE_AFTER)
    {
        s_pose_after = s_live_pose;
        s_has_pose_after = true;

        ESP_LOGW(TAG,
                 "SAVE AFTER: roll=%.2f pitch=%.2f yaw=%.2f dist=%.2f",
                 s_pose_after.roll,
                 s_pose_after.pitch,
                 s_pose_after.yaw,
                 s_pose_after.distance_m);
    }
}

static void on_io0_click(void)
{
    lvgl_port_lock(0);
    screen_mode_handle_button_click();
    lvgl_port_unlock();
}

static void refresh_live_pose_to_ui(void)
{
    float r, p, y;
    appTracking->getEuler(r, p, y);
    float dist = appTracking->getDistanceToTarget();

    s_live_pose.roll = r;
    s_live_pose.pitch = p;
    s_live_pose.yaw = y;
    s_live_pose.distance_m = dist;

    lvgl_port_lock(0);
    screen_mode_update_pose(r, p, y, dist);
    lvgl_port_unlock();
}

static bool run_boot_sequence(void)
{
    loading_ui_update(5, "Dang khoi dong bo cam bien IMU...", false);
    delay(80);

    loading_ui_update(20, "Dang on dinh tin hieu va danh gia sensor...", false);

    if (!appTracking->prepareForUse())
    {
        loading_ui_update(10, "Khong the on dinh sensor. Hay giu yen thiet bi va thu lai...", false);
        return false;
    }

    refresh_live_pose_to_ui();

    loading_ui_update(85, "Da hieu chuan xong. Dang khoa du lieu on dinh...", false);
    delay(120);
    loading_ui_update(100, "Cam bien san sang. Co the vao bai tap ngay.", true);
    return true;
}

// =====================================================
// MAIN TASK
// =====================================================
static void app_task(void *arg)
{
    ESP_LOGI(TAG, "Application started");

    bool tracking_ready = false;
    uint32_t last_ui = 0;

    while (!tracking_ready)
    {
        tracking_ready = run_boot_sequence();
        if (!tracking_ready)
        {
            delay(500);
        }
    }

    while (1)
    {
        appTracking->update();

        if (millis() - last_ui >= UI_DRAW_INTERVAL_MS)
        {
            last_ui = millis();
            refresh_live_pose_to_ui();
        }

        delay(10);
    }
}

// =====================================================
// PUBLIC API
// =====================================================
extern "C" void application_prepare_session(void)
{
    if (!appTracking || !appTracking->isReady())
        return;

    appTracking->resetPosition();
    refresh_live_pose_to_ui();
}

extern "C" bool application_get_pose_before(app_pose_t *out_pose)
{
    if (!out_pose || !s_has_pose_before)
        return false;

    *out_pose = s_pose_before;
    return true;
}

extern "C" bool application_get_pose_after(app_pose_t *out_pose)
{
    if (!out_pose || !s_has_pose_after)
        return false;

    *out_pose = s_pose_after;
    return true;
}

// =====================================================
// INIT
// =====================================================
extern "C" void application_start(i2c_master_bus_handle_t bus)
{
    ESP_LOGI(TAG, "Init application...");

    // ===== init IMU =====
    qmi8658_dev_t *dev = (qmi8658_dev_t *)malloc(sizeof(qmi8658_dev_t));
    memset(dev, 0, sizeof(qmi8658_dev_t));

    ESP_ERROR_CHECK(qmi8658_init(dev, bus, QMI8658_ADDRESS_HIGH));
    qmi = new SensorQMI8658(dev);

    // ===== init tracking =====
    appTracking = new AppTracking(qmi);
    appTracking->setup();

    // ===== register UI save callback =====
    screen_mode_register_save_callback(on_screen_mode_save);

    // ===== IO0 button =====
    button_io0_start(on_io0_click);

    // ===== initial loading text =====
    loading_ui_update(0, "Dang chuan bi bo cam bien...", false);

    // ===== start task =====
    xTaskCreatePinnedToCore(
        app_task,
        "app_task",
        8192,
        NULL,
        5,
        NULL,
        1);
}
