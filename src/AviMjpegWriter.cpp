#include "AviMjpegWriter.hpp"

// AVI (RIFF) yapisi - tum cok baytli degerler kucuk-endian:
//   RIFF <boyut> 'AVI '
//     LIST <boyut> 'hdrl'
//       'avih' 56  (MainAVIHeader)
//       LIST <boyut> 'strl'
//         'strh' 56 (AVISTREAMHEADER, 'vids'/'MJPG')
//         'strf' 40 (BITMAPINFOHEADER, biCompression='MJPG')
//     LIST <boyut> 'movi'
//       '00dc' <boyut> <jpeg>   (cift bayta yastiklanir)
//       ...
//     'idx1' <boyut> <kare basina 16 bayt indeks>
// RIFF/movi boyutlari ile kare sayilari kayit bitmeden bilinemedigi icin
// open() sirasinda 0 yazilir, close() icinde seek edilip yamalanir.

namespace {
constexpr quint32 AVIF_HASINDEX = 0x00000010;
constexpr quint32 AVIIF_KEYFRAME = 0x00000010;
}

AviMjpegWriter::~AviMjpegWriter() {
    if (isOpen()) close();
}

void AviMjpegWriter::writeU16(quint16 v) {
    char b[2] = { char(v & 0xFF), char((v >> 8) & 0xFF) };
    file.write(b, 2);
}

void AviMjpegWriter::writeU32(quint32 v) {
    char b[4] = { char(v & 0xFF), char((v >> 8) & 0xFF),
                  char((v >> 16) & 0xFF), char((v >> 24) & 0xFF) };
    file.write(b, 4);
}

void AviMjpegWriter::writeFourcc(const char *cc) {
    file.write(cc, 4);
}

bool AviMjpegWriter::open(const QString &path, int width, int height, int frameRate) {
    if (isOpen()) return false;
    file.setFileName(path);
    if (!file.open(QIODevice::WriteOnly)) return false;

    w = width;
    h = height;
    fps = frameRate > 0 ? frameRate : 25;
    frameOffsets.clear();
    frameSizes.clear();

    writeFourcc("RIFF");
    riffSizePos = file.pos();
    writeU32(0); // yamalanacak
    writeFourcc("AVI ");

    // ---- hdrl ----
    writeFourcc("LIST");
    writeU32(4 + 8 + 56 + 8 + 4 + 8 + 56 + 8 + 40); // hdrl icerigi sabit boyutlu
    writeFourcc("hdrl");

    writeFourcc("avih");
    writeU32(56);
    writeU32(1000000u / static_cast<quint32>(fps)); // dwMicroSecPerFrame
    writeU32(0);                 // dwMaxBytesPerSec
    writeU32(0);                 // dwPaddingGranularity
    writeU32(AVIF_HASINDEX);     // dwFlags
    totalFramesPos = file.pos();
    writeU32(0);                 // dwTotalFrames - yamalanacak
    writeU32(0);                 // dwInitialFrames
    writeU32(1);                 // dwStreams
    writeU32(0);                 // dwSuggestedBufferSize
    writeU32(static_cast<quint32>(w));
    writeU32(static_cast<quint32>(h));
    for (int i = 0; i < 4; ++i) writeU32(0); // dwReserved

    writeFourcc("LIST");
    writeU32(4 + 8 + 56 + 8 + 40);
    writeFourcc("strl");

    writeFourcc("strh");
    writeU32(56);
    writeFourcc("vids");
    writeFourcc("MJPG");
    writeU32(0);                 // dwFlags
    writeU16(0);                 // wPriority
    writeU16(0);                 // wLanguage
    writeU32(0);                 // dwInitialFrames
    writeU32(1);                 // dwScale
    writeU32(static_cast<quint32>(fps)); // dwRate -> fps = dwRate/dwScale
    writeU32(0);                 // dwStart
    streamLengthPos = file.pos();
    writeU32(0);                 // dwLength (kare sayisi) - yamalanacak
    writeU32(0);                 // dwSuggestedBufferSize
    writeU32(0xFFFFFFFFu);       // dwQuality (-1 = varsayilan)
    writeU32(0);                 // dwSampleSize
    writeU16(0); writeU16(0);    // rcFrame sol, ust
    writeU16(static_cast<quint16>(w));
    writeU16(static_cast<quint16>(h));

    writeFourcc("strf");
    writeU32(40);
    writeU32(40);                // biSize
    writeU32(static_cast<quint32>(w));
    writeU32(static_cast<quint32>(h));
    writeU16(1);                 // biPlanes
    writeU16(24);                // biBitCount
    writeFourcc("MJPG");         // biCompression
    writeU32(static_cast<quint32>(w * h * 3)); // biSizeImage (yaklasik, sorun degil)
    writeU32(0); writeU32(0);    // biXPelsPerMeter, biYPelsPerMeter
    writeU32(0); writeU32(0);    // biClrUsed, biClrImportant

    // ---- movi ----
    writeFourcc("LIST");
    moviSizePos = file.pos();
    writeU32(0); // yamalanacak
    writeFourcc("movi");
    moviDataStart = file.pos();
    return true;
}

void AviMjpegWriter::addFrame(const QByteArray &jpeg) {
    if (!isOpen() || jpeg.isEmpty()) return;

    // idx1'deki dwOffset, 'movi' fourcc'sine gore olculur ve chunk'in
    // '00dc' baslangicini gosterir (fourcc'nin kendisi dahil, 4 bayt geriden).
    frameOffsets.append(static_cast<quint32>(file.pos() - moviDataStart + 4));
    frameSizes.append(static_cast<quint32>(jpeg.size()));

    writeFourcc("00dc");
    writeU32(static_cast<quint32>(jpeg.size()));
    file.write(jpeg);
    if (jpeg.size() % 2 != 0) file.write("\0", 1); // RIFF chunk'lari cift bayta hizalanir
}

void AviMjpegWriter::close() {
    if (!isOpen()) return;

    const qint64 moviEnd = file.pos();
    const quint32 frames = static_cast<quint32>(frameOffsets.size());

    // ---- idx1 ----
    writeFourcc("idx1");
    writeU32(frames * 16);
    for (quint32 i = 0; i < frames; ++i) {
        writeFourcc("00dc");
        writeU32(AVIIF_KEYFRAME); // MJPEG'te her kare bagimsizdir (keyframe)
        writeU32(frameOffsets[i]);
        writeU32(frameSizes[i]);
    }
    const qint64 fileEnd = file.pos();

    // ---- boyut/sayi yamalari ----
    file.seek(riffSizePos);
    writeU32(static_cast<quint32>(fileEnd - riffSizePos - 4));
    file.seek(totalFramesPos);
    writeU32(frames);
    file.seek(streamLengthPos);
    writeU32(frames);
    file.seek(moviSizePos);
    writeU32(static_cast<quint32>(moviEnd - moviSizePos - 4));

    file.close();
    frameOffsets.clear();
    frameSizes.clear();
}
