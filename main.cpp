#include <QApplication>
#include "yazilim.h"
#include <cstdlib>

int main(int argc, char *argv[]) {
    // Ağ ayarlarını yapılandır (Python kodundaki gibi)
    system("sudo ip addr flush dev enp46s0");
    system("sudo ip addr add 192.168.88.1/24 dev enp46s0");
    system("sudo ip link set enp46s0 up");
    // Ping komutunu arayüzde çalıştıracağımız için terminal komutunu kaldırdık

    QApplication app(argc, argv);
    PixhawkGUI window;
    window.showFullScreen();
    return app.exec();
}
