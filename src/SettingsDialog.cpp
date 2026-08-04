#include "SettingsDialog.hpp"
#include "MotorMixer.hpp"

#include <QFormLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(const RovSettings &current, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle("Ayarlar");
    setModal(true);
    setMinimumWidth(420);

    QVBoxLayout *root = new QVBoxLayout(this);

    // ---- Ağ ----
    QGroupBox *net_group = new QGroupBox("Ağ");
    QFormLayout *net_form = new QFormLayout(net_group);

    edit_ana_host = new QLineEdit(current.anaHost);
    net_form->addRow("Ana ROV köprü IP:", edit_ana_host);

    spin_bridge_port = new QSpinBox();
    spin_bridge_port->setRange(1, 65535);
    spin_bridge_port->setValue(current.anaBridgePort);
    net_form->addRow("Ana ROV köprü port:", spin_bridge_port);

    edit_ana_cam = new QLineEdit(current.anaCamAddr);
    edit_ana_cam->setPlaceholderText("örn. 192.168.2.220:81");
    net_form->addRow("Ana ROV kamera (ip[:port]):", edit_ana_cam);

    edit_mini_cam = new QLineEdit(current.miniCamIp);
    net_form->addRow("Mini ROV kamera IP:", edit_mini_cam);

    root->addWidget(net_group);

    // ---- Kontrol kazançları ----
    QGroupBox *gain_group = new QGroupBox("Kontrol Kazançları (canlı uygulanır)");
    QFormLayout *gain_form = new QFormLayout(gain_group);

    spin_stab_gain = new QDoubleSpinBox();
    spin_stab_gain->setRange(0.0, 20.0);
    spin_stab_gain->setSingleStep(0.5);
    spin_stab_gain->setValue(current.stabGain);
    spin_stab_gain->setToolTip("roll/pitch (derece) / 90 × kazanç → dikey itki düzeltmesi.\n"
                               "4.0 ile ~22.5°'de tam düzeltme. Aşırı tepkiye karşı azaltın.");
    gain_form->addRow("Sabitleme kazancı:", spin_stab_gain);

    spin_depth_gain = new QDoubleSpinBox();
    spin_depth_gain->setRange(0.0, 20.0);
    spin_depth_gain->setSingleStep(0.5);
    spin_depth_gain->setValue(current.depthHoldGain);
    spin_depth_gain->setToolTip("hedef−mevcut derinlik (m) × kazanç → dikey itki.\n"
                                "2.0 ile ~0.5m sapmada tam düzeltme.");
    gain_form->addRow("Derinlik sabitleme kazancı:", spin_depth_gain);

    spin_min_us = new QSpinBox();
    spin_min_us->setRange(1000, 2000);
    spin_min_us->setSuffix(" us");
    spin_min_us->setValue(current.minUs);
    spin_min_us->setToolTip("Tam geri darbe (alt sınır). ESP32 firmware'i (AnaRovBeyin.ino/\n"
                             "sualtiesp.ino) zaten kendi MIN_US'una (1295us) göre de kırpar -\n"
                             "burayı ondan DAR seçmek ekstra bir yumuşak sınır ekler.");
    gain_form->addRow("Motor min darbesi (tam geri):", spin_min_us);

    spin_max_us = new QSpinBox();
    spin_max_us->setRange(1000, 2000);
    spin_max_us->setSuffix(" us");
    spin_max_us->setValue(current.maxUs);
    spin_max_us->setToolTip("Tam ileri darbe (üst sınır). ESP32 firmware'i zaten kendi\n"
                             "MAX_US'una (1600us) göre de kırpar - burayı ondan GENİŞ seçmenin\n"
                             "bir zararı yok, sadece etkisi olmaz.");
    gain_form->addRow("Motor max darbesi (tam ileri):", spin_max_us);

    root->addWidget(gain_group);

    QGroupBox *motor_neutral_group = new QGroupBox("Motor Nötr Noktaları (ESC bazlı, canlı uygulanır)");
    QGridLayout *motor_neutral_grid = new QGridLayout(motor_neutral_group);
    for (int i = 0; i < 8; ++i) {
        QSpinBox *spin = new QSpinBox();
        spin->setRange(1000, 2000);
        spin->setSuffix(" us");
        spin->setValue(current.motorNeutralUs[i]);
        spin_motor_neutral_us[i] = spin;
        motor_neutral_grid->addWidget(new QLabel(QString("M%1:").arg(i + 1)), i / 4, (i % 4) * 2);
        motor_neutral_grid->addWidget(spin, i / 4, (i % 4) * 2 + 1);
    }
    root->addWidget(motor_neutral_group);

    // ---- Torpido ----
    QGroupBox *torpedo_group = new QGroupBox("Torpido (ESC bazlı, bağlantı/ARM sonrası ESP32'ye gönderilir)");
    QGridLayout *torpedo_grid = new QGridLayout(torpedo_group);
    torpedo_grid->addWidget(new QLabel("Min (arm):"), 0, 1);
    torpedo_grid->addWidget(new QLabel("Nötr:"), 0, 2);
    torpedo_grid->addWidget(new QLabel("Max (ateş):"), 0, 3);
    for (int i = 0; i < 3; ++i) {
        torpedo_grid->addWidget(new QLabel(QString("T%1:").arg(i + 1)), i + 1, 0);

        QSpinBox *spinMin = new QSpinBox();
        spinMin->setRange(1000, 2000);
        spinMin->setSuffix(" us");
        spinMin->setValue(current.torpedoMinUs[i]);
        spin_torpedo_min_us[i] = spinMin;
        torpedo_grid->addWidget(spinMin, i + 1, 1);

        QSpinBox *spinNeutral = new QSpinBox();
        spinNeutral->setRange(1000, 2000);
        spinNeutral->setSuffix(" us");
        spinNeutral->setValue(current.torpedoNeutralUs[i]);
        spin_torpedo_neutral_us[i] = spinNeutral;
        torpedo_grid->addWidget(spinNeutral, i + 1, 2);

        QSpinBox *spinMax = new QSpinBox();
        spinMax->setRange(1000, 2000);
        spinMax->setSuffix(" us");
        spinMax->setValue(current.torpedoMaxUs[i]);
        spin_torpedo_max_us[i] = spinMax;
        torpedo_grid->addWidget(spinMax, i + 1, 3);
    }
    root->addWidget(torpedo_group);

    QPushButton *btn_torpedo_reset = new QPushButton("Torpido Sayacını Sıfırla (3/3, sadece DISARMED)");
    connect(btn_torpedo_reset, &QPushButton::clicked, this, &SettingsDialog::torpedo_reset_requested);
    root->addWidget(btn_torpedo_reset);

    QLabel *note = new QLabel("Not: kamera IP değişikliği anında uygulanır; köprü IP/port "
                              "değişikliği bir sonraki bağlanmada geçerli olur. Torpido darbeleri "
                              "yalnızca Ana ROV'a bağlıyken, kaydedilince ESP32'ye gönderilir.");
    note->setWordWrap(true);
    note->setStyleSheet("color: gray; font-size: 11px;");
    root->addWidget(note);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Save)->setText("Kaydet");
    buttons->button(QDialogButtonBox::Cancel)->setText("İptal");
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
}

RovSettings SettingsDialog::values() const {
    RovSettings r;
    r.anaHost       = edit_ana_host->text().trimmed();
    r.anaBridgePort = static_cast<quint16>(spin_bridge_port->value());
    r.anaCamAddr    = edit_ana_cam->text().trimmed();
    r.miniCamIp     = edit_mini_cam->text().trimmed();
    r.stabGain      = static_cast<float>(spin_stab_gain->value());
    r.depthHoldGain = static_cast<float>(spin_depth_gain->value());
    for (int i = 0; i < 8; ++i) r.motorNeutralUs[i] = spin_motor_neutral_us[i]->value();
    r.minUs         = spin_min_us->value();
    r.maxUs         = spin_max_us->value();
    for (int i = 0; i < 3; ++i) {
        r.torpedoMinUs[i]     = spin_torpedo_min_us[i]->value();
        r.torpedoNeutralUs[i] = spin_torpedo_neutral_us[i]->value();
        r.torpedoMaxUs[i]     = spin_torpedo_max_us[i]->value();
    }
    return r;
}
