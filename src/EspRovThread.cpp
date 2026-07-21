#include "EspRovThread.hpp"

#include <QSerialPort>
#include <QStringList>

EspRovThread::EspRovThread(const QString &port, int baudrate, QObject *parent)
    : QThread(parent), portName(port), baudRate(baudrate), running(true), serial(nullptr) {}

void EspRovThread::run() {
    serial = new QSerialPort();
    serial->setPortName(portName);
    serial->setBaudRate(baudRate);

    if (!serial->open(QIODevice::ReadWrite)) {
        emit status_signal(QString("Hata: %1 açılamadı").arg(portName));
        delete serial;
        serial = nullptr;
        return;
    }

    emit status_signal(QString("ESP32 seri port bağlantısı açıldı (%1)").arg(portName));

    QByteArray buffer;
    while (running) {
        write_pending_lines();

        if (serial->waitForReadyRead(20)) {
            buffer += serial->readAll();
            int nl;
            while ((nl = buffer.indexOf('\n')) != -1) {
                QString line = QString::fromUtf8(buffer.left(nl)).trimmed();
                buffer.remove(0, nl + 1);
                if (line.isEmpty()) continue;

                if (line == "ARMED") {
                    emit armed_signal(true);
                } else if (line == "DISARMED") {
                    emit armed_signal(false);
                } else if (line.startsWith("ATT:")) {
                    QStringList parts = line.mid(4).split(',');
                    if (parts.size() == 2) {
                        emit attitude_signal(parts[0].toFloat(), parts[1].toFloat());
                    }
                } else if (line.startsWith("DEPTH:")) {
                    // "DEPTH:1.23,0.045" periyodik veri satiri; "DEPTH: yuzey
                    // kalibrasyonu..." gibi insan-okunur mesajlar bu formatta
                    // olmadigi icin asagi, normal durum mesaji gibi loglanmasina dusulur.
                    QStringList parts = line.mid(6).split(',');
                    bool okDepth = false, okSpeed = false;
                    float meters = parts.size() == 2 ? parts[0].toFloat(&okDepth) : 0.0f;
                    float speed  = parts.size() == 2 ? parts[1].toFloat(&okSpeed) : 0.0f;
                    if (okDepth && okSpeed) {
                        emit depth_signal(meters, speed);
                    } else {
                        emit status_signal(line);
                    }
                } else {
                    emit status_signal(line);
                }
            }
        }
    }

    write_pending_lines();
    serial->close();
    delete serial;
    serial = nullptr;
}

void EspRovThread::write_pending_lines() {
    QMutexLocker locker(&writeMutex);
    while (!pendingWrites.isEmpty()) {
        serial->write(pendingWrites.dequeue());
    }
}

void EspRovThread::send_line(const QString &line) {
    QMutexLocker locker(&writeMutex);
    pendingWrites.enqueue((line + "\n").toUtf8());
}

void EspRovThread::arm() {
    send_line("ARM");
}

void EspRovThread::disarm() {
    send_line("DISARM");
}

void EspRovThread::set_motors(const std::array<int, 8> &pulses_us) {
    QStringList parts;
    for (int p : pulses_us) parts << QString::number(p);
    send_line("M:" + parts.join(','));
}

void EspRovThread::stop() {
    disarm();
    running = false;
    wait();
}
