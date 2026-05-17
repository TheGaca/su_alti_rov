#include "yazilim.h"
#include <QDateTime>
#include <QCloseEvent>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>

PixhawkGUI::PixhawkGUI(QWidget *parent)
    : QMainWindow(parent),
      ana_mav_thread(nullptr), mini_mav_thread(nullptr),
      ana_joy_thread(nullptr), mini_joy_thread(nullptr),
      cam_thread(nullptr), anarov_thread(nullptr),
      ana_lamp_on(false), mini_lamp_on(false), ana_autonomous(false), torpedo_ready(true), minirov_launched(false), dark_mode(false),
      cam_ping_proc(nullptr), anarov_ping_proc(nullptr),
      log_file(nullptr), log_stream(nullptr)
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    ui.setupUi(central);

    ana_axes_state[0] = ana_axes_state[1] = 0.0f;
    mini_axes_state[0] = mini_axes_state[1] = 0.0f;

    // Standart joystick buton haritası (XBox/PS uyumlu)
    button_map[0] = "A";
    button_map[1] = "B";
    button_map[2] = "X";
    button_map[3] = "Y";
    button_map[4] = "LB";
    button_map[5] = "RB";
    button_map[6] = "Back";
    button_map[7] = "Start";
    button_map[8] = "Guide";
    button_map[9] = "L3";
    button_map[10] = "R3";

    // Log dosyası
    QString logDir = QDir::homePath() + "/rov_logs";
    QDir().mkpath(logDir);
    QString logPath = logDir + "/rov_" +
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".log";
    log_file = new QFile(logPath);
    if (log_file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        log_stream = new QTextStream(log_file);
    }

    connect_signals();
    log_message("Sistem ve Loglama Başlatıldı.");

    // Kamera thread'lerini başlat
    start_camera_threads();

    // Ping işlemlerini başlat
    cam_ping_proc = new QProcess(this);
    connect(cam_ping_proc, &QProcess::readyReadStandardOutput,
            this, &PixhawkGUI::read_cam_ping);
    cam_ping_proc->start("ping", QStringList() << "-i" << "1" << "192.168.88.2");

    anarov_ping_proc = new QProcess(this);
    connect(anarov_ping_proc, &QProcess::readyReadStandardOutput,
            this, &PixhawkGUI::read_anarov_ping);
    anarov_ping_proc->start("ping", QStringList() << "-i" << "1" << "192.168.1.116");

    // Initialize default states for quick commands on GUI startup
    // Manuel active by default, Otonom inactive
    set_led(ui.anaRovPanel->led_manual, true);
    set_led(ui.anaRovPanel->led_autonomous, false);
    if (ui.anaRovPanel->led_minirov) set_led(ui.anaRovPanel->led_minirov, false);
    if (ui.anaRovPanel->led_torpedo) set_led(ui.anaRovPanel->led_torpedo, false);
    set_led(ui.anaRovPanel->led_lamp_on, false);
    set_led(ui.anaRovPanel->led_lamp_off, true);
    
    set_led(ui.miniRovPanel->led_manual, true);
    set_led(ui.miniRovPanel->led_autonomous, false);
    set_led(ui.miniRovPanel->led_lamp_on, false);
    set_led(ui.miniRovPanel->led_lamp_off, true);
}

PixhawkGUI::~PixhawkGUI() {
    if (ana_mav_thread)  { ana_mav_thread->stop();  delete ana_mav_thread; }
    if (mini_mav_thread) { mini_mav_thread->stop(); delete mini_mav_thread; }
    if (ana_joy_thread)  { ana_joy_thread->stop();  delete ana_joy_thread; }
    if (mini_joy_thread) { mini_joy_thread->stop(); delete mini_joy_thread; }
    if (cam_thread)      { cam_thread->stop();      delete cam_thread; }
    if (anarov_thread)   { anarov_thread->stop();   delete anarov_thread; }

    if (cam_ping_proc)    cam_ping_proc->kill();
    if (anarov_ping_proc) anarov_ping_proc->kill();

    if (log_stream) { delete log_stream; log_stream = nullptr; }
    if (log_file)   { log_file->close(); delete log_file; log_file = nullptr; }
}

void PixhawkGUI::closeEvent(QCloseEvent *event) {
    QMessageBox::StandardButton resBtn = QMessageBox::question(this, "SuGaca",
                                                                 "Çıkmak istediğinize emin misiniz?",
                                                                 QMessageBox::No | QMessageBox::Yes,
                                                                 QMessageBox::No);
    if (resBtn != QMessageBox::Yes) {
        event->ignore();
    } else {
        log_message("Uygulama Kapatılıyor...");
        event->accept();
    }
}

void PixhawkGUI::connect_signals() {
    PixhawkPanel *A = ui.anaRovPanel;
    PixhawkPanel *M = ui.miniRovPanel;

    // Bağlantı butonları
    connect(A->btn_connect, &QPushButton::clicked, this, &PixhawkGUI::toggle_ana_connection);
    connect(M->btn_connect, &QPushButton::clicked, this, &PixhawkGUI::toggle_mini_connection);

    // Joystick bağlantı butonları
    connect(A->btn_joy_connect, &QPushButton::clicked, this, &PixhawkGUI::toggle_ana_joystick);
    connect(M->btn_joy_connect, &QPushButton::clicked, this, &PixhawkGUI::toggle_mini_joystick);

    // Yön butonları (pressed/released)
    auto bindDir = [this](QPushButton *b, void (PixhawkGUI::*p)(), void (PixhawkGUI::*r)()) {
        connect(b, &QPushButton::pressed,  this, p);
        connect(b, &QPushButton::released, this, r);
    };
    for (QPushButton *b : {A->btn_forward, A->btn_backward, A->btn_left, A->btn_right,
                            A->btn_up, A->btn_down, A->btn_turn_left, A->btn_turn_right}) {
        bindDir(b, &PixhawkGUI::ana_dir_pressed, &PixhawkGUI::ana_dir_released);
    }
    for (QPushButton *b : {M->btn_forward, M->btn_backward, M->btn_left, M->btn_right,
                            M->btn_up, M->btn_down, M->btn_turn_left, M->btn_turn_right}) {
        bindDir(b, &PixhawkGUI::mini_dir_pressed, &PixhawkGUI::mini_dir_released);
    }

    // Hızlı komutlar
    connect(A->btn_emergency,      &QPushButton::clicked, this, &PixhawkGUI::on_emergency_ana);
    connect(M->btn_emergency,      &QPushButton::clicked, this, &PixhawkGUI::on_emergency_mini);
    connect(A->btn_stabilize,      &QPushButton::clicked, this, &PixhawkGUI::on_stabilize_ana);
    connect(M->btn_stabilize,      &QPushButton::clicked, this, &PixhawkGUI::on_stabilize_mini);
    connect(A->btn_autonomous,     &QPushButton::clicked, this, &PixhawkGUI::on_autonomous_ana);
    connect(A->btn_manual,         &QPushButton::clicked, this, &PixhawkGUI::on_manual_ana);
    connect(A->btn_minirov_launch, &QPushButton::clicked, this, &PixhawkGUI::on_minirov_launch);
    connect(A->btn_torpedo,        &QPushButton::clicked, this, &PixhawkGUI::on_torpedo_fire);
    connect(A->btn_lamp_on,        &QPushButton::clicked, this, &PixhawkGUI::on_lamp_on_ana);
    connect(A->btn_lamp_off,       &QPushButton::clicked, this, &PixhawkGUI::on_lamp_off_ana);
    connect(M->btn_lamp_on,        &QPushButton::clicked, this, &PixhawkGUI::on_lamp_on_mini);
    connect(M->btn_lamp_off,       &QPushButton::clicked, this, &PixhawkGUI::on_lamp_off_mini);

    // Temizlik / Tema Değişimi
    connect(ui.btn_theme, &QPushButton::clicked, this, &PixhawkGUI::toggle_theme);
}

void PixhawkGUI::start_camera_threads() {
    // MiniROV kamera (MJPEG @ 192.168.88.2/stream)
    cam_thread = new CameraThread("192.168.88.2", this);
    connect(cam_thread, &CameraThread::image_signal,  this, &PixhawkGUI::update_camera_frame);
    connect(cam_thread, &CameraThread::status_signal, this, &PixhawkGUI::update_camera_status);
    connect(cam_thread, &CameraThread::stats_signal,  this, &PixhawkGUI::update_camera_stats);
    cam_thread->start();

    // AnaROV kamera (WebSocket @ 192.168.1.116:85)
    anarov_thread = new AnaRovThread("192.168.1.116", this);
    connect(anarov_thread, &AnaRovThread::image_signal,  this, &PixhawkGUI::update_anarov_frame);
    connect(anarov_thread, &AnaRovThread::status_signal, this, &PixhawkGUI::update_anarov_status);
    connect(anarov_thread, &AnaRovThread::stats_signal,  this, &PixhawkGUI::update_anarov_stats);
    anarov_thread->start();
}

// ==================== Log ====================
void PixhawkGUI::log_message(const QString &msg, int target) {
    QString ts = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString line = QString("[%1] %2").arg(ts, msg);

    if (target == 0 || target == -1) ui.anaRovPanel->terminal_log->append(line);
    if (target == 1 || target == -1) ui.miniRovPanel->terminal_log->append(line);

    if (log_stream) {
        *log_stream << line << "\n";
        log_stream->flush();
    }
}

// ==================== Ana ROV Bağlantı ====================
void PixhawkGUI::toggle_ana_connection() {
    PixhawkPanel *A = ui.anaRovPanel;
    if (ana_mav_thread) {
        ana_mav_thread->stop();
        delete ana_mav_thread;
        ana_mav_thread = nullptr;
        A->btn_connect->setText("Bağlan");
        A->lbl_status->setText("Durum: Bağlantı Kesildi");
        reset_labels(A);
        log_message("Ana ROV bağlantısı kesildi.", 0);
        return;
    }
    QString port = A->port_combo->currentText();
    int baud = A->baud_combo->currentText().toInt();
    ana_mav_thread = new MAVLinkThread(port, baud, this);
    connect(ana_mav_thread, &MAVLinkThread::attitude_signal, this, &PixhawkGUI::update_ana_attitude);
    connect(ana_mav_thread, &MAVLinkThread::vfr_hud_signal,  this, &PixhawkGUI::update_ana_vfr_hud);
    connect(ana_mav_thread, &MAVLinkThread::battery_signal,  this, &PixhawkGUI::update_ana_battery);
    connect(ana_mav_thread, &MAVLinkThread::status_signal,   this, &PixhawkGUI::update_ana_status);
    ana_mav_thread->start();
    A->btn_connect->setText("Kes");
    log_message("Ana ROV bağlantı başlatılıyor: " + port, 0);
}

void PixhawkGUI::update_ana_attitude(float roll, float pitch, float yaw) {
    PixhawkPanel *A = ui.anaRovPanel;
    A->lbl_roll->setText(QString::number(roll, 'f', 1) + "°");
    A->lbl_pitch->setText(QString::number(pitch, 'f', 1) + "°");
    A->lbl_yaw->setText(QString::number(yaw, 'f', 1) + "°");
    A->attitude_indicator->set_attitude(roll, pitch);
}

void PixhawkGUI::update_ana_vfr_hud(float alt, float heading, float speed) {
    PixhawkPanel *A = ui.anaRovPanel;
    A->lbl_alt->setText(QString::number(alt, 'f', 2));
    A->lbl_heading->setText(QString::number(heading, 'f', 0) + "°");
    A->lbl_speed->setText(QString::number(speed, 'f', 2));
    int v = qBound(0, static_cast<int>(alt * 5), 100);
    A->bar_alt->setValue(v);
}

void PixhawkGUI::update_ana_battery(float voltage, int remaining) {
    PixhawkPanel *A = ui.anaRovPanel;
    A->lbl_voltage->setText(QString::number(voltage, 'f', 2) + " V");
    A->lbl_battery->setText(QString::number(remaining) + " %");
}

void PixhawkGUI::update_ana_status(const QString &msg) {
    ui.anaRovPanel->lbl_status->setText("Durum: " + msg);
    log_message("[ANA] " + msg, 0);
}

// ==================== Mini ROV Bağlantı ====================
void PixhawkGUI::toggle_mini_connection() {
    PixhawkPanel *M = ui.miniRovPanel;
    if (mini_mav_thread) {
        mini_mav_thread->stop();
        delete mini_mav_thread;
        mini_mav_thread = nullptr;
        M->btn_connect->setText("Mini Rov Bağlantısı");
        M->lbl_status->setText("Durum: Bağlantı Kesildi");
        reset_labels(M);
        set_led(M->led_autonomous, false);
        log_message("Mini ROV bağlantısı kesildi.", 1);
        return;
    }
    QString port = M->port_combo->currentText();
    int baud = M->baud_combo->currentText().toInt();
    mini_mav_thread = new MAVLinkThread(port, baud, this);
    connect(mini_mav_thread, &MAVLinkThread::attitude_signal, this, &PixhawkGUI::update_mini_attitude);
    connect(mini_mav_thread, &MAVLinkThread::vfr_hud_signal,  this, &PixhawkGUI::update_mini_vfr_hud);
    connect(mini_mav_thread, &MAVLinkThread::battery_signal,  this, &PixhawkGUI::update_mini_battery);
    connect(mini_mav_thread, &MAVLinkThread::status_signal,   this, &PixhawkGUI::update_mini_status);
    mini_mav_thread->start();
    M->btn_connect->setText("Kes");
    set_led(M->led_autonomous, true);
    log_message("Mini ROV bağlantı başlatılıyor: " + port, 1);
}

void PixhawkGUI::update_mini_attitude(float roll, float pitch, float yaw) {
    PixhawkPanel *M = ui.miniRovPanel;
    M->lbl_roll->setText(QString::number(roll, 'f', 1) + "°");
    M->lbl_pitch->setText(QString::number(pitch, 'f', 1) + "°");
    M->lbl_yaw->setText(QString::number(yaw, 'f', 1) + "°");
    M->attitude_indicator->set_attitude(roll, pitch);
}

void PixhawkGUI::update_mini_vfr_hud(float alt, float heading, float speed) {
    PixhawkPanel *M = ui.miniRovPanel;
    M->lbl_alt->setText(QString::number(alt, 'f', 2));
    M->lbl_heading->setText(QString::number(heading, 'f', 0) + "°");
    M->lbl_speed->setText(QString::number(speed, 'f', 2));
    int v = qBound(0, static_cast<int>(alt * 5), 100);
    M->bar_alt->setValue(v);
}

void PixhawkGUI::update_mini_battery(float voltage, int remaining) {
    PixhawkPanel *M = ui.miniRovPanel;
    M->lbl_voltage->setText(QString::number(voltage, 'f', 2) + " V");
    M->lbl_battery->setText(QString::number(remaining) + " %");
}

void PixhawkGUI::update_mini_status(const QString &msg) {
    ui.miniRovPanel->lbl_status->setText("Durum: " + msg);
    log_message("[MİNİ] " + msg, 1);
}

// ==================== Joystick ====================
void PixhawkGUI::toggle_ana_joystick() {
    PixhawkPanel *A = ui.anaRovPanel;
    if (ana_joy_thread) {
        ana_joy_thread->stop();
        delete ana_joy_thread;
        ana_joy_thread = nullptr;
        A->btn_joy_connect->setText("Kol Bağla");
        A->lbl_pad_status->setText("Durum: Kol Bağlı Değil");
        return;
    }
    QString dev = A->joy_combo->currentText();
    ana_joy_thread = new JoystickThread(dev, this);
    connect(ana_joy_thread, &JoystickThread::status_signal, this, &PixhawkGUI::update_ana_joy_status);
    connect(ana_joy_thread, &JoystickThread::button_signal, this, &PixhawkGUI::update_ana_joy_button);
    connect(ana_joy_thread, &JoystickThread::axis_signal,   this, &PixhawkGUI::update_ana_joy_axis);
    ana_joy_thread->start();
    A->btn_joy_connect->setText("Kes");
}

void PixhawkGUI::toggle_mini_joystick() {
    PixhawkPanel *M = ui.miniRovPanel;
    if (mini_joy_thread) {
        mini_joy_thread->stop();
        delete mini_joy_thread;
        mini_joy_thread = nullptr;
        M->btn_joy_connect->setText("Kol Bağla");
        M->lbl_pad_status->setText("Durum: Kol Bağlı Değil");
        return;
    }
    QString dev = M->joy_combo->currentText();
    mini_joy_thread = new JoystickThread(dev, this);
    connect(mini_joy_thread, &JoystickThread::status_signal, this, &PixhawkGUI::update_mini_joy_status);
    connect(mini_joy_thread, &JoystickThread::button_signal, this, &PixhawkGUI::update_mini_joy_button);
    connect(mini_joy_thread, &JoystickThread::axis_signal,   this, &PixhawkGUI::update_mini_joy_axis);
    mini_joy_thread->start();
    M->btn_joy_connect->setText("Kes");
}

void PixhawkGUI::update_ana_joy_status(const QString &msg) {
    ui.anaRovPanel->lbl_pad_status->setText("Durum: " + msg);
    log_message("[ANA-KOL] " + msg, 0);
}

void PixhawkGUI::update_ana_joy_button(int btn_id, int state) {
    QString name = button_map.value(btn_id, QString("BTN%1").arg(btn_id));
    if (state) ana_pressed_buttons.insert(name);
    else       ana_pressed_buttons.remove(name);
    QString lst = ana_pressed_buttons.isEmpty() ? "Yok" :
                  QStringList(ana_pressed_buttons.values()).join(", ");
    ui.anaRovPanel->lbl_pad_buttons->setText("Basılan Tuş: " + lst);

    // Buton bazlı aksiyon: A=lamba aç, B=lamba kapat, Y=torpido, X=miniROV bırak
    if (state && ana_mav_thread) {
        if (name == "A")      on_lamp_on_ana();
        else if (name == "B") on_lamp_off_ana();
        else if (name == "Y") on_torpedo_fire();
        else if (name == "X") on_minirov_launch();
    }
}

void PixhawkGUI::update_ana_joy_axis(int axis_id, float value) {
    if (axis_id == 0) ana_axes_state[0] = value;
    if (axis_id == 1) ana_axes_state[1] = value;
    ui.anaRovPanel->lbl_pad_axes->setText(
        QString("Sol Stick: X: %1  Y: %2")
            .arg(ana_axes_state[0], 0, 'f', 2)
            .arg(ana_axes_state[1], 0, 'f', 2));

    // Eksen → PWM (1100-1900 arası, 1500 nötr)
    if (ana_mav_thread && !ana_autonomous) {
        int pwm_x = 1500 + static_cast<int>(ana_axes_state[0] * 400);
        int pwm_y = 1500 - static_cast<int>(ana_axes_state[1] * 400);
        ana_mav_thread->set_servo(1, pwm_x); // roll
        ana_mav_thread->set_servo(2, pwm_y); // pitch
    }
}

void PixhawkGUI::update_mini_joy_status(const QString &msg) {
    ui.miniRovPanel->lbl_pad_status->setText("Durum: " + msg);
    log_message("[MİNİ-KOL] " + msg, 1);
}

void PixhawkGUI::update_mini_joy_button(int btn_id, int state) {
    if (!minirov_launched) return;
    QString name = button_map.value(btn_id, QString("BTN%1").arg(btn_id));
    if (state) mini_pressed_buttons.insert(name);
    else       mini_pressed_buttons.remove(name);
    QString lst = mini_pressed_buttons.isEmpty() ? "Yok" :
                  QStringList(mini_pressed_buttons.values()).join(", ");
    ui.miniRovPanel->lbl_pad_buttons->setText("Basılan Tuş: " + lst);

    if (state && mini_mav_thread) {
        if (name == "A")      on_lamp_on_mini();
        else if (name == "B") on_lamp_off_mini();
    }
}

void PixhawkGUI::update_mini_joy_axis(int axis_id, float value) {
    if (!minirov_launched) return;
    if (axis_id == 0) mini_axes_state[0] = value;
    if (axis_id == 1) mini_axes_state[1] = value;
    ui.miniRovPanel->lbl_pad_axes->setText(
        QString("Sol Stick: X: %1  Y: %2")
            .arg(mini_axes_state[0], 0, 'f', 2)
            .arg(mini_axes_state[1], 0, 'f', 2));

    if (mini_mav_thread) {
        int pwm_x = 1500 + static_cast<int>(mini_axes_state[0] * 400);
        int pwm_y = 1500 - static_cast<int>(mini_axes_state[1] * 400);
        mini_mav_thread->set_servo(1, pwm_x);
        mini_mav_thread->set_servo(2, pwm_y);
    }
}

// ==================== Kamera Frame ====================
void PixhawkGUI::update_camera_frame(const QImage &img) {
    QLabel *lbl = ui.miniRovPanel->lbl_cam_stream;
    lbl->setPixmap(QPixmap::fromImage(img).scaled(lbl->size(),
                                                  Qt::KeepAspectRatio,
                                                  Qt::SmoothTransformation));
}

void PixhawkGUI::update_camera_status(const QString &msg) {
    log_message("[CAM-MİNİ] " + msg, 1);
}

void PixhawkGUI::update_camera_stats(int fps, float kbps, int w, int h) {
    PixhawkPanel *M = ui.miniRovPanel;
    M->lbl_cam_fps->setText(QString("FPS: %1").arg(fps));
    M->lbl_cam_bitrate->setText(QString("Veri Hızı: %1 KB/s").arg(kbps, 0, 'f', 1));
    M->lbl_cam_res->setText(QString("Çözünürlük: %1x%2").arg(w).arg(h));
}

void PixhawkGUI::update_anarov_frame(const QImage &img) {
    QLabel *lbl = ui.anaRovPanel->lbl_cam_stream;
    lbl->setPixmap(QPixmap::fromImage(img).scaled(lbl->size(),
                                                  Qt::KeepAspectRatio,
                                                  Qt::SmoothTransformation));
}

void PixhawkGUI::update_anarov_status(const QString &msg) {
    log_message("[CAM-ANA] " + msg, 0);
}

void PixhawkGUI::update_anarov_stats(int fps, float kbps, int w, int h) {
    PixhawkPanel *A = ui.anaRovPanel;
    A->lbl_cam_fps->setText(QString("FPS: %1").arg(fps));
    A->lbl_cam_bitrate->setText(QString("Veri Hızı: %1 KB/s").arg(kbps, 0, 'f', 1));
    A->lbl_cam_res->setText(QString("Çözünürlük: %1x%2").arg(w).arg(h));
}

// ==================== Ping ====================
void PixhawkGUI::read_cam_ping() {
    if (!cam_ping_proc) return;
    QByteArray data = cam_ping_proc->readAllStandardOutput();
    QString text = QString::fromUtf8(data);
    QRegExp re("time=([0-9.]+) ms");
    if (re.indexIn(text) >= 0) {
        ui.miniRovPanel->lbl_cam_ping->setText("Ping: " + re.cap(1) + " ms");
    }
}

void PixhawkGUI::read_anarov_ping() {
    if (!anarov_ping_proc) return;
    QByteArray data = anarov_ping_proc->readAllStandardOutput();
    QString text = QString::fromUtf8(data);
    QRegExp re("time=([0-9.]+) ms");
    if (re.indexIn(text) >= 0) {
        ui.anaRovPanel->lbl_cam_ping->setText("Ping: " + re.cap(1) + " ms");
    }
}

// ==================== Yön Butonları ====================
// PWM Kanalları: 1=Roll/Yan, 2=Pitch/İleri-Geri, 3=Throttle/Z, 4=Yaw/Dönme
void PixhawkGUI::ana_dir_pressed() {
    if (!ana_mav_thread) {
        log_message("[ANA] Önce bağlanın!", 0);
        return;
    }
    if (ana_autonomous) {
        log_message("[ANA] Otonom mod aktifken manuel hareket engellendi!", 0);
        return;
    }
    QPushButton *b = qobject_cast<QPushButton*>(sender());
    if (!b) return;
    PixhawkPanel *A = ui.anaRovPanel;
    if      (b == A->btn_forward)    { ana_mav_thread->set_servo(2, 1700); log_message("[ANA] İleri", 0); }
    else if (b == A->btn_backward)   { ana_mav_thread->set_servo(2, 1300); log_message("[ANA] Geri", 0); }
    else if (b == A->btn_left)       { ana_mav_thread->set_servo(1, 1300); log_message("[ANA] Sol", 0); }
    else if (b == A->btn_right)      { ana_mav_thread->set_servo(1, 1700); log_message("[ANA] Sağ", 0); }
    else if (b == A->btn_up)         { ana_mav_thread->set_servo(3, 1700); log_message("[ANA] Z+", 0); }
    else if (b == A->btn_down)       { ana_mav_thread->set_servo(3, 1300); log_message("[ANA] Z-", 0); }
    else if (b == A->btn_turn_left)  { ana_mav_thread->set_servo(4, 1300); log_message("[ANA] Sola Dön", 0); }
    else if (b == A->btn_turn_right) { ana_mav_thread->set_servo(4, 1700); log_message("[ANA] Sağa Dön", 0); }
}

void PixhawkGUI::ana_dir_released() {
    if (!ana_mav_thread) return;
    if (ana_autonomous) return;
    QPushButton *b = qobject_cast<QPushButton*>(sender());
    if (!b) return;
    PixhawkPanel *A = ui.anaRovPanel;
    if (b == A->btn_forward || b == A->btn_backward)       ana_mav_thread->set_servo(2, 1500);
    else if (b == A->btn_left || b == A->btn_right)        ana_mav_thread->set_servo(1, 1500);
    else if (b == A->btn_up || b == A->btn_down)           ana_mav_thread->set_servo(3, 1500);
    else if (b == A->btn_turn_left || b == A->btn_turn_right) ana_mav_thread->set_servo(4, 1500);
}

void PixhawkGUI::mini_dir_pressed() {
    if (!mini_mav_thread) {
        log_message("[MİNİ] Önce bağlanın!", 1);
        return;
    }
    if (!minirov_launched) {
        log_message("[MİNİ] Mini ROV henüz bırakılmadı! Hareket engellendi.", 1);
        return;
    }
    QPushButton *b = qobject_cast<QPushButton*>(sender());
    if (!b) return;
    PixhawkPanel *M = ui.miniRovPanel;
    if      (b == M->btn_forward)    { mini_mav_thread->set_servo(2, 1700); log_message("[MİNİ] İleri", 1); }
    else if (b == M->btn_backward)   { mini_mav_thread->set_servo(2, 1300); log_message("[MİNİ] Geri", 1); }
    else if (b == M->btn_left)       { mini_mav_thread->set_servo(1, 1300); log_message("[MİNİ] Sol", 1); }
    else if (b == M->btn_right)      { mini_mav_thread->set_servo(1, 1700); log_message("[MİNİ] Sağ", 1); }
    else if (b == M->btn_up)         { mini_mav_thread->set_servo(3, 1700); log_message("[MİNİ] Z+", 1); }
    else if (b == M->btn_down)       { mini_mav_thread->set_servo(3, 1300); log_message("[MİNİ] Z-", 1); }
    else if (b == M->btn_turn_left)  { mini_mav_thread->set_servo(4, 1300); log_message("[MİNİ] Sola Dön", 1); }
    else if (b == M->btn_turn_right) { mini_mav_thread->set_servo(4, 1700); log_message("[MİNİ] Sağa Dön", 1); }
}

void PixhawkGUI::mini_dir_released() {
    if (!mini_mav_thread) return;
    if (!minirov_launched) return;
    QPushButton *b = qobject_cast<QPushButton*>(sender());
    if (!b) return;
    PixhawkPanel *M = ui.miniRovPanel;
    if (b == M->btn_forward || b == M->btn_backward)       mini_mav_thread->set_servo(2, 1500);
    else if (b == M->btn_left || b == M->btn_right)        mini_mav_thread->set_servo(1, 1500);
    else if (b == M->btn_up || b == M->btn_down)           mini_mav_thread->set_servo(3, 1500);
    else if (b == M->btn_turn_left || b == M->btn_turn_right) mini_mav_thread->set_servo(4, 1500);
}

// ==================== Hızlı Komutlar ====================
void PixhawkGUI::on_emergency_ana() {
    if (ana_mav_thread) {
        ana_mav_thread->set_servo(1, 1500);
        ana_mav_thread->set_servo(2, 1500);
        ana_mav_thread->set_servo(3, 1500);
        ana_mav_thread->set_servo(4, 1500);
    }
    log_message("[ANA] ACİL DURDURMA tetiklendi!", 0);
}

void PixhawkGUI::on_emergency_mini() {
    if (mini_mav_thread) {
        mini_mav_thread->set_servo(1, 1500);
        mini_mav_thread->set_servo(2, 1500);
        mini_mav_thread->set_servo(3, 1500);
        mini_mav_thread->set_servo(4, 1500);
    }
    log_message("[MİNİ] ACİL DURDURMA tetiklendi!", 1);
}

void PixhawkGUI::on_stabilize_ana() {
    log_message("[ANA] Stabilize / Sabitle modu", 0);
    if (ana_mav_thread) ana_mav_thread->arm_vehicle();
}

void PixhawkGUI::on_stabilize_mini() {
    log_message("[MİNİ] Stabilize / Sabitle modu", 1);
    if (mini_mav_thread) mini_mav_thread->arm_vehicle();
}

void PixhawkGUI::on_autonomous_ana() {
    ana_autonomous = true;
    set_led(ui.anaRovPanel->led_autonomous, true);
    set_led(ui.anaRovPanel->led_manual, false);
    log_message("[ANA] Otonom moda geçildi", 0);
}

void PixhawkGUI::on_manual_ana() {
    ana_autonomous = false;
    set_led(ui.anaRovPanel->led_manual, true);
    set_led(ui.anaRovPanel->led_autonomous, false);
    log_message("[ANA] Manuel moda geçildi", 0);
}

void PixhawkGUI::on_minirov_launch() {
    if (ana_mav_thread) ana_mav_thread->set_servo(7, 2000);
    minirov_launched = true;
    set_led(ui.anaRovPanel->led_minirov, true);
    log_message("[ANA] MiniROV Bırakıldı!", 0);
}

void PixhawkGUI::on_torpedo_fire() {
    if (torpedo_ready) {
        if (ana_mav_thread) ana_mav_thread->set_servo(8, 2000);
        torpedo_ready = false;
        set_led(ui.anaRovPanel->led_torpedo, true); // Active state (Green)
        ui.anaRovPanel->btn_torpedo->setText("Torpidoyu\nHazırla");
        log_message("[ANA] Torpido Fırlatıldı!", 0);
    } else {
        if (ana_mav_thread) ana_mav_thread->set_servo(8, 1000);
        torpedo_ready = true;
        set_led(ui.anaRovPanel->led_torpedo, false); // Inactive state (Red)
        ui.anaRovPanel->btn_torpedo->setText("Torpido\nFırlat");
        log_message("[ANA] Torpido Yeniden Hazırlandı!", 0);
    }
}

void PixhawkGUI::on_lamp_on_ana() {
    if (ana_mav_thread) {
        ana_mav_thread->set_servo(5, 2000);
        ana_mav_thread->set_servo(6, 2000);
    }
    ana_lamp_on = true;
    set_led(ui.anaRovPanel->led_lamp_on, true);
    set_led(ui.anaRovPanel->led_lamp_off, false);
    ui.anaRovPanel->lbl_lamp_status->setText("Açık");
    ui.anaRovPanel->lbl_lamp_status->setStyleSheet("color:#2ecc71;font-weight:bold;font-size:14px;");
    log_message("[ANA] Lamba Açıldı", 0);
}

void PixhawkGUI::on_lamp_off_ana() {
    if (ana_mav_thread) {
        ana_mav_thread->set_servo(5, 1000);
        ana_mav_thread->set_servo(6, 1000);
    }
    ana_lamp_on = false;
    set_led(ui.anaRovPanel->led_lamp_on, false);
    set_led(ui.anaRovPanel->led_lamp_off, true);
    ui.anaRovPanel->lbl_lamp_status->setText("Kapalı");
    ui.anaRovPanel->lbl_lamp_status->setStyleSheet("color:#c0392b;font-weight:bold;font-size:14px;");
    log_message("[ANA] Lamba Kapatıldı", 0);
}

void PixhawkGUI::on_lamp_on_mini() {
    if (mini_mav_thread) {
        mini_mav_thread->set_servo(5, 2000);
        mini_mav_thread->set_servo(6, 2000);
    }
    mini_lamp_on = true;
    set_led(ui.miniRovPanel->led_lamp_on, true);
    set_led(ui.miniRovPanel->led_lamp_off, false);
    ui.miniRovPanel->lbl_lamp_status->setText("Açık");
    ui.miniRovPanel->lbl_lamp_status->setStyleSheet("color:#2ecc71;font-weight:bold;font-size:14px;");
    log_message("[MİNİ] Lamba Açıldı", 1);
}

void PixhawkGUI::on_lamp_off_mini() {
    if (mini_mav_thread) {
        mini_mav_thread->set_servo(5, 1000);
        mini_mav_thread->set_servo(6, 1000);
    }
    mini_lamp_on = false;
    set_led(ui.miniRovPanel->led_lamp_on, false);
    set_led(ui.miniRovPanel->led_lamp_off, true);
    ui.miniRovPanel->lbl_lamp_status->setText("Kapalı");
    ui.miniRovPanel->lbl_lamp_status->setStyleSheet("color:#c0392b;font-weight:bold;font-size:14px;");
    log_message("[MİNİ] Lamba Kapatıldı", 1);
}

// ==================== Yardımcılar ====================
void PixhawkGUI::reset_labels(PixhawkPanel *panel) {
    panel->lbl_roll->setText("---");
    panel->lbl_pitch->setText("---");
    panel->lbl_yaw->setText("---");
    panel->lbl_alt->setText("---");
    panel->lbl_heading->setText("---");
    panel->lbl_speed->setText("---");
    panel->lbl_voltage->setText("---");
    panel->lbl_battery->setText("---");
    panel->attitude_indicator->set_attitude(0, 0);
    panel->bar_alt->setValue(0);
}

void PixhawkGUI::set_led(QLabel *led, bool on) {
    if (!led) return;
    
    bool isGreen = led->property("isGreen").toBool();
    QLabel *redPartner = static_cast<QLabel*>(led->property("redPartner").value<void*>());
    
    if (redPartner) {
        // This is a dual LED button (Otonom, Manuel, MiniROV Bırak, Torpido Fırlat, Mini Rov Bağlantısı)
        // led is the Green LED, redPartner is the Red LED
        if (on) {
            // Active state: Green LED is bright, Red LED is dim
            led->setStyleSheet("background-color: #2ecc71; border: 2px solid #27ae60; border-radius: 7px;");
            redPartner->setStyleSheet("background-color: #2e1515; border: 2px solid #1f0d0d; border-radius: 7px;");
        } else {
            // Inactive state: Green LED is dim, Red LED is bright
            led->setStyleSheet("background-color: #152e1a; border: 2px solid #0d1f11; border-radius: 7px;");
            redPartner->setStyleSheet("background-color: #e74c3c; border: 2px solid #c0392b; border-radius: 7px;");
        }
    } else {
        // Single LED (e.g. Lamp Aç/Kapa separate LEDs)
        if (isGreen) {
            if (on) {
                led->setStyleSheet("background-color: #2ecc71; border: 2px solid #27ae60; border-radius: 8px;");
            } else {
                led->setStyleSheet("background-color: #152e1a; border: 2px solid #0d1f11; border-radius: 8px;");
            }
        } else {
            if (on) {
                led->setStyleSheet("background-color: #e74c3c; border: 2px solid #c0392b; border-radius: 8px;");
            } else {
                led->setStyleSheet("background-color: #2e1515; border: 2px solid #1f0d0d; border-radius: 8px;");
            }
        }
    }
    
    // Dynamically style the corresponding button for feedback
    PixhawkPanel *panel = nullptr;
    if (led == ui.anaRovPanel->led_autonomous || led == ui.anaRovPanel->led_manual ||
        led == ui.anaRovPanel->led_minirov || led == ui.anaRovPanel->led_torpedo ||
        led == ui.anaRovPanel->led_lamp_on || led == ui.anaRovPanel->led_lamp_off) {
        panel = ui.anaRovPanel;
    } else if (led == ui.miniRovPanel->led_autonomous || led == ui.miniRovPanel->led_manual ||
               led == ui.miniRovPanel->led_minirov || led == ui.miniRovPanel->led_torpedo ||
               led == ui.miniRovPanel->led_lamp_on || led == ui.miniRovPanel->led_lamp_off) {
        panel = ui.miniRovPanel;
    }
    
    if (panel) {
        // High-tech, glowing active buttons, premium dark inactive buttons
        QString activeStyle = "background-color: #10b981; color: white; border: 2px solid #047857; font-weight: bold; font-size: 13px; border-radius: 4px;";
        QString inactiveStyle = "background-color: #334155; color: white; border: 1px solid #1e293b; font-weight: bold; font-size: 13px; border-radius: 4px;";
        QString lampOnActiveStyle = "background-color: #fbbf24; color: #111827; border: 2px solid #d97706; font-weight: bold; font-size: 13px; border-radius: 4px;";
        QString lampOffActiveStyle = "background-color: #ef4444; color: white; border: 2px solid #b91c1c; font-weight: bold; font-size: 13px; border-radius: 4px;";
        
        if (led == panel->led_autonomous) {
            if (panel->btn_autonomous) {
                panel->btn_autonomous->setStyleSheet(on ? activeStyle : inactiveStyle);
            } else if (panel->btn_connect) {
                panel->btn_connect->setStyleSheet(on ? activeStyle : inactiveStyle);
            }
        } else if (led == panel->led_manual) {
            if (panel->btn_manual) {
                panel->btn_manual->setStyleSheet(on ? activeStyle : inactiveStyle);
            }
        } else if (led == panel->led_minirov && panel->btn_minirov_launch) {
            panel->btn_minirov_launch->setStyleSheet(on ? activeStyle : inactiveStyle);
        } else if (led == panel->led_torpedo && panel->btn_torpedo) {
            panel->btn_torpedo->setStyleSheet(on ? activeStyle : inactiveStyle);
        } else if (led == panel->led_lamp_on) {
            panel->btn_lamp_on->setStyleSheet(on ? lampOnActiveStyle : inactiveStyle);
        } else if (led == panel->led_lamp_off) {
            panel->btn_lamp_off->setStyleSheet(on ? lampOffActiveStyle : inactiveStyle);
        }
    }
}

void PixhawkGUI::toggle_theme() {
    dark_mode = !dark_mode;
    ui.apply_styles(this, dark_mode);
    ui.apply_styles(centralWidget(), dark_mode);
    if (dark_mode) {
        ui.btn_theme->setText("☀️ Aydınlık Mod");
        ui.btn_theme->setStyleSheet("font-size: 13px; font-weight: bold; background-color: #e2e8f0; color: #0b0f19; border: none; border-radius: 4px; padding: 6px 12px;");
        log_message("Koyu Tema Aktif Edildi.");
    } else {
        ui.btn_theme->setText("🌙 Koyu Mod");
        ui.btn_theme->setStyleSheet("font-size: 13px; font-weight: bold; background-color: #334155; color: white; border: none; border-radius: 4px; padding: 6px 12px;");
        log_message("Aydınlık Tema Aktif Edildi.");
    }
}