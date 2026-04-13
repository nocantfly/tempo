#include "AppTracking.h"
#include "config.h"

static float clampfLocal(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

AppTracking::AppTracking(Arduino_GFX *gfx, SensorQMI8658 *qmi) {
    _gfx = gfx;
    _imu = new IMUWrapper(qmi);
    _filter = new MadgwickFilter();
    _kinematics = new KinematicsEngine();
}

void AppTracking::resetSessionState() {
    currentPos = {0.0f, 0.0f, 0.0f};
    targetPos = {0.0f, 0.0f, 0.0f};
    finalPos = {0.0f, 0.0f, 0.0f};
    rollDeg = pitchDeg = yawDeg = 0.0f;
    actionRecording = false;
    actionMotionStarted = false;
    resetStabilityWindow();
    resetTrajectory();
}

void AppTracking::resetStabilityWindow() {
    stabilityHead = 0;
    stabilityCount = 0;
}

void AppTracking::resetTrajectory() {
    trajectoryCount = 0;
    trajectoryOverflow = false;
}

void AppTracking::recordTrajectorySample(unsigned long timestampMs) {
    if (!actionRecording) return;
    if (trajectoryCount < TRAJECTORY_MAX_POINTS) {
        trajectory[trajectoryCount++] = {currentPos, timestampMs};
    } else {
        trajectoryOverflow = true;
    }
}

void AppTracking::pushStabilitySample(unsigned long timestampMs) {
    size_t idx;
    if (stabilityCount < STABILITY_BUFFER_SIZE) {
        idx = (stabilityHead + stabilityCount) % STABILITY_BUFFER_SIZE;
        stabilityCount++;
    } else {
        stabilityHead = (stabilityHead + 1) % STABILITY_BUFFER_SIZE;
        idx = (stabilityHead + stabilityCount - 1) % STABILITY_BUFFER_SIZE;
    }

    stabilityWindow[idx] = {currentPos, timestampMs};

    while (stabilityCount > 0 &&
           (timestampMs - stabilityWindow[stabilityHead].timestampMs) > STABLE_WINDOW_MS) {
        stabilityHead = (stabilityHead + 1) % STABILITY_BUFFER_SIZE;
        stabilityCount--;
    }
}

float AppTracking::distanceBetween(const PositionData &a, const PositionData &b) const {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

void AppTracking::updateEulerDegrees(float q0, float q1, float q2, float q3) {
    float sinr_cosp = 2.0f * (q0 * q1 + q2 * q3);
    float cosr_cosp = 1.0f - 2.0f * (q1 * q1 + q2 * q2);
    rollDeg = atan2f(sinr_cosp, cosr_cosp) * 180.0f / PI;

    float sinp = 2.0f * (q0 * q2 - q3 * q1);
    sinp = clampfLocal(sinp, -1.0f, 1.0f);
    pitchDeg = asinf(sinp) * 180.0f / PI;

    float siny_cosp = 2.0f * (q0 * q3 + q1 * q2);
    float cosy_cosp = 1.0f - 2.0f * (q2 * q2 + q3 * q3);
    yawDeg = atan2f(siny_cosp, cosy_cosp) * 180.0f / PI;
}

void AppTracking::setup() {
    _imu->setup();
    resetSessionState();
    lastUpdateMicros = micros();
}

void AppTracking::start() {
    isRunning = true;
    _filter->reset();
    _kinematics->reset();
    resetSessionState();
    lastUpdateMicros = micros();
}

bool AppTracking::runStaticCalibration() {
    if (!isRunning) start();
    drawText("CALIBRATING... KEEP STILL", YELLOW);

    bool ok = _imu->calibrateStatic(STATIC_CALIB_SAMPLES);
    _filter->reset();
    _kinematics->reset();
    resetSessionState();
    lastUpdateMicros = micros();
    return ok;
}

void AppTracking::stop() {
    isRunning = false;
    actionRecording = false;
}

void AppTracking::resetPosition() {
    if (!isRunning) start();
    resetSessionState();
    _kinematics->startMotionCalibration();
    lastUpdateMicros = micros();
}

bool AppTracking::isMotionCalibrated() {
    return !_kinematics->isCalibrating();
}

void AppTracking::markTargetPosition() {
    targetPos = {currentPos.x, currentPos.y, currentPos.z};
    finalPos = targetPos;
    actionRecording = true;
    actionMotionStarted = false;
    resetStabilityWindow();
    resetTrajectory();
    recordTrajectorySample(millis());

    _gfx->fillCircle(LCD_WIDTH - 20, 20, 8, YELLOW);
}

void AppTracking::markFinalPosition() {
    finalPos = {currentPos.x, currentPos.y, currentPos.z};
    actionRecording = false;
}

float AppTracking::getDistanceToTarget() {
    PositionData a = {targetPos.x, targetPos.y, targetPos.z};
    PositionData b = {finalPos.x, finalPos.y, finalPos.z};
    return distanceBetween(a, b);
}

bool AppTracking::isStable() {
    if (!actionRecording || !actionMotionStarted || stabilityCount == 0) {
        return false;
    }

    const TimedPositionSample &oldest = stabilityWindow[stabilityHead];
    unsigned long nowMs = millis();
    if ((nowMs - oldest.timestampMs) < STABLE_WINDOW_MS) {
        return false;
    }

    return distanceBetween(currentPos, oldest.pos) < STABLE_DELTA_THRESHOLD;
}

void AppTracking::update() {
    if (!isRunning) return;

    IMUData d;
    if (_imu->read(d)) {
        unsigned long nowUs = micros();
        float dt = (nowUs - lastUpdateMicros) / 1000000.0f;
        if (dt <= 0.0f || dt > DT_MAX_SEC) dt = DT_FALLBACK_SEC;
        lastUpdateMicros = nowUs;

        _filter->update(d.gx, d.gy, d.gz, d.ax, d.ay, d.az, dt, d.accMag, d.gyroMag);

        float q0, q1, q2, q3;
        _filter->getQuaternion(q0, q1, q2, q3);
        updateEulerDegrees(q0, q1, q2, q3);

        currentPos = _kinematics->update(d.ax, d.ay, d.az, q0, q1, q2, q3, dt);

        if (actionRecording) {
            unsigned long nowMs = millis();
            recordTrajectorySample(nowMs);
            pushStabilitySample(nowMs);

            PositionData target = {targetPos.x, targetPos.y, targetPos.z};
            if (!actionMotionStarted && distanceBetween(currentPos, target) >= ACTION_START_DISTANCE) {
                actionMotionStarted = true;
            }
        }
    }
}

void AppTracking::drawText(String text, uint16_t color) {
    _gfx->fillScreen(BLACK);
    _gfx->setTextColor(color);
    _gfx->setTextSize(2);
    _gfx->setCursor(16, 180);
    _gfx->println(text);
}

void AppTracking::drawResult(bool success, float distance) {
    _gfx->fillScreen(success ? GREEN : RED);
    _gfx->setTextColor(BLACK);

    _gfx->setTextSize(4);
    _gfx->setCursor(40, 90);
    _gfx->println(success ? "SUCCESS" : "FAILED");

    _gfx->setTextSize(2);
    _gfx->setCursor(40, 170);
    _gfx->print("Dist: ");
    _gfx->println(distance, 2);

    _gfx->setCursor(40, 210);
    _gfx->print("Points: ");
    _gfx->println((int)trajectoryCount);

    if (trajectoryOverflow) {
        _gfx->setCursor(40, 250);
        _gfx->println("Trajectory clipped");
    }
}

void AppTracking::draw() {
    if (!isRunning || (millis() - lastDrawMillis) < UI_DRAW_INTERVAL_MS) return;

    if (_kinematics->isCalibrating()) {
        _gfx->fillScreen(BLACK);
        _gfx->setTextColor(WHITE);
        _gfx->setTextSize(2);
        _gfx->setCursor(20, 160);
        _gfx->println("ALIGN MOTION");
        _gfx->setCursor(20, 200);
        _gfx->println("SWING STRAIGHT ->");
        lastDrawMillis = millis();
        return;
    }

    _gfx->fillScreen(BLACK);

    _gfx->setTextSize(2);
    _gfx->setTextColor(WHITE);
    _gfx->setCursor(10, 12);
    _gfx->print("X:");
    _gfx->setTextColor(GREEN);
    _gfx->print(currentPos.x, 1);

    _gfx->setTextColor(WHITE);
    _gfx->setCursor(10, 48);
    _gfx->print("Y:");
    _gfx->setTextColor(BLUE);
    _gfx->print(currentPos.y, 1);

    _gfx->setTextColor(WHITE);
    _gfx->setCursor(10, 84);
    _gfx->print("Z:");
    _gfx->setTextColor(RED);
    _gfx->print(currentPos.z, 1);

    _gfx->setTextColor(WHITE);
    _gfx->setCursor(10, 150);
    _gfx->print("R:");
    _gfx->print(rollDeg, 1);
    _gfx->print(" deg");

    _gfx->setCursor(10, 186);
    _gfx->print("P:");
    _gfx->print(pitchDeg, 1);
    _gfx->print(" deg");

    _gfx->setCursor(10, 222);
    _gfx->print("Y:");
    _gfx->print(yawDeg, 1);
    _gfx->print(" deg");

    if (actionRecording) {
        _gfx->setCursor(10, 290);
        _gfx->setTextColor(actionMotionStarted ? YELLOW : GREY);
        _gfx->print("REC: ");
        _gfx->println(actionMotionStarted ? "MOVING" : "ARMED");
    }

    lastDrawMillis = millis();
}
