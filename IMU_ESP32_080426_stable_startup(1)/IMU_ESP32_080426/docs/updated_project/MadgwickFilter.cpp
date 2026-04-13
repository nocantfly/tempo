#include "MadgwickFilter.h"

MadgwickFilter::MadgwickFilter() { reset(); }

void MadgwickFilter::reset() {
    q0 = 1.0f; q1 = 0.0f; q2 = 0.0f; q3 = 0.0f;
    beta = MADGWICK_BETA;
}

void MadgwickFilter::getQuaternion(float &q0_out, float &q1_out, float &q2_out, float &q3_out) {
    q0_out = q0;
    q1_out = q1;
    q2_out = q2;
    q3_out = q3;
}

void MadgwickFilter::update(float gx, float gy, float gz,
                            float ax, float ay, float az,
                            float dt, float accMag, float gyroMag) {
    if (fabsf(accMag - 1.0f) > EXTERNAL_FORCE_THRESHOLD) {
        beta = MADGWICK_BETA_MOTION;
    } else {
        beta = MADGWICK_BETA;
    }
    if (fabsf(accMag - 1.0f) < MOVE_SENSITIVITY && gyroMag < GYRO_STATIC_THRESHOLD) {
        beta = MADGWICK_BETA_STATIC;
    }

    float recipNorm;
    float s0, s1, s2, s3;
    float qDot1, qDot2, qDot3, qDot4;
    float _2q0, _2q1, _2q2, _2q3, _4q0, _4q1, _4q2, _8q1, _8q2;
    float q0q0, q1q1, q2q2, q3q3;

    qDot1 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz);
    qDot2 = 0.5f * ( q0 * gx + q2 * gz - q3 * gy);
    qDot3 = 0.5f * ( q0 * gy - q1 * gz + q3 * gx);
    qDot4 = 0.5f * ( q0 * gz + q1 * gy - q2 * gx);

    float accNormSq = ax * ax + ay * ay + az * az;
    if (beta > 0.0f && accNormSq > 1e-8f) {
        recipNorm = 1.0f / sqrtf(accNormSq);
        ax *= recipNorm;
        ay *= recipNorm;
        az *= recipNorm;

        _2q0 = 2.0f * q0;
        _2q1 = 2.0f * q1;
        _2q2 = 2.0f * q2;
        _2q3 = 2.0f * q3;
        _4q0 = 4.0f * q0;
        _4q1 = 4.0f * q1;
        _4q2 = 4.0f * q2;
        _8q1 = 8.0f * q1;
        _8q2 = 8.0f * q2;
        q0q0 = q0 * q0;
        q1q1 = q1 * q1;
        q2q2 = q2 * q2;
        q3q3 = q3 * q3;

        s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
        s1 = _4q1 * q3q3 - _2q3 * ax + 4.0f * q0q0 * q1 - _2q0 * ay
           - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
        s2 = 4.0f * q0q0 * q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay
           - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
        s3 = 4.0f * q1q1 * q3 - _2q1 * ax + 4.0f * q2q2 * q3 - _2q2 * ay;

        float stepNormSq = s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3;
        if (stepNormSq > 1e-12f) {
            recipNorm = 1.0f / sqrtf(stepNormSq);
            s0 *= recipNorm;
            s1 *= recipNorm;
            s2 *= recipNorm;
            s3 *= recipNorm;

            qDot1 -= beta * s0;
            qDot2 -= beta * s1;
            qDot3 -= beta * s2;
            qDot4 -= beta * s3;
        }
    }

    q0 += qDot1 * dt;
    q1 += qDot2 * dt;
    q2 += qDot3 * dt;
    q3 += qDot4 * dt;

    float qNormSq = q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3;
    if (qNormSq > 1e-12f) {
        recipNorm = 1.0f / sqrtf(qNormSq);
        q0 *= recipNorm;
        q1 *= recipNorm;
        q2 *= recipNorm;
        q3 *= recipNorm;
    } else {
        reset();
    }
}
