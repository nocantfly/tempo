#include "config.h"
#include "AppTracking.h"
#include "WatchFace.h"
#include <XPowersLib.h>

// --- PHẦN CỨNG ---
XPowersAXP2101 power;
SensorQMI8658 qmi;
Arduino_DataBus *bus = new Arduino_ESP32QSPI(LCD_CS, LCD_SCK, LCD_D0, LCD_D1, LCD_D2, LCD_D3);
Arduino_GFX *gfx = new Arduino_CO5300(bus, LCD_RST, 0, LCD_WIDTH, LCD_HEIGHT, 22, 0, 0, 0);

AppTracking *appTracking = NULL;
WatchFace *watchFace = NULL;

// --- STATE MACHINE DEFINITION ---
enum AppState {
    STATE_IDLE,
    STATE_CALIB_STATIC,
    STATE_ALIGN_MOTION,
    STATE_TRACKING_READY,
    STATE_RECORDING_ACTION,
    STATE_WAIT_FOR_REVIEW,
    STATE_REVIEW_RESULT
};

AppState currentState = STATE_IDLE;

static bool shouldRunTrackingUpdate(AppState state) {
    return state == STATE_ALIGN_MOTION ||
           state == STATE_TRACKING_READY ||
           state == STATE_RECORDING_ACTION;
}

bool checkButtonPress() {
    static int lastState = HIGH;
    static unsigned long pressTime = 0;
    int state = digitalRead(BTN_BOOT);
    bool isClick = false;

    if (lastState == HIGH && state == LOW) {
        pressTime = millis();
    } else if (lastState == LOW && state == HIGH) {
        if (millis() - pressTime > 50) isClick = true;
    }
    lastState = state;
    return isClick;
}

void setup() {
    Serial.begin(115200);
    pinMode(BTN_BOOT, INPUT_PULLUP);

    Wire.begin(I2C_SDA, I2C_SCL);
    if (!power.begin(Wire, AXP2101_SLAVE_ADDRESS, I2C_SDA, I2C_SCL)) {
        while (1) {}
    }
    power.enableALDO1();
    power.enableALDO2();
    power.enableALDO3();
    power.enableALDO4();
    power.enableBLDO1();
    power.enableBLDO2();
    power.setALDO1Voltage(3300);

    qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, I2C_SDA, I2C_SCL);

    gfx->begin();
    ((Arduino_CO5300 *)gfx)->setBrightness(255);

    appTracking = new AppTracking(gfx, &qmi);
    watchFace = new WatchFace(gfx);

    appTracking->setup();
    watchFace->draw(true);
}

void loop() {
    bool btnClick = checkButtonPress();

    if (shouldRunTrackingUpdate(currentState)) {
        appTracking->update();
    }

    switch (currentState) {
        case STATE_IDLE:
            if (btnClick) {
                appTracking->start();
                currentState = STATE_CALIB_STATIC;
            }
            break;

        case STATE_CALIB_STATIC:
            if (appTracking->runStaticCalibration()) {
                appTracking->resetPosition();
                currentState = STATE_ALIGN_MOTION;
            } else {
                appTracking->drawText("CALIB FAILED", RED);
                delay(800);
                appTracking->stop();
                watchFace->draw(true);
                currentState = STATE_IDLE;
            }
            break;

        case STATE_ALIGN_MOTION:
            appTracking->draw();
            if (appTracking->isMotionCalibrated()) {
                currentState = STATE_TRACKING_READY;
                appTracking->drawText("READY [0,0,0]", GREEN);
                delay(300);
            }
            break;

        case STATE_TRACKING_READY:
            appTracking->draw();
            if (btnClick) {
                appTracking->markTargetPosition();
                appTracking->drawText("SWING NOW!", WHITE);
                delay(150);
                currentState = STATE_RECORDING_ACTION;
            }
            break;

        case STATE_RECORDING_ACTION:
            appTracking->draw();
            if (appTracking->isStable()) {
                appTracking->markFinalPosition();
                appTracking->drawText("STOPPED. PRESS REVIEW", GREEN);
                currentState = STATE_WAIT_FOR_REVIEW;
            }
            break;

        case STATE_WAIT_FOR_REVIEW:
            if (btnClick) {
                currentState = STATE_REVIEW_RESULT;
                float dist = appTracking->getDistanceToTarget();
                bool isWin = (dist <= TARGET_RADIUS);
                appTracking->drawResult(isWin, dist);
            }
            break;

        case STATE_REVIEW_RESULT:
            if (btnClick) {
                appTracking->stop();
                watchFace->draw(true);
                currentState = STATE_IDLE;
            }
            break;
    }
}
