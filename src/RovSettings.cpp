#include "RovSettings.hpp"

#include <QSettings>

// Organizasyon/uygulama adi main.cpp'deki setOrganizationName/setApplicationName
// ile ayni olmali ki QSettings ayni dosyaya (~/.config/ROV Team/...) yazsin.
namespace {
QSettings makeStore() {
    return QSettings("ROV Team", "Su Altı ROV Kontrol");
}
}

RovSettings RovSettings::load() {
    QSettings s = makeStore();
    RovSettings def; // varsayilanlar struct tanimindan gelir
    RovSettings r;
    r.anaHost       = s.value("ana/host", def.anaHost).toString();
    r.anaBridgePort = static_cast<quint16>(s.value("ana/bridge_port", def.anaBridgePort).toUInt());
    r.anaCamAddr    = s.value("ana/cam_addr", def.anaCamAddr).toString();
    r.miniCamIp     = s.value("mini/cam_ip", def.miniCamIp).toString();
    r.stabGain      = s.value("gains/stabilize", def.stabGain).toFloat();
    r.depthHoldGain = s.value("gains/depth_hold", def.depthHoldGain).toFloat();
    int legacyNeutral = s.value("control/neutral_us", def.motorNeutralUs[0]).toInt();
    for (int i = 0; i < 8; ++i) {
        r.motorNeutralUs[i] = s.value(QString("control/neutral_us_%1").arg(i + 1), legacyNeutral).toInt();
    }
    r.minUs         = s.value("control/min_us", def.minUs).toInt();
    r.maxUs         = s.value("control/max_us", def.maxUs).toInt();
    int legacyTorpedoMin = s.value("torpedo/min_us", def.torpedoMinUs[0]).toInt();
    int legacyTorpedoNeutral = s.value("torpedo/neutral_us", def.torpedoNeutralUs[0]).toInt();
    int legacyTorpedoMax = s.value("torpedo/max_us", def.torpedoMaxUs[0]).toInt();
    for (int i = 0; i < 3; ++i) {
        r.torpedoMinUs[i]     = s.value(QString("torpedo/min_us_%1").arg(i + 1), legacyTorpedoMin).toInt();
        r.torpedoNeutralUs[i] = s.value(QString("torpedo/neutral_us_%1").arg(i + 1), legacyTorpedoNeutral).toInt();
        r.torpedoMaxUs[i]     = s.value(QString("torpedo/max_us_%1").arg(i + 1), legacyTorpedoMax).toInt();
    }
    return r;
}

void RovSettings::save() const {
    QSettings s = makeStore();
    s.setValue("ana/host", anaHost);
    s.setValue("ana/bridge_port", anaBridgePort);
    s.setValue("ana/cam_addr", anaCamAddr);
    s.setValue("mini/cam_ip", miniCamIp);
    s.setValue("gains/stabilize", stabGain);
    s.setValue("gains/depth_hold", depthHoldGain);
    for (int i = 0; i < 8; ++i) {
        s.setValue(QString("control/neutral_us_%1").arg(i + 1), motorNeutralUs[i]);
    }
    s.setValue("control/min_us", minUs);
    s.setValue("control/max_us", maxUs);
    for (int i = 0; i < 3; ++i) {
        s.setValue(QString("torpedo/min_us_%1").arg(i + 1), torpedoMinUs[i]);
        s.setValue(QString("torpedo/neutral_us_%1").arg(i + 1), torpedoNeutralUs[i]);
        s.setValue(QString("torpedo/max_us_%1").arg(i + 1), torpedoMaxUs[i]);
    }
}
