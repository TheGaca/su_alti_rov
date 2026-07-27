#ifndef JOYSTICKTHREAD_H
#define JOYSTICKTHREAD_H

#include <QThread>
#include <QString>
#include <array>

// Linux joystick driver okuyucu thread (/dev/input/jsX)
class JoystickThread : public QThread {
    Q_OBJECT
public:
    explicit JoystickThread(const QString &device = "/dev/input/js0",
                            QObject *parent = nullptr);
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

    // Analog stick, dururken bile kucuk titremelerle (drift/gurultu) saniyede
    // yuzlerce eksen olayi uretebiliyor; her biri GUI thread'ine kuyruklu
    // sinyal + widget repaint olarak dusup (bkz. RovGUI::update_ana_joy_axis)
    // kamera goruntusuyle ayni event loop'unda yarisiyor ve kasmaya yol
    // aciyordu. Onceki yayinlanan degerden yeterince farkli olmayan eksen
    // olaylari burada, kaynakta filtrelenir. -2.0f: "henuz yayinlanmadi"
    // sentinel'i (gecerli araligin -1..1 disinda), ilk olayin daima gecmesini
    // saglar.
    std::array<float, 32> lastAxisValues{};
};

#endif // JOYSTICKTHREAD_H
