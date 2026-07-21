#include "AnaRovThread.hpp"

#include <QWebSocket>
#include <QEventLoop>
#include <QTimer>
#include <QDateTime>
#include <opencv2/opencv.hpp>

AnaRovThread::AnaRovThread(const QString &ip, QObject *parent)
    : QThread(parent), ipAddr(ip), running(true) {}

void AnaRovThread::run() {
    emit status_signal("AnaRov Bağlanıyor...");
    QEventLoop loop;
    QWebSocket webSocket;

    int frame_count = 0;
    int bytes_count = 0;
    double start_time = QDateTime::currentMSecsSinceEpoch() / 1000.0;

    auto connectWs = [&]() {
        webSocket.open(QUrl(QString("ws://%1:85").arg(ipAddr)));
    };

    QObject::connect(&webSocket, &QWebSocket::connected, [&]() {
        emit status_signal("AnaRov Bağlandı!");
        start_time = QDateTime::currentMSecsSinceEpoch() / 1000.0;
        frame_count = 0;
        bytes_count = 0;
    });

    QObject::connect(&webSocket, &QWebSocket::disconnected, [&]() {
        emit status_signal("AnaRov Koptu, Yeniden Bağlanıyor...");
    });

    QObject::connect(&webSocket,
        QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
        [&](QAbstractSocket::SocketError) {
            emit status_signal(QString("AnaRov Hatası: %1").arg(webSocket.errorString()));
        });

    QObject::connect(&webSocket, &QWebSocket::binaryMessageReceived,
        [&](const QByteArray &message) {
            std::vector<uchar> data(message.begin(), message.end());
            cv::Mat frame = cv::imdecode(data, cv::IMREAD_COLOR);
            if (!frame.empty()) {
                cv::flip(frame, frame, -1);
                cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
                QImage img(frame.data, frame.cols, frame.rows,
                           frame.step, QImage::Format_RGB888);
                emit image_signal(img.copy());

                frame_count++;
                bytes_count += message.size();

                double current_time = QDateTime::currentMSecsSinceEpoch() / 1000.0;
                double elapsed = current_time - start_time;
                if (elapsed >= 1.0) {
                    int fps = static_cast<int>(frame_count / elapsed);
                    float kbps = (bytes_count / 1024.0f) / elapsed;
                    emit stats_signal(fps, kbps, frame.cols, frame.rows);
                    frame_count = 0;
                    bytes_count = 0;
                    start_time = current_time;
                }
            }
        });

    connectWs();

    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        if (!running) {
            webSocket.close();
            loop.quit();
        } else if (webSocket.state() == QAbstractSocket::UnconnectedState) {
            connectWs();
        }
    });
    timer.start(1000);

    loop.exec();
}

void AnaRovThread::stop() {
    running = false;
    wait();
}
