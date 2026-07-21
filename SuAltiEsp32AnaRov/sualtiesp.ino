/*
 * ESP32 WROOM - 8 Motor/ESC Kontrol Kodu (Ana ROV govdesi, su altina safe pinler)
 * ---------------------------------------------------------------------------
 * Motor duzeni (foto/5a5bd473784e48cfed6daa1d1cb0c4e51b705469.png):
 *   1,2 = on capraz yatay itki   3,4 = arka capraz yatay itki
 *   5,6,7,8 = dikey (derinlik) itki motorlari
 * motorPins dizisindeki index 0..7, motor numarasi 1..8'e karsilik gelir.
 *
 * Seri protokol (115200 baud, satir sonu '\n'):
 *   "ARM"                     -> ESC'leri ARM eder (2sn 1000us tutar, sonra notre gecer)
 *   "DISARM"                  -> Tum pinlerden PWM sinyalini tamamen keser (ESC durur)
 *   "M:p1,p2,p3,p4,p5,p6,p7,p8" -> motor 1..8 icin darbe genisligi (us, 1000-2000)
 * Cihaz durum bildirimi icin "READY", "ARMING", "ARMED", "DISARMED", "ERR:..." satirlari yollar.
 *
 * Guvenlik: ARMED durumdayken 500ms boyunca yeni "M:" komutu gelmezse
 * (baglanti kopmasi ihtimaline karsi) tum motorlar otomatik notre (1490us) cekilir.
 */
#include <Arduino.h>

#define PWM_FREQ_HZ   50
#define PWM_RES       16
#define NEUTRAL_US    1490
#define MIN_US        1000
#define MAX_US        2000
#define ARM_HOLD_MS   2000
#define FAILSAFE_MS   500

const int motorPins[8] = {13, 4, 14, 27, 26, 25, 33, 32};

enum RovState { DISARMED, ARMING, ARMED };
RovState state = DISARMED;
unsigned long armStartedAt = 0;
unsigned long lastCommandAt = 0;
uint16_t motorPulse[8];

uint32_t usToDuty(uint16_t pulse_us) {
    if (pulse_us < MIN_US) pulse_us = MIN_US;
    if (pulse_us > MAX_US) pulse_us = MAX_US;
    uint32_t period_us = 1000000UL / PWM_FREQ_HZ;
    uint32_t max_duty = (1UL << PWM_RES) - 1;
    return (uint32_t)(((uint64_t)pulse_us * max_duty) / period_us);
}

void writeMotor(int idx, uint16_t pulse_us) {
    motorPulse[idx] = pulse_us;
    ledcWrite(motorPins[idx], usToDuty(pulse_us));
}

void attachMotors() {
    for (int i = 0; i < 8; i++) {
        ledcAttach(motorPins[i], PWM_FREQ_HZ, PWM_RES);
    }
}

void detachMotors() {
    for (int i = 0; i < 8; i++) {
        ledcDetach(motorPins[i]);
    }
}

void allMotorsTo(uint16_t pulse_us) {
    for (int i = 0; i < 8; i++) writeMotor(i, pulse_us);
}

void startArming() {
    attachMotors();
    for (int i = 0; i < 8; i++) writeMotor(i, MIN_US); // ESC arm sinyali (dusuk darbe)
    armStartedAt = millis();
    state = ARMING;
    Serial.println("ARMING");
}

void finishArming() {
    allMotorsTo(NEUTRAL_US);
    lastCommandAt = millis();
    state = ARMED;
    Serial.println("ARMED");
}

void disarmNow() {
    detachMotors(); // pinlere hic sinyal gitmez
    state = DISARMED;
    Serial.println("DISARMED");
}

void handleMotorCommand(const String &line) {
    if (state != ARMED) {
        Serial.println("ERR:NOTARMED");
        return;
    }
    int vals[8];
    int idx = 0;
    int start = 2; // "M:" sonrasi
    while (idx < 8) {
        int comma = line.indexOf(',', start);
        String tok = (comma == -1) ? line.substring(start) : line.substring(start, comma);
        vals[idx++] = tok.toInt();
        if (comma == -1) break;
        start = comma + 1;
    }
    if (idx != 8) {
        Serial.println("ERR:BADFORMAT");
        return;
    }
    for (int i = 0; i < 8; i++) writeMotor(i, (uint16_t)vals[i]);
    lastCommandAt = millis();
}

void handleLine(const String &line) {
    if (line == "ARM") {
        if (state == DISARMED) startArming();
    } else if (line == "DISARM") {
        disarmNow();
    } else if (line.startsWith("M:")) {
        handleMotorCommand(line);
    } else {
        Serial.println("ERR:UNKNOWN");
    }
}

void setup() {
    Serial.begin(115200);
    for (int i = 0; i < 8; i++) motorPulse[i] = NEUTRAL_US;
    Serial.println("SUALTIESP32 READY");
}

void loop() {
    while (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) handleLine(line);
    }

    if (state == ARMING && millis() - armStartedAt >= ARM_HOLD_MS) {
        finishArming();
    }

    if (state == ARMED && millis() - lastCommandAt > FAILSAFE_MS) {
        allMotorsTo(NEUTRAL_US);
    }
}
