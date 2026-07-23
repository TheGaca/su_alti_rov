/*
 * ESP32 WROOM - 8 Motor/ESC Kontrol Kodu (Ana ROV govdesi, su altina safe pinler)
 * ---------------------------------------------------------------------------
 * Motor duzeni (foto/5a5bd473784e48cfed6daa1d1cb0c4e51b705469.png):
 *   1,2 = on capraz yatay itki   3,4 = arka capraz yatay itki
 *   5,6,7,8 = dikey (derinlik) itki motorlari
 * motorPins dizisindeki index 0..7, motor numarasi 1..8'e karsilik gelir.
 *
 * Seri protokol (115200 baud, satir sonu '\n'):
 *   "ARM"                     -> ESC'leri ARM eder (2sn 850us tutar, sonra notre gecer)
 *   "DISARM"                  -> Tum pinlerden PWM sinyalini tamamen keser (ESC durur)
 *   "M:p1,p2,p3,p4,p5,p6,p7,p8" -> motor 1..8 icin darbe genisligi (us, 850-1850)
 * Cihaz durum bildirimi icin "READY", "ARMING", "ARMED", "DISARMED", "ERR:..." satirlari yollar.
 * IMU takiliysa 100ms'de bir "ATT:roll,pitch" (derece) satiri da yollanir.
 * Basinc sensoru takiliysa 200ms'de bir "DEPTH:metre,dikey_hiz_buyuklugu_m/s"
 * satiri da yollanir. Derinlik EMA filtresiyle yumusatilir; hiz bu yumusatilmis
 * derinligin degisim oraninin MUTLAK DEGERIDIR (yon yok, hep >=0, kucuk
 * degerler gurultu sayilip 0'a yuvarlanir) - SADECE dikey (batma/yukselme)
 * hareketi yansitir; ileri/yanal hiz icin DVL/akis sensoru gerekir, bu
 * donanimda yok.
 *
 * Guvenlik: ARMED durumdayken 500ms boyunca yeni "M:" komutu gelmezse
 * (baglanti kopmasi ihtimaline karsi) tum motorlar otomatik notre (1490us) cekilir.
 *
 * PWM darbe sinirlari (ESC): MIN_US=850us (tam geri), NEUTRAL_US=1490us (notr/dur),
 * MAX_US=1850us (tam ileri). "M:" ile gelen hedef darbeler dogrudan uygulanmaz;
 * ani tam-ileri<->tam-geri gibi sicramalarda ESC'lerin takilmasini/stall olmasini
 * onlemek icin motorPulse, updateMotorSlew() tarafindan MOTOR_SLEW_STEP_US
 * adimlarla (varsayilan: 20ms'de 40us, yani ~500ms'de tam MIN_US<->MAX_US) hedefe
 * yaklastirilir.
 *
 * Basinc/Derinlik sensoru (analog, 5V, 0.5-4.5V cikis, 0-1.6 MPa) baglantisi:
 *   Kirmizi (V+)    -> ESP32 kartinin 5V/VIN pini (3.3V DEGIL, sensor 5V ister)
 *   Siyah (GND)     -> GND
 *   Sinyal (0.5-4.5V) -> ESP32 ADC 3.3V'u gecemez! Once gerilim bolucuden gecir:
 *       Sinyal --[10k]-- GPIO34 --[20k]-- GND   (bolucu orani = 20/(10+20) = 2/3)
 *   ONEMLI: ESP32 acilirken (setup icinde) sensor HAVADA/SUYA GIRMEMIS olmali;
 *   o andaki basinc "yuzey/atmosfer" referansi olarak kalibre edilir. Sudayken
 *   acarsan derinlik yanlis (sifirdan farkli baslar) okunur.
 *
 * OTA (kablosuz kod yukleme): ESP32 kendi WiFi erisim noktasini (AP) acar;
 * disarida bir router/internet GEREKMEZ, bilgisayarindan dogrudan OTA_AP_SSID
 * agina baglanip Arduino IDE'de Tools > Port'tan agdaki OTA_HOSTNAME
 * cihazini secerek kod atabilirsin (Arduino IDE bunu birkac saniyede otomatik bulur).
 * Bu SADECE firmware guncellemek icin; motor komutlari halen USB Serial'dan
 * geliyor, yukaridaki seri protokol degismedi. OTA sirasinda guncelleme
 * baslar baslamaz motorlar guvenlik icin DISARM edilir (bkz. setupOTA()).
 *
 * IMU (MPU-6050, I2C) baglantisi:
 *   VCC -> 3V3        GND -> GND
 *   SCL -> GPIO 22    SDA -> GPIO 21
 *   XDA, XCL -> bos birak (auxiliary I2C, kullanilmiyor)
 *   AD0 -> GND (I2C adresini 0x68 sabitler)
 *   INT -> kullanilmiyor, bos birak
 *   Bu pinler motorPins dizisiyle cakismaz, ESP32'nin varsayilan donanimsal
 *   I2C hatti oldugu icin ek kutuphane/pin ayari gerekmez.
 *
 *   ONEMLI: Acilista (setup icinde) ~1 saniye surecek bir jiroskop
 *   kalibrasyonu yapilir. Bu sirada ROV'u SABIT ve olabildigince DUZ tutun;
 *   o an hareket ederse kalibrasyon yanlis sifir noktasi ogrenir ve roll/pitch
 *   degerleri kalici bir sapmayla gelir (ESP32'yi resetleyip tekrar sabit
 *   tutarak duzeltilir).
 */
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

// ---- OTA (kablosuz kod yukleme) ayarlari ----
// ESP32 kendi erisim noktasini actigi icin dis bir WiFi agina/internete ihtiyac yok.
#define OTA_AP_SSID     "AnaROV(DenizAlti)"
#define OTA_AP_PASS     "sualti123"  // WPA2 min. 8 karakter sart - 6 harfli "sualti" AP'nin hic acilmamasina yol aciyordu
#define OTA_PASSWORD    "sualti123"  // Arduino IDE kod atarken ayrica sorulan OTA sifresi
#define OTA_HOSTNAME    "Ana-Rov-DenizAlti"  // mDNS hostname: sadece harf/rakam/tire gecerli, parantez/bosluk KULLANMA (Tools>Port'ta cihazi bulmayi engeller)

#define PWM_FREQ_HZ   50
#define PWM_RES       16
#define NEUTRAL_US    1490
#define MIN_US        850
#define MAX_US        1850
#define ARM_HOLD_MS   2000
#define FAILSAFE_MS   500
#define MOTOR_SLEW_STEP_US    40  // her slew adiminda izin verilen max darbe degisimi (us)
#define MOTOR_SLEW_PERIOD_MS  20  // slew adimlari arasi sure (ms); 40us/20ms = 1000us'luk tam
                                   // aralik (MIN_US<->MAX_US) yaklasik 500ms'de kat edilir.
                                   // Ani yon degisiminde (ors. tam ileri -> tam geri) ESC'lerin
                                   // takilmasini/stall olmasini onlemek icin PWM kademeli degisir.

#define IMU_SDA_PIN   21
#define IMU_SCL_PIN   22
#define IMU_PERIOD_MS 100

const int motorPins[8] = {13, 4, 14, 27, 26, 25, 33, 32};

// ==================== Basinc/Derinlik Sensoru ====================
#define DEPTH_ADC_PIN      34   // input-only pin, baska hicbir seyle cakismaz
#define DEPTH_PERIOD_MS    200
#define DEPTH_ADC_SAMPLES  32   // ESP32 ADC gurultulu; ortalama alarak stabilize edilir

#define DEPTH_DIVIDER_RATIO 0.6667f // 10k+20k bolucu: 20/(10+20)
#define DEPTH_ADC_VREF      3.3f
#define DEPTH_ADC_MAX_COUNTS 4095.0f

#define DEPTH_SENSOR_V_MIN  0.5f   // sensorun 0 MPa'daki ciktisi
#define DEPTH_SENSOR_V_MAX  4.5f   // sensorun 1.6 MPa'daki ciktisi
#define DEPTH_SENSOR_MPA_MAX 1.6f

#define WATER_DENSITY_KGM3  1000.0f // tatli su; tuzlu suda ~1025 kullan
#define GRAVITY_MS2         9.80665f

#define DEPTH_FILTER_ALPHA  0.25f // 0-1 arasi; kucuk=daha yumusak/gecikmeli, buyuk=daha hizli/gurultulu
#define DEPTH_SPEED_DEADBAND 0.02f // bu esigin (m/s) altindaki degerler gurultu sayilip 0'a yuvarlanir

float depthSurfaceMPa = 0.101325f; // acilista kalibre edilen yuzey/atmosfer basinci
float depthMeters = 0.0f;          // EMA ile yumusatilmis derinlik
float depthSpeedMs = 0.0f;         // dikey hiz buyuklugu (m/s), her zaman >=0 (yon bilgisi yok)
bool depthFilterInit = false;
unsigned long lastDepthAt = 0;
unsigned long lastDepthUpdateMs = 0;

// ADC'den birkac ornek ortalamasiyla, gerilim bolucuyu tersine cevirip
// sensorun gercek ciktisini (0.5-4.5V) ve oradan da MPa cinsinden basinci hesaplar.
float depthReadPressureMPa() {
    long sum = 0;
    for (int i = 0; i < DEPTH_ADC_SAMPLES; i++) sum += analogRead(DEPTH_ADC_PIN);
    float avgCounts = sum / (float)DEPTH_ADC_SAMPLES;

    float vAtPin = (avgCounts / DEPTH_ADC_MAX_COUNTS) * DEPTH_ADC_VREF;
    float vSensor = vAtPin / DEPTH_DIVIDER_RATIO;

    float pressureMPa = (vSensor - DEPTH_SENSOR_V_MIN) /
                         (DEPTH_SENSOR_V_MAX - DEPTH_SENSOR_V_MIN) * DEPTH_SENSOR_MPA_MAX;
    return (pressureMPa < 0) ? 0 : pressureMPa;
}

// Acilista (ROV suya girmeden once) o anki basinci "yuzey" referansi olarak kaydeder.
void depthCalibrateSurface() {
    depthSurfaceMPa = depthReadPressureMPa();
    Serial.print("DEPTH: yuzey kalibrasyonu tamamlandi, referans = ");
    Serial.print(depthSurfaceMPa, 4);
    Serial.println(" MPa");
}

void depthUpdate() {
    float pressureMPa = depthReadPressureMPa();
    float gaugeMPa = pressureMPa - depthSurfaceMPa; // sadece su kolonunun basinci
    if (gaugeMPa < 0) gaugeMPa = 0;
    float rawDepth = (gaugeMPa * 1000000.0f) / (WATER_DENSITY_KGM3 * GRAVITY_MS2);

    unsigned long now = millis();
    float dt = (now - lastDepthUpdateMs) / 1000.0f;
    lastDepthUpdateMs = now;

    if (!depthFilterInit) {
        // Ilk okuma: filtreyi ham degerle baslat, hiz henuz hesaplanamaz.
        depthMeters = rawDepth;
        depthSpeedMs = 0.0f;
        depthFilterInit = true;
        return;
    }

    // Dusuk gecirgen (EMA) filtre: ADC/basinc gurultusunden gelen ani
    // sicramalari yumusatir, boylece "İrtifa" degeri surekli oynamaz.
    float prevDepth = depthMeters;
    depthMeters = prevDepth + DEPTH_FILTER_ALPHA * (rawDepth - prevDepth);

    // Dikey hiz buyuklugu, yumusatilmis derinligin degisim oraninin mutlak
    // degeridir (m/s). Deadband altindaki degerler (ROV gercekte durgunken
    // sensor gurultusunden gelen ~birkac mm/s'lik kirinti) 0'a yuvarlanir,
    // boylece hareketsizken "Hiz" tam olarak 0.00 gorunur.
    if (dt > 0.0f && dt < 2.0f) {
        float instSpeed = fabs(depthMeters - prevDepth) / dt;
        depthSpeedMs = (instSpeed < DEPTH_SPEED_DEADBAND) ? 0.0f : instSpeed;
    }
}

// ==================== MPU-6050 IMU (ivmeolcer + jiroskop) ====================
#define MPU6050_ADDR      0x68   // AD0 GND'ye cekiliyse 0x68; VCC'ye cekiliyse 0x69
#define REG_WHO_AM_I      0x75
#define REG_PWR_MGMT_1    0x6B
#define REG_ACCEL_CONFIG  0x1C
#define REG_GYRO_CONFIG   0x1B
#define REG_ACCEL_XOUT_H  0x3B
#define REG_GYRO_XOUT_H   0x43

#define GYRO_LSB_PER_DPS  131.0f // +-250 deg/s araliginda (GYRO_CONFIG=0x00)
#define GYRO_RETRY_MS     3000   // IMU bulunamazsa bu araliklarla yeniden dene

bool imuReady = false;
unsigned long lastImuAt = 0;
unsigned long lastImuMicros = 0;
unsigned long lastImuRetryAt = 0;
float rollDeg = 0, pitchDeg = 0;
float gyroBiasX = 0, gyroBiasY = 0;

uint8_t imuReadReg(uint8_t reg) {
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU6050_ADDR, 1);
    return Wire.available() ? Wire.read() : 0;
}

void imuWriteReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

// SDA/SCL hattinda cevap veren tum I2C adreslerini tarayip yazdirir.
// IMU bulunamadiginda "kablolama mi yanlis, adres mi farkli" ayrimini yapmak icin.
void imuScanI2CBus() {
    Serial.println("IMU: I2C hatti taraniyor (0x01-0x7F)...");
    int found = 0;
    for (uint8_t addr = 1; addr < 0x7F; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.print("IMU: -> cihaz bulundu, adres 0x");
            Serial.println(addr, HEX);
            found++;
        }
    }
    if (found == 0) {
        Serial.println("IMU: Hicbir I2C cihazi bulunamadi! Kontrol et: SDA=GPIO21, SCL=GPIO22, VCC=3V3, GND, AD0 durumu ve SDA/SCL uzerinde pull-up direnci (bare modulse 4.7k harici gerekebilir).");
    } else {
        Serial.println("IMU: Yukaridaki adres(ler) MPU-6050'nin 0x68/0x69'undan farkliysa modul farkli bir sensor/adres olabilir (AD0 pinini kontrol et).");
    }
}

// Ivmeolcer (3 eksen) + jiroskop X/Y (roll/pitch icin yeterli, yaw icin
// gereken Z ekseni okunmuyor) tek seferde okunur. I2C hattinda gecici bir
// hata/gurultu olursa (motor ESC'lerinden kaynaklanabilir) false doner ve
// cagiran taraf eski aci degerlerini korur; cip hicbir zaman yanlis/yari
// veriyle guncellenmez.
bool imuReadAccelGyro(int16_t out[5]) {
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(REG_ACCEL_XOUT_H);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(MPU6050_ADDR, 6) != 6) return false;
    out[0] = (int16_t)((Wire.read() << 8) | Wire.read());
    out[1] = (int16_t)((Wire.read() << 8) | Wire.read());
    out[2] = (int16_t)((Wire.read() << 8) | Wire.read());

    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(REG_GYRO_XOUT_H);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom(MPU6050_ADDR, 4) != 4) return false;
    out[3] = (int16_t)((Wire.read() << 8) | Wire.read());
    out[4] = (int16_t)((Wire.read() << 8) | Wire.read());
    return true;
}

bool imuInit() {
    Wire.begin(IMU_SDA_PIN, IMU_SCL_PIN);
    delay(50); // sensor guc-acilis suresi
    uint8_t who = imuReadReg(REG_WHO_AM_I);
    Serial.print("IMU: adres 0x68 WHO_AM_I = 0x");
    Serial.println(who, HEX);

    // MPU-6050 icin beklenen deger genelde 0x68, bazi klon/varyantlarda 0x72/0x98 de gorulebilir
    if (who != 0x68 && who != 0x72 && who != 0x98) {
        Serial.println("IMU: MPU-6050 0x68 adresinde taninmadi (beklenen WHO_AM_I ~0x68).");
        imuScanI2CBus();
        return false;
    }

    imuWriteReg(REG_PWR_MGMT_1, 0x01);     // uyku modundan cik, gyro-X saat referansi (dahili RC osilatorden daha kararli)
    imuWriteReg(REG_ACCEL_CONFIG, 0x00);   // ivmeolcer araligi +-2g
    imuWriteReg(REG_GYRO_CONFIG, 0x00);    // jiroskop araligi +-250 derece/s
    delay(50);

    Serial.println("IMU: Jiroskop kalibrasyonu basliyor - ROV'u SABIT tutun...");
    long sumGx = 0, sumGy = 0;
    const int CAL_SAMPLES = 200;
    int okSamples = 0;
    for (int i = 0; i < CAL_SAMPLES; i++) {
        int16_t v[5];
        if (imuReadAccelGyro(v)) {
            sumGx += v[3];
            sumGy += v[4];
            okSamples++;
        }
        delay(3);
    }
    if (okSamples > CAL_SAMPLES / 2) { // orneklerin yarisindan fazlasi basarisizsa I2C guvenilir degil demektir
        gyroBiasX = sumGx / (float)okSamples;
        gyroBiasY = sumGy / (float)okSamples;
    } else {
        Serial.println("IMU: Kalibrasyon sirasinda okumalarin cogu basarisiz oldu, I2C hatti guvenilir degil olabilir.");
        return false;
    }

    lastImuMicros = 0; // ilk imuUpdate() cagrisinda dogrudan ivmeolcer acisiyla baslanacak
    Serial.println("IMU: MPU-6050 aktif, ATT: satirlari basliyor.");
    return true;
}

// Sadece ivmeolcerden hesaplanan aci (roll/pitch) motor titresiminde sicrar/
// gurultulu olur; sadece jiroskop entegrasyonu ise kisa vadede pürüzsuz ama
// zamanla kayar (drift). Tamamlayici (complementary) filtre ikisini
// birlestirir: kisa vadede jiroskopa, uzun vadede ivmeolcere guvenir.
// Yaw hala hesaplanmiyor - Z eksen jiroskopu tek basina manyetometre
// olmadan zamanla surer, "sabitleme" icin guvenilir bir referans olmaz.
//
// NOT: IMU govdeye 90 derece donuk monte edilmis - havuz/masa testinde
// X ekseni tabanli hesap (asagidaki eski "pitch" formulu) sag/sol yatmayi
// (roll), Y/Z ekseni tabanli hesap (eski "roll" formulu) ise yukari/asagi
// bakmayi (pitch) verdigi gozlemlendi. Bu yuzden asagida eksenler kasitli
// olarak "capraz" atanmistir. IMU'yu fiziksel olarak duz (0 derece) cevirip
// takarsan bu eslemeyi eski haline (ax<->ay,az) geri almali/duzeltmelisin.
void imuUpdate() {
    int16_t v[5];
    if (!imuReadAccelGyro(v)) return; // basarisiz okuma: eski aci degerlerini koru

    float ax = v[0], ay = v[1], az = v[2];
    float gyroPitchRate = (v[3] - gyroBiasX) / GYRO_LSB_PER_DPS; // derece/sn (govde: yukari/asagi)
    float gyroRollRate  = (v[4] - gyroBiasY) / GYRO_LSB_PER_DPS; // derece/sn (govde: sag/sol)

    float accelPitch = atan2(ay, az) * 180.0 / PI;                      // govde: yukari/asagi bakma
    float accelRoll  = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI; // govde: sag/sol yatma

    unsigned long now = micros();
    float dt = (lastImuMicros == 0) ? 0.0f : (now - lastImuMicros) / 1000000.0f;
    lastImuMicros = now;

    if (dt <= 0.0f || dt > 0.5f) {
        // ilk okuma ya da anormal buyuk bosluk (ornegin gecici I2C kesintisi):
        // jiroskop entegrasyonuna guvenilmez, dogrudan ivmeolcer acisina atla.
        rollDeg = accelRoll;
        pitchDeg = accelPitch;
    } else {
        rollDeg  = 0.98f * (rollDeg  + gyroRollRate  * dt) + 0.02f * accelRoll;
        pitchDeg = 0.98f * (pitchDeg + gyroPitchRate * dt) + 0.02f * accelPitch;
    }
}

enum RovState { DISARMED, ARMING, ARMED };
RovState state = DISARMED;
unsigned long armStartedAt = 0;
unsigned long lastCommandAt = 0;
unsigned long lastSlewAt = 0;
uint16_t motorPulse[8];
uint16_t motorTarget[8]; // handleMotorCommand'in yazdigi hedef; motorPulse buna kademeli yaklasir

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

void allTargetsTo(uint16_t pulse_us) {
    for (int i = 0; i < 8; i++) motorTarget[i] = pulse_us;
}

// motorPulse'u motorTarget'a MOTOR_SLEW_STEP_US adimlarla yaklastirir; boylece
// M: komutuyla gelen ani tam-ileri<->tam-geri gibi hedef degisiklikleri
// motorlara tek adimda degil kademeli uygulanir.
void updateMotorSlew() {
    if (millis() - lastSlewAt < MOTOR_SLEW_PERIOD_MS) return;
    lastSlewAt = millis();
    for (int i = 0; i < 8; i++) {
        int diff = (int)motorTarget[i] - (int)motorPulse[i];
        if (diff == 0) continue;
        int step = (diff > 0) ? min(diff, MOTOR_SLEW_STEP_US) : max(diff, -MOTOR_SLEW_STEP_US);
        writeMotor(i, (uint16_t)((int)motorPulse[i] + step));
    }
}

void startArming() {
    attachMotors();
    for (int i = 0; i < 8; i++) writeMotor(i, MIN_US); // ESC arm sinyali (dusuk darbe)
    allTargetsTo(MIN_US);
    armStartedAt = millis();
    state = ARMING;
    Serial.println("ARMING");
}

void finishArming() {
    allMotorsTo(NEUTRAL_US);
    allTargetsTo(NEUTRAL_US);
    lastCommandAt = millis();
    state = ARMED;
    Serial.println("ARMED");
}

void disarmNow() {
    detachMotors(); // pinlere hic sinyal gitmez
    // Donanima yazmadan (ledcWrite detach sonrasi gecersiz) sadece durum
    // dizilerini sifirla; sonraki ARM'da slew eski hedeften baslamasin.
    for (int i = 0; i < 8; i++) motorPulse[i] = NEUTRAL_US;
    allTargetsTo(NEUTRAL_US);
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
    // Dogrudan writeMotor yerine hedefi guncelliyoruz; gercek darbe
    // updateMotorSlew() tarafindan kademeli olarak buna yaklastirilir (bkz.
    // MOTOR_SLEW_STEP_US) - ani tam-ileri/tam-geri komutlarinda ESC'lerin
    // takilmasini/stall olmasini onler.
    for (int i = 0; i < 8; i++) {
        int v = vals[i];
        if (v < MIN_US) v = MIN_US;
        if (v > MAX_US) v = MAX_US;
        motorTarget[i] = (uint16_t)v;
    }
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

// ESP32'yi kendi WiFi erisim noktasi (AP) yapar; PC/telefon dogrudan bu aga
// baglanip Arduino IDE'den kablosuz kod atabilir. Router/internet gerekmez -
// tamamen yerel, tezgah/dokta kullanim icindir (motor komutlari bundan
// etkilenmez, onlar hala USB Serial'dan geliyor).
void setupOTA() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(OTA_AP_SSID, OTA_AP_PASS);
    Serial.print("OTA AP acildi -> SSID: ");
    Serial.print(OTA_AP_SSID);
    Serial.print("  IP: ");
    Serial.println(WiFi.softAPIP());

    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.onStart([]() {
        // Yukleme sirasinda loop() duzenli calismayabilir; motorlari guvenlik
        // icin onceden kapat (DISARM) - yoksa failsafe de tetiklenemeyebilir.
        disarmNow();
        Serial.println("OTA: guncelleme basladi, motorlar DISARM edildi.");
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("OTA HATA[%u]\n", error);
    });
    ArduinoOTA.begin();
}

void setup() {
    Serial.begin(115200);
    for (int i = 0; i < 8; i++) motorPulse[i] = NEUTRAL_US;
    imuReady = imuInit();
    Serial.println(imuReady ? "SUALTIESP32 READY (IMU OK)" : "SUALTIESP32 READY (IMU YOK)");

    analogSetPinAttenuation(DEPTH_ADC_PIN, ADC_11db); // 0-3.3V tam araligi kullan
    depthCalibrateSurface();

    setupOTA();
}

void loop() {
    ArduinoOTA.handle();

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
        allTargetsTo(NEUTRAL_US); // hedefi de sifirla, yoksa baglanti donunce eski komuta ani sicrar
    }

    if (state == ARMED) {
        updateMotorSlew();
    }

    if (imuReady && millis() - lastImuAt >= IMU_PERIOD_MS) {
        lastImuAt = millis();
        imuUpdate();
        Serial.print("ATT:");
        Serial.print(rollDeg, 1);
        Serial.print(',');
        Serial.println(pitchDeg, 1);
    }

    // IMU baslangicta bulunamadiysa (kablolama gec duzeltildiyse, gevsek
    // temas vs.) tekrar reset atmaya gerek kalmadan periyodik yeniden dener.
    if (!imuReady && millis() - lastImuRetryAt >= GYRO_RETRY_MS) {
        lastImuRetryAt = millis();
        Serial.println("IMU: yeniden baglanmaya calisiliyor...");
        imuReady = imuInit();
    }

    if (millis() - lastDepthAt >= DEPTH_PERIOD_MS) {
        lastDepthAt = millis();
        depthUpdate();
        Serial.print("DEPTH:");
        Serial.print(depthMeters, 2);
        Serial.print(',');
        Serial.println(depthSpeedMs, 3);
    }
}