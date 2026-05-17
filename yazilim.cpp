#include "yazilim.h"
#include <QTimer>
#include <QCloseEvent>
#include <QPixmap>
#include <QDir>
#include <QDateTime>
#include <QMessageBox>

PixhawkGUI::PixhawkGUI(QWidget *parent)
    : QMainWindow(parent),
      ana_mav_thread(nullptr), mini_mav_thread(nullptr),
      ana_joy_thread(nullptr), mini_joy_thread(nullptr),
      cam_thread(nullptr), anarov_thread(nullptr),
      servo_open(false), servo_pin(9)
{
    ui.setupUi(this);

    button_map[0] = "A";
    button_map[1] = "B";
    button_map[2] = "X";
    button_map[3] = "Y";
    button_map[4] = "LB";
    button_map[5] = "RB";
    button_map[6] = "Back";
    button_map[7] = "Start";
    button_map[8] = "Logitech";
    button_map[9] = "Sol Stick Tık";
    button_map[10] = "Sağ Stick Tık";

    ana_axes_state[0] = 0.0f;
    ana_axes_state[1] = 0.0f;
    mini_axes_state[0] = 0.0f;
    mini_axes_state[1] = 0.0f;

    connect(ui.anaRovPanel->btn_connect, &QPushButton::clicked, this, &PixhawkGUI::toggle_ana_connection);
    connect(ui.miniRovPanel->btn_connect, &QPushButton::clicked, this, &PixhawkGUI::toggle_mini_connection);

    cam_ping_proc = new QProcess(this);
    anarov_ping_proc = new QProcess(this);
    connect(cam_ping_proc, &QProcess::readyReadStandardOutput, this, &PixhawkGUI::read_cam_ping);
    connect(anarov_ping_proc, &QProcess::readyReadStandardOutput, this, &PixhawkGUI::read_anarov_ping);

    cam_ping_proc->start("ping", QStringList() << "192.168.88.2");
    anarov_ping_proc->start("ping", QStringList() << "192.168.1.116");

    QTimer::singleShot(500, this, &PixhawkGUI::start_camera);
    QTimer::singleShot(500, this, &PixhawkGUI::start_anarov);
    
    connect(ui.anaRovPanel->btn_joy_connect, &QPushButton::clicked, this, [=]() {
        start_ana_joystick(ui.anaRovPanel->joy_combo->currentText());
    });
    connect(ui.miniRovPanel->btn_joy_connect, &QPushButton::clicked, this, [=]() {
        start_mini_joystick(ui.miniRovPanel->joy_combo->currentText());
    });

    QDir dir;
    if (!dir.exists("loglar")) {
        dir.mkpath("loglar");
    }
    QString log_path = QString("loglar/log_%1.txt").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss"));
    log_file = new QFile(log_path, this);
    if (log_file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        log_stream = new QTextStream(log_file);
    } else {
        log_stream = nullptr;
    }
    
    // Ana ROV Butonları
    connect(ui.anaRovPanel->btn_emergency, &QPushButton::clicked, this, [=]() { log_message("ACİL DURDURMA!", 0); });
    connect(ui.anaRovPanel->btn_stabilize, &QPushButton::clicked, this, [=]() { log_message("Sabitle", 0); });
    connect(ui.anaRovPanel->btn_start, &QPushButton::clicked, this, [=]() { log_message("Start", 0); });
    connect(ui.anaRovPanel->btn_torpedo, &QPushButton::clicked, this, [=]() { log_message("Torpido Fırlat", 0); });
    connect(ui.anaRovPanel->btn_minirov_launch, &QPushButton::clicked, this, [=]() { log_message("MiniROV Fırlat", 0); });
    connect(ui.anaRovPanel->btn_up, &QPushButton::clicked, this, [=]() { log_message("YÖN: Yukarı (Z+)", 0); });
    connect(ui.anaRovPanel->btn_down, &QPushButton::clicked, this, [=]() { log_message("YÖN: Aşağı (Z-)", 0); });
    connect(ui.anaRovPanel->btn_left, &QPushButton::clicked, this, [=]() { log_message("YÖN: Sola Dön", 0); });
    connect(ui.anaRovPanel->btn_right, &QPushButton::clicked, this, [=]() { log_message("YÖN: Sağa Dön", 0); });
    connect(ui.anaRovPanel->btn_forward, &QPushButton::clicked, this, [=]() { log_message("YÖN: İleri", 0); });
    connect(ui.anaRovPanel->btn_backward, &QPushButton::clicked, this, [=]() { log_message("YÖN: Geri", 0); });

    // Mini ROV Butonları
    connect(ui.miniRovPanel->btn_emergency, &QPushButton::clicked, this, [=]() { log_message("ACİL DURDURMA!", 1); });
    connect(ui.miniRovPanel->btn_stabilize, &QPushButton::clicked, this, [=]() { log_message("Sabitle", 1); });
    connect(ui.miniRovPanel->btn_start, &QPushButton::clicked, this, [=]() { log_message("Start", 1); });
    connect(ui.miniRovPanel->btn_up, &QPushButton::clicked, this, [=]() { log_message("YÖN: Yukarı (Z+)", 1); });
    connect(ui.miniRovPanel->btn_down, &QPushButton::clicked, this, [=]() { log_message("YÖN: Aşağı (Z-)", 1); });
    connect(ui.miniRovPanel->btn_left, &QPushButton::clicked, this, [=]() { log_message("YÖN: Sola Dön", 1); });
    connect(ui.miniRovPanel->btn_right, &QPushButton::clicked, this, [=]() { log_message("YÖN: Sağa Dön", 1); });
    connect(ui.miniRovPanel->btn_forward, &QPushButton::clicked, this, [=]() { log_message("YÖN: İleri", 1); });
    connect(ui.miniRovPanel->btn_backward, &QPushButton::clicked, this, [=]() { log_message("YÖN: Geri", 1); });

    log_message("Sistem ve Loglama Başlatıldı.", -1);
}

PixhawkGUI::~PixhawkGUI() {
    if (log_file && log_file->isOpen()) {
        log_file->close();
    }
    cam_ping_proc->kill();
    anarov_ping_proc->kill();
}

// ================== ANA ROV BAĞLANTISI ==================

void PixhawkGUI::toggle_ana_connection() {
    if (!ana_mav_thread || !ana_mav_thread->isRunning()) {
        start_ana_connection();
    } else {
        stop_ana_connection();
    }
}

void PixhawkGUI::start_ana_connection() {
    if (ana_mav_thread && ana_mav_thread->isRunning()) return;

    QString port = ui.anaRovPanel->port_combo->currentText();
    int baudrate = ui.anaRovPanel->baud_combo->currentText().toInt();

    ana_mav_thread = new MAVLinkThread(port, baudrate, this);
    connect(ana_mav_thread, &MAVLinkThread::attitude_signal, this, &PixhawkGUI::update_ana_attitude);
    connect(ana_mav_thread, &MAVLinkThread::vfr_hud_signal, this, &PixhawkGUI::update_ana_vfr_hud);
    connect(ana_mav_thread, &MAVLinkThread::battery_signal, this, &PixhawkGUI::update_ana_battery);
    connect(ana_mav_thread, &MAVLinkThread::status_signal, this, &PixhawkGUI::update_ana_status);

    ana_mav_thread->start();
    ui.anaRovPanel->btn_connect->setText("Bağlantıyı Kes");
    ui.anaRovPanel->btn_connect->setStyleSheet("background-color: #e74c3c; color: white;");
}

void PixhawkGUI::stop_ana_connection() {
    if (ana_mav_thread) {
        ana_mav_thread->stop();
        ana_mav_thread->deleteLater();
        ana_mav_thread = nullptr;
    }
    ui.anaRovPanel->btn_connect->setText("Bağlan");
    ui.anaRovPanel->btn_connect->setStyleSheet("");
    update_ana_status("Durum: Bağlantı Kesildi");
    reset_labels(ui.anaRovPanel);
}

void PixhawkGUI::update_ana_attitude(float roll, float pitch, float yaw) {
    ui.anaRovPanel->bar_roll->setValue((int)roll);
    roll += 90.0f;
    ui.anaRovPanel->lbl_roll->setText(QString::number(roll, 'f', 2) + "°");
    ui.anaRovPanel->lbl_pitch->setText(QString::number(pitch, 'f', 2) + "°");
    ui.anaRovPanel->lbl_yaw->setText(QString::number(yaw, 'f', 2) + "°");
    ui.anaRovPanel->attitude_indicator->set_attitude(roll, pitch);
}

void PixhawkGUI::update_ana_vfr_hud(float alt, float heading, float speed) {
    ui.anaRovPanel->lbl_alt->setText(QString::number(alt, 'f', 2));
    ui.anaRovPanel->lbl_heading->setText(QString::number(heading, 'f', 0) + "°");
    ui.anaRovPanel->lbl_speed->setText(QString::number(speed, 'f', 2));
}

void PixhawkGUI::update_ana_battery(float voltage, int remaining) {
    ui.anaRovPanel->lbl_voltage->setText(QString::number(voltage, 'f', 2) + "V");
    ui.anaRovPanel->lbl_battery->setText(QString::number(remaining) + "%");
}

void PixhawkGUI::update_ana_status(const QString &msg) {
    ui.anaRovPanel->lbl_status->setText(msg);
    if (msg.contains("Hata")) {
        ui.anaRovPanel->lbl_status->setStyleSheet("color: red; font-weight: bold;");
        stop_ana_connection();
    } else if (msg.contains("Başarılı")) {
        ui.anaRovPanel->lbl_status->setStyleSheet("color: green; font-weight: bold;");
    }
}

// ================== MİNİ ROV BAĞLANTISI ==================

void PixhawkGUI::toggle_mini_connection() {
    if (!mini_mav_thread || !mini_mav_thread->isRunning()) {
        start_mini_connection();
    } else {
        stop_mini_connection();
    }
}

void PixhawkGUI::start_mini_connection() {
    if (mini_mav_thread && mini_mav_thread->isRunning()) return;

    QString port = ui.miniRovPanel->port_combo->currentText();
    int baudrate = ui.miniRovPanel->baud_combo->currentText().toInt();

    mini_mav_thread = new MAVLinkThread(port, baudrate, this);
    connect(mini_mav_thread, &MAVLinkThread::attitude_signal, this, &PixhawkGUI::update_mini_attitude);
    connect(mini_mav_thread, &MAVLinkThread::vfr_hud_signal, this, &PixhawkGUI::update_mini_vfr_hud);
    connect(mini_mav_thread, &MAVLinkThread::battery_signal, this, &PixhawkGUI::update_mini_battery);
    connect(mini_mav_thread, &MAVLinkThread::status_signal, this, &PixhawkGUI::update_mini_status);

    mini_mav_thread->start();
    ui.miniRovPanel->btn_connect->setText("Bağlantıyı Kes");
    ui.miniRovPanel->btn_connect->setStyleSheet("background-color: #e74c3c; color: white;");
}

void PixhawkGUI::stop_mini_connection() {
    if (mini_mav_thread) {
        mini_mav_thread->stop();
        mini_mav_thread->deleteLater();
        mini_mav_thread = nullptr;
    }
    ui.miniRovPanel->btn_connect->setText("Bağlan");
    ui.miniRovPanel->btn_connect->setStyleSheet("");
    update_mini_status("Durum: Bağlantı Kesildi");
    reset_labels(ui.miniRovPanel);
}

void PixhawkGUI::update_mini_attitude(float roll, float pitch, float yaw) {
    ui.miniRovPanel->bar_roll->setValue((int)roll);
    roll += 90.0f;
    ui.miniRovPanel->lbl_roll->setText(QString::number(roll, 'f', 2) + "°");
    ui.miniRovPanel->lbl_pitch->setText(QString::number(pitch, 'f', 2) + "°");
    ui.miniRovPanel->lbl_yaw->setText(QString::number(yaw, 'f', 2) + "°");
    ui.miniRovPanel->attitude_indicator->set_attitude(roll, pitch);
}

void PixhawkGUI::update_mini_vfr_hud(float alt, float heading, float speed) {
    ui.miniRovPanel->lbl_alt->setText(QString::number(alt, 'f', 2));
    ui.miniRovPanel->lbl_heading->setText(QString::number(heading, 'f', 0) + "°");
    ui.miniRovPanel->lbl_speed->setText(QString::number(speed, 'f', 2));
}

void PixhawkGUI::update_mini_battery(float voltage, int remaining) {
    ui.miniRovPanel->lbl_voltage->setText(QString::number(voltage, 'f', 2) + "V");
    ui.miniRovPanel->lbl_battery->setText(QString::number(remaining) + "%");
}

void PixhawkGUI::update_mini_status(const QString &msg) {
    ui.miniRovPanel->lbl_status->setText(msg);
    if (msg.contains("Hata")) {
        ui.miniRovPanel->lbl_status->setStyleSheet("color: red; font-weight: bold;");
        stop_mini_connection();
    } else if (msg.contains("Başarılı")) {
        ui.miniRovPanel->lbl_status->setStyleSheet("color: green; font-weight: bold;");
    }
}

// ================== ORTAK ==================

void PixhawkGUI::start_ana_joystick(QString port) {
    if (ana_joy_thread) {
        ana_joy_thread->stop();
        ana_joy_thread->deleteLater();
    }
    ana_joy_thread = new JoystickThread(port, this);
    connect(ana_joy_thread, &JoystickThread::button_signal, this, &PixhawkGUI::update_ana_joy_button);
    connect(ana_joy_thread, &JoystickThread::axis_signal, this, &PixhawkGUI::update_ana_joy_axis);
    connect(ana_joy_thread, &JoystickThread::status_signal, this, &PixhawkGUI::update_ana_joy_status);
    ana_joy_thread->start();
}

void PixhawkGUI::start_mini_joystick(QString port) {
    if (mini_joy_thread) {
        mini_joy_thread->stop();
        mini_joy_thread->deleteLater();
    }
    mini_joy_thread = new JoystickThread(port, this);
    connect(mini_joy_thread, &JoystickThread::button_signal, this, &PixhawkGUI::update_mini_joy_button);
    connect(mini_joy_thread, &JoystickThread::axis_signal, this, &PixhawkGUI::update_mini_joy_axis);
    connect(mini_joy_thread, &JoystickThread::status_signal, this, &PixhawkGUI::update_mini_joy_status);
    mini_joy_thread->start();
}

void PixhawkGUI::start_camera() {
    cam_thread = new CameraThread("192.168.88.2", this);
    connect(cam_thread, &CameraThread::image_signal, this, &PixhawkGUI::update_camera_frame);
    connect(cam_thread, &CameraThread::status_signal, this, &PixhawkGUI::update_camera_status);
    connect(cam_thread, &CameraThread::stats_signal, this, &PixhawkGUI::update_camera_stats);
    cam_thread->start();
}

void PixhawkGUI::start_anarov() {
    anarov_thread = new AnaRovThread("192.168.1.116", this);
    connect(anarov_thread, &AnaRovThread::image_signal, this, &PixhawkGUI::update_anarov_frame);
    connect(anarov_thread, &AnaRovThread::status_signal, this, &PixhawkGUI::update_anarov_status);
    connect(anarov_thread, &AnaRovThread::stats_signal, this, &PixhawkGUI::update_anarov_stats);
    anarov_thread->start();
}

// ANA ROV KUMANDA SLOTLARI
void PixhawkGUI::update_ana_joy_status(const QString &msg) {
    ui.anaRovPanel->lbl_pad_status->setText("Durum: " + msg);
    if (msg.contains("Bağlandı")) {
        ui.anaRovPanel->lbl_pad_status->setStyleSheet("color: green; font-weight: bold;");
    } else {
        ui.anaRovPanel->lbl_pad_status->setStyleSheet("color: red;");
    }
}

void PixhawkGUI::update_ana_joy_button(int btn_id, int state) {
    QString btn_name = button_map.value(btn_id, QString("Tuş %1").arg(btn_id));
    if (btn_id == 7 && state == 1) { // START button -> toggle servo
        servo_open = !servo_open;
        int pwm_val = servo_open ? 1900 : 1100;
        if (servo_open) {
            ui.anaRovPanel->lbl_servo_status->setText("Açık (90°)");
            ui.anaRovPanel->lbl_servo_status->setStyleSheet("color: green;");
        } else {
            ui.anaRovPanel->lbl_servo_status->setText("Kapalı");
            ui.anaRovPanel->lbl_servo_status->setStyleSheet("color: black;");
        }
        if (ana_mav_thread && ana_mav_thread->isRunning()) ana_mav_thread->set_servo(servo_pin, pwm_val);
    }
    if (state == 1) {
        ana_pressed_buttons.insert(btn_name);
        log_message(QString("KOL: %1 tuşuna basıldı").arg(btn_name), 0);
    } else if (state == 0) {
        ana_pressed_buttons.remove(btn_name);
    }
    if (!ana_pressed_buttons.isEmpty()) {
        ui.anaRovPanel->lbl_pad_buttons->setText("Basılan Tuş: " + QStringList(ana_pressed_buttons.values()).join(", "));
    } else {
        ui.anaRovPanel->lbl_pad_buttons->setText("Basılan Tuş: Yok");
    }
}

void PixhawkGUI::update_ana_joy_axis(int axis_id, float value) {
    if (axis_id == 0 || axis_id == 1) {
        ana_axes_state[axis_id] = value;
        ui.anaRovPanel->lbl_pad_axes->setText(QString("Sol Stick: X: %1  Y: %2")
            .arg(ana_axes_state[0], 0, 'f', 2).arg(ana_axes_state[1], 0, 'f', 2));
    } else if (axis_id == 6 || axis_id == 7) {
        QString name;
        if (axis_id == 6 && value > 0) name = "Sağ";
        else if (axis_id == 6 && value < 0) name = "Sol";
        else if (axis_id == 7 && value > 0) name = "Aşağı";
        else if (axis_id == 7 && value < 0) name = "Yukarı";
        ana_pressed_buttons.remove("Sağ"); ana_pressed_buttons.remove("Sol");
        ana_pressed_buttons.remove("Yukarı"); ana_pressed_buttons.remove("Aşağı");
        if (!name.isEmpty()) ana_pressed_buttons.insert(name);
        if (!ana_pressed_buttons.isEmpty()) {
            ui.anaRovPanel->lbl_pad_buttons->setText("Basılan Tuş: " + QStringList(ana_pressed_buttons.values()).join(", "));
        } else {
            ui.anaRovPanel->lbl_pad_buttons->setText("Basılan Tuş: Yok");
        }
    }
}

// MİNİ ROV KUMANDA SLOTLARI
void PixhawkGUI::update_mini_joy_status(const QString &msg) {
    ui.miniRovPanel->lbl_pad_status->setText("Durum: " + msg);
    if (msg.contains("Bağlandı")) {
        ui.miniRovPanel->lbl_pad_status->setStyleSheet("color: green; font-weight: bold;");
    } else {
        ui.miniRovPanel->lbl_pad_status->setStyleSheet("color: red;");
    }
}

void PixhawkGUI::update_mini_joy_button(int btn_id, int state) {
    QString btn_name = button_map.value(btn_id, QString("Tuş %1").arg(btn_id));
    if (btn_id == 7 && state == 1) { // START button
        servo_open = !servo_open;
        int pwm_val = servo_open ? 1900 : 1100;
        if (servo_open) {
            ui.miniRovPanel->lbl_servo_status->setText("Açık (90°)");
            ui.miniRovPanel->lbl_servo_status->setStyleSheet("color: green;");
        } else {
            ui.miniRovPanel->lbl_servo_status->setText("Kapalı");
            ui.miniRovPanel->lbl_servo_status->setStyleSheet("color: black;");
        }
        if (mini_mav_thread && mini_mav_thread->isRunning()) mini_mav_thread->set_servo(servo_pin, pwm_val);
    }
    if (state == 1) {
        mini_pressed_buttons.insert(btn_name);
        log_message(QString("KOL: %1 tuşuna basıldı").arg(btn_name), 1);
    } else if (state == 0) {
        mini_pressed_buttons.remove(btn_name);
    }
    if (!mini_pressed_buttons.isEmpty()) {
        ui.miniRovPanel->lbl_pad_buttons->setText("Basılan Tuş: " + QStringList(mini_pressed_buttons.values()).join(", "));
    } else {
        ui.miniRovPanel->lbl_pad_buttons->setText("Basılan Tuş: Yok");
    }
}

void PixhawkGUI::update_mini_joy_axis(int axis_id, float value) {
    if (axis_id == 0 || axis_id == 1) {
        mini_axes_state[axis_id] = value;
        ui.miniRovPanel->lbl_pad_axes->setText(QString("Sol Stick: X: %1  Y: %2")
            .arg(mini_axes_state[0], 0, 'f', 2).arg(mini_axes_state[1], 0, 'f', 2));
    } else if (axis_id == 6 || axis_id == 7) {
        QString name;
        if (axis_id == 6 && value > 0) name = "Sağ";
        else if (axis_id == 6 && value < 0) name = "Sol";
        else if (axis_id == 7 && value > 0) name = "Aşağı";
        else if (axis_id == 7 && value < 0) name = "Yukarı";
        mini_pressed_buttons.remove("Sağ"); mini_pressed_buttons.remove("Sol");
        mini_pressed_buttons.remove("Yukarı"); mini_pressed_buttons.remove("Aşağı");
        if (!name.isEmpty()) mini_pressed_buttons.insert(name);
        if (!mini_pressed_buttons.isEmpty()) {
            ui.miniRovPanel->lbl_pad_buttons->setText("Basılan Tuş: " + QStringList(mini_pressed_buttons.values()).join(", "));
        } else {
            ui.miniRovPanel->lbl_pad_buttons->setText("Basılan Tuş: Yok");
        }
    }
}

void PixhawkGUI::reset_labels(PixhawkPanel *panel) {
    panel->lbl_roll->setText("---");
    panel->lbl_pitch->setText("---");
    panel->lbl_yaw->setText("---");
    panel->bar_roll->setValue(0);
    panel->lbl_alt->setText("---");
    panel->lbl_heading->setText("---");
    panel->lbl_speed->setText("---");
    panel->lbl_voltage->setText("---");
    panel->lbl_battery->setText("---");
    panel->attitude_indicator->set_attitude(0, 0);
}

void PixhawkGUI::update_camera_frame(const QImage &q_img) {
    QPixmap pix = QPixmap::fromImage(q_img).scaled(ui.miniRovPanel->lbl_cam_stream->size(), Qt::KeepAspectRatio);
    ui.miniRovPanel->lbl_cam_stream->setPixmap(pix);
}

void PixhawkGUI::update_camera_status(const QString &msg) {
    if (msg.contains("Hata") || msg.contains("Bekleniyor")) {
        ui.miniRovPanel->lbl_cam_stream->setText(msg);
    }
}

void PixhawkGUI::update_camera_stats(int fps, float kbps, int width, int height) {
    ui.miniRovPanel->lbl_cam_fps->setText(QString("FPS: %1").arg(fps));
    ui.miniRovPanel->lbl_cam_bitrate->setText(QString("Veri Hızı: %1 KB/s").arg(kbps, 0, 'f', 1));
    ui.miniRovPanel->lbl_cam_res->setText(QString("Çözünürlük: %1x%2").arg(width).arg(height));
}

void PixhawkGUI::update_anarov_frame(const QImage &q_img) {
    QPixmap pix = QPixmap::fromImage(q_img).scaled(ui.anaRovPanel->lbl_cam_stream->size(), Qt::KeepAspectRatio);
    ui.anaRovPanel->lbl_cam_stream->setPixmap(pix);
}

void PixhawkGUI::update_anarov_status(const QString &msg) {
    if (msg.contains("Hata") || msg.contains("Bekleniyor")) {
        ui.anaRovPanel->lbl_cam_stream->setText(msg);
    }
}

void PixhawkGUI::update_anarov_stats(int fps, float kbps, int width, int height) {
    ui.anaRovPanel->lbl_cam_fps->setText(QString("FPS: %1").arg(fps));
    ui.anaRovPanel->lbl_cam_bitrate->setText(QString("Veri Hızı: %1 KB/s").arg(kbps, 0, 'f', 1));
    ui.anaRovPanel->lbl_cam_res->setText(QString("Çözünürlük: %1x%2").arg(width).arg(height));
}

void PixhawkGUI::read_cam_ping() {
    while (cam_ping_proc->canReadLine()) {
        QString line = cam_ping_proc->readLine().trimmed();
        int time_idx = line.indexOf("time=");
        if (time_idx != -1) {
            QString time_str = line.mid(time_idx + 5);
            ui.miniRovPanel->lbl_cam_ping->setText("Ping: " + time_str);
        } else if (line.contains("Unreachable") || line.contains("timeout")) {
            ui.miniRovPanel->lbl_cam_ping->setText("Ping: Zaman Aşımı");
        }
    }
}

void PixhawkGUI::read_anarov_ping() {
    while (anarov_ping_proc->canReadLine()) {
        QString line = anarov_ping_proc->readLine().trimmed();
        int time_idx = line.indexOf("time=");
        if (time_idx != -1) {
            QString time_str = line.mid(time_idx + 5);
            ui.anaRovPanel->lbl_cam_ping->setText("Ping: " + time_str);
        } else if (line.contains("Unreachable") || line.contains("timeout")) {
            ui.anaRovPanel->lbl_cam_ping->setText("Ping: Zaman Aşımı");
        }
    }
}

void PixhawkGUI::log_message(const QString &msg, int target) {
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString log_text = QString("[%1] %2").arg(timestamp).arg(msg);
    
    if (target == 0) {
        ui.anaRovPanel->terminal_log->append(log_text);
    } else if (target == 1) {
        ui.miniRovPanel->terminal_log->append(log_text);
    } else {
        ui.anaRovPanel->terminal_log->append(log_text);
        ui.miniRovPanel->terminal_log->append(log_text);
    }
    
    if (log_stream) {
        *log_stream << (target == 0 ? "[ANA] " : (target == 1 ? "[MINI] " : "[SYS] ")) << log_text << "\n";
        log_stream->flush();
    }
}

void PixhawkGUI::closeEvent(QCloseEvent *event) {
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Çıkış Onayı", "Uygulamadan çıkmak istediğinize emin misiniz?",
                                  QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        stop_ana_connection();
        stop_mini_connection();
        if (ana_joy_thread) ana_joy_thread->stop();
        if (mini_joy_thread) mini_joy_thread->stop();
        if (cam_thread) cam_thread->stop();
        if (anarov_thread) anarov_thread->stop();
        event->accept();
    } else {
        event->ignore();
    }
}
