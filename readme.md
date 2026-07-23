# Su Altı ROV Kontrol Arayüzü

ESP32 tabanlı su altı ROV (Ana ROV + Mini ROV, her biri 8 motorlu) için Qt5/C++ kontrol arayüzü.
Her iki ROV'un motor kontrol kartıyla (ESP32, bkz. `SuAltiEsp32AnaRov/sualtiesp.ino`) basit bir
metin tabanlı seri protokol üzerinden haberleşir, MJPEG ve WebSocket kamera akışlarını gösterir,
Linux joystick'ten anlık komutlar üretir.

> **Not:** Proje daha önce Pixhawk + MAVLink tabanlıydı. Artık MAVLink/Pixhawk tamamen kaldırıldı;
> her iki ROV da doğrudan ESP32 üzerinden 8 motor ile kontrol ediliyor. Bu yüzden Yönelim/VFR HUD/
> Batarya panelleri arayüzde duruyor (geriye dönük uyumluluk ve ileride sensör eklenirse diye) ama
> şu an veri akıtmıyor - bu ESP32 kartlarında IMU/batarya sensörü yok.

![alt text](<foto/Screenshot from 2026-05-18 00-09-30.png>)
Şekil 1: Ana Kontrol Arayüzü Beyaz Tema
![alt text](<foto/Screenshot from 2026-05-18 00-09-38.png>)
Şekil 2: Ana Kontrol Arayüzü Siyah Tema
![alt text](<foto/5a5bd473784e48cfed6daa1d1cb0c4e51b705469.png>)
Şekil 3: 8 motor yerleşimi (1,2 = ön çapraz yatay itki; 3,4 = arka çapraz yatay itki; 5,6,7,8 = dikey/derinlik itkisi). Arayüzdeki motor diyagramı bu numaralandırmayı birebir kullanır.

---

## 1. Sistem Gereksinimleri

- **İşletim Sistemi:** Ubuntu 20.04 / 22.04 / 24.04 (veya türevi: Debian, Pop!_OS, Linux Mint)
- **Mimari:** x86_64 (ARM/Raspberry Pi için de derlenir; OpenCV kurulumu farklı)
- **RAM:** En az 2 GB
- **Disk:** Yaklaşık 2 GB (Qt + OpenCV + bağımlılıklar)

---

## 2. Bağımlılıkları Kur

Aşağıdaki tek komutu terminale yapıştır:

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    git \
    qtbase5-dev \
    qtbase5-dev-tools \
    libqt5serialport5-dev \
    libqt5websockets5-dev \
    libqt5network5 \
    qtchooser \
    qt5-qmake \
    libopencv-dev \
    iputils-ping \
    joystick \
    jstest-gtk
```

**Kurulumu doğrula:**

```bash
qmake --version           # Qt 5.x görmeli
cmake --version           # 3.10+ görmeli
pkg-config --modversion opencv4   # OpenCV sürümü görmeli
```

---

## 3. Proje Yapısı

Her sınıf kendi başlık (`include/*.hpp`) ve kaynak (`src/*.cpp`) dosyasında tutulur:

```
su_alti_rov/
├── CMakeLists.txt
├── include/
│   ├── EspRovThread.hpp      ← ESP32 seri port haberleşmesi (ARM/DISARM/motor komutu)
│   ├── JoystickThread.hpp    ← Linux joystick okuyucu
│   ├── CameraThread.hpp      ← MiniROV MJPEG kamera akışı
│   ├── AnaRovThread.hpp      ← AnaROV WebSocket kamera akışı
│   ├── ColorPickerButton.hpp
│   ├── AttitudeIndicator.hpp
│   ├── MotorDiagramWidget.hpp← Canlı 8 motor durumu diyagramı
│   ├── PixhawkPanel.hpp      ← Tek bir ROV paneli (Ana/Mini ortak)
│   ├── Ui_MainWindow.hpp
│   └── PixhawkGUI.hpp        ← Ana pencere / uygulama mantığı
├── src/
│   ├── main.cpp
│   └── (yukarıdaki her .hpp için aynı isimli .cpp)
├── SuAltiEsp32AnaRov/
│   └── sualtiesp.ino         ← Ana ROV'un 8 motor ESP32 firmware'i
└── foto/
```

---

## 4. ESP32 Firmware'ini Yükle

`SuAltiEsp32AnaRov/sualtiesp.ino` dosyasını Arduino IDE ile aç, ESP32 WROOM kartını seç ve yükle.

Bu firmware 8 ESC'yi (pinler: `13, 4, 14, 27, 26, 25, 33, 32`) 50Hz PWM ile sürer ve PC ile
115200 baud'da şu basit satır tabanlı protokolü konuşur:

| Komut (PC → ESP32) | Anlamı |
|---|---|
| `ARM\n` | ESC'leri arm eder (2 sn 1000us tutar, sonra 1490us nötre geçer) |
| `DISARM\n` | Tüm motor pinlerine giden PWM sinyalini tamamen keser |
| `M:p1,p2,...,p8\n` | 8 motorun darbe genişliğini (us, 1000-2000) ayarlar (sadece ARMED iken) |

ESP32 taraf: `READY`, `ARMING`, `ARMED`, `DISARMED`, `ERR:...` satırlarını geri yollar.
**Güvenlik:** ARMED durumdayken 500ms boyunca yeni `M:` komutu gelmezse (bağlantı kopması
ihtimaline karşı) tüm motorlar otomatik olarak nötre (1490us) çekilir.

Mini ROV için de aynı protokolü konuşan bir ESP32 kartı/firmware'i kullanılması beklenir
(şu an için `sualtiesp.ino` sadece Ana ROV'un 8 motorluk gövdesi için yazıldı; Mini ROV'un
kendi donanımı farklıysa pin sayısını/yerleşimini bu dosyaya göre uyarlaman gerekir).

---

## 5. Ağ Arayüzü Adını Kontrol Et (Kameralar İçin)

`main.cpp` içinde `enp46s0` arayüz adı var. Senin sisteminde farklı olabilir. Bu sadece MiniROV'un
MJPEG kamera akışına (IP ağı üzerinden) erişim için kullanılıyor, motor kontrolüyle ilgisi yok.

```bash
ip link show
```

Çıktıda Ethernet portunu bul (`enp...`, `eth0`, `eno1` gibi). Farklıysa `src/main.cpp`'de üç yerde değiştir:

```cpp
ret = system("sudo -n ip addr flush dev ENP_KENDI 2>/dev/null");
ret = system("sudo -n ip addr add 192.168.88.1/24 dev ENP_KENDI 2>/dev/null");
ret = system("sudo -n ip link set ENP_KENDI up 2>/dev/null");
```

**Şifresiz sudo (opsiyonel ama önerilen):**
Uygulama her açılışta şifre sormasın diye:

```bash
sudo visudo
```

Açılan dosyanın sonuna ekle (kullanıcı adını kendininkiyle değiştir):

```
KULLANICI ALL=(ALL) NOPASSWD: /usr/sbin/ip
```

---

## 6. Derle

```bash
cd su_alti_rov
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

**Beklenen çıktı:** `build/su_alti_rov` adında çalıştırılabilir dosya.

**Yaygın hatalar:**

| Hata | Çözüm |
|------|-------|
| `Qt5SerialPort not found` | `sudo apt install libqt5serialport5-dev` |
| `Qt5WebSockets not found` | `sudo apt install libqt5websockets5-dev` |
| `OpenCV not found` | `sudo apt install libopencv-dev` |
| `undefined reference to cv::...` | `find_package(OpenCV REQUIRED)` çalışmıyor; `pkg-config opencv4` dene |

---

## 7. Çalıştırmadan Önce — Donanım Hazırlığı

### 7.1 ESP32 Motor Kartları (Ana ROV / Mini ROV)

İki ESP32 USB ile bağlandığında genelde şu portlarda görünür:

```bash
ls /dev/ttyUSB*
# /dev/ttyUSB0  /dev/ttyUSB1
```

**Kullanıcıya yetki ver (bir kere yap, sonra logout/login):**

```bash
sudo usermod -a -G dialout $USER
```

### 7.2 Joystick (Kol)

```bash
ls /dev/input/js*
# /dev/input/js0   (varsa /dev/input/js1)

# Test et:
jstest /dev/input/js0
# Eksenler/butonlar canlı görünmeli
```

**Kullanıcıya yetki ver:**

```bash
sudo usermod -a -G input $USER
# logout/login gerekli
```

### 7.3 Kameralar

| Kaynak | Adres | Protokol |
|--------|-------|----------|
| MiniROV kamera | `http://192.168.88.2/stream` | MJPEG over HTTP |
| AnaROV kamera | `ws://192.168.1.116:85` | WebSocket (binary JPEG) |

IP'ler farklıysa `src/PixhawkGUI.cpp` içinde `start_camera_threads()` fonksiyonundaki adresleri düzenle.

**Hızlı test:**

```bash
 sudo ip addr flush dev enp46s0
 sudo ip addr add 192.168.88.1/24 dev enp46s0
 sudo ip link set enp46s0 up
 ping 192.168.88.2
```

---

## 8. Çalıştır

```bash
cd su_alti_rov/build
./su_alti_rov
```

Uygulama fullscreen açılır (pencere çerçevesi/kapatma düğmesi görünmez). Kapatmak için
`Ctrl+Q` veya `Esc` tuşuna bas (her ikisi de uygulama içinde tanımlı bir kısayoldur, "Çıkmak
istediğinize emin misiniz?" onayı çıkar) — `Alt+F4` da pencere yöneticin destekliyorsa çalışır.

### İlk Açılışta Görmen Gerekenler

- **İki panel yan yana:** ANA ROV (sol) ve MİNİ ROV (sağ)
- **Üstte port seçici:** `/dev/ttyUSB0`, `/dev/ttyUSB1` listelenir
- **Joystick combobox:** `/dev/input/js0`, `js1` listelenir
- **Kamera alanları:** "Kamera Bekleniyor..." → bağlanınca canlı görüntü
- **Motor diyagramı:** Yön butonlarının yanında, 8 motoru canlı gösteren şekil
- **Terminal logu (alt):** `[HH:MM:SS.zzz] Sistem ve Loglama Başlatıldı.`

---

## 9. Kullanım

### 9.1 ROV Bağlantısı (ESP32 seri port)

1. Üstteki **Port** combobox'tan doğru `/dev/ttyUSB*` seç
2. **Baudrate** = `115200` bırak (ESP32 firmware varsayılanı)
3. **Bağlan** butonuna bas
4. Durum: `ESP32 seri port bağlantısı açıldı` görmeli

### 9.2 ARM / DISARM

Eskiden "Sabitle" olan büyük mavi buton artık **ARM Et / DISARM Et** görevi görüyor:

1. Bağlandıktan sonra **ARM Et** butonuna bas → ESC'ler 2 saniyede arm olur, buton kırmızı
   **DISARM Et** haline döner, durum `ARMED` olur.
2. Yön butonları / joystick sadece **ARMED** durumdayken motorlara komut gönderir.
3. Tekrar **DISARM Et** butonuna basarsan motorlara giden PWM sinyali tamamen kesilir (ESC durur).

### 9.3 Joystick Bağlantısı (Logitech kumanda vb.)

1. Kumanda Durumu kısmından `/dev/input/js0` seç (Logitech kumandalar genelde bu şekilde görünür)
2. **Kol Bağla** butonuna bas
3. Durum: `Kol Bağlandı!` (yeşil yazı)
4. Stick'leri oynat → "Sol Stick: X:0.55 Y:-0.23  Sağ Stick: X:0.00 Y:0.00" gibi değerler değişir
   ve (ARMED ise) motor diyagramı buna göre renklenir

Her iki stick de kullanılıyor:

| Kumanda | Etki |
|---------|------|
| Sol Stick | İleri/Geri + Sol/Sağ (yanal kayma) |
| Sağ Stick | Dönüş (yaw, sağ-sol) + Yukarı/Aşağı (derinlik) |
| A / B | Lamba Aç / Kapa |
| X / Y | MiniROV Bırak / Torpido Fırlat (sadece Ana ROV) |

> Eksen numaraları kumanda modeline göre değişebilir. `jstest /dev/input/js0` ile hangi eksenin
> hangi stick'e ait olduğunu gör; `src/PixhawkGUI.cpp` başındaki `AXIS_LEFT_X/Y` ve
> `AXIS_RIGHT_X/Y` sabitleri kendi kumandanla eşleşmiyorsa oradan düzelt.

### 9.4 Yön Butonları (Kamera kısmının yanında) — 8 Motor Karışımı

Yön butonları artık tek bir AUX servo kanalına değil, doğrudan 8 motorun karışımına (mixing)
karşılık gelir (bkz. `src/PixhawkGUI.cpp` içindeki `compute_motor_mix()`):

| Buton | Etki |
|-------|------|
| İleri / Geri | 4 yatay motor (1,2,3,4) aynı yönde döner |
| Sol / Sağ | 1,3 motorları ile 2,4 motorları zıt yönde döner (yanal kayma) |
| Sola Dön / Sağa Dön | Çapraz motor çiftleri zıt yönde döner (dönüş/yaw) |
| Z+ / Z- | 4 dikey motor (5,6,7,8) aynı yönde döner |

> Motor yönleri ilk tahmindir. Bench testinde bir motor beklenenin tersine dönerse
> `compute_motor_mix()` içindeki ilgili ağırlığı (-1 ↔ +1) ters çevirmek yeterli.

### 9.5 Hızlı Komutlar

- **ACİL DURDURMA** → Tüm 8 motoru anında nötre (1490us) çeker
- **Otonom / Manuel** → Mod LED'i değişir, manuel yön butonları Otonom'da devre dışı kalır
- **MiniROV Bırak / Torpido Fırlat / Lamba Aç-Kapa** → Bu ESP32 kartında bu işlevler için ayrı
  bir çıkış tanımlı değil (8 pinin tamamı motor kontrolünde); butonlar arayüzde durum/LED
  güncellemesi yapar ve log'a yazar ama şu an bir donanım sinyali göndermez

---

## 10. Log Dosyaları

Her çalıştırmada otomatik oluşur:

```bash
~/rov_logs/rov_YYYYMMDD_HHMMSS.log
```

Örnek içerik:

```
[19:14:38.109] Sistem ve Loglama Başlatıldı.
[19:15:02.451] Ana ROV (ESP32) bağlantısı başlatılıyor: /dev/ttyUSB0
[19:15:02.892] [ANA] ESP32 seri port bağlantısı açıldı (/dev/ttyUSB0)
[19:15:05.010] [ANA] ARMED - ESC'ler hazır.
[19:15:18.044] [ANA] İleri
```

---

## 11. Sorun Giderme

**Bağlanmıyor / `/dev/ttyUSB0 açılamadı`:**
```bash
groups | grep dialout    # dialout görmeli
# yoksa: sudo usermod -a -G dialout $USER && relogin
```

**Joystick "Kol Bağlı Değil" kalıyor:**
```bash
ls -la /dev/input/js0
# crw-rw---- 1 root input  ← input grubunda olmalı
groups | grep input
```

**ARM Et'e basınca hiçbir şey olmuyor:**
- ESP32'nin doğru portta ve 115200 baud'da bağlı olduğunu kontrol et
- `screen /dev/ttyUSB0 115200` ile elle bağlanıp `ARM` yazıp Enter'a basarak firmware'in
  `ARMING` / `ARMED` yanıtı verdiğini doğrula

**Bir motor ters yönde dönüyor:**
- `src/PixhawkGUI.cpp` başındaki `W_SURGE` / `W_LATERAL` / `W_YAW` dizilerinde ilgili motorun
  ağırlığını (-1 ↔ +1) ters çevir ve yeniden derle

**Kamera görüntüsü gelmiyor:**
```bash
# MiniROV'a ping at
ping -c 3 192.168.88.2

# Ağ arayüzü ayarlanmış mı?
ip addr show enp46s0
# inet 192.168.88.1/24 görmeli
```

**FPS düşük / takılıyor:**
- Ağ kablo bağlantısını kontrol et
- `htop` ile CPU kullanımına bak (>%80 ise donanım sınırı)
- OpenCV decode darboğazsa `cv::imdecode` çağrılarına eklemeli

**Uygulama açılır açılmaz kapanıyor:**
```bash
./su_alti_rov 2>&1 | tee crash.log
# crash.log'a bak, segfault varsa bana ilet
```

---

## 12. Geliştirici Notları

**IP/Port değiştirme yerleri:**

| Dosya | Yer | Ne |
|-------|------|----|
| `src/PixhawkGUI.cpp` | `start_camera_threads()` | Kamera IP'leri |
| `src/main.cpp` | başlangıç | Ağ arayüz adı (`enp46s0`) |

**Motor mixing / kanal eşlemesi** `src/PixhawkGUI.cpp` → `compute_motor_mix()`,
`apply_ana_motor_mix()`, `apply_mini_motor_mix()` içinde.

**ESP32 seri protokolü** `include/EspRovThread.hpp` / `src/EspRovThread.cpp` (PC tarafı) ve
`SuAltiEsp32AnaRov/sualtiesp.ino` (kart tarafı) içinde; ikisi birbiriyle senkron tutulmalı.

**Motor diyagramı** `include/MotorDiagramWidget.hpp` / `src/MotorDiagramWidget.cpp` içinde;
motor numaralandırması `foto/5a5bd473784e48cfed6daa1d1cb0c4e51b705469.png` ile birebir eşleşir.

**Stylesheet** `src/Ui_MainWindow.cpp` → `Ui_MainWindow::apply_styles()` içinde.

---

## 13. Hızlı Başlangıç Özeti (TL;DR)

```bash
# Tek seferlik kurulum
sudo apt install -y build-essential cmake qtbase5-dev libqt5serialport5-dev \
    libqt5websockets5-dev libopencv-dev iputils-ping joystick
sudo usermod -a -G dialout,input $USER
# logout / login

# ESP32 firmware'ini yükle (Arduino IDE ile SuAltiEsp32AnaRov/sualtiesp.ino)

# Derle
cd su_alti_rov
mkdir -p build && cd build
cmake .. && make -j$(nproc)

# Çalıştır
./su_alti_rov
```
cd su_alti_rov
---

**Sürüm:** 2.0 (ESP32 8 motor mimarisi, MAVLink/Pixhawk kaldırıldı)
**Lisans:** SuGaca
