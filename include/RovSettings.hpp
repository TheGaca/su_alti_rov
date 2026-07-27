#ifndef ROVSETTINGS_H
#define ROVSETTINGS_H

#include <QString>

// Kullanicinin degistirebildigi, QSettings ile kalici saklanan calisma
// ayarlari. Eskiden koda gomulu sabitlerdi (IP'ler RovGUI.cpp'de,
// STAB_GAIN/DEPTH_HOLD_GAIN constexpr) - havuz basinda her degisiklik
// yeniden derleme gerektiriyordu. Artik Ayarlar penceresinden (bkz.
// SettingsDialog) calisirken degistirilebilir.
//
// Kazanclar canli uygulanir; IP/port degisiklikleri kamera icin aninda
// (thread yeniden baslatilir), ESP koprusu icin bir sonraki baglanmada
// gecerli olur.
struct RovSettings {
    // Ana ROV: kamera ESP'sindeki TCP koprusu (komut/telemetri) ve MJPEG akisi
    QString anaHost = "192.168.2.220";
    quint16 anaBridgePort = 8888;
    QString anaCamAddr = "192.168.2.220:81"; // "ip" veya "ip:port" (http://<addr>/stream)

    // Mini ROV kamerasi (MJPEG @ http://<ip>/stream)
    QString miniCamIp = "192.168.88.2";

    // Sabitleme (stabilize): roll/pitch (derece) / 90 * stabGain dikey itki
    // duzeltmesi (-1..1 sinirli). 4.0 ile ~22.5 derecede doygunluk.
    float stabGain = 4.0f;

    // Derinlik sabitleme: hedef-mevcut farki (metre) * depthHoldGain dikey
    // itki (-1..1 sinirli). 2.0 ile ~0.5m sapmada doygunluk.
    float depthHoldGain = 2.0f;

    static RovSettings load();
    void save() const;
};

#endif // ROVSETTINGS_H
