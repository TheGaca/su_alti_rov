#ifndef ALGORITMA_H
#define ALGORITMA_H

#include <QThread>
#include <QImage>
#include <QString>

// Forward declarations
class QSerialPort;
class QNetworkAccessManager;
class QNetworkReply;
class QWebSocket;

// MAVLink Thread
class MAVLinkThread : public QThread {
    Q_OBJECT
public:
    explicit MAVLinkThread(const QString &port = "/dev/ttyACM0", int baudrate = 115200, QObject *parent = nullptr);
    void stop();
    void arm_vehicle();
    void set_servo(int servo_no, int pwm);

signals:
    void attitude_signal(float roll, float pitch, float yaw);
    void vfr_hud_signal(float alt, float heading, float speed);
    void battery_signal(float voltage, int remaining);
    void status_signal(const QString &msg);

protected:
    void run() override;

private:
    QString portName;
    int baudRate;
    bool running;
    // MAVLink vars (we'll handle sysid/compid in cpp)
    int target_system;
    int target_component;
    QSerialPort *serial;
};

// Joystick Thread
class JoystickThread : public QThread {
    Q_OBJECT
public:
    explicit JoystickThread(const QString &device = "/dev/input/js0", QObject *parent = nullptr);
    void stop();

signals:
    void button_signal(int button_id, int state);
    void axis_signal(int axis_id, float value);
    void status_signal(const QString &msg);

protected:
    void run() override;

private:
    QString devicePath;
    bool running;
};

// Camera Thread
class CameraThread : public QThread {
    Q_OBJECT
public:
    explicit CameraThread(const QString &ip = "192.168.88.2", QObject *parent = nullptr);
    void stop();

signals:
    void image_signal(const QImage &image);
    void status_signal(const QString &msg);
    void stats_signal(int fps, float kbps, int width, int height);

protected:
    void run() override;

private:
    QString ipAddr;
    bool running;
};

// AnaRov Thread
class AnaRovThread : public QThread {
    Q_OBJECT
public:
    explicit AnaRovThread(const QString &ip = "192.168.1.116", QObject *parent = nullptr);
    void stop();

signals:
    void image_signal(const QImage &image);
    void status_signal(const QString &msg);
    void stats_signal(int fps, float kbps, int width, int height);

protected:
    void run() override;

private:
    QString ipAddr;
    bool running;
};

#endif // ALGORITMA_H
