#include <Arduino.h>
#include "esp_camera.h"

#include <ETH.h>
#include <SPI.h>

#include "board_config.h"

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

static bool ethernetConnected = false;
static bool cameraServerStarted = false;

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

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(false); // true iken ESP-IDF'in dahili (wifi/heap/vs.) debug loglari da Serial'a akip cok gurultu yapiyordu
  delay(1500);

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
  if (ethernetConnected && !cameraServerStarted) {
    Serial.println("Kamera web sunucusu baslatiliyor...");

    startCameraServer();
    cameraServerStarted = true;

    Serial.println("================================");
    Serial.print("Tarayicidan ac: http://");
    Serial.println(ETH.localIP());
    Serial.println("================================");
  }

  delay(1000);
}