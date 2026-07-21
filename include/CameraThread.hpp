#ifndef CAMERATHREAD_H
#define CAMERATHREAD_H

#include <QThread>
#include <QImage>
#include <QString>

// MJPEG HTTP stream alici (MiniROV kamerasi)
class CameraThread : public QThread {
    Q_OBJECT
public:
    explicit CameraThread(const QString &ip = "192.168.88.2",
                          QObject *parent = nullptr);
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

#endif // CAMERATHREAD_H
