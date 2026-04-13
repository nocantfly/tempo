#include "KinematicsEngine.h"

KinematicsEngine::KinematicsEngine() {
    reset();
}

void KinematicsEngine::zeroDynamicState() {
    for (int i = 0; i < 3; i++) {
        vel[i] = 0.0f;
        pos[i] = 0.0f;
        lastAcc[i] = 0.0f;
    }
}

void KinematicsEngine::reset() {
    motionCos = 1.0f;
    motionSin = 0.0f;
    _isMotionCalibrating = false;
    zTare = 0.0f;
    zTareAccum = 0.0f;
    zTareSamples = 0;
    zeroDynamicState();
}

void KinematicsEngine::startMotionCalibration() {
    zeroDynamicState();
    zTare = 0.0f;
    zTareAccum = 0.0f;
    zTareSamples = 0;
    _isMotionCalibrating = true;
}

void KinematicsEngine::removeGravityAndToWorldFrame(float ax, float ay, float az,
                                                    float q0, float q1, float q2, float q3,
                                                    float &wX, float &wY, float &wZ) {
    // 1. Trọng lực dự đoán trong sensor frame
    float gravX = 2.0f * (q1 * q3 - q0 * q2);
    float gravY = 2.0f * (q0 * q1 + q2 * q3);
    float gravZ = q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3;

    // 2. Trừ trọng lực -> linear acceleration trong sensor frame
    float linX = ax - gravX;
    float linY = ay - gravY;
    float linZ = az - gravZ;

    // 3. Xoay sang world frame
    float q0q0 = q0 * q0;
    float q1q1 = q1 * q1;
    float q2q2 = q2 * q2;
    float q3q3 = q3 * q3;

    wX = (q0q0 + q1q1 - q2q2 - q3q3) * linX + 2.0f * (q1 * q2 - q0 * q3) * linY + 2.0f * (q1 * q3 + q0 * q2) * linZ;
    wY = 2.0f * (q1 * q2 + q0 * q3) * linX + (q0q0 - q1q1 + q2q2 - q3q3) * linY + 2.0f * (q2 * q3 - q0 * q1) * linZ;
    wZ = 2.0f * (q1 * q3 - q0 * q2) * linX + 2.0f * (q2 * q3 + q0 * q1) * linY + (q0q0 - q1q1 - q2q2 + q3q3) * linZ;
}

void KinematicsEngine::alignMotion(float &wX, float &wY) {
    float tempX = wX;
    float tempY = wY;
    wX = tempX * motionCos - tempY * motionSin;
    wY = tempX * motionSin + tempY * motionCos;
}

PositionData KinematicsEngine::update(float ax, float ay, float az,
                                      float q0, float q1, float q2, float q3,
                                      float dt) {
    float wX, wY, wZ;
    removeGravityAndToWorldFrame(ax, ay, az, q0, q1, q2, q3, wX, wY, wZ);

    // --- Motion Alignment calibration ---
    if (_isMotionCalibrating) {
        if (fabsf(wZ) < MOTION_CALIB_Z_CAPTURE_MAX) {
            zTareAccum += wZ;
            zTareSamples++;
        }

        float magXY = sqrtf(wX * wX + wY * wY);
        if (magXY > MOTION_ALIGN_THRESHOLD) {
            float angle = atan2f(wY, wX);
            float delta = (PI / 2.0f) - angle;
            motionCos = cosf(delta);
            motionSin = sinf(delta);

            if (zTareSamples > 0) {
                zTare = zTareAccum / zTareSamples;
            }

            _isMotionCalibrating = false;
            zeroDynamicState();
        }
        return {0.0f, 0.0f, 0.0f};
    }

    // --- Spatial transformation ---
    alignMotion(wX, wY);
    wZ -= zTare; // Hard Z-Tare theo workflow

    // --- Deadzone ---
    if (fabsf(wX) < ACC_DEADZONE) wX = 0.0f;
    if (fabsf(wY) < ACC_DEADZONE) wY = 0.0f;
    if (fabsf(wZ) < ACC_DEADZONE) wZ = 0.0f;

    // --- Zero Crossing Brake + Virtual Friction + Integration ---
    float accInput[3] = {wX, wY, wZ};
    const float gScale = 981.0f * POSITION_GAIN; // cm/s^2 nếu accel ở đơn vị g

    for (int i = 0; i < 3; i++) {
        float a = accInput[i];

        bool zeroCross = (a * lastAcc[i] < 0.0f) &&
                         (fabsf(a) > ACC_ZERO_CROSS_MIN) &&
                         (fabsf(lastAcc[i]) > ACC_ZERO_CROSS_MIN);
        if (zeroCross) {
            vel[i] = 0.0f;
        }

        vel[i] += a * gScale * dt;
        vel[i] *= VEL_FRICTION;
        pos[i] += vel[i] * dt;
        lastAcc[i] = a;
    }

    return {pos[0], pos[1], pos[2]};
}
