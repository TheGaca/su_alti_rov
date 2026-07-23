#include "esp_camera.h"
#include <WiFi.h>       
#include <ETH.h>
#include <WebServer.h>
#include <SPI.h>

// =======================================================
// STRAPPING (BOOT) ENGELİNE TAKILMAYAN GÜVENLİ YENİ PİNLER
// =======================================================
#define ETH_SPI_MOSI  13  // 23'ten 13'e alındı (kamera HREF ile çakışıyordu)
#define ETH_SPI_MISO  14  // 19'dan 14'e alındı (kamera Y5 ile çakışıyordu)
#define ETH_SPI_SCLK  15  // 18'den 15'e alındı (kamera Y4 ile çakışıyordu)
#define ETH_SPI_CS    32  // 13'ten 32'ye alındı (Boot engeli bitti!)
#define ETH_SPI_RST   33  // 12'den 33'e alındı (Boot engeli bitti!)

// =======================================================
// ESP32-WROVER-DEV ORİJİNAL KAMERA PİNLERİ
// =======================================================
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    21
#define SIOD_GPIO_NUM    26  
#define SIOC_GPIO_NUM    27

#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      19  
#define Y4_GPIO_NUM      18  
#define Y3_GPIO_NUM       5  
#define Y2_GPIO_NUM       4  
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

WebServer server(80);

void handleStream() {
  WiFiClient client = server.client();
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  client.print(response);

  while (client.connected()) {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Kamera karesi alinamadi!");
      delay(20);
      continue;
    }
    client.print("--frame\r\n");
    client.print("Content-Type: image/jpeg\r\n");
    client.print("Content-Length: " + String(fb->len) + "\r\n\r\n");
    client.write(fb->buf, fb->len);
    client.print("\r\n");
    esp_camera_fb_return(fb);
    delay(5); 
  }
}

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:        Serial.println("Ethernet donanimi baslatildi."); break;
    case ARDUINO_EVENT_ETH_CONNECTED:    Serial.println("Ethernet kablosu takildi."); break;
    case ARDUINO_EVENT_ETH_GOT_IP:       Serial.println("Ethernet Baglantisi Basarili!"); break;
    case ARDUINO_EVENT_ETH_DISCONNECTED: Serial.println("Ethernet kablosu cikarildi!"); break;
    default: break;
  }
}

void setup() {
  // Yeni güvenli pin üzerinden W5500 modülünü resetliyoruz
  pinMode(ETH_SPI_RST, OUTPUT);
  digitalWrite(ETH_SPI_RST, LOW);  delay(50);  
  digitalWrite(ETH_SPI_RST, HIGH); delay(100); 

  Serial.begin(115200);
  Serial.setDebugOutput(false);
  Serial.println("\nCakismaz Statik IP Modu Baslatiliyor...");

  WiFi.onEvent(WiFiEvent);

  // DOĞRUDAN PC İÇİN STATİK IP
  IPAddress local_IP(192, 168, 88, 2);   
  IPAddress gateway(192, 168, 88, 1);    
  IPAddress subnet(255, 255, 255, 0);

  ETH.config(local_IP, gateway, subnet);

  // ONEMLI: SPI hattini ETH_SPI_* ile tanimladigimiz pinlere (13/14/15) gercekten
  // tasiyan tek satir burasi. Bu cagri olmadan SPI donanimi varsayilan VSPI
  // pinlerini (SCLK=18, MISO=19, MOSI=23) kullanmaya devam eder ve W5500'e
  // hicbir veri gitmez (kamera pinleriyle ayni oldugu icin de karisirdi).
  SPI.begin(ETH_SPI_SCLK, ETH_SPI_MISO, ETH_SPI_MOSI, ETH_SPI_CS);

  // Modülü donanımsal SPI hattı ve yeni güvenli pinlerle başlatıyoruz
  // begin(type, phy_addr, cs, irq, rst, spi) -> RST pini burada dogru yere (5. parametre) verildi
  // SPI hizi varsayilan 20MHz'den 4MHz'e dusuruldu (jumper kablo/breadboard'da 20MHz cok hata verebiliyor)
  ETH.begin(ETH_PHY_W5500, -1, ETH_SPI_CS, -1, ETH_SPI_RST, SPI, 4);
  
  delay(1500); 

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
  config.pin_sscb_sda = SIOD_GPIO_NUM; 
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  
  config.xclk_freq_hz = 12000000; 
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA;  
    config.jpeg_quality = 10;           
    config.fb_count = 2;               
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Kamera Baslatilamadi! Hata: 0x%x\n", err);
    return;
  }
  Serial.println("Kamera basariyla baslatildi.");

  server.on("/stream", HTTP_GET, handleStream);
  server.begin();
  
  Serial.println("HTTP Sunucusu Aktif.");
  Serial.println("ESP32 URL: http://192.168.88.2/stream");
}

void loop() {
  server.handleClient();
}
