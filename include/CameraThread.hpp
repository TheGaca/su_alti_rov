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

    // Ayni karenin HAM JPEG baytlari (SOI..EOI) - video kaydi bunlari
    // yeniden kodlamadan dogrudan AVI'ye yazar (bkz. AviMjpegWriter);
    // image_signal ise ekranda gosterim/HUD icin cozulmus halidir.
    void jpeg_signal(const QByteArray &jpeg);

protected:
    void run() override;

private:
    QString ipAddr;
    bool running;
};

#endif // CAMERATHREAD_H
