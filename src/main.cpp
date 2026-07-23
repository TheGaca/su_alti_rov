#include <QApplication>
#include <QFile>
#include <QScreen>
#include "RovGUI.hpp"
#include <cstdlib>

int main(int argc, char *argv[]) {
    // Ağ ayarlarını yapılandır (sudo gerektirir; çalışmazsa atla)
    // Eğer ağ arayüz adı farklıysa burayı düzenle
    int ret;
    ret = system("sudo -n ip addr flush dev enp46s0 2>/dev/null");
    ret = system("sudo -n ip addr add 192.168.88.1/24 dev enp46s0 2>/dev/null");
    ret = system("sudo -n ip link set enp46s0 up 2>/dev/null");
    (void)ret; // sessiz tut

    QApplication app(argc, argv);
    app.setApplicationName("Su Altı ROV Kontrol");
    app.setOrganizationName("ROV Team");

    RovGUI window;

    // showFullScreen() bazi pencere yoneticilerinde/masaustu ortamlarinda
    // (WM'nin _NET_WM_STATE_FULLSCREEN'i tam desteklememesi/gormezden
    // gelmesi durumunda) baslik cubugunu, pencere dugmelerini ve dock/paneli
    // gizlemeden sadece normal bir pencere gibi kalabiliyor. Cerceveyi
    // WM'den bagimsiz olarak Qt'nin kendisine ciz dirmek (Frameless) ve
    // gercek ekran piksellerini (dock/panel dahil) kapsayacak sekilde
    // geometriyi elle ayarlamak bunu WM'den bagimsiz sekilde garantiler.
    // NOT: Bu flag show()'dan ONCE ayarlanmali; zaten gorunur bir pencerede
    // degistirmek native pencerenin yeniden olusturulmasini gerektirir.
    window.setWindowFlags(window.windowFlags() | Qt::FramelessWindowHint);
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        window.setGeometry(screen->geometry());
    }
    window.showFullScreen();
    return app.exec();
}
