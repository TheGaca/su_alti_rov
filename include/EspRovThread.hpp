#ifndef ESPROVTHREAD_H
#define ESPROVTHREAD_H

#include <QThread>
#include <QString>
#include <array>

class QSerialPort;

// ESP32 tabanli 8 motorlu ROV govdesiyle (bkz. SuAltiEsp32AnaRov/sualtiesp.ino)
// seri port uzerinden haberlesen thread. Eskiden MAVLink/Pixhawk uzerinden
// gonderilen set_servo() komutlarinin yerini bu sinif alir.
//
// Seri protokol:
//   "ARM"                        -> ESC'leri arm eder
//   "DISARM"                     -> tum motorlara giden sinyali keser
//   "M:p1,p2,...,p8"              -> 8 motorun darbe genisligini (us) gonderir
// ESP32 taraf: "READY" / "ARMING" / "ARMED" / "DISARMED" / "ERR:..." satirlari yollar.
class EspRovThread : public QThread {
    Q_OBJECT
public:
    explicit EspRovThread(const QString &port = "/dev/ttyUSB0",
                         int baudrate = 115200,
                         QObject *parent = nullptr);
    void stop();

    void arm();
    void disarm();
    void set_motors(const std::array<int, 8> &pulses_us);

signals:
    void status_signal(const QString &msg);
    void armed_signal(bool armed);

protected:
    void run() override;

private:
    void send_line(const QString &line);

    QString portName;
    int baudRate;
    bool running;
    QSerialPort *serial;
};

#endif // ESPROVTHREAD_H
