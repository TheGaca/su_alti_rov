#ifndef AVIMJPEGWRITER_H
#define AVIMJPEGWRITER_H

#include <QFile>
#include <QString>
#include <QByteArray>
#include <QVector>

// Kameradan gelen MJPEG karelerini oldugu gibi (yeniden kodlamadan) standart
// bir AVI kabina yazar - VLC/mpv/ffmpeg dahil her oynaticinin actigi eski ama
// saglam bir format. Harici kutuphane (ffmpeg/GStreamer) gerektirmedigi icin
// tercih edildi; kareler zaten JPEG geldiginden CPU maliyeti sifira yakindir.
//
// Kullanim: open() -> her karede addFrame(jpegBytes) -> close().
// close() cagirilmadan uygulama kapanirsa dosya sonundaki index yazilmamis
// olur; cogu oynatici yine de acar ama close() cagirmak esastir.
class AviMjpegWriter {
public:
    ~AviMjpegWriter(); // acik kaldiysa close() cagirir

    // fps: oynatma hizi (kayit sirasindaki gercek kare hizini gecin; kamera
    // hizi degiskense ortalama bir deger kucuk hiz sapmalarina yol acar,
    // kayit icerigi etkilenmez).
    bool open(const QString &path, int width, int height, int fps);
    bool isOpen() const { return file.isOpen(); }

    // jpeg: tam bir JPEG karesi (SOI..EOI). Boyutu ilk kareden farkli
    // kareler de yazilir - MJPG cozuculer kare basina boyut okur.
    void addFrame(const QByteArray &jpeg);

    // RIFF/movi boyutlarini, kare sayilarini ve idx1 indeksini tamamlayip
    // dosyayi kapatir.
    void close();

    int frameCount() const { return frameOffsets.size(); }

private:
    void writeU16(quint16 v);
    void writeU32(quint32 v);
    void writeFourcc(const char *cc);

    QFile file;
    int w = 0, h = 0, fps = 25;

    // close() sirasinda geri donup yamalanacak alanlarin dosya konumlari
    qint64 riffSizePos = 0;
    qint64 totalFramesPos = 0;
    qint64 streamLengthPos = 0;
    qint64 moviSizePos = 0;
    qint64 moviDataStart = 0; // 'movi' fourcc'sinin hemen sonrasi

    // idx1 icin: her karenin 'movi' icindeki goreli konumu ve boyutu
    QVector<quint32> frameOffsets;
    QVector<quint32> frameSizes;
};

#endif // AVIMJPEGWRITER_H
