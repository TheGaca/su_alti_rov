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
}
