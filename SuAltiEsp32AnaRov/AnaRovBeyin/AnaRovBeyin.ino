/*
 * ESP32 WROOM - 8 Motor/ESC Kontrol Kodu (Ana ROV govdesi, su altina safe pinler)
 * ---------------------------------------------------------------------------
 * Motor duzeni (foto/5a5bd473784e48cfed6daa1d1cb0c4e51b705469.png):
 *   1,2 = on capraz yatay itki   3,4 = arka capraz yatay itki
 *   5,6,7,8 = dikey (derinlik) itki motorlari
 * motorPins dizisindeki index 0..7, motor numarasi 1..8'e karsilik gelir.
 *
 * Seri protokol (115200 baud, satir sonu '\n'). Komutlar SADECE kamera
 * ESP'sine giden kopruden (Serial2, GPIO16=RX/GPIO17=TX - bkz. BRIDGE_*
 * tanimlari) okunur; USB (Serial) artik komut dinlemez, sadece durum
 * satirlarini da alan bir debug/bench-test ciktisi olarak calisir (bkz.
 * sendStatus()) - denizde zaten USB baglanmayacak, sadece guc (VIN/GND) gider:
 *   "ARM"                     -> ESC'leri ARM eder (2sn 850us tutar, sonra notre gecer)
 *   "DISARM"                  -> Tum pinlerden PWM sinyalini tamamen keser (ESC durur)
 *   "M:p1,p2,p3,p4,p5,p6,p7,p8" -> motor 1..8 icin darbe genisligi (us, 850-1850)
 * Cihaz durum bildirimi icin "READY", "ARMING", "ARMED", "DISARMED", "ERR:..." satirlari yollar.
 * IMU takiliysa 100ms'de bir "ATT:roll,pitch,yaw" (derece) satiri da yollanir.
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
 * IMU (Adafruit BNO055, 9-DOF fusion, I2C) baglantisi:
 *   VIN -> 3V3         GND -> GND
 *   SCL -> GPIO 22      SDA -> GPIO 21
 *   RST, INT, PS0, PS1, 3Vo -> KULLANILMIYOR, hepsi bosta birakilir (PS0/PS1
 *     bosken karttaki pull-down'lar sayesinde otomatik I2C moduna gecer).
 *   ADR -> bosta/GND (I2C adresi 0x28 sabitlenir; 3.3V'a baglanirsa 0x29 olur,
 *     o zaman asagidaki BNO055_I2C_ADDR degerini de guncelle).
 *   Bu pinler motorPins dizisiyle cakismaz, ESP32'nin varsayilan donanimsal
 *   I2C hatti oldugu icin ek pin ayari gerekmez. Fusion (roll/pitch/yaw
 *   hesabi) cipin kendi icinde yapilir - bizim ayrica complementary filter
 *   yazmamiza gerek yok (bkz. asagidaki IMU bolumu).
 *
 *   ONEMLI: Kutuphane kurulumu gerekir - Arduino IDE > Library Manager'dan
 *   "Adafruit BNO055" kur (bagimliliklari Adafruit Unified Sensor ve Adafruit
 *   BusIO otomatik gelir).
 *
 *   NOT: Acilista ekstra bir kalibrasyon bekletmesi YOK (eski MPU6050 kodundaki
 *   1sn'lik jiroskop kalibrasyonunun aksine); ama BNO055'in ilk birkac saniyede
 *   (ozellikle manyetometre icin) kendi kendine kalibre olmasi biraz zaman
 *   alabilir, bu sure zarfinda roll/pitch degerleri hafif kaymali gelebilir.
 */
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

// ---- OTA (kablosuz kod yukleme) ayarlari ----
// ESP32 kendi erisim noktasini actigi icin dis bir WiFi agina/internete ihtiyac yok.
#define OTA_AP_SSID     "AnaROV(DenizAlti)"
#define OTA_AP_PASS     "sualti123"  // WPA2 min. 8 karakter sart - 6 harfli "sualti" AP'nin hic acilmamasina yol aciyordu
#define OTA_PASSWORD    "sualti123"  // Arduino IDE kod atarken ayrica sorulan OTA sifresi
#define OTA_HOSTNAME    "Ana-Rov-DenizAlti"  // mDNS hostname: sadece harf/rakam/tire gecerli, parantez/bosluk KULLANMA (Tools>Port'ta cihazi bulmayi engeller)

// ---- Kamera ESP'sine (AnaRovKamera.ino) UART koprusu ----
// PC artik komutlari USB yerine Ethernet uzerinden kamera ESP'sine gonderiyor;
// kamera ESP'si bunlari bu ikinci donanimsal seri porttan (Serial2) bu karta
// iletiyor. USB (Serial) debug/bench test icin ayrica calismaya devam ediyor -
// iki kaynaktan da ayni komutlar kabul edilir (bkz. loop()).
// Kablolama: kamera ESP GPIO33(TX) -> bu kart GPIO16(RX), kamera ESP GPIO32(RX)
// <- bu kart GPIO17(TX), GND ortak.
#define BRIDGE_RX_PIN 16
#define BRIDGE_TX_PIN 17
#define BRIDGE_BAUD   115200

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

// Protokol/durum satirlarini (READY, ARMED, ATT:, DEPTH: vb.) HEM USB'ye HEM
// de kamera ESP'sine giden koprude (Serial2) yollar - PC hangisinden baglanmis
// olursa olsun ayni bilgiyi gorur. Sadece yerel/bench debug mesajlari (IMU I2C
// tarama, OTA kurulum vs.) icin Serial.print kullanilmaya devam eder.
void sendStatus(const String &line) {
    Serial.println(line);
    Serial2.println(line);
}

// Acilista (ROV suya girmeden once) o anki basinci "yuzey" referansi olarak kaydeder.
void depthCalibrateSurface() {
    depthSurfaceMPa = depthReadPressureMPa();
    sendStatus("DEPTH: yuzey kalibrasyonu tamamlandi, referans = " + String(depthSurfaceMPa, 4) + " MPa");
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

// ==================== BNO055 IMU (9-DOF donanimsal fusion) ====================
#define BNO055_I2C_ADDR 0x28   // ADR pini bosta/GND -> 0x28; 3.3V'a baglanirsa 0x29
#define IMU_RETRY_MS     3000  // IMU bulunamazsa bu araliklarla yeniden dene

Adafruit_BNO055 bno = Adafruit_BNO055(55, BNO055_I2C_ADDR, &Wire);

bool imuReady = false;
unsigned long lastImuAt = 0;
unsigned long lastImuRetryAt = 0;
float rollDeg = 0, pitchDeg = 0;
float yawDeg = 0; // BNO055'in manyetometreli fusion'u sayesinde artik mevcut,
                   // ATT: satirina 3. deger olarak ekleniyor (bkz. loop()).

bool imuInit() {
    Wire.begin(IMU_SDA_PIN, IMU_SCL_PIN);
    if (!bno.begin()) {
        Serial.println("IMU: BNO055 bulunamadi! Kontrol et: SDA=GPIO21, SCL=GPIO22, VIN=3V3, GND, ADR durumu.");
        return false;
    }
    delay(50);
    bno.setExtCrystalUse(true); // breakout uzerindeki harici kristali kullan, dahili osilatorden daha kararli
    Serial.println("IMU: BNO055 aktif, ATT: satirlari basliyor.");
    return true;
}

// BNO055 fusion'u (ivmeolcer+jiroskop+manyetometre birlestirmesi) cipin kendi
// icindeki islemcide yapilir. ONEMLI: Euler acilarini (VECTOR_EULER) ciktiktan
// SONRA y/z'yi yer degistirmek/isaret cevirmek matematiksel olarak guvenilir
// DEGIL - uc Euler bileseni birbirine bagli hesaplanir, ciktida "hangisi roll
// hangisi pitch" diye yer degistirmek gercek bir eksen donusumu yapmiyor, hala
// eksenler arasi sizinti (coupling) oluyor (bench testinde burun kaldirilinca
// roll'un degismesi buna isaret ediyordu).
//
// Bunun yerine cekim (gravity) vektorunu (VECTOR_GRAVITY) okuyoruz - bu 3
// BAGIMSIZ sayidir (eski MPU6050 kodunun ivmeolcerden roll/pitch turetmesiyle
// ayni mantik, ama artik BNO055'in filtrelenmis/motor-titresimine dayanikli
// ciktisiyla). Boylece eksen atamasi/isareti dogru sekilde degistirilebilir.
//
// GECICI TEHIS MODU: Dogru gx/gy/gz -> roll/pitch eslemesi henuz kesinlesmedi;
// asagidaki Serial.print ile ham degerleri USB'den izleyip (Serial Monitor,
// 115200) ROV'u elle hareket ettirerek hangi eksenin hangi harekete tepki
// verdigini gorebiliriz. Bu satirlari kaldirmadan once bu veriyi paylas.
void imuUpdate() {
    imu::Vector<3> gravity = bno.getVector(Adafruit_BNO055::VECTOR_GRAVITY);
    Serial.print("GRAV: gx=");
    Serial.print(gravity.x(), 2);
    Serial.print(" gy=");
    Serial.print(gravity.y(), 2);
    Serial.print(" gz=");
    Serial.println(gravity.z(), 2);

    // Ilk denemede eksenler arasi sizinti (coupling) yoktu, sadece ikisinin de
    // isareti tersti (yukari kaldirinca asagi, sola yatirinca saga gosteriyordu)
    // - bu yuzden asagida her ikisi de eksi ile ceviriliyor.
    rollDeg = -atan2(gravity.y(), gravity.z()) * 180.0 / PI;
    pitchDeg = -atan2(-gravity.x(), sqrt(gravity.y() * gravity.y() + gravity.z() * gravity.z())) * 180.0 / PI;

    sensors_event_t event;
    bno.getEvent(&event, Adafruit_BNO055::VECTOR_EULER);
    yawDeg = event.orientation.x; // yaw tek basina okundugu icin coupling sorunu yok
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
    sendStatus("ARMING");
}

void finishArming() {
    allMotorsTo(NEUTRAL_US);
    allTargetsTo(NEUTRAL_US);
    lastCommandAt = millis();
    state = ARMED;
    sendStatus("ARMED");
}

void disarmNow() {
    detachMotors(); // pinlere hic sinyal gitmez
    // Donanima yazmadan (ledcWrite detach sonrasi gecersiz) sadece durum
    // dizilerini sifirla; sonraki ARM'da slew eski hedeften baslamasin.
    for (int i = 0; i < 8; i++) motorPulse[i] = NEUTRAL_US;
    allTargetsTo(NEUTRAL_US);
    state = DISARMED;
    sendStatus("DISARMED");
}

void handleMotorCommand(const String &line) {
    if (state != ARMED) {
        sendStatus("ERR:NOTARMED");
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
        sendStatus("ERR:BADFORMAT");
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
        sendStatus("ERR:UNKNOWN");
    }
}

// ESP32'yi kendi WiFi erisim noktasi (AP) yapar; PC/telefon dogrudan bu aga
// baglanip Arduino IDE'den kablosuz kod atabilir. Router/internet gerekmez -
// tamamen yerel, tezgah/dokta kullanim icindir (motor komutlari bundan
// etkilenmez, onlar USB Serial ve/veya kamera ESP koprusunden (Serial2) geliyor).
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
    Serial2.begin(BRIDGE_BAUD, SERIAL_8N1, BRIDGE_RX_PIN, BRIDGE_TX_PIN);
    for (int i = 0; i < 8; i++) motorPulse[i] = NEUTRAL_US;
    imuReady = imuInit();
    sendStatus(imuReady ? "SUALTIESP32 READY (IMU OK)" : "SUALTIESP32 READY (IMU YOK)");

    analogSetPinAttenuation(DEPTH_ADC_PIN, ADC_11db); // 0-3.3V tam araligi kullan
    depthCalibrateSurface();

    setupOTA();
}

void loop() {
    ArduinoOTA.handle();

    // Komutlar artik SADECE kamera ESP'sinden (AnaRovKamera.ino) gelen kopruden
    // (Serial2) okunuyor - USB (Serial) dinlenmiyor, sadece debug/durum ciktisi
    // icin acik kaliyor (bkz. sendStatus()). Denizde zaten USB kablosu
    // olmayacagi icin bu dinleme gereksizdi.
    while (Serial2.available()) {
        String line = Serial2.readStringUntil('\n');
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
        sendStatus("ATT:" + String(rollDeg, 1) + "," + String(pitchDeg, 1) + "," + String(yawDeg, 1));
    }

    // IMU baslangicta bulunamadiysa (kablolama gec duzeltildiyse, gevsek
    // temas vs.) tekrar reset atmaya gerek kalmadan periyodik yeniden dener.
    if (!imuReady && millis() - lastImuRetryAt >= IMU_RETRY_MS) {
        lastImuRetryAt = millis();
        Serial.println("IMU: yeniden baglanmaya calisiliyor...");
        imuReady = imuInit();
    }

    if (millis() - lastDepthAt >= DEPTH_PERIOD_MS) {
        lastDepthAt = millis();
        depthUpdate();
        sendStatus("DEPTH:" + String(depthMeters, 2) + "," + String(depthSpeedMs, 3));
    }
}
