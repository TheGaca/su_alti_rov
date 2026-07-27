#include "SettingsDialog.hpp"

#include <QFormLayout>
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

    root->addWidget(gain_group);

    QLabel *note = new QLabel("Not: kamera IP değişikliği anında uygulanır; köprü IP/port "
                              "değişikliği bir sonraki bağlanmada geçerli olur.");
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
    return r;
}
