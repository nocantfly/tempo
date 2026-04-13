#ifndef APP_TRACKING_H
#define APP_TRACKING_H

#include "IMUWrapper.h"
#include "KinematicsEngine.h"
#include "MadgwickFilter.h"
#include <stddef.h>

struct Vector3
{
    float x, y, z;
};

struct TimedPositionSample
{
    PositionData pos;
    unsigned long timestampMs;
};

class AppTracking
{
public:
    explicit AppTracking(SensorQMI8658 *qmi);

    void setup();
    void start();
    bool prepareForUse();
    bool runStaticCalibration();
    void update();
    void draw();
    void stop();

    void resetPosition();      // Bắt đầu lại motion alignment và zero state
    void markTargetPosition(); // Lưu vị trí hiện tại làm mốc bắt đầu
    void markFinalPosition();  // Lưu vị trí sau khi vung tay
    float getDistanceToTarget();
    bool isStable();

    void getEuler(float &roll, float &pitch, float &yaw);
    void getPosition(float &x, float &y, float &z);

    bool isMotionCalibrated();
    bool isReady() const { return startupReady; }
    size_t getRecordedTrajectoryCount() const { return trajectoryCount; }

private:
    IMUWrapper *_imu;
    MadgwickFilter *_filter;
    KinematicsEngine *_kinematics;

    bool isRunning = false;
    bool startupReady = false;
    unsigned long lastUpdateMicros = 0;
    unsigned long lastDrawMillis = 0;

    PositionData currentPos = {0.0f, 0.0f, 0.0f};
    Vector3 targetPos = {0.0f, 0.0f, 0.0f};
    Vector3 finalPos = {0.0f, 0.0f, 0.0f};

    float rollDeg = 0.0f;
    float pitchDeg = 0.0f;
    float yawDeg = 0.0f;
    float yawZeroOffsetDeg = 0.0f;

    bool actionRecording = false;
    bool actionMotionStarted = false;

    TimedPositionSample stabilityWindow[STABILITY_BUFFER_SIZE];
    size_t stabilityHead = 0;
    size_t stabilityCount = 0;

    TimedPositionSample trajectory[TRAJECTORY_MAX_POINTS];
    size_t trajectoryCount = 0;
    bool trajectoryOverflow = false;

    void resetSessionState();
    void resetMotionState();
    void resetStabilityWindow();
    void pushStabilitySample(unsigned long timestampMs);
    void resetTrajectory();
    void recordTrajectorySample(unsigned long timestampMs);
    float distanceBetween(const PositionData &a, const PositionData &b) const;
    void updateEulerDegrees(float q0, float q1, float q2, float q3);
    bool warmupFilterUntilStable();
};
#endif
