#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include "RovSettings.hpp"

class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QLabel;

// Ayarlar penceresi: IP/port ve kontrol kazanclarini calisirken degistirmeyi
// saglar (bkz. RovSettings). Kaydet'e basilinca degerler dogrulanip QSettings'e
// yazilir; canli uygulama isini cagiran taraf (RovGUI::open_settings) yapar.
class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(const RovSettings &current, QWidget *parent = nullptr);

    // exec() == Accepted sonrasi kullanicinin girdigi degerler.
    RovSettings values() const;

private:
    QLineEdit *edit_ana_host;
    QSpinBox *spin_bridge_port;
    QLineEdit *edit_ana_cam;
    QLineEdit *edit_mini_cam;
    QDoubleSpinBox *spin_stab_gain;
    QDoubleSpinBox *spin_depth_gain;
};

#endif // SETTINGSDIALOG_H
