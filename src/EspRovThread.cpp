#include "EspRovThread.hpp"

#include <QSerialPort>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <QStringList>
#include <QMutexLocker>

EspRovThread::EspRovThread(const QString &port, int baudrate, QObject *parent)
    : EspRovThread(false, port, baudrate, parent) {}

EspRovThread *EspRovThread::createTcp(const QString &host, quint16 tcpPort, QObject *parent) {
    return new EspRovThread(true, host, tcpPort, parent);
}

EspRovThread::EspRovThread(bool tcpMode, const QString &hostOrPort_, int portOrBaud_, QObject *parent)
    : QThread(parent), useTcp(tcpMode), hostOrPort(hostOrPort_), portOrBaud(portOrBaud_),
      running(true), device(nullptr) {}

// Baglanti kurulamazsa ya da KOPARSA thread cikmaz: 2 sn arayla otomatik
// yeniden baglanmayi dener (kamera thread'inin yaptigi gibi). Boylece gecici
// bir ag kopmasi/ESP32 resetinde operatörün elle "Bağlan"a basmasi gerekmez.
void EspRovThread::run() {
    // stop() istegine hizli tepki verebilmek icin tek uzun sleep yerine
    // kucuk dilimlerle bekler.
    auto retryDelay = [this]() {
        for (int i = 0; i < 20 && running; ++i) QThread::msleep(100);
    };

    while (running) {
        QSerialPort serialDev;
        QTcpSocket tcpDev;
        QIODevice *dev = nullptr;

        if (useTcp) {
            tcpDev.connectToHost(hostOrPort, static_cast<quint16>(portOrBaud));
            if (!tcpDev.waitForConnected(3000)) {
                emit status_signal(QString("%1:%2 bağlantısı kurulamadı (%3) - yeniden denenecek")
                                        .arg(hostOrPort).arg(portOrBaud).arg(tcpDev.errorString()));
                retryDelay();
                continue;
            }
            dev = &tcpDev;
            emit status_signal(QString("Köprü bağlantısı açıldı (%1:%2)").arg(hostOrPort).arg(portOrBaud));
        } else {
            serialDev.setPortName(hostOrPort);
            serialDev.setBaudRate(portOrBaud);
            if (!serialDev.open(QIODevice::ReadWrite)) {
                emit status_signal(QString("%1 açılamadı - yeniden denenecek").arg(hostOrPort));
                retryDelay();
                continue;
            }
            dev = &serialDev;
            emit status_signal(QString("ESP32 seri port bağlantısı açıldı (%1)").arg(hostOrPort));
        }

        // Onceki (kopmus) oturumdan kuyrukta kalan komutlar yeni baglantiya
        // gitmesin: bunlar eski bir surus anina ait motor darbeleri olabilir
        // ve taze baglantida beklenmedik hareket yaratir.
        {
            QMutexLocker lock(&writeMutex);
            pendingWrites.clear();
        }

        device = dev;
        QByteArray buffer;
        bool linkUp = true;

        while (running && linkUp) {
            write_pending_lines();

            if (dev->waitForReadyRead(20)) {
                buffer += dev->readAll();
                int nl;
                while ((nl = buffer.indexOf('\n')) != -1) {
                    QString line = QString::fromUtf8(buffer.left(nl)).trimmed();
                    buffer.remove(0, nl + 1);
                    if (!line.isEmpty()) process_line(line);
                }
            }

            if (useTcp && tcpDev.state() != QAbstractSocket::ConnectedState) {
                emit status_signal("Köprü bağlantısı koptu - yeniden bağlanılıyor...");
                linkUp = false;
            } else if (!useTcp && serialDev.error() == QSerialPort::ResourceError) {
                // USB kablosu cekildi/cihaz kayboldu
                emit status_signal("Seri port koptu - yeniden bağlanılıyor...");
                linkUp = false;
            }
        }

        device = nullptr;
        if (useTcp) tcpDev.close();
        else        serialDev.close();

        if (running) retryDelay(); // kopma sonrasi kisa bekleyip yeniden dene
    }
}

void EspRovThread::process_line(const QString &line) {
    if (line == "ARMED") {
        emit armed_signal(true);
    } else if (line == "DISARMED") {
        emit armed_signal(false);
    } else if (line.startsWith("ATT:")) {
        QStringList parts = line.mid(4).split(',');
        if (parts.size() == 3) {
            emit attitude_signal(parts[0].toFloat(), parts[1].toFloat(), parts[2].toFloat());
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
    } else if (line.startsWith("NEM:")) {
        QStringList parts = line.mid(4).split(',');
        bool okHum = false, okTemp = false;
        float hum  = parts.size() == 2 ? parts[0].toFloat(&okHum) : 0.0f;
        float temp = parts.size() == 2 ? parts[1].toFloat(&okTemp) : 0.0f;
        if (okHum && okTemp) {
            emit nem_signal(hum, temp);
        } else {
            emit status_signal(line);
        }
    } else if (line.startsWith("BAT:")) {
        QStringList parts = line.mid(4).split(',');
        bool okV = false, okP = false;
        float volts = parts.size() == 2 ? parts[0].toFloat(&okV) : 0.0f;
        int pct     = parts.size() == 2 ? parts[1].toInt(&okP) : 0;
        if (okV && okP) {
            emit battery_signal(volts, pct);
        } else {
            emit status_signal(line);
        }
    } else if (line.startsWith("TORPEDO:")) {
        bool ok = false;
        int remaining = line.mid(8).toInt(&ok);
        if (ok) {
            emit torpedo_signal(remaining);
        } else {
            emit status_signal(line);
        }
    } else {
        emit status_signal(line);
    }
}

void EspRovThread::write_pending_lines() {
    QMutexLocker locker(&writeMutex);
    while (!pendingWrites.isEmpty()) {
        QByteArray data = pendingWrites.dequeue();
        if (device) device->write(data);
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

void EspRovThread::torpedo() {
    send_line("TORPEDO");
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
