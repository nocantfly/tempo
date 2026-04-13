#include "AppTracking.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "math.h"

#define PI 3.1415926535897932384626433832795f
#define millis() (esp_timer_get_time() / 1000ULL)
#define micros() (esp_timer_get_time())
#define delay(ms) vTaskDelay(pdMS_TO_TICKS(ms))

static float clampfLocal(float v, float lo, float hi)
{
    if (v < lo)
        return lo;
    if (v > hi)
        return hi;
    return v;
}

static float wrapAngle180Local(float angleDeg)
{
    while (angleDeg > 180.0f)
        angleDeg -= 360.0f;
    while (angleDeg < -180.0f)
        angleDeg += 360.0f;
    return angleDeg;
}

static float angleDeltaAbs(float aDeg, float bDeg)
{
    return fabsf(wrapAngle180Local(aDeg - bDeg));
}

AppTracking::AppTracking(SensorQMI8658 *qmi)
{
    _imu = new IMUWrapper(qmi);
    _filter = new MadgwickFilter();
    _kinematics = new KinematicsEngine();
}

void AppTracking::resetMotionState()
{
    currentPos = {0.0f, 0.0f, 0.0f};
    targetPos = {0.0f, 0.0f, 0.0f};
    finalPos = {0.0f, 0.0f, 0.0f};
    actionRecording = false;
    actionMotionStarted = false;
    resetStabilityWindow();
    resetTrajectory();
}

void AppTracking::resetSessionState()
{
    rollDeg = 0.0f;
    pitchDeg = 0.0f;
    yawDeg = 0.0f;
    yawZeroOffsetDeg = 0.0f;
    resetMotionState();
}

void AppTracking::resetStabilityWindow()
{
    stabilityHead = 0;
    stabilityCount = 0;
}

void AppTracking::resetTrajectory()
{
    trajectoryCount = 0;
    trajectoryOverflow = false;
}

void AppTracking::recordTrajectorySample(unsigned long timestampMs)
{
    if (!actionRecording)
        return;

    if (trajectoryCount < TRAJECTORY_MAX_POINTS)
    {
        trajectory[trajectoryCount++] = {currentPos, timestampMs};
    }
    else
    {
        trajectoryOverflow = true;
    }
}

void AppTracking::pushStabilitySample(unsigned long timestampMs)
{
    size_t idx;
    if (stabilityCount < STABILITY_BUFFER_SIZE)
    {
        idx = (stabilityHead + stabilityCount) % STABILITY_BUFFER_SIZE;
        stabilityCount++;
    }
    else
    {
        stabilityHead = (stabilityHead + 1) % STABILITY_BUFFER_SIZE;
        idx = (stabilityHead + stabilityCount - 1) % STABILITY_BUFFER_SIZE;
    }

    stabilityWindow[idx] = {currentPos, timestampMs};

    while (stabilityCount > 0 &&
           (timestampMs - stabilityWindow[stabilityHead].timestampMs) > STABLE_WINDOW_MS)
    {
        stabilityHead = (stabilityHead + 1) % STABILITY_BUFFER_SIZE;
        stabilityCount--;
    }
}

float AppTracking::distanceBetween(const PositionData &a, const PositionData &b) const
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

void AppTracking::updateEulerDegrees(float q0, float q1, float q2, float q3)
{
    float sinr_cosp = 2.0f * (q0 * q1 + q2 * q3);
    float cosr_cosp = 1.0f - 2.0f * (q2 * q2 + q1 * q1);
    rollDeg = wrapAngle180Local(atan2f(sinr_cosp, cosr_cosp) * 180.0f / PI);

    float sinp = 2.0f * (q0 * q2 - q3 * q1);
    sinp = clampfLocal(sinp, -1.0f, 1.0f);
    pitchDeg = asinf(sinp) * 180.0f / PI;

    float siny_cosp = 2.0f * (q0 * q3 + q1 * q2);
    float cosy_cosp = 1.0f - 2.0f * (q2 * q2 + q3 * q3);
    float rawYawDeg = atan2f(siny_cosp, cosy_cosp) * 180.0f / PI;
    yawDeg = wrapAngle180Local(rawYawDeg - yawZeroOffsetDeg);
}

void AppTracking::setup()
{
    _imu->setup();
    _filter->reset();
    _kinematics->reset();
    startupReady = false;
    isRunning = false;
    resetSessionState();
    lastUpdateMicros = micros();
}

void AppTracking::start()
{
    if (!startupReady)
        return;

    isRunning = true;
    lastUpdateMicros = micros();
}

bool AppTracking::warmupFilterUntilStable()
{
    unsigned long warmupStartMs = millis();
    float prevRoll = 0.0f;
    float prevPitch = 0.0f;
    float prevYaw = 0.0f;
    bool hasPrev = false;
    uint16_t stableCount = 0;

    while ((millis() - warmupStartMs) < IMU_STARTUP_MAX_WARMUP_MS)
    {
        IMUData d;
        if (!_imu->read(d))
        {
            delay(1);
            continue;
        }

        unsigned long nowUs = micros();
        float dt = (nowUs - lastUpdateMicros) / 1000000.0f;
        if (dt <= 0.0f || dt > DT_MAX_SEC)
            dt = DT_FALLBACK_SEC;
        lastUpdateMicros = nowUs;

        _filter->update(d.gx, d.gy, d.gz, d.ax, d.ay, d.az, dt, d.accMag, d.gyroMag);

        float q0, q1, q2, q3;
        _filter->getQuaternion(q0, q1, q2, q3);
        updateEulerDegrees(q0, q1, q2, q3);

        // Cho kinematics chạy cùng lúc để toàn bộ stack đã "nóng" trước khi ra UI.
        currentPos = _kinematics->update(d.ax, d.ay, d.az, q0, q1, q2, q3, dt);

        const bool stationary = (fabsf(d.accMag - 1.0f) <= IMU_STATIONARY_ACC_TOL_G) &&
                                (d.gyroMag <= GYRO_STATIC_THRESHOLD);

        if (stationary && hasPrev &&
            angleDeltaAbs(rollDeg, prevRoll) <= STARTUP_EULER_DELTA_DEG &&
            angleDeltaAbs(pitchDeg, prevPitch) <= STARTUP_EULER_DELTA_DEG &&
            angleDeltaAbs(yawDeg, prevYaw) <= STARTUP_YAW_DELTA_DEG)
        {
            stableCount++;
        }
        else
        {
            stableCount = 0;
        }

        prevRoll = rollDeg;
        prevPitch = pitchDeg;
        prevYaw = yawDeg;
        hasPrev = true;

        const unsigned long elapsedMs = millis() - warmupStartMs;
        if (elapsedMs >= IMU_STARTUP_WARMUP_MS && stableCount >= IMU_STARTUP_READY_SAMPLES)
        {
            return true;
        }

        delay(1);
    }

    return false;
}

bool AppTracking::prepareForUse()
{
    startupReady = false;
    isRunning = false;
    resetSessionState();
    _filter->reset();
    _kinematics->reset();

    IMUStartupReport startupReport;
    if (!_imu->runStartupCalibration(startupReport))
    {
        return false;
    }

    _filter->reset();
    _filter->initializeFromAccel(startupReport.avgAx,
                                 startupReport.avgAy,
                                 startupReport.avgAz,
                                 0.0f);

    lastUpdateMicros = micros();
    if (!warmupFilterUntilStable())
    {
        return false;
    }

    yawZeroOffsetDeg = yawDeg;
    float q0, q1, q2, q3;
    _filter->getQuaternion(q0, q1, q2, q3);
    updateEulerDegrees(q0, q1, q2, q3);

    _kinematics->reset();
    resetMotionState();
    startupReady = true;
    isRunning = true;
    lastUpdateMicros = micros();
    return true;
}

bool AppTracking::runStaticCalibration()
{
    return prepareForUse();
}

void AppTracking::stop()
{
    isRunning = false;
    actionRecording = false;
}

void AppTracking::resetPosition()
{
    if (!startupReady)
        return;

    if (!isRunning)
        start();

    resetMotionState();
    _kinematics->startMotionCalibration();
    lastUpdateMicros = micros();
}

bool AppTracking::isMotionCalibrated()
{
    return !_kinematics->isCalibrating();
}

void AppTracking::markTargetPosition()
{
    targetPos = {currentPos.x, currentPos.y, currentPos.z};
    finalPos = targetPos;
    actionRecording = true;
    actionMotionStarted = false;
    resetStabilityWindow();
    resetTrajectory();
    recordTrajectorySample(millis());
}

void AppTracking::markFinalPosition()
{
    finalPos = {currentPos.x, currentPos.y, currentPos.z};
    actionRecording = false;
}

float AppTracking::getDistanceToTarget()
{
    PositionData a = {targetPos.x, targetPos.y, targetPos.z};
    PositionData b = {finalPos.x, finalPos.y, finalPos.z};
    return distanceBetween(a, b);
}

bool AppTracking::isStable()
{
    if (!actionRecording || !actionMotionStarted || stabilityCount == 0)
    {
        return false;
    }

    const TimedPositionSample &oldest = stabilityWindow[stabilityHead];
    unsigned long nowMs = millis();
    if ((nowMs - oldest.timestampMs) < STABLE_WINDOW_MS)
    {
        return false;
    }

    return distanceBetween(currentPos, oldest.pos) < STABLE_DELTA_THRESHOLD;
}

void AppTracking::update()
{
    if (!isRunning || !startupReady)
        return;

    IMUData d;
    if (_imu->read(d))
    {
        unsigned long nowUs = micros();
        float dt = (nowUs - lastUpdateMicros) / 1000000.0f;
        if (dt <= 0.0f || dt > DT_MAX_SEC)
            dt = DT_FALLBACK_SEC;
        lastUpdateMicros = nowUs;

        _filter->update(d.gx, d.gy, d.gz, d.ax, d.ay, d.az, dt, d.accMag, d.gyroMag);

        float q0, q1, q2, q3;
        _filter->getQuaternion(q0, q1, q2, q3);
        updateEulerDegrees(q0, q1, q2, q3);

        currentPos = _kinematics->update(d.ax, d.ay, d.az, q0, q1, q2, q3, dt);

        if (actionRecording)
        {
            unsigned long nowMs = millis();
            recordTrajectorySample(nowMs);
            pushStabilitySample(nowMs);

            PositionData target = {targetPos.x, targetPos.y, targetPos.z};
            if (!actionMotionStarted && distanceBetween(currentPos, target) >= ACTION_START_DISTANCE)
            {
                actionMotionStarted = true;
            }
        }
    }
}

void AppTracking::getEuler(float &roll, float &pitch, float &yaw)
{
    roll = rollDeg;
    pitch = pitchDeg;
    yaw = yawDeg;
}

void AppTracking::getPosition(float &x, float &y, float &z)
{
    x = currentPos.x;
    y = currentPos.y;
    z = currentPos.z;
}
