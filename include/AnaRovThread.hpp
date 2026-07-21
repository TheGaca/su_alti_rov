#ifndef ANAROVTHREAD_H
#define ANAROVTHREAD_H

#include <QThread>
#include <QImage>
#include <QString>

// WebSocket binary JPEG alici (AnaROV kamerasi)
class AnaRovThread : public QThread {
    Q_OBJECT
public:
    explicit AnaRovThread(const QString &ip = "192.168.1.116",
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

#endif // ANAROVTHREAD_H
