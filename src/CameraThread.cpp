#include "CameraThread.hpp"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QTimer>
#include <QDateTime>

CameraThread::CameraThread(const QString &ip, QObject *parent)
    : QThread(parent), ipAddr(ip), running(true) {}

void CameraThread::run() {
    emit status_signal("Kamera Bağlanıyor...");
    QEventLoop loop;
    QNetworkAccessManager manager;
    QNetworkReply *reply = nullptr;

    QByteArray buffer;
    int frame_count = 0;
    int bytes_count = 0;
    double start_time = QDateTime::currentMSecsSinceEpoch() / 1000.0;

    auto connectStream = [&]() {
        buffer.clear();
        QNetworkRequest request(QUrl(QString("http://%1/stream").arg(ipAddr)));
        request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
        reply = manager.get(request);

        QObject::connect(reply, &QNetworkReply::readyRead, [&]() {
            emit status_signal("Kamera Bağlandı!");
            if (!reply) return;

            QByteArray chunk = reply->readAll();
            buffer.append(chunk);
            bytes_count += chunk.size();

            QByteArray soi = QByteArray::fromHex("ffd8");
            QByteArray eoi = QByteArray::fromHex("ffd9");

            int start = buffer.indexOf(soi);
            int end = buffer.indexOf(eoi);

            if (start != -1 && end != -1) {
                if (end > start) {
                    QByteArray jpg = buffer.mid(start, end - start + 2);
                    buffer.remove(0, end + 2);

                    QImage img;
                    if (img.loadFromData(jpg, "JPG")) {
                        emit image_signal(img.copy());
                        frame_count++;

                        double current_time = QDateTime::currentMSecsSinceEpoch() / 1000.0;
                        double elapsed = current_time - start_time;
                        if (elapsed >= 1.0) {
                            int fps = static_cast<int>(frame_count / elapsed);
                            float kbps = (bytes_count / 1024.0f) / elapsed;
                            emit stats_signal(fps, kbps, img.width(), img.height());
                            frame_count = 0;
                            bytes_count = 0;
                            start_time = current_time;
                        }
                    }
                } else {
                    buffer.remove(0, start);
                }
            } else if (buffer.size() > 2 * 1024 * 1024) {
                buffer.clear();
            }
        });

        QObject::connect(reply, &QNetworkReply::finished, [&]() {
            emit status_signal("Kamera Koptu, Yeniden Bekleniyor...");
            if (reply) {
                reply->deleteLater();
                reply = nullptr;
            }
        });
    };

    connectStream();

    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        if (!running) {
            if (reply) reply->abort();
            loop.quit();
        } else if (!reply || reply->isFinished()) {
            connectStream();
        }
    });
    timer.start(500);

    loop.exec();
}

void CameraThread::stop() {
    running = false;
    wait();
}
