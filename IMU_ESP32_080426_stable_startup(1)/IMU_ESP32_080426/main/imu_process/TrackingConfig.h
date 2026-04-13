#ifndef TRACKING_CONFIG_H
#define TRACKING_CONFIG_H

// --- CẤU HÌNH ĐỘNG TÁC / PHYSICS ---
#define MOVE_SENSITIVITY            0.08f
#define POSITION_GAIN               5.0f
#define VEL_FRICTION                0.96f
#define ACC_DEADZONE                0.01f
#define ACC_ZERO_CROSS_MIN          0.03f
#define EXTERNAL_FORCE_THRESHOLD    0.15f

// --- CẤU HÌNH MADGWICK ---
#define MADGWICK_BETA               0.10f
#define MADGWICK_BETA_STATIC        0.22f
#define MADGWICK_BETA_MOTION        0.03f
#define GYRO_STATIC_THRESHOLD       0.18f   // rad/s
#define GYRO_MEAS_ERROR             (PI * (40.0f / 180.0f))

// --- CẤU HÌNH KALMAN (PRE-FILTER SENSOR FRAME) ---
#define KALMAN_R                    0.15f
#define KALMAN_Q                    0.05f

// --- STARTUP / STABILIZATION ---
#define IMU_STARTUP_FLUSH_SAMPLES      120U
#define IMU_STARTUP_STABLE_SAMPLES     360U
#define IMU_STARTUP_TIMEOUT_MS         8000UL
#define IMU_STARTUP_WARMUP_MS          1200UL
#define IMU_STARTUP_MAX_WARMUP_MS      2800UL
#define IMU_STARTUP_READY_SAMPLES      120U
#define IMU_STATIONARY_ACC_TOL_G       0.08f
#define IMU_STATIONARY_GYRO_DPS        2.0f
#define IMU_GYRO_BIAS_ADAPT_ALPHA      0.015f
#define GYRO_DEADZONE_RAD              0.010f
#define STARTUP_EULER_DELTA_DEG        0.20f
#define STARTUP_YAW_DELTA_DEG          0.35f

// --- WORKFLOW / FSM ---
#define STATIC_CALIB_SAMPLES        IMU_STARTUP_STABLE_SAMPLES
#define STATIC_CALIB_TIMEOUT_MS     IMU_STARTUP_TIMEOUT_MS
#define UI_DRAW_INTERVAL_MS         100UL
#define DT_FALLBACK_SEC             0.002f
#define DT_MAX_SEC                  0.05f

// --- MOTION ALIGNMENT / Z-TARE ---
#define MOTION_ALIGN_THRESHOLD      0.30f
#define MOTION_CALIB_Z_CAPTURE_MAX  0.20f

// --- STABILITY / REVIEW ---
#define STABLE_WINDOW_MS            500UL
#define STABLE_DELTA_THRESHOLD      0.20f
#define ACTION_START_DISTANCE       0.75f
#define TARGET_RADIUS               5.0f
#define STABILITY_BUFFER_SIZE       320U
#define TRAJECTORY_MAX_POINTS       1024U

#endif
