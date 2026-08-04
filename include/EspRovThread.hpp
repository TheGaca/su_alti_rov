#ifndef ESPROVTHREAD_H
#define ESPROVTHREAD_H

#include <QThread>
#include <QString>
#include <QMutex>
#include <QQueue>
#include <array>
#include <atomic>

class QIODevice;

// ESP32 tabanli 8 motorlu ROV govdesiyle (bkz. SuAltiEsp32AnaRov/AnaRovBeyin.ino)
// haberlesen thread. Iki tasiyici modu var:
//  - Seri (USB):     explicit EspRovThread(port, baudrate, parent) - Mini ROV
//  - TCP (Ethernet):  EspRovThread::createTcp(host, tcpPort, parent) - Ana ROV;
//    komutlar kamera ESP'sindeki (SuAltiEsp32AnaRov/AnaRovKamera/AnaRovKamera.ino)
//    TCP koprusune gider, o da bunlari Serial2 uzerinden motor ESP'sine iletir.
// Protokol tasiyicidan bagimsiz, ikisinde de birebir aynidir:
//   "ARM"                        -> ESC'leri arm eder
//   "DISARM"                     -> tum motorlara giden sinyali keser
//   "M:p1,p2,...,p8"              -> 8 motorun darbe genisligini (us) gonderir
// ESP32 taraf: "READY" / "ARMING" / "ARMED" / "DISARMED" / "ERR:..." satirlari yollar.
// ESP32 ayrica IMU (BNO055) varsa periyodik "ATT:roll,pitch,yaw" (derece) satiri yollar.
// Basinc sensoru takiliysa "DEPTH:metre,dikey_hiz_m/s" satiri da yollar.
// DHT11 nem/sicaklik sensoru takiliysa "NEM:nem_yuzde,sicaklik_C" satiri da yollar.
// Pil olcum bolucusu takiliysa "BAT:volt,yuzde" satiri da yollar.
//   "TORPEDO"                    -> siradaki torpidoyu ates alir (kalan sayi ve
//                                    10sn bekleme suresi ESP32 tarafinda yonetilir)
//   "TORPEDO:0"/"TORPEDO:1"/"TORPEDO:2" -> belirli torpidoyu ates alir
// ESP32 taraf ates alma sonrasi "TORPEDO:kalan_sayi" satiri yollar; kalan yoksa
// veya bekleme suresi dolmadiysa "ERR:TORPEDOEMPTY" / "ERR:TORPEDOCOOLDOWN" doner.
//   "TCFG:min1,ntr1,max1,min2,ntr2,max2,min3,ntr3,max3" -> 3 torpido ESC'sinin
//                                    darbelerini (arm/notr/ates, us) canli gunceller
//                                    (bkz. torpedo_config()); ESP32 basarili olursa
//                                    "TCFG:OK" doner. Sadece Ana ROV'da anlamli.
class EspRovThread : public QThread {
    Q_OBJECT
public:
    explicit EspRovThread(const QString &port = "/dev/ttyUSB0",
                         int baudrate = 115200,
                         QObject *parent = nullptr);
    static EspRovThread *createTcp(const QString &host, quint16 tcpPort, QObject *parent = nullptr);
    void stop();

    void arm();
    void disarm();
    void set_motors(const std::array<int, 8> &pulses_us);
    void torpedo();
    void torpedo(int index);
    void torpedo_reverse(int index);
    void torpedo_reset();
    void torpedo_config(const std::array<int, 3> &min_us, const std::array<int, 3> &neutral_us, const std::array<int, 3> &max_us);

signals:
    void status_signal(const QString &msg);
    void armed_signal(bool armed);
    void attitude_signal(float roll, float pitch, float yaw);
    void depth_signal(float meters, float vertical_speed_ms);
    void nem_signal(float humidity_pct, float temperature_c);
    void battery_signal(float volts, int percent);
    void torpedo_signal(int remaining);

protected:
    void run() override;

private:
    EspRovThread(bool tcpMode, const QString &hostOrPort, int portOrBaud, QObject *parent);

    void process_line(const QString &line);
    void send_line(const QString &line);
    void write_pending_lines();

    bool useTcp;
    QString hostOrPort; // seri modda cihaz yolu (/dev/ttyUSB0), TCP modda host/IP
    int portOrBaud;     // seri modda baudrate, TCP modda TCP port numarasi
    std::atomic<bool> running;

    // run() icindeyken o an acik olan seri/TCP nesnesini gosterir; sadece
    // run()'in kendi thread'inde yazilir/okunur (bkz. write_pending_lines()).
    QIODevice *device;

    // arm()/disarm()/set_motors() cagrilari GUI thread'inden gelir; device
    // ise sadece run()'in kendi thread'inde yasar. Bu yuzden yazma istekleri
    // once bu kuyruga konur, gercek device->write() ise run() donugusunde
    // (dogru thread'de) yapilir.
    QMutex writeMutex;
    QQueue<QByteArray> pendingWrites;
};

#endif // ESPROVTHREAD_H
