#include "algoritma.h"
#include <QSerialPort>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QWebSocket>
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <QDateTime>
#include <opencv2/opencv.hpp>
#include <cmath>
#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>

#include "mavlink/common/mavlink.h"

// ==================== MAVLinkThread ====================
MAVLinkThread::MAVLinkThread(const QString &port, int baudrate, QObject *parent)
    : QThread(parent), portName(port), baudRate(baudrate), running(true),
      target_system(1), target_component(1), serial(nullptr) {}

void MAVLinkThread::run() {
    serial = new QSerialPort();
    serial->setPortName(portName);
    serial->setBaudRate(baudRate);

    if (!serial->open(QIODevice::ReadWrite)) {
        emit status_signal(QString("Hata: %1 açılamadı").arg(portName));
        delete serial;
        serial = nullptr;
        return;
    }

    emit status_signal(QString("Bağlantı Başarılı! Sistem ID: %1").arg(target_system));

    // HEARTBEAT iste (data stream request)
    mavlink_message_t req_msg;
    mavlink_msg_request_data_stream_pack(255, 0, &req_msg,
        target_system, target_component,
        MAV_DATA_STREAM_ALL, 10, 1);
    uint8_t reqbuf[MAVLINK_MAX_PACKET_LEN];
    uint16_t reqlen = mavlink_msg_to_send_buffer(reqbuf, &req_msg);
    serial->write(reinterpret_cast<const char*>(reqbuf), reqlen);

    mavlink_message_t msg;
    mavlink_status_t status;

    while (running) {
        if (serial->waitForReadyRead(100)) {
            QByteArray data = serial->readAll();
            for (int i = 0; i < data.size(); ++i) {
                if (mavlink_parse_char(MAVLINK_COMM_0,
                                       static_cast<uint8_t>(data.at(i)),
                                       &msg, &status)) {
                    if (msg.msgid == MAVLINK_MSG_ID_ATTITUDE) {
                        mavlink_attitude_t att;
                        mavlink_msg_attitude_decode(&msg, &att);
                        emit attitude_signal(att.roll  * 180.0f / M_PI,
                                             att.pitch * 180.0f / M_PI,
                                             att.yaw   * 180.0f / M_PI);
                    } else if (msg.msgid == MAVLINK_MSG_ID_VFR_HUD) {
                        mavlink_vfr_hud_t vfr;
                        mavlink_msg_vfr_hud_decode(&msg, &vfr);
                        emit vfr_hud_signal(vfr.alt, vfr.heading, vfr.groundspeed);
                    } else if (msg.msgid == MAVLINK_MSG_ID_SYS_STATUS) {
                        mavlink_sys_status_t sys;
                        mavlink_msg_sys_status_decode(&msg, &sys);
                        emit battery_signal(sys.voltage_battery / 1000.0f,
                                            sys.battery_remaining);
                    }
                }
            }
        }
    }

    serial->close();
    delete serial;
    serial = nullptr;
}

void MAVLinkThread::arm_vehicle() {
    if (!serial || !serial->isOpen()) return;
    mavlink_message_t msg;
    // MAV_CMD_COMPONENT_ARM_DISARM, param1=1 → ARM
    mavlink_msg_command_long_pack(255, 0, &msg,
        target_system, target_component,
        MAV_CMD_COMPONENT_ARM_DISARM, 0,
        1, 0, 0, 0, 0, 0, 0);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    serial->write(reinterpret_cast<const char*>(buf), len);
    emit status_signal("ARM komutu gönderildi");
}

void MAVLinkThread::set_servo(int servo_no, int pwm) {
    if (!serial || !serial->isOpen()) return;
    mavlink_message_t msg;
    mavlink_msg_command_long_pack(255, 0, &msg,
        target_system, target_component,
        MAV_CMD_DO_SET_SERVO, 0,
        servo_no, pwm, 0, 0, 0, 0, 0);
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
    serial->write(reinterpret_cast<const char*>(buf), len);
}

void MAVLinkThread::stop() {
    running = false;
    wait();
}

// ==================== JoystickThread ====================
JoystickThread::JoystickThread(const QString &device, QObject *parent)
    : QThread(parent), devicePath(device), running(true) {}

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
            e.type &= ~JS_EVENT_INIT;
            if (e.type == JS_EVENT_BUTTON) {
                emit button_signal(e.number, e.value);
            } else if (e.type == JS_EVENT_AXIS) {
                emit axis_signal(e.number, e.value / 32767.0f);
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

// ==================== CameraThread (MJPEG) ====================
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

// ==================== AnaRovThread (WebSocket) ====================
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