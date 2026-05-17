#include <QApplication>
#include <QFile>
#include "yazilim.h"
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

    PixhawkGUI window;
    window.showFullScreen();
    return app.exec();
}