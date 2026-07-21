#ifndef JOYSTICKTHREAD_H
#define JOYSTICKTHREAD_H

#include <QThread>
#include <QString>

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
};

#endif // JOYSTICKTHREAD_H
