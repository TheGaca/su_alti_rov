#include <Arduino.h>
#include "esp_camera.h"

#include <ETH.h>
#include <SPI.h>
#include <NetworkServer.h>
#include <NetworkClient.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

#include "board_config.h"

// ---- OTA (kablosuz kod yukleme) ayarlari ----
// Motor karti (AnaRovBeyin.ino) OTA'siyla ayni desen: ESP32 kendi WiFi erisim
// noktasini (AP) acar, dis bir router/internet gerekmez. SSID/hostname motor
// kartinkinden FARKLI - ikisi ayni anda acikken karismasin diye.
#define OTA_AP_SSID     "AnaRovKamera(DenizAlti)"
#define OTA_AP_PASS     "sualti123"  // WPA2 min. 8 karakter sart
#define OTA_PASSWORD    "sualti123"  // Arduino IDE kod atarken ayrica sorulan OTA sifresi
#define OTA_HOSTNAME    "Ana-Rov-Kamera-DenizAlti"  // mDNS hostname: sadece harf/rakam/tire, parantez/bosluk KULLANMA

// ======================
// W5500 Ethernet pinleri
// ======================

#define ETH_PHY_TYPE ETH_PHY_W5500
#define ETH_PHY_ADDR 1

#define ETH_CS   15
#define ETH_IRQ  -1
#define ETH_RST  -1

#define ETH_SCK  14
#define ETH_MISO 12
#define ETH_MOSI 13

// ======================
// Motor ESP'sine (AnaRovBeyin.ino) UART koprusu
// ======================
// PC artik motor komutlarini (ARM/DISARM/M:...) USB yerine bu karta Ethernet
// (TCP) uzerinden gonderiyor; bu kart gelen/giden her byte'i oldugu gibi
// Serial2'ye/den aktarir - protokolu hic yorumlamaz, sadece kopru gorevi gorur.
// Kablolama: bu kart GPIO33(TX) -> motor ESP GPIO16(RX), bu kart GPIO32(RX)
// <- motor ESP GPIO17(TX), GND ortak. 16/17'yi KULLANMA - bu kartta (WROVER-KIT)
// PSRAM'a ayrilmis.
#define BRIDGE_TCP_PORT 8888
#define BRIDGE_RX_PIN   32
#define BRIDGE_TX_PIN   33
#define BRIDGE_BAUD     115200

NetworkServer bridgeServer(BRIDGE_TCP_PORT);
NetworkClient bridgeClient;

static bool ethernetConnected = false;
static bool cameraServerStarted = false;
static bool bridgeServerStarted = false;

void startCameraServer();
void setupLedFlash();

void onNetworkEvent(arduino_event_id_t event,
                    arduino_event_info_t info)
{
  switch (event) {

    case ARDUINO_EVENT_ETH_START:
      Serial.println("Ethernet baslatildi");
      ETH.setHostname("esp32-camera");
      break;

    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("Ethernet kablosu baglandi");
      break;

    case ARDUINO_EVENT_ETH_GOT_IP:
      ethernetConnected = true;

      Serial.println("Ethernet IP hazir");

      Serial.print("IP adresi: ");
      Serial.println(ETH.localIP());

      Serial.print("MAC adresi: ");
      Serial.println(ETH.macAddress());

      Serial.print("Kamera adresi: http://");
      Serial.println(ETH.localIP());

      break;

    case ARDUINO_EVENT_ETH_LOST_IP:
      Serial.println("Ethernet IP kayboldu");
      ethernetConnected = false;
      break;

    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("Ethernet kablosu ayrildi");
      ethernetConnected = false;
      break;

    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("Ethernet durduruldu");
      ethernetConnected = false;
      break;

    default:
      break;
  }
}

bool startEthernet()
{
  Network.onEvent(onNetworkEvent);

  pinMode(ETH_CS, OUTPUT);
  digitalWrite(ETH_CS, HIGH);

  SPI.begin(
    ETH_SCK,
    ETH_MISO,
    ETH_MOSI,
    ETH_CS
  );

  Serial.println("W5500 baslatiliyor...");

  if (!ETH.begin(
        ETH_PHY_TYPE,
        ETH_PHY_ADDR,
        ETH_CS,
        ETH_IRQ,
        ETH_RST,
        SPI)) {

    Serial.println("HATA: W5500 baslatilamadi");
    return false;
  }

  IPAddress ip(192, 168, 2, 220);
  IPAddress gateway(192, 168, 2, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns(192, 168, 2, 1);

  if (!ETH.config(ip, gateway, subnet, dns)) {
    Serial.println("HATA: Sabit IP ayarlanamadi");
    return false;
  }

  Serial.println("Sabit IP ayarlandi");
  return true;
}

bool startCamera()
{
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;

  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;

  config.frame_size = FRAMESIZE_UXGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if (psramFound()) {
    Serial.println("PSRAM bulundu");

    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    Serial.println("PSRAM bulunamadi");

    config.frame_size = FRAMESIZE_SVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);

  if (err != ESP_OK) {
    Serial.printf(
      "Kamera baslatma hatasi: 0x%x\n",
      err
    );

    return false;
  }

  sensor_t *sensor = esp_camera_sensor_get();

  if (sensor != nullptr) {
    // İlk açılışı daha hafif çözünürlükte yap
    sensor->set_framesize(sensor, FRAMESIZE_VGA);
  }

#if defined(LED_GPIO_NUM)
  setupLedFlash();
#endif

  Serial.println("Kamera baslatildi");
  return true;
}

// PC<->motor ESP arasinda ham byte kopruleme yapar; protokolu hic bilmez.
// Ayni anda tek PC baglantisi destekler - yeni biri baglanirsa (ya da eskisi
// koptuysa) onun yerine gecer.
void handleBridge()
{
  if (!bridgeClient || !bridgeClient.connected()) {
    NetworkClient newClient = bridgeServer.accept();
    if (newClient) {
      bridgeClient = newClient;
      Serial.println("Kopru: PC baglandi (motor komutlari)");
    }
  }

  while (bridgeClient && bridgeClient.connected() && bridgeClient.available()) {
    Serial2.write(bridgeClient.read());
  }

  while (Serial2.available()) {
    uint8_t b = Serial2.read();
    if (bridgeClient && bridgeClient.connected()) {
      bridgeClient.write(b);
    }
  }
}

// ESP32'yi kendi WiFi erisim noktasi (AP) yapar; PC/telefon dogrudan bu aga
// baglanip Arduino IDE'den kablosuz kod atabilir. Router/internet gerekmez -
// tamamen yerel, tezgah/dokta kullanim icindir. WiFi ile Ethernet (ETH) ayni
// anda, birbirinden bagimsiz calisir (ayri donanim) - kamera/kopru bundan
// etkilenmez.
void setupOTA()
{
  WiFi.mode(WIFI_AP);
  WiFi.softAP(OTA_AP_SSID, OTA_AP_PASS);
  Serial.print("OTA AP acildi -> SSID: ");
  Serial.print(OTA_AP_SSID);
  Serial.print("  IP: ");
  Serial.println(WiFi.softAPIP());

  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
    Serial.println("OTA: guncelleme basladi.");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA HATA[%u]\n", error);
  });
  ArduinoOTA.begin();
}

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(false); // true iken ESP-IDF'in dahili (wifi/heap/vs.) debug loglari da Serial'a akip cok gurultu yapiyordu
  Serial2.begin(BRIDGE_BAUD, SERIAL_8N1, BRIDGE_RX_PIN, BRIDGE_TX_PIN);
  delay(1500);

  // Kamera/Ethernet basarisiz olsa bile OTA ile kurtarilabilsin diye en once
  // kurulur (asagidaki return'lerden etkilenmez).
  setupOTA();

  Serial.println();
  Serial.println("Kamera + Ethernet sistemi basliyor");

  if (!startCamera()) {
    Serial.println("Kamera baslatilamadi");
    return;
  }

  if (!startEthernet()) {
    Serial.println("Ethernet baslatilamadi");
    return;
  }
}

void loop()
{
  ArduinoOTA.handle();

  if (ethernetConnected && !cameraServerStarted) {
    Serial.println("Kamera web sunucusu baslatiliyor...");

    startCameraServer();
    cameraServerStarted = true;

    Serial.println("================================");
    Serial.print("Tarayicidan ac: http://");
    Serial.println(ETH.localIP());
    Serial.println("================================");
  }

  if (ethernetConnected && !bridgeServerStarted) {
    bridgeServer.begin();
    bridgeServerStarted = true;
    Serial.print("Motor komut koprusu (TCP) hazir, port: ");
    Serial.println(BRIDGE_TCP_PORT);
  }

  if (bridgeServerStarted) {
    handleBridge();
  }

  // ONEMLI: eskiden buradaki delay(1000) motor komutlarina saniyede 1 kez
  // bakilmasina yol acardi (kumandayla hareket icin kullanilmaz hale gelirdi).
  // Kamera HTTP sunucusu kendi FreeRTOS gorevinde calistigi icin bu delay'in
  // kaldirilmasi kamerayi etkilemez.
}