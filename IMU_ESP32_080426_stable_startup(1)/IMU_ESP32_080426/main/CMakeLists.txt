set(LV_DEMO_DIR ../managed_components/lvgl__lvgl/demos)
file(GLOB_RECURSE LV_DEMOS_SOURCES ${LV_DEMO_DIR}/*.c)

idf_component_register(
    SRCS main.c ${LV_DEMOS_SOURCES} application.cpp
    button_io0.c

    imu_process/SimpleKalmanFilter.cpp
    imu_process/MadgwickFilter.cpp
    imu_process/KinematicsEngine.cpp
    imu_process/MadgwickFilter.cpp
    imu_process/SensorQMI8658.cpp
    imu_process/IMUWrapper.cpp
    imu_process/AppTracking.cpp

    ui/ui.c
    ui/components/ui_header.c
    ui/screens/screen_loading.c
    ui/screens/screen_main.c
    ui/screens/screen_personal.c
    ui/screens/screen_team.c
    ui/screens/screen_mode.c
    ui/screens/screen_select.c

    ui/fonts/font_noto_basic_20_4.c
    ui/fonts/font_noto_basic_30_4.c
    ui/fonts/ui_font_Font72Bold.c
    ui/fonts/ui_font_Font100.c
    ui/fonts/ui_font_Font130.c

    ui/images/ui_img_bgr_png.c

    INCLUDE_DIRS . ${LV_DEMO_DIR} imu_process 
    ui ui/components ui/screens 
    PRIV_REQUIRES driver nvs_flash esp_timer esp_lcd
    
)

idf_component_get_property(LVGL_LIB lvgl__lvgl COMPONENT_LIB)

target_compile_options(
    ${LVGL_LIB}
    PRIVATE
    -DLV_LVGL_H_INCLUDE_SIMPLE
    -DLV_USE_DEMO_MUSIC
)
