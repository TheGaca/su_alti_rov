
![alt text](<../foto/espdevkit.png>)

# Motor ESP (Beyin)

| GPIO | Bağlı |
|---|---|
| 13 | Motor 1 |
| 4  | Motor 2 |
| 14 | Motor 3 |
| 27 | Motor 4 |
| 26 | Motor 5 |
| 25 | Motor 6 |
| 33 | Motor 7 |
| 32 | Motor 8 |
| 21 | BNO055 SDA |
| 22 | BNO055 SCL |
| 39 (VN) | Derinlik sensörü sinyal (ADC, 10k/20k bölücüden sonra) |
| 34 | Pil voltajı (ADC; Pil+ --[100k]-- GPIO34 --[33k]-- GND bölücüden sonra) |
| 18 | Torpido |
| 19 | Torpido |
| 23 | Torpido |
| 16 | Torpido |
| 17 | Torpido |
| 3V3 | BNO055 VIN |
| VIN/5V | Derinlik sensörü V+ |
| GND | BNO055 GND, derinlik sensörü GND, Kamera ESP GND |

BNO055: RST, INT, PS0, PS1, 3Vo, ADR → boşta.

Torpido için 5 pin ayrıldı: 18/19/23 (zaten kablolu) + 16/17 (eskiden
Serial2/UART köprüsündü, ESP-NOW'a geçince boşaldı - bkz. aşağıdaki "ESP-NOW"
bölümü). Şu an kodda sadece pasif/LOW olarak başlatılıyor, ateşleme/komut
mantığı henüz eklenmedi.

![alt text](<../foto/espcam.png>)

# Kamera ESP

Kamera modülü pinleri (XCLK, SIOD, SIOC, Y2-Y9, VSYNC, HREF, PCLK) ESP32-CAM üzerinde sabit/lehimli, bunlar yazılmadı.

| GPIO | Bağlı |
|---|---|
| 33 | W5500 CS |
| 13 | W5500 SCK |
| 14 | W5500 MISO |
| 32 | W5500 MOSI |
| GND | Motor ESP GND |
| 3V3 | — |

16, 17 → KULLANMA (PSRAM). 15, 12 artık boşta — W5500'ün tüm sinyalleri
(CS, SCK, MISO, MOSI) strapping pinlerinden (0, 2, 5, 12, 15) tamamen
uzaklaştırıldı. SCK önce GPIO32'ye denendi ama W5500 hiç cevap vermedi
("reset timeout") - SCK zamanlama-hassas olduğu için daha önce MOSI olarak
kanıtlanmış GPIO13'e alındı, MOSI da GPIO32'ye taşındı. 32, 33 eskiden
Serial2/UART köprüsündü, Motor ESP ile haberleşme artık kablosuz ESP-NOW ile
yapılıyor (bkz. aşağıdaki "ESP-NOW"
bölümü) - bu yüzden W5500'e ayrıldı.

![alt text](<../foto/Screenshot from 2026-07-23 18-08-17.png>)
# W5500

| Pin | Bağlı |
|---|---|
| RST | 3V3 |
| 3V3 | boşta |
| NC | boşta |
| INT | boşta |
| GND (hepsi) | GND |

GND-3V3 arası kapasitör: 330uF 25V (modül üzerinde hazır).

# Absolute Orient IMU Fusion Breakout
![alt text](<../foto/Screenshot from 2026-07-23 18-50-48.png>)

| Pin | Bağlı |
|---|---|
| VIN | 3V3 (Motor ESP) |
| GND | GND (Motor ESP) |
| SDA | GPIO21 (Motor ESP) |
| SCL | GPIO22 (Motor ESP) |
| RST | boşta |
| INT | boşta |
| PS0 | boşta |
| PS1 | boşta |
| ADR | boşta (adres 0x28) |
| 3Vo | boşta |

# IP / Port

| | |
|---|---|
| Kamera ESP IP | 192.168.2.220 |
| Kamera akışı | port 81 |
| Motor komut köprüsü (TCP) | port 8888 |

# ESP-NOW (Motor ESP ↔ Kamera ESP köprüsü)

Motor ESP ile Kamera ESP arasındaki eski Serial2/UART kablosu kaldırıldı;
haberleşme artık kablosuz ESP-NOW ile yapılıyor (PC↔Kamera ESP TCP bağlantısı
ve kamera akışı bundan etkilenmez, aynı kalıyor).

| | |
|---|---|
| Kanal | 1 (her iki kartta da `ESPNOW_CHANNEL` ile sabit) |
| Şifreleme | Açık (PMK/LMK, her iki kartta birebir aynı sabit anahtar) |
| Motor ESP MAC | B8:D6:1A:41:CD:B1 |
| Kamera ESP MAC | C0:49:EF:30:E9:8D |

MAC adresleri her iki kartın `setup()`'unda geçici olarak eklenen
`Serial.println(WiFi.macAddress())` satırıyla USB Serial Monitor'dan okunup
karşı karttaki `cameraEspMac[]`/`motorEspMac[]` dizisine yazılır. Herhangi bir
kart fiziksel olarak değişirse (MAC değişir) bu adım tekrarlanmalı.

# OTA WiFi

| Kart | SSID | Şifre | mDNS |
|---|---|---|---|
| Motor ESP | AnaROV(DenizAlti) | sualti123 | Ana-Rov-DenizAlti |
| Kamera ESP | AnaRovKamera(DenizAlti) | sualti123 | Ana-Rov-Kamera-DenizAlti |
