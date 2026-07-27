/*
 * ESP32 WROOM - 8 Motor/ESC Kontrol Kodu (Ana ROV govdesi, su altina safe pinler)
 * ---------------------------------------------------------------------------
 * Motor duzeni (foto/5a5bd473784e48cfed6daa1d1cb0c4e51b705469.png):
 *   1,2 = on capraz yatay itki   3,4 = arka capraz yatay itki
 *   5,6,7,8 = dikey (derinlik) itki motorlari
 * motorPins dizisindeki index 0..7, motor numarasi 1..8'e karsilik gelir.
 *
 * Seri protokol (satir sonu '\n'). Komutlar SADECE kamera ESP'sinden
 * (AnaRovKamera.ino) ESP-NOW (kablosuz, bkz. asagidaki ESPNOW_* tanimlari)
 * ile gelir; USB (Serial) artik komut dinlemez, sadece durum satirlarini da
 * alan bir debug/bench-test ciktisi olarak calisir (bkz. sendStatus()) -
 * denizde zaten USB baglanmayacak, sadece guc (VIN/GND) gider:
 *   "ARM"                     -> ESC'leri ARM eder (2sn 850us tutar, sonra notre gecer)
 *   "DISARM"                  -> Tum pinlerden PWM sinyalini tamamen keser (ESC durur)
 *   "M:p1,p2,p3,p4,p5,p6,p7,p8" -> motor 1..8 icin darbe genisligi (us, 1295-1600)
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
 * PWM darbe sinirlari (ESC): MIN_US=1295us (tam geri), NEUTRAL_US=1490us (notr/dur),
 * MAX_US=1600us (tam ileri). "M:" ile gelen hedef darbeler dogrudan uygulanmaz;
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
 * Bu SADECE firmware guncellemek icin; motor komutlari halen kamera ESP'sinden
 * ESP-NOW ile geliyor, yukaridaki seri protokol degismedi. OTA sirasinda
 * guncelleme baslar baslamaz motorlar guvenlik icin DISARM edilir (bkz. setupOTA()).
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
#include <esp_now.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <DHT.h>

// ---- OTA (kablosuz kod yukleme) ayarlari ----
// ESP32 kendi erisim noktasini actigi icin dis bir WiFi agina/internete ihtiyac yok.
#define OTA_AP_SSID     "AnaROV(DenizAlti)"
#define OTA_AP_PASS     "sualti123"  // WPA2 min. 8 karakter sart - 6 harfli "sualti" AP'nin hic acilmamasina yol aciyordu
#define OTA_PASSWORD    "sualti123"  // Arduino IDE kod atarken ayrica sorulan OTA sifresi
#define OTA_HOSTNAME    "Ana-Rov-DenizAlti"  // mDNS hostname: sadece harf/rakam/tire gecerli, parantez/bosluk KULLANMA (Tools>Port'ta cihazi bulmayi engeller)

// ---- Kamera ESP'sine (AnaRovKamera.ino) ESP-NOW koprusu ----
// PC komutlari (Ethernet/TCP) once kamera ESP'sine ulasir, o da bunlari
// kablosuz ESP-NOW ile bu karta iletir - iki kart arasinda artik fiziksel
// kablo YOK (eskiden Serial2/UART kullaniliyordu, GPIO16/17 kabloluydu; bu
// pinler artik bosta - ileride eklenecek ek motor kanallari icin
// kullanilabilir). ESP-NOW baglantisiz/dusuk gecikmeli bir protokol; her
// paket zaten trim edilmis tek bir protokol satiridir ("ARM", "M:..." vb.),
// ek framing gerekmez (bkz. espNowSendLine(), onEspNowRecv()).
//
// Kanal, PMK/LMK ve karsi tarafin (Kamera ESP) MAC adresi her iki kartta da
// BIREBIR AYNI olmali - biri degisirse diger kart da guncellenip yeniden
// flaslanmali.
#define ESPNOW_CHANNEL 1

// Kamera ESP'sinin WiFi(AP) MAC adresi. Bring-up: her iki kartta setup()
// icindeki "Serial.println(WiFi.macAddress())" satiriyla kendi MAC'ini
// USB Serial Monitor'a bastirir; Kamera ESP'nin bastirdigi MAC'i buraya
// yazip bu karti YENIDEN flasla. SIFIRLARLA birakilirsa ESP-NOW calismaz.
uint8_t cameraEspMac[6] = {0xC0, 0x49, 0xEF, 0x30, 0xE9, 0x8D};

// ESP-NOW yerlesik sifrelemesi (PMK/LMK) - sabit/hardcoded anahtar, rotasyon
// yok; amac rastgele/firsatci mudahaleye karsi ucuz bir bariyer olmak,
// kriptografik olarak guclu bir garanti degil. Her iki kartta BIREBIR AYNI
// olmali (16 bayt).
// String literal yerine acik byte listesi kullanildi: bir string literal
// array boyutuyla (16) TAM eslesirse bazi derleyiciler (bu toolchain dahil)
// null-sonlandiriciyi dusurme konusunda hataya dusuyor ("initializer-string...
// too long") - acik liste bu belirsizligi tamamen ortadan kaldirir.
static const uint8_t ESPNOW_PMK[16] = {'S','u','A','l','t','i','R','o','v','P','M','K','2','0','2','6'};
static const uint8_t ESPNOW_LMK[16] = {'S','u','A','l','t','i','R','o','v','L','M','K','2','0','2','6'};

#define PWM_FREQ_HZ   50
#define PWM_RES       16
#define NEUTRAL_US    1490
#define MIN_US        1295
#define MAX_US        1600
#define ARM_HOLD_MS   2000
#define FAILSAFE_MS   500
#define BOOT_SETTLE_MS 2000 // acilistan sonra bu sure boyunca ESP-NOW'dan gelen komutlar yok
                             // sayilir - ucuz savunma katmani (eskiden fiziksel UART hattinda
                             // olusabilen gurultuye karsiydi; ESP-NOW'a gecince o risk yapisal
                             // olarak ortadan kalkti ama bu koruma zarasiz oldugu icin duruyor)
#define MOTOR_SLEW_STEP_US    40  // her slew adiminda izin verilen max darbe degisimi (us)
#define MOTOR_SLEW_PERIOD_MS  20  // slew adimlari arasi sure (ms); 40us/20ms = 1000us'luk tam
                                   // aralik (MIN_US<->MAX_US) yaklasik 500ms'de kat edilir.
                                   // Ani yon degisiminde (ors. tam ileri -> tam geri) ESC'lerin
                                   // takilmasini/stall olmasini onlemek icin PWM kademeli degisir.

#define IMU_SDA_PIN   21
#define IMU_SCL_PIN   22
#define IMU_PERIOD_MS 100

const int motorPins[8] = {13, 4, 14, 27, 26, 25, 33, 32};

// ==================== Torpido pinleri ====================
// 18/19/23 PcbReadme'de zaten "Torpido" olarak kablolu ama kodda hic
// kullanilmamisti. 16/17 ESP-NOW'a gecince bosalan (eskiden Serial2/UART
// bridge) guvenli pinler - toplam 5 torpido pini bu ikisiyle tamamlaniyor.
// setup() icinde hepsi aciktan LOW (pasif) yazilarak baslatilir.
//
// Ates alma sirali: "TORPEDO" komutu her geldiginde torpedoPins[torpedoIndex]
// kisa bir sure HIGH'a cekilir (TORPEDO_PULSE_MS), sonra torpedoIndex bir
// artar - hangi torpidonun dolu/bos oldugunu operatorun elle takip etmesine
// gerek kalmaz. Ard arda yanlislikla ates almayi onlemek icin ates alma
// sonrasi TORPEDO_COOLDOWN_MS boyunca yeni komut reddedilir (ERR:TORPEDOCOOLDOWN).
// Kalan torpido sayisi her ates almada "TORPEDO:kalan" olarak PC'ye bildirilir.
#define TORPEDO_PULSE_MS    1000  // pin bu kadar sure HIGH tutulur - mekanizmaya gore ayarlanabilir
#define TORPEDO_COOLDOWN_MS 10000 // ates alma sonrasi bir sonrakine kadar bekleme suresi

const int torpedoPins[5] = {18, 19, 23, 16, 17};
int torpedoIndex = 0;                // siradaki (henuz atilmamis) torpido, 0-4
int torpedoesRemaining = 5;
unsigned long torpedoCooldownUntil = 0;
int torpedoFiringPin = -1;           // su an HIGH tutulan pin (-1 = yok)
unsigned long torpedoPulseOffAt = 0;

void fireTorpedo() {
    unsigned long now = millis();
    if (torpedoesRemaining <= 0) {
        sendStatus("ERR:TORPEDOEMPTY");
        return;
    }
    if (now < torpedoCooldownUntil) {
        sendStatus("ERR:TORPEDOCOOLDOWN");
        return;
    }
    torpedoFiringPin = torpedoPins[torpedoIndex];
    digitalWrite(torpedoFiringPin, HIGH);
    torpedoPulseOffAt = now + TORPEDO_PULSE_MS;
    torpedoIndex++;
    torpedoesRemaining--;
    torpedoCooldownUntil = now + TORPEDO_COOLDOWN_MS;
    sendStatus("TORPEDO:" + String(torpedoesRemaining));
}

// loop() icinden her turda cagirilir - pulse suresi dolan torpido pinini
// (delay() kullanmadan, failsafe/heartbeat'i bloklamadan) LOW'a geri ceker.
void updateTorpedoPulse() {
    if (torpedoFiringPin != -1 && millis() >= torpedoPulseOffAt) {
        digitalWrite(torpedoFiringPin, LOW);
        torpedoFiringPin = -1;
    }
}

// ==================== DHT11 Nem/Sicaklik Sensoru ====================
// GPIO5: strapping pini ama acilista guvenli/varsayilan durumu zaten HIGH
// (GPIO12'nin aksine) - DHT11 modulunun DATA hattindaki pull-up de onu
// HIGH'a cektigi icin herhangi bir acilis catismasi olusmuyor.
// Kablolama: modul VCC->3V3, GND->GND, OUT/DATA/SIG->GPIO5.
#define DHT_PIN  5
#define DHT_TYPE DHT11
#define NEM_PERIOD_MS 2000 // DHT11 yavastir, saniyede birden fazla okuma guvenilmez

DHT dht(DHT_PIN, DHT_TYPE);
unsigned long lastNemAt = 0;
float humidityPct = 0.0f;
float dhtTempC = 0.0f;

// ==================== ESP-NOW aktivite LED'i ====================
// Kamera ESP'sine (PC'ye) her veri gonderildiginde kisa bir sure yanip
// soner - Kamera ESP'deki ayni mantigin (GPIO2 uzerindeki Ethernet LED'i)
// karsiligi. GPIO2 motorPins/IMU/derinlik pinleriyle cakismiyor, bosta.
#define LED_PIN 2
#define LED_FLASH_MS 50
volatile unsigned long ledOffAtMs = 0;

void flashLed() {
    digitalWrite(LED_PIN, HIGH);
    ledOffAtMs = millis() + LED_FLASH_MS;
}

void updateLed() {
    if (ledOffAtMs != 0 && millis() >= ledOffAtMs) {
        digitalWrite(LED_PIN, LOW);
        ledOffAtMs = 0;
    }
}

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

// Onemli/nadir durum satirlarini (READY, ARMING, ARMED, DISARMED, ERR:...,
// derinlik kalibrasyonu vb.) HEM USB konsoluna HEM de kamera ESP'sine
// ESP-NOW ile yollar, LED'i yakar. ATT:/DEPTH: gibi cok sik (100-200ms'de
// bir) tekrarlayan telemetri icin bunun yerine sendStatusQuiet() kullanilir
// - USB konsolu bu kadar sik veriyle bogulmasin diye.
void sendStatus(const String &line) {
    Serial.println(line);
    espNowSendLine(line);
    flashLed();
}

// ATT:/DEPTH: gibi sik tekrarlayan telemetri satirlari icin: PC'ye ESP-NOW
// ile gonderir ve LED'i yakar, ama USB konsoluna YAZMAZ.
void sendStatusQuiet(const String &line) {
    espNowSendLine(line);
    flashLed();
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
void imuUpdate() {
    imu::Vector<3> gravity = bno.getVector(Adafruit_BNO055::VECTOR_GRAVITY);

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
unsigned long bootSettleUntil = 0;
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
        // Onceden state==DISARMED disinda gelen ARM satiri sessizce yok
        // sayiliyordu; PC tarafinda kumanda kaynakli GUI kasmasi yuzunden
        // operator ilk basisin geri bildirimini gec gorup ikinci kez basarsa,
        // bu sessizlik "arm calismadi" sanisina yol aciyordu. Simdi durum acikca
        // bildiriliyor (bkz. EspRovThread::process_line -> status_signal).
        if (state == DISARMED) startArming();
        else if (state == ARMING) sendStatus("ERR:ALREADYARMING");
        else sendStatus("ERR:ALREADYARMED");
    } else if (line == "DISARM") {
        disarmNow();
    } else if (line.startsWith("M:")) {
        handleMotorCommand(line);
    } else if (line == "TORPEDO") {
        fireTorpedo();
    } else {
        sendStatus("ERR:UNKNOWN");
    }
}

// ==================== ESP-NOW (Kamera ESP koprusu) ====================
// ESP-NOW paketleri WiFi/LWIP task context'inde (gercek ISR degil) gelir;
// onEspNowRecv() burada agir is yapmadan (Serial.print bile YAZMADAN) sadece
// gonderici MAC'ini dogrulayip veriyi bir kuyruga koyar, gercek isleme
// (handleLine) loop() icinde, ana dongude yapilir - Serial2.available() ile
// ayni "once topla, sonra isle" deseni.
struct EspNowMsg {
    uint8_t len;
    uint8_t data[251]; // +1: null sonlandirma icin (esp_now payload max 250 bayt)
};

QueueHandle_t espNowRxQueue = nullptr;

void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    if (memcmp(info->src_addr, cameraEspMac, 6) != 0) return; // beklenmeyen gonderici, yok say
    if (len <= 0 || len > 250) return;
    EspNowMsg msg;
    msg.len = (uint8_t)len;
    memcpy(msg.data, data, len);
    msg.data[len] = '\0';
    xQueueSend(espNowRxQueue, &msg, 0); // kuyruk doluysa mesaj sessizce dusurulur
}

// sendStatus()'un kamera ESP'sine gonderdigi her satiri (\n eklemeden) ESP-NOW
// paketi olarak yollar - 250 bayt siniri protokoldeki en uzun satirdan
// (M: komutu, ~40 bayt) fazlasiyla genis.
void espNowSendLine(const String &line) {
    if (line.length() > 250) return; // olmamasi gereken bir durum, savunma amacli
    esp_now_send(cameraEspMac, (const uint8_t *)line.c_str(), line.length());
}

// ==================== ESP-NOW baglanti durumu ====================
// ESP-NOW baglantisiz bir protokol (TCP gibi "bagli/kopuk" kavrami yok),
// ama esp_now_send() her paket sonrasi MAC-katmani ACK'ine gore basarili/
// basarisiz bilgisi verir (bkz. onEspNowSent()). Bu bilgiyi kullanarak kendi
// "baglanti var/yok" durumumuzu turetiyoruz: ESPNOW_LINK_TIMEOUT_MS suresince
// hic basarili gonderim olmazsa "YOK" sayilir. Sadece durum DEGISTIGINDE
// (surekli degil) konsola yazilir - PC/kamera ESP baglantisi kesilip
// gelince fark edilsin diye.
#define ESPNOW_LINK_TIMEOUT_MS 1000
unsigned long espNowLastOkAt = 0;
bool espNowLinkUp = false;

// NOT: bu core surumunde (arduino-esp32 3.3.8) esp_now_send_cb_t imzasi
// eski "const uint8_t *mac_addr" yerine "const wifi_tx_info_t *tx_info"
// bekliyor (onEspNowRecv()'deki esp_now_recv_info_t degisikligiyle ayni
// turden bir API guncellemesi) - mac adresine burada zaten ihtiyacimiz yok.
void onEspNowSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        espNowLastOkAt = millis();
    }
}

void updateEspNowLinkStatus() {
    bool nowUp = espNowLastOkAt != 0 && (millis() - espNowLastOkAt < ESPNOW_LINK_TIMEOUT_MS);
    if (nowUp != espNowLinkUp) {
        espNowLinkUp = nowUp;
        Serial.println(espNowLinkUp ? "Kamera ESP: ESP-NOW baglantisi VAR" : "Kamera ESP: ESP-NOW baglantisi YOK");
    }
}

void setupEspNow() {
    espNowRxQueue = xQueueCreate(8, sizeof(EspNowMsg));

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW: baslatilamadi!");
        return;
    }
    esp_now_set_pmk(ESPNOW_PMK);
    esp_now_register_recv_cb(onEspNowRecv);
    esp_now_register_send_cb(onEspNowSent);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, cameraEspMac, 6);
    peerInfo.channel = ESPNOW_CHANNEL;
    peerInfo.ifidx = WIFI_IF_AP; // bu kart sadece WIFI_AP modunda (STA yok) - arayuz acikca belirtilmezse varsayilan STA(0) olur ve gonderim basarisiz olur
    peerInfo.encrypt = true;
    memcpy(peerInfo.lmk, ESPNOW_LMK, 16);
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("ESP-NOW: peer eklenemedi!");
    }
}

// ESP32'yi kendi WiFi erisim noktasi (AP) yapar; PC/telefon dogrudan bu aga
// baglanip Arduino IDE'den kablosuz kod atabilir. Router/internet gerekmez -
// tamamen yerel, tezgah/dokta kullanim icindir (motor komutlari bundan
// etkilenmez, onlar USB Serial ve/veya kamera ESP koprusunden (ESP-NOW) geliyor).
// ESPNOW_CHANNEL'a sabitlenmis kanal, kamera ESP'siyle ESP-NOW gorusmesi icin
// ikisinin de ayni kanalda olmasini garanti eder.
void setupOTA() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(OTA_AP_SSID, OTA_AP_PASS, ESPNOW_CHANNEL, 0, 4);
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

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Torpido pinleri - hicbir komut mantigi yok, sadece pasif/LOW baslatiliyor
    // ki acilista/yanlislikla aktif olmasinlar.
    for (int i = 0; i < 5; i++) {
        pinMode(torpedoPins[i], OUTPUT);
        digitalWrite(torpedoPins[i], LOW);
    }

    // ESP-NOW, sendStatus() (dolayisiyla asagidaki imuInit()/depthCalibrateSurface()
    // gibi sendStatus() cagiran her sey) kullanilmadan ONCE hazir olmali - aksi
    // halde esp_now_send() henuz baslatilmamis bir surucuye erisir ve cip cakilir
    // (LoadProhibited crash - bu sirayla yasandi, bkz. commit mesaji/konusma).
    setupOTA();     // WiFi.mode(WIFI_AP) burada baslar - ESP-NOW'dan once gerekli
    setupEspNow();

    // Bring-up icin: bu karti MAC adresini USB Serial Monitor'a basar. ONEMLI:
    // WiFi.macAddress() STA arayuzunun MAC'ini dondurur - biz STA hic acmiyoruz
    // (sadece WIFI_AP), o yuzden o hep 00:00:00:00:00:00 basardi. ESP-NOW'u
    // WIFI_IF_AP'a baglamis olduk (bkz. setupEspNow()), o yuzden gercek/kullanilan
    // adres softAPmacAddress()'tir.
    Serial.print("Bu kartin WiFi MAC adresi (ESP-NOW bring-up icin): ");
    Serial.println(WiFi.softAPmacAddress());

    imuReady = imuInit();

    analogSetPinAttenuation(DEPTH_ADC_PIN, ADC_11db); // 0-3.3V tam araligi kullan
    depthCalibrateSurface();

    dht.begin();

    sendStatus((imuReady ? String("SUALTIESP32 READY (IMU OK) MAC=") : String("SUALTIESP32 READY (IMU YOK) MAC=")) + WiFi.softAPmacAddress());
    sendStatus("TORPEDO:" + String(torpedoesRemaining)); // GUI acilista "5/5" gorsun diye

    bootSettleUntil = millis() + BOOT_SETTLE_MS;
}

void loop() {
    ArduinoOTA.handle();
    updateLed(); // flashLed() ile yakilan LED'i zamani gelince sondurur
    updateEspNowLinkStatus(); // baglanti VAR/YOK degistiginde konsola yazar
    updateTorpedoPulse(); // pulse suresi dolan torpido pinini LOW'a geri ceker

    // Komutlar artik SADECE kamera ESP'sinden (AnaRovKamera.ino) ESP-NOW ile
    // geliyor - USB (Serial) dinlenmiyor, sadece debug/durum ciktisi icin acik
    // kaliyor (bkz. sendStatus()). Denizde zaten USB kablosu olmayacagi icin
    // bu dinleme gereksizdi. onEspNowRecv() zaten gonderici MAC'ini dogrulamis
    // olarak kuyruga koyuyor; burada sadece isliyoruz.
    {
        EspNowMsg msg;
        int drained = 0;
        while (drained++ < 32 && xQueueReceive(espNowRxQueue, &msg, 0) == pdTRUE) {
            // Acilistan sonraki BOOT_SETTLE_MS boyunca hatta ne gelirse gelsin
            // yok say - ESP-NOW paketleri MAC-katmaninda dogrulanmis olsa da bu
            // ucuz savunma katmani korunuyor (bkz. tanimindaki not).
            if (millis() < bootSettleUntil) continue;
            String line((const char *)msg.data);
            line.trim();
            if (line.length() > 0) handleLine(line);
        }
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
        sendStatusQuiet("ATT:" + String(rollDeg, 1) + "," + String(pitchDeg, 1) + "," + String(yawDeg, 1));
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
        sendStatusQuiet("DEPTH:" + String(depthMeters, 2) + "," + String(depthSpeedMs, 3));
    }

    if (millis() - lastNemAt >= NEM_PERIOD_MS) {
        lastNemAt = millis();
        float h = dht.readHumidity();
        float t = dht.readTemperature();
        if (!isnan(h) && !isnan(t)) { // DHT11 okuma hatasinda NaN doner - o turu atla
            humidityPct = h;
            dhtTempC = t;
            sendStatusQuiet("NEM:" + String(humidityPct, 1) + "," + String(dhtTempC, 1));
        }
    }
}
