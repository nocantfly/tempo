#ifndef KINEMATICS_ENGINE_H
#define KINEMATICS_ENGINE_H

#include "TrackingConfig.h"
#include "stdint.h"
struct PositionData {
    float x, y, z;
};

class KinematicsEngine {
public:
    KinematicsEngine();
    void reset();
    void zeroDynamicState();
    void startMotionCalibration();
    bool isCalibrating() const { return _isMotionCalibrating; }

    // Hàm chính: Xử lý vật lý và trả về vị trí
    PositionData update(float ax, float ay, float az,
                        float q0, float q1, float q2, float q3,
                        float dt);

private:
    // Motion Alignment vars
    float motionCos = 1.0f;
    float motionSin = 0.0f;
    bool _isMotionCalibrating = false;

    // Hard Z-Tare
    float zTare = 0.0f;
    float zTareAccum = 0.0f;
    uint16_t zTareSamples = 0;

    // Kinematics vars
    float vel[3] = {0.0f, 0.0f, 0.0f};
    float pos[3] = {0.0f, 0.0f, 0.0f};
    float lastAcc[3] = {0.0f, 0.0f, 0.0f};

    // Helper functions
    void removeGravityAndToWorldFrame(float ax, float ay, float az,
                                      float q0, float q1, float q2, float q3,
                                      float &wX, float &wY, float &wZ);
    void alignMotion(float &wX, float &wY);
};
#endif
