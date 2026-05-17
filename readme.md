# Su Altı ROV Kontrol Arayüzü

Pixhawk tabanlı su altı ROV (Ana ROV + Mini ROV) için Qt5/C++ kontrol arayüzü. MAVLink üzerinden iki adet Pixhawk ile haberleşir, MJPEG ve WebSocket kamera akışlarını gösterir, Linux joystick'ten anlık komutlar üretir.
![alt text](<foto/Screenshot from 2026-05-18 00-09-30.png>)
Şekil 1: Ana Kontrol Arayüzü Beyaz Temaa
![alt text](<foto/Screenshot from 2026-05-18 00-09-38.png>)
Şekil 2: Ana Kontrol Arayüzü Siyah Temaa


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

## 3. Proje Dosyalarını Yerleştir

Dosyaları bir klasöre topla:

```
su_alti_rov/
├── CMakeLists.txt
├── main.cpp
├── tasarim.h
├── tasarim.cpp
├── yazilim.h
├── yazilim.cpp
├── algoritma.h
├── algoritma.cpp
└── mavlink/         ← bir sonraki adımda buraya MAVLink başlıkları gelecek
```

---

## 4. MAVLink Başlık Dosyalarını İndir

`algoritma.cpp` `#include "mavlink/common/mavlink.h"` ifadesini kullanıyor — bu dosya MAVLink resmi reposundan gelir.

```bash
cd su_alti_rov

# c_library_v2 reposunu indir
git clone https://github.com/mavlink/c_library_v2.git mavlink_src

# Sadece gerekli olanları proje içine kopyala
mkdir -p mavlink
cp -r mavlink_src/* mavlink/

# (İsteğe bağlı) indirilen kaynak klasörü temizle
rm -rf mavlink_src
```

**Doğrulama:**

```bash
ls mavlink/common/mavlink.h
# Çıktı: mavlink/common/mavlink.h
```

Dosya yoksa derleme `fatal error: mavlink/common/mavlink.h: No such file or directory` hatası verir.

---

## 5. Ağ Arayüzü Adını Kontrol Et

`main.cpp` içinde `enp46s0` arayüz adı var. Senin sisteminde farklı olabilir.

```bash
ip link show
```

Çıktıda Ethernet portunu bul (`enp...`, `eth0`, `eno1` gibi). Farklıysa `main.cpp`'de üç yerde değiştir:

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
| `mavlink/common/mavlink.h: No such file` | Adım 4'ü tekrar yap |
| `undefined reference to cv::...` | `find_package(OpenCV REQUIRED)` çalışmıyor; `pkg-config opencv4` dene |

---

## 7. Çalıştırmadan Önce — Donanım Hazırlığı

### 7.1 Pixhawk'lar

İki Pixhawk USB ile bağlandığında genelde şu portlarda görünür:

```bash
ls /dev/ttyACM*
# /dev/ttyACM0  /dev/ttyACM1
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

IP'ler farklıysa `yazilim.cpp` içinde `start_camera_threads()` fonksiyonundaki adresleri düzenle.

**Hızlı test:**

```bash
# MiniROV kamerası açık mı?
curl -I http://192.168.88.2/stream

# AnaROV ping:
ping -c 3 192.168.1.116
```

---

## 8. Çalıştır

```bash
cd su_alti_rov/build
./su_alti_rov
```

Uygulama fullscreen açılır. Kapatmak için `Alt+F4` veya `Ctrl+Q`.

### İlk Açılışta Görmen Gerekenler

- **İki panel yan yana:** ANA ROV (sol) ve MİNİ ROV (sağ)
- **Üstte port seçici:** `/dev/ttyACM0`, `/dev/ttyACM1` listelenir
- **Joystick combobox:** `/dev/input/js0`, `js1` listelenir
- **Kamera alanları:** "Kamera Bekleniyor..." → bağlanınca canlı görüntü
- **Terminal logu (alt):** `[HH:MM:SS.zzz] Sistem ve Loglama Başlatıldı.`

---

## 9. Kullanım

### 9.1 ROV Bağlantısı

1. Üstteki **Port** combobox'tan doğru `/dev/ttyACM*` seç
2. **Baudrate** = `115200` bırak (Pixhawk varsayılanı)
3. **Bağlan** butonuna bas
4. Durum: `Bağlantı Başarılı! Sistem ID: 1` görmeli
5. Yönelim/VFR HUD verileri akmaya başlar

### 9.2 Joystick Bağlantısı

1. Kumanda Durumu kısmından `/dev/input/js0` seç
2. **Kol Bağla** butonuna bas
3. Durum: `Kol Bağlandı!`
4. Stick'leri oynat → "Sol Stick: X: 0.55 Y: -0.23" gibi değerler değişir

### 9.3 Yön Butonları (Kamera kısmının yanında)

| Buton | Kanal | Basılıyken PWM | Bırakınca |
|-------|-------|----------------|-----------|
| İleri | 2 | 1700 | 1500 |
| Geri | 2 | 1300 | 1500 |
| Sol | 1 | 1300 | 1500 |
| Sağ | 1 | 1700 | 1500 |
| Z+ | 3 | 1700 | 1500 |
| Z- | 3 | 1300 | 1500 |
| Sola Dön | 4 | 1300 | 1500 |
| Sağa Dön | 4 | 1700 | 1500 |
| Sabitle | — | ARM komutu | — |

### 9.4 Hızlı Komutlar

- **ACİL DURDURMA** → Tüm kanalları 1500'e (nötr) çeker
- **Otonom / Manuel** → Mod LED'i değişir, log'a yazılır
- **MiniROV Bırak** → Servo 7'ye 2000 PWM gönderir
- **Torpido Fırlat** → Servo 8'e 2000 PWM gönderir
- **Lamba Aç/Kapa** → Servo 5'e 2000/1000 PWM gönderir

### 9.5 Joystick Buton Kısayolları

| Buton | Aksiyon |
|-------|---------|
| A | Lamba Aç |
| B | Lamba Kapa |
| X | MiniROV Bırak (sadece Ana) |
| Y | Torpido Fırlat (sadece Ana) |

---

## 10. Log Dosyaları

Her çalıştırmada otomatik oluşur:

```bash
~/rov_logs/rov_YYYYMMDD_HHMMSS.log
```

Örnek içerik:

```
[19:14:38.109] Sistem ve Loglama Başlatıldı.
[19:15:02.451] Ana ROV bağlantı başlatılıyor: /dev/ttyACM0
[19:15:02.892] [ANA] Bağlantı Başarılı! Sistem ID: 1
[19:15:18.044] [ANA] İleri
```

---

## 11. Sorun Giderme

**Bağlanmıyor / `/dev/ttyACM0 açılamadı`:**
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

| Dosya | Satır (yaklaşık) | Ne |
|-------|------|----|
| `yazilim.cpp` | `start_camera_threads()` | Kamera IP'leri |
| `main.cpp` | başlangıç | Ağ arayüz adı (`enp46s0`) |
| `algoritma.h` | constructor default'ları | Varsayılan port/IP'ler |

**Servo kanal eşlemesi `yazilim.cpp` → `ana_dir_pressed()` ve `mini_dir_pressed()` fonksiyonlarında.**

**Stylesheet `tasarim.cpp` → `Ui_MainWindow::apply_styles()` içinde.**

---

## 13. Hızlı Başlangıç Özeti (TL;DR)

```bash
# Tek seferlik kurulum
sudo apt install -y build-essential cmake qtbase5-dev libqt5serialport5-dev \
    libqt5websockets5-dev libopencv-dev iputils-ping joystick
sudo usermod -a -G dialout,input $USER
# logout / login

# Projeyi al
cd su_alti_rov
git clone https://github.com/mavlink/c_library_v2.git mavlink_src
mkdir -p mavlink && cp -r mavlink_src/* mavlink/ && rm -rf mavlink_src

# Derle
mkdir -p build && cd build
cmake .. && make -j$(nproc)

# Çalıştır
./su_alti_rov
```

---

**Sürüm:** 1.0
**Lisans:** Proje sahibine ait
