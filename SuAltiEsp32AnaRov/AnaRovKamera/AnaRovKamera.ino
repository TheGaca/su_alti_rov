#include <Arduino.h>
#include "esp_camera.h"

#include <ETH.h>
#include <SPI.h>
#include <NetworkServer.h>
#include <NetworkClient.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <esp_now.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "board_config.h"
#include "WegSh.h"

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

// GPIO12 (MTDI) ve GPIO15 (MTDO) ESP32'nin strapping pinleridir - MISO/CS
// buralarda oldugunda W5500'un boot anindaki pull-up/tri-state durumu bu
// pinleri yanlislikla yanlis okutup acilis arizasina (kasma/boot etmeme) yol
// acabiliyordu. CS, strapping olmayan bosta GPIO33'e tasindi (eskiden
// Serial2/UART bridge buradaydi, ESP-NOW'a gecince bosaldi).
//
// SCK'yi GPIO32'ye tasimayi denedik ama W5500 hic cevap vermedi ("reset
// timeout" / "reset w5500 failed") - SPI'da en zamanlama-hassas sinyal SCK
// oldugu icin bu pinde/kabloda bir sorun oldugu anlasildi. Bunun yerine SCK,
// daha once MOSI olarak sorunsuz calistigi kanitlanmis GPIO13'e alindi; MOSI
// da GPIO32'ye tasindi (MOSI, CS gibi SCK kadar zamanlama-kritik degil).
#define ETH_CS   33
#define ETH_IRQ  -1
#define ETH_RST  -1

#define ETH_SCK  13
#define ETH_MISO 14
#define ETH_MOSI 32

// ======================
// Motor ESP'sine (AnaRovBeyin.ino) ESP-NOW koprusu
// ======================
// PC motor komutlarini (ARM/DISARM/M:...) Ethernet (TCP) uzerinden bu karta
// gonderiyor; bu kart bunlari protokolu yorumlamadan (ham satir olarak)
// kablosuz ESP-NOW ile motor ESP'sine iletir - iki kart arasinda artik
// fiziksel kablo YOK (eskiden Serial2/UART kullaniliyordu, GPIO32/33
// kabloluydu; bu pinler artik bosta). 16/17'yi hala KULLANMA - bu kartta
// (WROVER-KIT) PSRAM'a ayrilmis, bu bridge'den bagimsiz bir kisit.
//
// Kanal, PMK/LMK ve karsi tarafin (Motor ESP) MAC adresi motor ESP'deki
// degerlerle BIREBIR AYNI olmali - biri degisirse diger kart da guncellenip
// yeniden flaslanmali.
#define BRIDGE_TCP_PORT 8888
#define ESPNOW_CHANNEL  1

// Motor ESP'sinin WiFi(AP) MAC adresi. Bring-up: her iki kartta setup()
// icindeki "Serial.println(WiFi.macAddress())" satiriyla kendi MAC'ini USB
// Serial Monitor'a bastirir; Motor ESP'nin bastirdigi MAC'i buraya yazip bu
// karti YENIDEN flasla. SIFIRLARLA birakilirsa ESP-NOW calismaz.
uint8_t motorEspMac[6] = {0xB8, 0xD6, 0x1A, 0x41, 0xCD, 0xB1};

// ESP-NOW yerlesik sifrelemesi (PMK/LMK) - motor ESP'deki degerlerle birebir
// ayni olmali (16 bayt).
// String literal yerine acik byte listesi kullanildi: bir string literal
// array boyutuyla (16) TAM eslesirse bazi derleyiciler (bu toolchain dahil)
// null-sonlandiriciyi dusurme konusunda hataya dusuyor ("initializer-string...
// too long") - acik liste bu belirsizligi tamamen ortadan kaldirir. Motor
// ESP'deki degerlerle BIREBIR AYNI olmali.
static const uint8_t ESPNOW_PMK[16] = {'S','u','A','l','t','i','R','o','v','P','M','K','2','0','2','6'};
static const uint8_t ESPNOW_LMK[16] = {'S','u','A','l','t','i','R','o','v','L','M','K','2','0','2','6'};

NetworkServer bridgeServer(BRIDGE_TCP_PORT);
NetworkClient bridgeClient;

// ==================== Ethernet aktivite LED'i ====================
// ESP32 uzerindeki onboard LED (GPIO2, cogu devkit'te mavi) - PC'ye Ethernet
// uzerinden veri gonderilen her anda kisa bir sure yanip soner. GPIO2
// strapping pini olsa da burada sadece boot bittikten SONRA, cikis olarak
// kullaniliyor - dislarindan bir sey onu surmedigi icin acilis riski yok.
#define LED_PIN 2
#define LED_FLASH_MS 50
volatile unsigned long ledOffAtMs = 0;

void flashLed()
{
  digitalWrite(LED_PIN, HIGH);
  ledOffAtMs = millis() + LED_FLASH_MS;
}

void updateLed()
{
  if (ledOffAtMs != 0 && millis() >= ledOffAtMs) {
    digitalWrite(LED_PIN, LOW);
    ledOffAtMs = 0;
  }
}

// PC->Motor ESP yonu icin, TCP'den gelen byte akisini satirlara ayirmak
// amacli birikim tamponu (ESP-NOW paket-bazli oldugu icin komple bir satir
// birikmeden gonderilemiyor, bkz. handleBridge()).
char bridgeLineBuf[250];
size_t bridgeLineLen = 0;

// ==================== ESP-NOW (Motor ESP koprusu) ====================
// ESP-NOW paketleri WiFi/LWIP task context'inde (gercek ISR degil) gelir;
// onEspNowRecv() burada agir is yapmadan sadece gonderici MAC'ini dogrulayip
// veriyi bir kuyruga koyar, gercek isleme (PC'ye yazma) loop() icinde
// handleBridge() tarafindan yapilir.
struct EspNowMsg {
    uint8_t len;
    uint8_t data[251]; // +1: null sonlandirma icin (esp_now payload max 250 bayt)
};

QueueHandle_t espNowRxQueue = nullptr;

void onEspNowRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    if (memcmp(info->src_addr, motorEspMac, 6) != 0) return; // beklenmeyen gonderici, yok say
    if (len <= 0 || len > 250) return;
    EspNowMsg msg;
    msg.len = (uint8_t)len;
    memcpy(msg.data, data, len);
    msg.data[len] = '\0';
    xQueueSend(espNowRxQueue, &msg, 0); // kuyruk doluysa mesaj sessizce dusurulur
}

void setupEspNow() {
    espNowRxQueue = xQueueCreate(8, sizeof(EspNowMsg));

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW: baslatilamadi!");
        return;
    }
    esp_now_set_pmk(ESPNOW_PMK);
    esp_now_register_recv_cb(onEspNowRecv);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, motorEspMac, 6);
    peerInfo.channel = ESPNOW_CHANNEL;
    peerInfo.ifidx = WIFI_IF_AP; // bu kart sadece WIFI_AP modunda (STA yok) - arayuz acikca belirtilmezse varsayilan STA(0) olur ve gonderim basarisiz olur
    peerInfo.encrypt = true;
    memcpy(peerInfo.lmk, ESPNOW_LMK, 16);
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("ESP-NOW: peer eklenemedi!");
    }
}

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

// PC<->motor ESP arasinda kopruleme yapar (PC tarafi TCP, motor ESP tarafi
// ESP-NOW); protokolu hic yorumlamaz, sadece satirlari tasir. Ayni anda tek
// PC baglantisi destekler - yeni biri baglanirsa (ya da eskisi koptuysa)
// onun yerine gecer.
void handleBridge()
{
  if (!bridgeClient || !bridgeClient.connected()) {
    NetworkClient newClient = bridgeServer.accept();
    if (newClient) {
      bridgeClient = newClient;
      bridgeLineLen = 0; // eski/yarim satir varsa at, yeni baglantiyla karismasin
      Serial.println("Kopru: PC baglandi (motor komutlari)");
    }
  }

  // PC -> Motor ESP: TCP'den gelen byte'lari satir tamponunda biriktir, '\n'
  // gorunce tek bir ESP-NOW paketi olarak gonder (ESP-NOW paket-bazlidir,
  // Serial2.write ile byte-byte aktarmanin aksine komple satir gerekir).
  // NOT: Motor ESP'sinden surekli telemetri (ATT: her 100ms, DEPTH: her 200ms)
  // aktigi icin bu donguler sinirsiz birakilirsa loop() uzun sure geri
  // donemeyip WiFi/Ethernet/kamera gorevlerini ac birakabilir (donma/kilitlenme
  // sebebiydi) - bu yuzden her cagrida en fazla 256 byte islenir, geri kalani
  // bir sonraki loop() turunde islenir.
  int guard = 0;
  while (bridgeClient && bridgeClient.connected() && bridgeClient.available() && guard++ < 256) {
    char c = (char)bridgeClient.read();
    if (c == '\n') {
      if (bridgeLineLen > 0) {
        esp_now_send(motorEspMac, (const uint8_t *)bridgeLineBuf, bridgeLineLen);
        bridgeLineLen = 0;
      }
    } else if (c != '\r') {
      if (bridgeLineLen < sizeof(bridgeLineBuf)) {
        bridgeLineBuf[bridgeLineLen++] = c;
      } else {
        // Satir 250 bayti asti (protokolde olmamasi gereken bir durum) -
        // crash yerine tamponu atip yeniden basliyoruz.
        bridgeLineLen = 0;
      }
    }
  }

  // Motor ESP -> PC: ESP-NOW kuyrugunu bosalt, her mesaji satir olarak
  // (sonuna '\n' ekleyerek) PC'ye yaz. Mesaj sayisi da ayni anti-freeze
  // mantigiyla sinirlandirilir.
  EspNowMsg msg;
  int drained = 0;
  while (drained++ < 32 && xQueueReceive(espNowRxQueue, &msg, 0) == pdTRUE) {
    if (bridgeClient && bridgeClient.connected()) {
      bridgeClient.write(msg.data, msg.len);
      bridgeClient.write((uint8_t)'\n');
      flashLed(); // Ethernet'e veri gitti - LED'i yak, kisa sure sonra soner
    }
  }
}

// ==================== WegSh (kirmizi iz takibi) ====================
// Kamera karesini periyodik olarak WegSh.h/.cpp ile analiz edip sonucu (yaw,
// gorunurluk) dogrudan PC'ye (bridgeClient uzerinden TCP ile) telemetri
// olarak yazar - Motor ESP/ESP-NOW'a HIC gitmez. PC bu veriyi sadece Otonom
// modda motor karisimina katar (bkz. RovGUI.cpp); burada ayri bir mod
// anahtarlama mantigina gerek yok, WegSh her zaman calisip raporlar.
//
// DIKKAT: esp_camera_fb_get()/esp_camera_fb_return() burada, kamera HTTP
// akis gorevinden (app_httpd.cpp) BAGIMSIZ bir gorevden (bridgeTask, Core 0)
// cagriliyor. config.fb_count=2 (PSRAM varken) bu tur birden fazla
// tuketiciyi desteklemek icin zaten boyle ayarlanmisti, ama gercek donanimda
// kararsizlik (kilitlenme, bozuk kare vb.) gozlenirse ilk supheli yer burasi
// olmali.
#define WEGSH_PERIOD_MS 200 // ~5Hz - kamera akisini/koprusunu yavaslatmayacak kadar seyrek

void updateWegSh()
{
  static unsigned long lastWegShAt = 0;
  if (millis() - lastWegShAt < WEGSH_PERIOD_MS) return;
  lastWegShAt = millis();

  if (!cameraServerStarted) return; // kamera henuz hazir degil

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) return;

  float yaw = 0.0f;
  bool visible = false;
  bool ok = wegshAnalyze(fb, yaw, visible);
  esp_camera_fb_return(fb);

  if (!ok) return; // JPEG cozme/bellek basarisiz oldu - bu turu atla, bir sonrakinde tekrar denenir

  if (bridgeClient && bridgeClient.connected()) {
    bridgeClient.print("WEGSH:");
    bridgeClient.print(yaw, 3);
    bridgeClient.print(",");
    bridgeClient.print(visible ? 1 : 0);
    bridgeClient.print("\n");
    flashLed();
  }
}

// handleBridge()'i ayri bir FreeRTOS gorevinde, Core 0'da calistirir - boylece
// ESP-NOW/TCP kopru trafigi, Core 1'de calisan ana loop()'taki ArduinoOTA.handle()
// gibi islerden bagimsiz, kesintisiz akar (kamera HTTP sunucusu zaten kendi
// gorevinde calisiyordu, bkz. loop()'taki eski not). Performans/duyarlilik
// amacli bir iyilestirme.
//
// Ayrica burada LED_PIN icin bir "kalp atisi" (heartbeat) darbesi de
// uretiliyor: saniyede bir flashLed() cagrilir, boylece sistem calisirken
// LED duzenli araliklarla yanip soner (eskiden bu amacla konsola "ALIVE"
// yaziliyordu, artik onun yerine gorsel LED kullanılıyor). Gercek Ethernet
// verisi giden anlarda handleBridge() icinden ayrica flashLed() cagrilmasi
// bu kalp atisinin ustune biner, sorun degil.
void bridgeTask(void *pvParameters)
{
  unsigned long lastHeartbeatAt = 0;
  for (;;) {
    if (bridgeServerStarted) {
      handleBridge();
    }
    if (millis() - lastHeartbeatAt >= 1000) {
      lastHeartbeatAt = millis();
      flashLed();
    }
    updateWegSh(); // kendi icinde WEGSH_PERIOD_MS ile hiz sinirlandirilmis
    updateLed(); // flashLed() ile yakilan LED'i zamani gelince sondurur
    vTaskDelay(pdMS_TO_TICKS(2)); // digerlerine CPU birak, sikica dongulemesin
  }
}

// ESP32'yi kendi WiFi erisim noktasi (AP) yapar; PC/telefon dogrudan bu aga
// baglanip Arduino IDE'den kablosuz kod atabilir. Router/internet gerekmez -
// tamamen yerel, tezgah/dokta kullanim icindir. WiFi ile Ethernet (ETH) ayni
// anda, birbirinden bagimsiz calisir (ayri donanim) - kamera/kopru bundan
// etkilenmez. ESPNOW_CHANNEL'a sabitlenmis kanal, motor ESP'siyle ESP-NOW
// gorusmesi icin ikisinin de ayni kanalda olmasini garanti eder.
void setupOTA()
{
  WiFi.mode(WIFI_AP);
  WiFi.softAP(OTA_AP_SSID, OTA_AP_PASS, ESPNOW_CHANNEL, 0, 4);
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

// EN pini/acilis guvenilirligi tanisi icin: her boot'ta reset sebebini
// bastirir. Bu, ayri EnPinTest.ino sketch'inde denenip calistigi dogrulanan
// tanisin, gercek kamera firmware'i icine tasinmis halidir - boylece gercek
// sistemde de her boot'ta neden reset attigini (POWERON/BROWNOUT/SW/vb.)
// gorebiliriz.
void printResetReason()
{
  esp_reset_reason_t reason = esp_reset_reason();
  Serial.print("Reset sebebi: ");
  switch (reason) {
    case ESP_RST_POWERON:  Serial.println("POWERON (gercek guc acilisi ya da EN pini toggle)"); break;
    case ESP_RST_EXT:      Serial.println("EXT (harici reset pini)"); break;
    case ESP_RST_SW:       Serial.println("SW (yazilimsal reset - esp_restart())"); break;
    case ESP_RST_PANIC:    Serial.println("PANIC (kod cokmesi)"); break;
    case ESP_RST_INT_WDT:  Serial.println("INT_WDT (ic watchdog)"); break;
    case ESP_RST_TASK_WDT: Serial.println("TASK_WDT (gorev watchdog'u)"); break;
    case ESP_RST_WDT:      Serial.println("WDT (diger watchdog)"); break;
    case ESP_RST_BROWNOUT: Serial.println("BROWNOUT - GERILIM DUSUSU! Guc kaynagi yetersiz/kararsiz."); break;
    case ESP_RST_SDIO:     Serial.println("SDIO"); break;
    default:               Serial.println((int)reason); break;
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(false); // true iken ESP-IDF'in dahili (wifi/heap/vs.) debug loglari da Serial'a akip cok gurultu yapiyordu
  delay(1500);

  Serial.println();
  Serial.println("=== EN/BOOT TANISI ===");
  printResetReason();

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Kamera/Ethernet basarisiz olsa bile OTA ile kurtarilabilsin diye en once
  // kurulur (asagidaki return'lerden etkilenmez).
  setupOTA();     // WiFi.mode(WIFI_AP) burada baslar - ESP-NOW'dan once gerekli
  setupEspNow();

  // Bring-up icin: bu karti MAC adresini USB Serial Monitor'a basar. ONEMLI:
  // WiFi.macAddress() STA arayuzunun MAC'ini dondurur - biz STA hic acmiyoruz
  // (sadece WIFI_AP), o yuzden o hep 00:00:00:00:00:00 basardi. ESP-NOW'u
  // WIFI_IF_AP'a baglamis olduk (bkz. setupEspNow()), o yuzden gercek/kullanilan
  // adres softAPmacAddress()'tir.
  Serial.print("Bu kartin WiFi MAC adresi (ESP-NOW bring-up icin): ");
  Serial.println(WiFi.softAPmacAddress());

  Serial.println();
  Serial.println("Kamera + Ethernet sistemi basliyor");

  if (!startCamera()) {
    Serial.println("Kamera baslatilamadi");
    return;
  }

  // W5500, ESP32'den ayri olarak dogrudan pilden besleniyor (sadece GND
  // ortak) - acilis aninda iki ayri kaynagin farkli hizlarda ayaga kalkmasi
  // (power-up sequencing) yuzunden W5500'un gucu ESP32 SPI ile konusmayi
  // denedigi anda henuz tam oturmamis olabilir, bu da ilk denemenin
  // basarisiz olmasina yol acabilir (elle RST basinca duzelmesinin sebebi
  // buydu - o an W5500'un gucu zaten uzun suredir stabildi). Elle mudahale
  // yerine birkac kez otomatik tekrar deneyerek bu gecikmeyi tolere ediyoruz.
  // Olay dinleyicisi tek sefer kaydedilir - startEthernet() tekrar tekrar
  // denense de her seferinde yeniden kaydolup ayni olayin birden fazla kez
  // islenmesine (ve log'un tekrarlanmasina) yol acmasin diye.
  Network.onEvent(onNetworkEvent);

  bool ethOk = false;
  for (int attempt = 1; attempt <= 5 && !ethOk; attempt++) {
    ethOk = startEthernet();
    if (!ethOk) {
      Serial.printf("Ethernet baslatma denemesi %d basarisiz, 1sn sonra tekrar denenecek...\n", attempt);
      delay(1000);
    }
  }
  if (!ethOk) {
    Serial.println("Ethernet baslatilamadi (5 deneme sonunda)");
    return;
  }

  // Kopru islemeyi Core 0'a sabitlenmis ayri bir goreve tasi (bkz. bridgeTask
  // yorumu) - ana loop() (Core 1) sadece ArduinoOTA.handle() ve durum
  // bayraklarini yonetiyor artik.
  xTaskCreatePinnedToCore(bridgeTask, "bridgeTask", 4096, NULL, 1, NULL, 0);
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

  // handleBridge() artik burada DEGIL, bridgeTask() icinde (Core 0) cagriliyor
  // - burada da cagirmak iki cekirdegin ayni verilere (bridgeClient,
  // bridgeLineBuf vb.) es zamanli erismesine (race condition) yol acardi.

  // ONEMLI: eskiden buradaki delay(1000) motor komutlarina saniyede 1 kez
  // bakilmasina yol acardi (kumandayla hareket icin kullanilmaz hale gelirdi).
  // Kamera HTTP sunucusu kendi FreeRTOS gorevinde calistigi icin bu delay'in
  // kaldirilmasi kamerayi etkilemez.
}
