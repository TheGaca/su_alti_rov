#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <WebSocketsServer.h>

// AI Thinker ESP32-CAM Pinleri
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Wi-Fi Bilgilerin
const char *ssid = "FiberHGW_HUV6UJ";
const char *password = "V7bU4EeD9TvL";

WebSocketsServer webSocket = WebSocketsServer(85);
uint8_t client_num = 0;
bool is_connected = false;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      is_connected = false;
      Serial.printf("[%u] Baglanti kesildi!\n", num);
      break;
    case WStype_CONNECTED:
      client_num = num;
      is_connected = true;
      Serial.printf("[%u] Python baglandi!\n", num);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  
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
  
  // Sinyal gurultusunu ve kasmayi onlemek icin frekansi sabitledik
  config.xclk_freq_hz = 10000000; 
  
  // HIZ ICIN OPTIMIZASYON:
  // Eger VGA hala yavas gelirse burayi FAMERSIZE_CIF (400x296) yapabilirsin.
  config.frame_size = FRAMESIZE_VGA; 
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST; // En guncel kareyi yakala (Gecikmeyi onler)
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 15; // Kaliteyi hafif dusurduk, paket boyutu kuculdu, veri hizi artti.
  config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Kamera hatasi: 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s->id.PID == OV2640_PID) {
    s->set_vflip(s, 1);   
    s->set_hmirror(s, 1); 
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Baglandi!");
  Serial.print("ESP32-CAM IP: ");
  Serial.println(WiFi.localIP());

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  webSocket.loop();

  if (is_connected) {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) return;
    
    // Veriyi binary olarak firlat
    webSocket.sendBIN(client_num, fb->buf, fb->len);
    
    esp_camera_fb_return(fb);
  }
}
