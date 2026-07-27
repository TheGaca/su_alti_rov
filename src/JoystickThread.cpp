#include "JoystickThread.hpp"

#include <cmath>
#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>

// Bu esikten kucuk eksen degisiklikleri gurultu/drift sayilip yayinlanmaz
// (bkz. JoystickThread.hpp'deki lastAxisValues aciklamasi).
static constexpr float AXIS_CHANGE_THRESHOLD = 0.02f;

JoystickThread::JoystickThread(const QString &device, QObject *parent)
    : QThread(parent), devicePath(device), running(true) {
    lastAxisValues.fill(-2.0f);
}

void JoystickThread::run() {
    int fd = -1;
    while (running) {
        if (fd < 0) {
            fd = open(devicePath.toStdString().c_str(), O_RDONLY | O_NONBLOCK);
            if (fd < 0) {
                emit status_signal("Kol Bağlı Değil");
                QThread::sleep(2);
                continue;
            } else {
                emit status_signal("Kol Bağlandı!");
            }
        }

        struct js_event e;
        while (read(fd, &e, sizeof(e)) > 0) {
            // Cihaz acilirken cekirdek her buton/eksen icin o anki durumu
            // JS_EVENT_INIT bayrakli "sahte" olaylarla bildirir - kullanici
            // hicbir seye basmamis olsa bile. Bunlari gercek basisymis gibi
            // isleyip ARM/lamba/torpido gibi aksiyonlari tetiklemek tehlikeli
            // oldugundan (bkz. beklenmedik ARM olayi), INIT olaylarini yok say.
            if (e.type & JS_EVENT_INIT) continue;

            if (e.type == JS_EVENT_BUTTON) {
                emit button_signal(e.number, e.value);
            } else if (e.type == JS_EVENT_AXIS) {
                float value = e.value / 32767.0f;
                if (e.number < lastAxisValues.size() &&
                    std::fabs(value - lastAxisValues[e.number]) < AXIS_CHANGE_THRESHOLD) {
                    continue;
                }
                if (e.number < lastAxisValues.size()) lastAxisValues[e.number] = value;
                emit axis_signal(e.number, value);
            }
        }
        QThread::msleep(10);
    }

    if (fd >= 0) close(fd);
}

void JoystickThread::stop() {
    running = false;
    wait();
}
