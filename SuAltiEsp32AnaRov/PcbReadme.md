
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
| 34 | Derinlik sensörü sinyal (ADC) |
| 16 | Serial2 RX ← Kamera ESP GPIO33 |
| 17 | Serial2 TX → Kamera ESP GPIO32 |
| 18 | Torpido |
| 19 | Torpido |
| 23 | Torpido |
| 3V3 | BNO055 VIN |
| VIN/5V | Derinlik sensörü V+ |
| GND | BNO055 GND, derinlik sensörü GND, Kamera ESP GND |

BNO055: RST, INT, PS0, PS1, 3Vo, ADR → boşta.

![alt text](<../foto/espcam.png>)

# Kamera ESP

Kamera modülü pinleri (XCLK, SIOD, SIOC, Y2-Y9, VSYNC, HREF, PCLK) ESP32-CAM üzerinde sabit/lehimli, bunlar yazılmadı.

| GPIO | Bağlı |
|---|---|
| 15 | W5500 CS |
| 14 | W5500 SCK |
| 12 | W5500 MISO |
| 13 | W5500 MOSI |
| 33 | Serial2 TX → Motor ESP GPIO16 |
| 32 | Serial2 RX ← Motor ESP GPIO17 |
| GND | Motor ESP GND |
| 3V3 | — |

16, 17 → KULLANMA (PSRAM).

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

# IP / Port

| | |
|---|---|
| Kamera ESP IP | 192.168.2.220 |
| Kamera akışı | port 81 |
| Motor komut köprüsü (TCP) | port 8888 |

# OTA WiFi

| Kart | SSID | Şifre | mDNS |
|---|---|---|---|
| Motor ESP | AnaROV(DenizAlti) | sualti123 | Ana-Rov-DenizAlti |
| Kamera ESP | AnaRovKamera(DenizAlti) | sualti123 | Ana-Rov-Kamera-DenizAlti |
