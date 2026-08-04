#include "RovGUI.hpp"
#include "MotorMixer.hpp"
#include "SettingsDialog.hpp"

#include <QDateTime>
#include <QCloseEvent>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QStatusBar>
#include <QRegExp>
#include <QShortcut>
#include <QKeySequence>
#include <QPainter>
#include <QFontMetrics>

namespace {
// Logitech tipi kumandalarda tipik eksen numaralari. Kumandan farkli davranirsa
// "jstest /dev/input/js0" ile gercek eksen numaralarini gor ve asagidakileri guncelle.
constexpr int AXIS_LEFT_X  = 0; // Sol stick X  -> yanal kayma (Sol/Sağ)
constexpr int AXIS_LEFT_Y  = 1; // Sol stick Y  -> ileri/geri
constexpr int AXIS_RIGHT_X = 3; // Sağ stick X  -> dönüş (yaw)
constexpr int AXIS_RIGHT_Y = 4; // Sağ stick Y  -> yukarı/aşağı (derinlik)

// Yön butonları bırakıldığında motorların çekileceği durma darbesi.
// settings.motorNeutralUs'tan (joystick/otonom merkezi, Ayarlar penceresinden
// canlı değiştirilebilir) BİLEREK ayrı tutuluyor: buton bırakılınca
// motorların tam durmayıp hafif dönmeye devam etmesi (bkz.
// ana_dir_released()/mini_dir_released()) bench'te gözlendi, bu değer
// sadece o durum için ayrıca ayarlanabilsin diye.
constexpr int BUTTON_STOP_US = 1478;
}

RovGUI::RovGUI(QWidget *parent)
    : QMainWindow(parent),
      ana_esp_thread(nullptr), mini_esp_thread(nullptr),
      ana_joy_thread(nullptr), mini_joy_thread(nullptr),
      cam_thread(nullptr), anarov_thread(nullptr),
      ana_lamp_on(false), mini_lamp_on(false), ana_autonomous(false), minirov_launched(false), dark_mode(false),
      ana_armed(false), mini_armed(false), ana_cam_connected(false),
      ana_stabilize(false), ana_roll(0.0f), ana_pitch(0.0f), ana_yaw(0.0f),
      ana_current_depth(0.0f), ana_depth_target(0.0f),
      cam_ping_proc(nullptr), anarov_ping_proc(nullptr),
      log_file(nullptr), log_stream(nullptr),
      motor_heartbeat_timer(nullptr)
{
    settings = RovSettings::load();

    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    ui.setupUi(central);

    for (int i = 0; i < 4; ++i) {
        ana_axes_state[i] = 0.0f;
        mini_axes_state[i] = 0.0f;
    }
    for (int i = 0; i < 3; ++i) ana_torpedo_fired[i] = false;
    ana_last_pulses = settings.motorNeutralUs;
    mini_last_pulses = settings.motorNeutralUs;
    ui.anaRovPanel->motor_diagram->set_neutral_us(settings.motorNeutralUs);
    ui.miniRovPanel->motor_diagram->set_neutral_us(settings.motorNeutralUs);

    // Uygulama fullscreen açıldığı için pencere çerçevesi/kapatma düğmesi görünmez;
    // Ctrl+Q veya Esc ile çıkış kısayolu (Alt+F4 zaten pencere yöneticisi tarafından desteklenir)
    QShortcut *quitShortcutCtrlQ = new QShortcut(QKeySequence("Ctrl+Q"), this);
    connect(quitShortcutCtrlQ, &QShortcut::activated, this, &QWidget::close);
    QShortcut *quitShortcutEsc = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(quitShortcutEsc, &QShortcut::activated, this, &QWidget::close);

    // SPACE = ACİL DURDURMA (iki araçta birden motor nötrle + DISARM).
    // ApplicationShortcut: odak hangi widget'ta olursa olsun çalışır; yan
    // etkisi Space'in artık odaklı düğmeleri "tıklamaması"dır - E-STOP için
    // bilinçli tercih.
    QShortcut *estopShortcut = new QShortcut(QKeySequence(Qt::Key_Space), this);
    estopShortcut->setContext(Qt::ApplicationShortcut);
    connect(estopShortcut, &QShortcut::activated, this, &RovGUI::on_global_emergency);

    // Uyari mesaji süresi dolup statusBar bosalinca kirmizi stili de kaldir
    connect(statusBar(), &QStatusBar::messageChanged, this, [this](const QString &msg) {
        if (msg.isEmpty()) statusBar()->setStyleSheet("");
    });

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

    // Kamera thread'lerini ve ping süreçlerini başlat (ayarlardaki IP'lerle;
    // ayarlar değişince open_settings() bunları yeniden başlatır)
    start_camera_threads();
    start_ping_processes();

    // Initialize default states for quick commands on GUI startup
    // Manuel active by default, Otonom inactive
    set_led(ui.anaRovPanel->led_manual, true);
    set_led(ui.anaRovPanel->led_autonomous, false);
    if (ui.anaRovPanel->led_minirov) set_led(ui.anaRovPanel->led_minirov, false);
    if (ui.anaRovPanel->led_torpedo1) set_led(ui.anaRovPanel->led_torpedo1, false);
    if (ui.anaRovPanel->led_torpedo2) set_led(ui.anaRovPanel->led_torpedo2, false);
    if (ui.anaRovPanel->led_torpedo3) set_led(ui.anaRovPanel->led_torpedo3, false);
    set_led(ui.anaRovPanel->led_lamp_on, false);
    set_led(ui.anaRovPanel->led_lamp_off, true);

    set_led(ui.miniRovPanel->led_lamp_on, false);
    set_led(ui.miniRovPanel->led_lamp_off, true);

    // ESP32 tarafindaki FAILSAFE_MS 500ms; kumanda/tus basili tutulup eksen/buton
    // olayi tekrar tetiklenmeyince (stick sabit konumda) yeni komut gitmiyor ve
    // motorlar notrleniyordu. 150ms'de bir son komutu tazeleyerek bunu onluyoruz.
    motor_heartbeat_timer = new QTimer(this);
    connect(motor_heartbeat_timer, &QTimer::timeout, this, &RovGUI::send_motor_heartbeat);
    motor_heartbeat_timer->start(150);

    // Ana ROV sabit IP/port'a baglandigi (elle secilecek bir port/baudrate yok)
    // icin uygulama acilir acilmaz otomatik baglanir - elle "Bağlan" tiklamaya
    // gerek kalmaz. Mini ROV gercek seri port sectirdigi icin otomatik
    // baglanmiyor, kullanici port'u secip elle baglanmaya devam ediyor.
    toggle_ana_connection();
}

RovGUI::~RovGUI() {
    if (ana_esp_thread)  { ana_esp_thread->stop();  delete ana_esp_thread; }
    if (mini_esp_thread) { mini_esp_thread->stop(); delete mini_esp_thread; }
    if (ana_joy_thread)  { ana_joy_thread->stop();  delete ana_joy_thread; }
    if (mini_joy_thread) { mini_joy_thread->stop(); delete mini_joy_thread; }
    if (cam_thread)      { cam_thread->stop();      delete cam_thread; }
    if (anarov_thread)   { anarov_thread->stop();   delete anarov_thread; }

    if (cam_ping_proc)    cam_ping_proc->kill();
    if (anarov_ping_proc) anarov_ping_proc->kill();

    if (log_stream) { delete log_stream; log_stream = nullptr; }
    if (log_file)   { log_file->close(); delete log_file; log_file = nullptr; }
}

void RovGUI::closeEvent(QCloseEvent *event) {
    QMessageBox::StandardButton resBtn = QMessageBox::question(this, "SuGaca",
                                                                 "Çıkmak istediğinize emin misiniz?",
                                                                 QMessageBox::No | QMessageBox::Yes,
                                                                 QMessageBox::No);
    if (resBtn != QMessageBox::Yes) {
        event->ignore();
    } else {
        // ~RovGUI() zaten ana_esp_thread/mini_esp_thread->stop() uzerinden
        // disarm() gonderiyor (bkz. EspRovThread::stop()), ama burada da
        // acikca cagirmak niyeti nettesir ve komutu bir an once kuyruga
        // sokar - motorlar kapanis sirasinda armed KALMASIN diye.
        if (ana_esp_thread)  ana_esp_thread->disarm();
        if (mini_esp_thread) mini_esp_thread->disarm();
        log_message("Uygulama Kapatılıyor - motorlar DISARM ediliyor...");
        event->accept();
    }
}

void RovGUI::connect_signals() {
    RovPanel *A = ui.anaRovPanel;
    RovPanel *M = ui.miniRovPanel;

    // Bağlantı butonları
    connect(A->btn_connect, &QPushButton::clicked, this, &RovGUI::toggle_ana_connection);
    connect(M->btn_connect, &QPushButton::clicked, this, &RovGUI::toggle_mini_connection);

    // Joystick bağlantı butonları
    connect(A->btn_joy_connect, &QPushButton::clicked, this, &RovGUI::toggle_ana_joystick);
    connect(M->btn_joy_connect, &QPushButton::clicked, this, &RovGUI::toggle_mini_joystick);

    // Yön butonları (pressed/released)
    auto bindDir = [this](QPushButton *b, void (RovGUI::*p)(), void (RovGUI::*r)()) {
        connect(b, &QPushButton::pressed,  this, p);
        connect(b, &QPushButton::released, this, r);
    };
    for (QPushButton *b : {A->btn_forward, A->btn_backward, A->btn_left, A->btn_right,
                            A->btn_up, A->btn_down, A->btn_turn_left, A->btn_turn_right}) {
        bindDir(b, &RovGUI::ana_dir_pressed, &RovGUI::ana_dir_released);
    }
    for (QPushButton *b : {M->btn_forward, M->btn_backward, M->btn_left, M->btn_right,
                            M->btn_up, M->btn_down, M->btn_turn_left, M->btn_turn_right}) {
        bindDir(b, &RovGUI::mini_dir_pressed, &RovGUI::mini_dir_released);
    }

    // Hızlı komutlar
    connect(A->btn_emergency,      &QPushButton::clicked, this, &RovGUI::on_emergency_ana);
    connect(M->btn_emergency,      &QPushButton::clicked, this, &RovGUI::on_emergency_mini);
    connect(A->btn_stabilize,      &QPushButton::clicked, this, &RovGUI::on_stabilize_ana);
    connect(M->btn_stabilize,      &QPushButton::clicked, this, &RovGUI::on_stabilize_mini);
    connect(A->btn_autonomous,     &QPushButton::clicked, this, &RovGUI::on_autonomous_ana);
    connect(A->btn_manual,         &QPushButton::clicked, this, &RovGUI::on_manual_ana);
    connect(A->btn_minirov_launch, &QPushButton::clicked, this, &RovGUI::on_minirov_launch);
    connect(A->btn_torpedo1,       &QPushButton::clicked, this, [this]() { on_torpedo_fire(0); });
    connect(A->btn_torpedo2,       &QPushButton::clicked, this, [this]() { on_torpedo_fire(1); });
    connect(A->btn_torpedo3,       &QPushButton::clicked, this, [this]() { on_torpedo_fire(2); });
    connect(A->btn_lamp_on,        &QPushButton::clicked, this, &RovGUI::on_lamp_on_ana);
    connect(A->btn_lamp_off,       &QPushButton::clicked, this, &RovGUI::on_lamp_off_ana);
    connect(M->btn_lamp_on,        &QPushButton::clicked, this, &RovGUI::on_lamp_on_mini);
    connect(M->btn_lamp_off,       &QPushButton::clicked, this, &RovGUI::on_lamp_off_mini);

    // Temizlik / Tema Değişimi
    connect(ui.btn_theme, &QPushButton::clicked, this, &RovGUI::toggle_theme);
    connect(ui.btn_settings, &QPushButton::clicked, this, &RovGUI::open_settings);

    // Snapshot / video kaydı
    connect(A->btn_snapshot, &QPushButton::clicked, this, &RovGUI::on_ana_snapshot);
    connect(A->btn_record,   &QPushButton::clicked, this, &RovGUI::toggle_ana_record);
    connect(M->btn_snapshot, &QPushButton::clicked, this, &RovGUI::on_mini_snapshot);
    connect(M->btn_record,   &QPushButton::clicked, this, &RovGUI::toggle_mini_record);
}

void RovGUI::start_camera_threads() {
    // Ayarlar degisince yeniden cagrilabilir - once mevcutlari durdur
    if (cam_thread)    { cam_thread->stop();    delete cam_thread;    cam_thread = nullptr; }
    if (anarov_thread) { anarov_thread->stop(); delete anarov_thread; anarov_thread = nullptr; }

    // MiniROV kamera (MJPEG @ http://<ip>/stream)
    cam_thread = new CameraThread(settings.miniCamIp, this);
    connect(cam_thread, &CameraThread::image_signal,  this, &RovGUI::update_camera_frame);
    connect(cam_thread, &CameraThread::status_signal, this, &RovGUI::update_camera_status);
    connect(cam_thread, &CameraThread::stats_signal,  this, &RovGUI::update_camera_stats);
    connect(cam_thread, &CameraThread::jpeg_signal,   this, &RovGUI::on_mini_jpeg);
    cam_thread->start();

    // AnaROV kamera (MJPEG @ http://<addr>/stream - bkz. AnaRovKamera/AnaRovKamera.ino,
    // Espressif CameraWebServer ornegi: ana sunucu 80'de, stream sunucusu 80+1=81'de acilir)
    anarov_thread = new CameraThread(settings.anaCamAddr, this);
    connect(anarov_thread, &CameraThread::image_signal,  this, &RovGUI::update_anarov_frame);
    connect(anarov_thread, &CameraThread::status_signal, this, &RovGUI::update_anarov_status);
    connect(anarov_thread, &CameraThread::stats_signal,  this, &RovGUI::update_anarov_stats);
    connect(anarov_thread, &CameraThread::jpeg_signal,   this, &RovGUI::on_ana_jpeg);
    anarov_thread->start();
}

void RovGUI::start_ping_processes() {
    // Ayarlar degisince yeniden cagrilabilir - once mevcutlari durdur
    if (cam_ping_proc)    { cam_ping_proc->kill();    cam_ping_proc->deleteLater();    cam_ping_proc = nullptr; }
    if (anarov_ping_proc) { anarov_ping_proc->kill(); anarov_ping_proc->deleteLater(); anarov_ping_proc = nullptr; }

    cam_ping_proc = new QProcess(this);
    connect(cam_ping_proc, &QProcess::readyReadStandardOutput,
            this, &RovGUI::read_cam_ping);
    cam_ping_proc->start("ping", QStringList() << "-i" << "1" << settings.miniCamIp);

    anarov_ping_proc = new QProcess(this);
    connect(anarov_ping_proc, &QProcess::readyReadStandardOutput,
            this, &RovGUI::read_anarov_ping);
    anarov_ping_proc->start("ping", QStringList() << "-i" << "1" << settings.anaHost);
}

void RovGUI::open_settings() {
    SettingsDialog dlg(settings, this);
    connect(&dlg, &SettingsDialog::torpedo_reset_requested, this, [this]() {
        if (!ana_esp_thread) { log_message("[ANA] Önce bağlanın!", 0); return; }
        ana_esp_thread->torpedo_reset();
        for (int i = 0; i < 3; ++i) ana_torpedo_fired[i] = false;
        log_message("[ANA] Torpido sayacı sıfırlama komutu gönderildi.", 0);
    });
    if (dlg.exec() != QDialog::Accepted) return;

    RovSettings fresh = dlg.values();
    bool camsChanged = fresh.miniCamIp != settings.miniCamIp ||
                       fresh.anaCamAddr != settings.anaCamAddr ||
                       fresh.anaHost != settings.anaHost;
    settings = fresh;
    settings.save();
    ui.anaRovPanel->motor_diagram->set_neutral_us(settings.motorNeutralUs);
    ui.miniRovPanel->motor_diagram->set_neutral_us(settings.motorNeutralUs);

    // Torpido darbeleri ESP32'de calisiyor (ana motor min/max/notr gibi
    // GUI'de degil) - bagliysa degisikligi hemen gonder, ARM beklemeye gerek yok.
    if (ana_esp_thread) {
        ana_esp_thread->torpedo_config(settings.torpedoMinUs, settings.torpedoNeutralUs, settings.torpedoMaxUs);
    }

    // Kazanclar zaten settings uzerinden okundugu icin aninda etkili;
    // kamera/ping IP degisikligi thread/surec yeniden baslatmayi gerektirir.
    if (camsChanged) {
        start_camera_threads();
        start_ping_processes();
        log_message("Ayarlar kaydedildi - kamera bağlantıları yeni IP'lerle yeniden başlatıldı.");
    } else {
        log_message("Ayarlar kaydedildi.");
    }
    if (ana_esp_thread) {
        log_message("Not: köprü IP/port değişikliği bir sonraki bağlanmada geçerli olur.", 0);
    }
}

void RovGUI::show_screen_warning(const QString &msg) {
    statusBar()->setStyleSheet("background-color:#dc2626;color:white;font-weight:bold;font-size:14px;padding:4px;");
    statusBar()->showMessage("⚠ " + msg, 6000);
}

// ==================== Log ====================
void RovGUI::log_message(const QString &msg, int target) {
    QString ts = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    QString line = QString("[%1] %2").arg(ts, msg);

    if (target == 0 || target == -1) ui.anaRovPanel->terminal_log->append(line);
    if (target == 1 || target == -1) ui.miniRovPanel->terminal_log->append(line);

    if (log_stream) {
        *log_stream << line << "\n";
        log_stream->flush();
    }
}

// ==================== ESP32 8 motor karisimi ====================
void RovGUI::apply_ana_motor_mix(float surge, float lateral, float yaw, float vertical) {
    // ARM edilmeden (ana_armed=false) motor komutu gonderilmesin - ESP32
    // firmware'i zaten ARMED olmayan durumda M: komutlarini reddediyor
    // (ERR:NOTARMED), ama bunu burada da kontrol etmek ikinci bir guvenlik
    // katmani: ornegin ESP32 onceki bir oturumdan hala fiilen armed kalmis
    // olsa bile (GUI yeniden baglaninca ana_armed sifirlanir) operator bu
    // oturumda ARM'a basmadan kumandayla motor hareket ettiremez.
    if (!ana_esp_thread || !ana_armed) return;
    float rollCorr = 0.0f, pitchCorr = 0.0f;
    if (ana_stabilize) {
        rollCorr  = qBound(-1.0f, (ana_roll  / 90.0f) * settings.stabGain, 1.0f);
        pitchCorr = qBound(-1.0f, (ana_pitch / 90.0f) * settings.stabGain, 1.0f);
    }
    auto pulses = MotorMixer::compute(surge, lateral, yaw, vertical, rollCorr, pitchCorr,
                                       settings.motorNeutralUs, settings.minUs, settings.maxUs);
    ana_esp_thread->set_motors(pulses);
    ana_last_pulses = pulses;

    // Motor diyagraminda hangi motorlara ne kadar stabilize duzeltmesi
    // gittigini gostermek icin (bkz. MotorMixer::compute'taki M5-M8 isaretleri).
    std::array<int, 8> correctionUs{};
    correctionUs[4] = static_cast<int>(( rollCorr - pitchCorr) * MotorMixer::DELTA_US); // M5 on-sag
    correctionUs[5] = static_cast<int>((-rollCorr - pitchCorr) * MotorMixer::DELTA_US); // M6 on-sol
    correctionUs[6] = static_cast<int>(( rollCorr + pitchCorr) * MotorMixer::DELTA_US); // M7 arka-sag
    correctionUs[7] = static_cast<int>((-rollCorr + pitchCorr) * MotorMixer::DELTA_US); // M8 arka-sol
    ui.anaRovPanel->motor_diagram->set_motor_pulses(pulses, correctionUs);
}

void RovGUI::apply_mini_motor_mix(float surge, float lateral, float yaw, float vertical) {
    if (!mini_esp_thread) return;
    auto pulses = MotorMixer::compute(surge, lateral, yaw, vertical, 0.0f, 0.0f,
                                       settings.motorNeutralUs, settings.minUs, settings.maxUs);
    mini_esp_thread->set_motors(pulses);
    mini_last_pulses = pulses;
    ui.miniRovPanel->motor_diagram->set_motor_pulses(pulses);
}

void RovGUI::send_motor_heartbeat() {
    if (ana_esp_thread && ana_armed) ana_esp_thread->set_motors(ana_last_pulses);
    if (mini_esp_thread && mini_armed) mini_esp_thread->set_motors(mini_last_pulses);
}

// ==================== Ana ROV Bağlantı (ESP32 seri port) ====================
void RovGUI::toggle_ana_connection() {
    RovPanel *A = ui.anaRovPanel;
    QString activeStyle = "background-color: #10b981; color: white; border: 2px solid #047857; font-weight: bold; font-size: 13px; border-radius: 4px;";

    if (ana_esp_thread) {
        ana_esp_thread->stop();
        delete ana_esp_thread;
        ana_esp_thread = nullptr;
        ana_armed = false;
        ana_stabilize = false;
        ana_roll = ana_pitch = 0.0f;
        if (A->lbl_stabilize_status) {
            A->lbl_stabilize_status->setText("Kapalı");
            A->lbl_stabilize_status->setStyleSheet("color:#c0392b;font-weight:bold;font-size:14px;");
        }
        A->btn_connect->setText("Bağlan");
        A->btn_connect->setStyleSheet("");
        A->lbl_status->setText("Durum: Bağlantı Kesildi");
        A->btn_stabilize->setText("ARM Et");
        A->btn_stabilize->setStyleSheet("");
        reset_labels(A);
        log_message("Ana ROV (ESP32) bağlantısı kesildi.", 0);
        return;
    }
    // Ana ROV artik USB yerine kamera ESP'sindeki (AnaRovKamera/AnaRovKamera.ino) TCP
    // koprusune baglaniyor - o da komutlari Serial2 uzerinden motor ESP'sine
    // iletir (bkz. EspRovThread.hpp basindaki aciklama). port_combo/baud_combo
    // artik Ana ROV icin kullanilmiyor (Mini ROV hala USB/seri, degismedi).
    const QString anaRovHost = settings.anaHost;
    const quint16 anaRovBridgePort = settings.anaBridgePort;
    ana_esp_thread = EspRovThread::createTcp(anaRovHost, anaRovBridgePort, this);
    connect(ana_esp_thread, &EspRovThread::status_signal, this, &RovGUI::update_ana_status);
    connect(ana_esp_thread, &EspRovThread::armed_signal,  this, &RovGUI::update_ana_armed);
    connect(ana_esp_thread, &EspRovThread::attitude_signal, this, &RovGUI::update_ana_attitude);
    connect(ana_esp_thread, &EspRovThread::depth_signal, this, &RovGUI::update_ana_depth);
    connect(ana_esp_thread, &EspRovThread::nem_signal, this, &RovGUI::update_ana_nem);
    connect(ana_esp_thread, &EspRovThread::battery_signal, this, &RovGUI::update_ana_battery);
    connect(ana_esp_thread, &EspRovThread::torpedo_signal, this, &RovGUI::update_ana_torpedo);
    ana_esp_thread->start();
    A->btn_connect->setText("Kes");
    A->btn_connect->setStyleSheet(activeStyle);
    log_message(QString("Ana ROV bağlantısı başlatılıyor: %1:%2").arg(anaRovHost).arg(anaRovBridgePort), 0);
}

void RovGUI::update_ana_status(const QString &msg) {
    ui.anaRovPanel->lbl_status->setText("Durum: " + msg);
    log_message("[ANA] " + msg, 0);
}

void RovGUI::update_ana_armed(bool armed) {
    ana_armed = armed;
    RovPanel *A = ui.anaRovPanel;
    if (armed) {
        A->btn_stabilize->setText("DISARM Et");
        A->btn_stabilize->setStyleSheet("background-color:#dc2626;color:white;font-weight:bold;border:1px solid #991b1b;border-radius:4px;font-size:16px;");
        log_message("[ANA] ARMED - ESC'ler hazır.", 0);
        // Ayarlar penceresinden degistirilmis torpido darbelerini (bkz.
        // RovSettings::torpedoMinUs/torpedoNeutralUs/torpedoMaxUs) her ARM'da
        // ESP32'ye tazeler - baglanti ARM'dan once koptuysa/ESP32 resetlendiyse
        // kendi varsayilanlarina donmus olabilir.
        if (ana_esp_thread) ana_esp_thread->torpedo_config(settings.torpedoMinUs, settings.torpedoNeutralUs, settings.torpedoMaxUs);
    } else {
        A->btn_stabilize->setText("ARM Et");
        A->btn_stabilize->setStyleSheet("");
        log_message("[ANA] DISARMED - motorlara sinyal kesildi.", 0);
    }
}

void RovGUI::update_ana_attitude(float roll, float pitch, float yaw) {
    ana_roll = roll;
    ana_pitch = pitch;
    ana_yaw = yaw;
    RovPanel *A = ui.anaRovPanel;
    A->lbl_roll->setText(QString::number(roll, 'f', 1) + "°");
    A->lbl_pitch->setText(QString::number(pitch, 'f', 1) + "°");
    A->lbl_yaw->setText(QString::number(yaw, 'f', 1) + "°");
    A->lbl_heading->setText(QString::number(yaw, 'f', 1) + "°");
    A->attitude_indicator->set_attitude(roll, pitch);

    // Sabitleme acikken kumandaya dokunulmasa bile duzeltme surekli guncel
    // kalsin diye, her yeni IMU okumasinda (yaklasik 10Hz) son bilinen
    // kumanda pozisyonuyla motor karisimi yeniden hesaplanip gonderilir.
    // Boylece kumandayi biraksan bile ROV kendi kendine seviyeyi korumaya
    // calisir; sadece joystick hareketine bagli kalinsaydi, kumanda sabit
    // durdugunda yeni eksen sinyali gelmedigi icin duzeltme hic guncellenmezdi.
    if (ana_stabilize && ana_esp_thread && !ana_autonomous) {
        apply_ana_motor_mix(-ana_axes_state[1], ana_axes_state[0],
                             ana_axes_state[2], -ana_axes_state[3]);
    } else if (ana_autonomous) {
        // apply_ana_motor_mix() roll/pitch duzeltmesini (ana_stabilize acikken)
        // her cagrida kendi icinde ekliyor; otonom modda da bu IMU okumasi tek
        // basina degisince guncel kalsin diye derinlik girdisiyle birlikte
        // yeniden hesaplanip gonderiyoruz.
        recompute_ana_autonomous_mix();
    }
}

void RovGUI::update_ana_depth(float meters, float vertical_speed_ms) {
    ana_current_depth = meters;
    RovPanel *A = ui.anaRovPanel;
    A->lbl_alt->setText(QString::number(meters, 'f', 2));
    // Dikey bar 0-100 araliginda; varsayilan olcek 0-20m'yi tam bara yayar
    // (bkz. RovPanel.cpp bar_alt->setRange(0,100)). Daha derin calisilacaksa
    // asagidaki 20.0f'i gercek maksimum derinlige gore guncelle.
    int barVal = qBound(0, 100, static_cast<int>(meters / 20.0f * 100.0f));
    A->bar_alt->setValue(barVal);

    // NOT: Bu SADECE dikey (batma/yukselme) hizidir - basinc sensorunden
    // turetiliyor. Ileri/yanal hiz icin DVL/akis sensoru gerekir, bu ROV'da yok.
    A->lbl_speed->setText(QString::number(vertical_speed_ms, 'f', 2));

    // Otonom modda irtifa (derinlik) sabitleme: her yeni derinlik okumasinda
    // hedeften sapmayi dikey itkiye duzeltme olarak uygula (bkz. on_autonomous_ana()
    // - hedef oraya gecerken kilitlenir).
    if (ana_autonomous && ana_esp_thread) {
        recompute_ana_autonomous_mix();
    }
}

// DHT11 nem/sicaklik sensorunden gelen periyodik veri - gemi govdesi icinde
// nem yukselmesi (su sizintisi belirtisi) takibi icin.
void RovGUI::update_ana_nem(float humidity_pct, float temperature_c) {
    RovPanel *A = ui.anaRovPanel;
    if (A->lbl_humidity) A->lbl_humidity->setText(QString::number(humidity_pct, 'f', 1) + "%");
    if (A->lbl_dht_temp) A->lbl_dht_temp->setText(QString::number(temperature_c, 'f', 1) + "°C");
}

// ESP32'nin GPIO34'teki gerilim bolucuden okudugu pil durumu ("BAT:volt,yuzde"
// satiri, bkz. AnaRovBeyin.ino Pil bolumu). %20 ve altinda operatorun gozunden
// kacmasin diye kirmizi gosterilir.
void RovGUI::update_ana_battery(float volts, int percent) {
    RovPanel *A = ui.anaRovPanel;
    A->lbl_voltage->setText(QString::number(volts, 'f', 2) + " V");
    A->lbl_battery->setText(QString::number(percent) + "%");
    A->lbl_battery->setStyleSheet(percent <= 20 ? "color:#dc2626;font-weight:bold;" : "");
}

// ESP32'nin "TORPEDO:kalan" satirina karsilik gelir - hangi torpidonun
// dolu/bos oldugunu operatorun elle takip etmesine gerek kalmasin diye
// "Torpido Durumu" "Hazır" yerine "kalan/3" seklinde gosterilir.
void RovGUI::update_ana_torpedo(int remaining) {
    ui.anaRovPanel->lbl_servo_status->setText(QString("%1/3").arg(remaining));
    if (remaining == 3) {
        for (int i = 0; i < 3; ++i) ana_torpedo_fired[i] = false;
        set_led(ui.anaRovPanel->led_torpedo1, true);
        set_led(ui.anaRovPanel->led_torpedo2, true);
        set_led(ui.anaRovPanel->led_torpedo3, true);
    }
}

// Otonom moddaki tek M: komutunu ureten ortak yer: mevcut derinlik-hold
// duzeltmesini (Motor ESP, basinc sensoru) motor karisimina uygular. ATT:/
// DEPTH: satirlarindan hangisi gelirse gelsin, son bilinen degerlerle burasi
// cagrilir - boylece ayri guncelleme yerlerinde ayni mantik tekrarlanmaz ve
// Motor ESP'ye cakisan iki ayri komut kaynagi gitmez (roll/pitch duzeltmesi
// apply_ana_motor_mix() icinde, ana_stabilize acikken, ayrica ekleniyor).
void RovGUI::recompute_ana_autonomous_mix() {
    if (!ana_autonomous || !ana_esp_thread) return;

    float depthErr = ana_depth_target - ana_current_depth; // pozitif: hedeften daha sigda, asagi itki gerekir
    float depthCorr = qBound(-1.0f, depthErr * settings.depthHoldGain, 1.0f);

    apply_ana_motor_mix(0.0f, 0.0f, 0.0f, -depthCorr);
}

// ==================== Mini ROV Bağlantı (ESP32 seri port) ====================
void RovGUI::toggle_mini_connection() {
    RovPanel *M = ui.miniRovPanel;
    QString activeStyle = "background-color: #10b981; color: white; border: 2px solid #047857; font-weight: bold; font-size: 13px; border-radius: 4px;";

    if (mini_esp_thread) {
        mini_esp_thread->stop();
        delete mini_esp_thread;
        mini_esp_thread = nullptr;
        mini_armed = false;
        M->btn_connect->setText("Bağlan");
        M->btn_connect->setStyleSheet("");
        M->lbl_status->setText("Durum: Bağlantı Kesildi");
        M->btn_stabilize->setText("ARM Et");
        M->btn_stabilize->setStyleSheet("");
        reset_labels(M);
        log_message("Mini ROV (ESP32) bağlantısı kesildi.", 1);
        return;
    }
    QString port = M->port_combo->currentText();
    int baud = M->baud_combo->currentText().toInt();
    mini_esp_thread = new EspRovThread(port, baud, this);
    connect(mini_esp_thread, &EspRovThread::status_signal, this, &RovGUI::update_mini_status);
    connect(mini_esp_thread, &EspRovThread::armed_signal,  this, &RovGUI::update_mini_armed);
    mini_esp_thread->start();
    M->btn_connect->setText("Kes");
    M->btn_connect->setStyleSheet(activeStyle);
    log_message("Mini ROV (ESP32) bağlantısı başlatılıyor: " + port, 1);
}

void RovGUI::update_mini_status(const QString &msg) {
    ui.miniRovPanel->lbl_status->setText("Durum: " + msg);
    log_message("[MİNİ] " + msg, 1);
}

void RovGUI::update_mini_armed(bool armed) {
    mini_armed = armed;
    RovPanel *M = ui.miniRovPanel;
    if (armed) {
        M->btn_stabilize->setText("DISARM Et");
        M->btn_stabilize->setStyleSheet("background-color:#dc2626;color:white;font-weight:bold;border:1px solid #991b1b;border-radius:4px;font-size:16px;");
        log_message("[MİNİ] ARMED - ESC'ler hazır.", 1);
    } else {
        M->btn_stabilize->setText("ARM Et");
        M->btn_stabilize->setStyleSheet("");
        log_message("[MİNİ] DISARMED - motorlara sinyal kesildi.", 1);
    }
}

// ==================== Joystick ====================
void RovGUI::toggle_ana_joystick() {
    RovPanel *A = ui.anaRovPanel;
    if (ana_joy_thread) {
        ana_joy_thread->stop();
        delete ana_joy_thread;
        ana_joy_thread = nullptr;
        A->btn_joy_connect->setText("Kol Bağla");
        A->lbl_pad_status->setText("Durum: Kol Bağlı Değil");
        A->lbl_pad_status->setStyleSheet("color: #ef4444; font-weight: bold; font-size: 14px;");
        return;
    }
    if (mini_joy_thread) {
        A->lbl_pad_status->setText("Hata: Kol Diğer Araçta Aktif!");
        A->lbl_pad_status->setStyleSheet("color: #ef4444; font-weight: bold; font-size: 14px;");
        log_message("Hata: Kol zaten Mini ROV tarafında bağlı!", 0);
        return;
    }
    QString dev = A->joy_combo->currentText();
    ana_joy_thread = new JoystickThread(dev, this);
    connect(ana_joy_thread, &JoystickThread::status_signal, this, &RovGUI::update_ana_joy_status);
    connect(ana_joy_thread, &JoystickThread::button_signal, this, &RovGUI::update_ana_joy_button);
    connect(ana_joy_thread, &JoystickThread::axis_signal,   this, &RovGUI::update_ana_joy_axis);
    connect(ana_joy_thread, &JoystickThread::disconnected_signal, this, &RovGUI::on_ana_joystick_lost);
    ana_joy_thread->start();
    A->btn_joy_connect->setText("Kes");
}

void RovGUI::toggle_mini_joystick() {
    RovPanel *M = ui.miniRovPanel;
    if (mini_joy_thread) {
        mini_joy_thread->stop();
        delete mini_joy_thread;
        mini_joy_thread = nullptr;
        M->btn_joy_connect->setText("Kol Bağla");
        M->lbl_pad_status->setText("Durum: Kol Bağlı Değil");
        M->lbl_pad_status->setStyleSheet("color: #ef4444; font-weight: bold; font-size: 14px;");
        return;
    }
    if (ana_joy_thread) {
        M->lbl_pad_status->setText("Hata: Kol Diğer Araçta Aktif!");
        M->lbl_pad_status->setStyleSheet("color: #ef4444; font-weight: bold; font-size: 14px;");
        log_message("Hata: Kol zaten Ana ROV tarafında bağlı!", 1);
        return;
    }
    QString dev = M->joy_combo->currentText();
    mini_joy_thread = new JoystickThread(dev, this);
    connect(mini_joy_thread, &JoystickThread::status_signal, this, &RovGUI::update_mini_joy_status);
    connect(mini_joy_thread, &JoystickThread::button_signal, this, &RovGUI::update_mini_joy_button);
    connect(mini_joy_thread, &JoystickThread::axis_signal,   this, &RovGUI::update_mini_joy_axis);
    connect(mini_joy_thread, &JoystickThread::disconnected_signal, this, &RovGUI::on_mini_joystick_lost);
    mini_joy_thread->start();
    M->btn_joy_connect->setText("Kes");
}

void RovGUI::update_ana_joy_status(const QString &msg) {
    QLabel *lbl = ui.anaRovPanel->lbl_pad_status;
    lbl->setText("Durum: " + msg);
    if (msg.contains("Bağlandı")) {
        lbl->setStyleSheet("color: #2ecc71; font-weight: bold; font-size: 14px;");
    } else {
        lbl->setStyleSheet("color: #ef4444; font-weight: bold; font-size: 14px;");
    }
    log_message("[ANA-KOL] " + msg, 0);
}

void RovGUI::update_ana_joy_button(int btn_id, int state) {
    QString name = button_map.value(btn_id, QString("BTN%1").arg(btn_id));
    if (state) ana_pressed_buttons.insert(name);
    else       ana_pressed_buttons.remove(name);
    QString lst = ana_pressed_buttons.isEmpty() ? "Yok" :
                  QStringList(ana_pressed_buttons.values()).join(", ");
    ui.anaRovPanel->lbl_pad_buttons->setText("Basılan Tuş: " + lst);

    // Buton bazlı aksiyon: A/B/X=torpido 1/2/3 (Y basılıyken=ters), Y tek
    // başına bir şey yapmaz (torpido modifier'ı), LB=sabitleme modu hızlı
    // aç/kapa, RB=otonom, Back=manuel, Start=ARM/DISARM, L3=acil durdurma
    // (lamba aç/kapa ve miniROV bırak artık sadece ekrandaki butonlarla yapılır)
    if (state && ana_esp_thread) {
        bool yHeld = ana_pressed_buttons.contains("Y");
        if (name == "A")          yHeld ? on_torpedo_reverse(0) : on_torpedo_fire(0);
        else if (name == "B")     yHeld ? on_torpedo_reverse(1) : on_torpedo_fire(1);
        else if (name == "X")     yHeld ? on_torpedo_reverse(2) : on_torpedo_fire(2);
        else if (name == "LB")    toggle_stabilize_mode_ana();
        else if (name == "RB")    on_autonomous_ana();
        else if (name == "Back")  on_manual_ana();
        else if (name == "Start") on_stabilize_ana();
        else if (name == "L3")    on_emergency_ana();
    }
}

void RovGUI::update_ana_joy_axis(int axis_id, float value) {
    if (axis_id == AXIS_LEFT_X)  ana_axes_state[0] = value;
    if (axis_id == AXIS_LEFT_Y)  ana_axes_state[1] = value;
    if (axis_id == AXIS_RIGHT_X) ana_axes_state[2] = value;
    if (axis_id == AXIS_RIGHT_Y) ana_axes_state[3] = value;
    ui.anaRovPanel->lbl_pad_axes->setText(
        QString("Sol Stick: X:%1 Y:%2  Sağ Stick: X:%3 Y:%4")
            .arg(ana_axes_state[0], 0, 'f', 2)
            .arg(ana_axes_state[1], 0, 'f', 2)
            .arg(ana_axes_state[2], 0, 'f', 2)
            .arg(ana_axes_state[3], 0, 'f', 2));

    // Sol stick: ileri/geri + yanal kayma, Sağ stick: dönüş (yaw) + derinlik (Z)
    if (ana_esp_thread && !ana_autonomous) {
        apply_ana_motor_mix(-ana_axes_state[1], ana_axes_state[0],
                             ana_axes_state[2], -ana_axes_state[3]);
    }
}

void RovGUI::update_mini_joy_status(const QString &msg) {
    QLabel *lbl = ui.miniRovPanel->lbl_pad_status;
    lbl->setText("Durum: " + msg);
    if (msg.contains("Bağlandı")) {
        lbl->setStyleSheet("color: #2ecc71; font-weight: bold; font-size: 14px;");
    } else {
        lbl->setStyleSheet("color: #ef4444; font-weight: bold; font-size: 14px;");
    }
    log_message("[MİNİ-KOL] " + msg, 1);
}

void RovGUI::update_mini_joy_button(int btn_id, int state) {
    QString name = button_map.value(btn_id, QString("BTN%1").arg(btn_id));
    if (state) mini_pressed_buttons.insert(name);
    else       mini_pressed_buttons.remove(name);
    QString lst = mini_pressed_buttons.isEmpty() ? "Yok" :
                  QStringList(mini_pressed_buttons.values()).join(", ");
    ui.miniRovPanel->lbl_pad_buttons->setText("Basılan Tuş: " + lst);

    // Start (ARM/DISARM) ve L3 (acil durdurma) ekrandaki karsiliklari gibi
    // minirov_launched sartina bagli degil, her zaman calisir.
    if (state && mini_esp_thread) {
        if (name == "Start") { on_stabilize_mini(); return; }
        if (name == "L3")    { on_emergency_mini(); return; }
    }

    if (!minirov_launched) return;

    if (state && mini_esp_thread) {
        if (name == "A")      on_lamp_on_mini();
        else if (name == "B") on_lamp_off_mini();
    }
}

void RovGUI::update_mini_joy_axis(int axis_id, float value) {
    if (!minirov_launched) return;
    if (axis_id == AXIS_LEFT_X)  mini_axes_state[0] = value;
    if (axis_id == AXIS_LEFT_Y)  mini_axes_state[1] = value;
    if (axis_id == AXIS_RIGHT_X) mini_axes_state[2] = value;
    if (axis_id == AXIS_RIGHT_Y) mini_axes_state[3] = value;
    ui.miniRovPanel->lbl_pad_axes->setText(
        QString("Sol Stick: X:%1 Y:%2  Sağ Stick: X:%3 Y:%4")
            .arg(mini_axes_state[0], 0, 'f', 2)
            .arg(mini_axes_state[1], 0, 'f', 2)
            .arg(mini_axes_state[2], 0, 'f', 2)
            .arg(mini_axes_state[3], 0, 'f', 2));

    if (mini_esp_thread) {
        apply_mini_motor_mix(-mini_axes_state[1], mini_axes_state[0],
                              mini_axes_state[2], -mini_axes_state[3]);
    }
}

// ==================== Kamera Frame ====================
void RovGUI::update_camera_frame(const QImage &img) {
    QImage disp = img.convertToFormat(QImage::Format_RGB32);
    if (mini_recorder.isOpen()) draw_rec_badge(disp);
    mini_last_frame = disp;
    QLabel *lbl = ui.miniRovPanel->lbl_cam_stream;
    lbl->setPixmap(QPixmap::fromImage(disp).scaled(lbl->size(),
                                                   Qt::KeepAspectRatio,
                                                   Qt::SmoothTransformation));
}

void RovGUI::update_camera_status(const QString &msg) {
    log_message("[CAM-MİNİ] " + msg, 1);
}

void RovGUI::update_camera_stats(int fps, float kbps, int w, int h) {
    mini_cam_fps = fps;
    RovPanel *M = ui.miniRovPanel;
    M->lbl_cam_fps->setText(QString("FPS: %1").arg(fps));
    M->lbl_cam_bitrate->setText(QString("Veri Hızı: %1 KB/s").arg(kbps, 0, 'f', 1));
    M->lbl_cam_res->setText(QString("Çözünürlük: %1x%2").arg(w).arg(h));
}

void RovGUI::update_anarov_frame(const QImage &img) {
    QImage disp = img.convertToFormat(QImage::Format_RGB32);
    draw_ana_hud(disp);
    ana_last_frame = disp; // snapshot HUD'lu kareyi alir
    QLabel *lbl = ui.anaRovPanel->lbl_cam_stream;
    lbl->setPixmap(QPixmap::fromImage(disp).scaled(lbl->size(),
                                                   Qt::KeepAspectRatio,
                                                   Qt::SmoothTransformation));
}

void RovGUI::update_anarov_status(const QString &msg) {
    if (msg.contains("Bağlandı")) {
        ana_cam_connected = true;
    } else if (msg.contains("Koptu") || msg.contains("Hatası")) {
        ana_cam_connected = false;
        if (ana_autonomous) on_manual_ana();
    }
    log_message("[CAM-ANA] " + msg, 0);
}

void RovGUI::update_anarov_stats(int fps, float kbps, int w, int h) {
    ana_cam_fps = fps;
    RovPanel *A = ui.anaRovPanel;
    A->lbl_cam_fps->setText(QString("FPS: %1").arg(fps));
    A->lbl_cam_bitrate->setText(QString("Veri Hızı: %1 KB/s").arg(kbps, 0, 'f', 1));
    A->lbl_cam_res->setText(QString("Çözünürlük: %1x%2").arg(w).arg(h));
}

// ==================== HUD ====================
// Gercek ROV pilot arayuzlerindeki gibi telemetri dogrudan videonun uzerine
// bindirilir: operatorun gozu videodan ayrilmadan derinlik/pusula/ARM durumu
// gorunur. Goruntunun kendisine cizildigi icin snapshot'larda da yer alir
// (video kaydina ISLENMEZ - kayit ham kamera goruntusudur, bkz. on_ana_jpeg).
void RovGUI::draw_ana_hud(QImage &img) {
    if (img.isNull()) return;
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);

    const int W = img.width();
    const int H = img.height();
    const int fontPx = qMax(11, H / 22);
    QFont f("Monospace");
    f.setPixelSize(fontPx);
    f.setBold(true);
    p.setFont(f);
    const int pad = fontPx / 2;
    const int lineH = fontPx + 4;

    // ---- Sol ust: telemetri paneli ----
    const QStringList lines = {
        QString("DRN %1 m").arg(ana_current_depth, 5, 'f', 2),
        QString("PSL %1°").arg(ana_yaw, 5, 'f', 1),
        QString("R %1°  P %2°").arg(ana_roll, 5, 'f', 1).arg(ana_pitch, 5, 'f', 1),
    };
    int boxW = 0;
    QFontMetrics fm(f);
    for (const QString &s : lines) boxW = qMax(boxW, fm.horizontalAdvance(s));
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 120));
    p.drawRect(0, 0, boxW + pad * 2, lines.size() * lineH + pad * 2);
    p.setPen(QColor(0, 255, 140));
    for (int i = 0; i < lines.size(); ++i) {
        p.drawText(pad, pad + (i + 1) * lineH - 4, lines[i]);
    }

    // ---- Sol alt: durum satiri (ARM / mod / sabitleme) ----
    const QString armTxt  = ana_armed ? "ARMED" : "DISARM";
    const QString modeTxt = ana_autonomous ? "OTONOM" : "MANUEL";
    const QString stabTxt = ana_stabilize ? "STAB" : "";
    QString status = armTxt + "  " + modeTxt + (stabTxt.isEmpty() ? "" : "  " + stabTxt);
    const int stW = fm.horizontalAdvance(status);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 120));
    p.drawRect(0, H - lineH - pad * 2, stW + pad * 2, lineH + pad * 2);
    p.setPen(ana_armed ? QColor(255, 80, 80) : QColor(0, 255, 140));
    p.drawText(pad, H - pad - 4, status);

    // ---- Torpido nisangahi (merkez artisi yerine) ----
    p.end(); // draw_torpedo_reticle kendi QPainter'ini acar
    draw_torpedo_reticle(img);

    // ---- Sag ust: REC ----
    if (ana_recorder.isOpen()) draw_rec_badge(img);
}

// Torpido atis nisangahi.
//
// Fiziksel dizilim (kalibrasyon sabitleri):
//   - On kapak cami: 120x120 mm (yukari-asagi ve sagdan-sola), kamera bu
//     alanin tam ortasinda. Kamera goruntusunun tamami bu 120x120 mm'lik
//     alana 1:1 karsilik geldigi kabul edilir (frame kenarlari = cam
//     kenarlari), yani pxPerMm = frame_boyutu / 120.
//   - Torpido tup ici genisligi 55 mm; tup yatayda kameranin optik
//     eksenine tam ortalanmis oldugundan torpido her zaman goruntunun
//     dikey merkez cizgisi boyunca duser (yatay ofset yok).
//   - Torpido tupunun alt kenari, cam ust kenarinin 76 mm ustundedir
//     (goruntu disinda) - yani torpido serbest dustukten sonra goruntude
//     ilk kez cam ust kenarinda (y=0) belirir.
void RovGUI::draw_torpedo_reticle(QImage &img) {
    if (img.isNull()) return;
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);

    const int W = img.width();
    const int H = img.height();
    const int cx = W / 2;
    const int cy = H / 2;

    const double pxPerMmX = W / 120.0;
    const double pxPerMmY = H / 120.0;

    const QColor lineCol(0, 255, 140, 200);   // gorus eksenleri (HUD yesili)
    const QColor gateCol(255, 170, 0, 230);   // torpido kapisi (turuncu)
    const int thin  = qMax(1, H / 480);
    const int thick = qMax(2, H / 240);

    QFont f("Monospace");
    const int fontPx = qMax(9, H / 45);
    f.setPixelSize(fontPx);
    p.setFont(f);
    QFontMetrics fm(f);

    // ---- Ana eksenler: dusey = torpido dusme hatti, yatay = kamera
    // optik ekseni ----
    p.setPen(QPen(lineCol, thin));
    p.drawLine(cx, 0, cx, H);
    p.drawLine(0, cy, W, cy);

    // ---- Dikey mm cetveli (0 mm = cam ust kenari ... 120 mm = cam alt
    // kenari), 10 mm'de kisa, 20 mm'de uzun+etiketli cizgi ----
    for (int mm = 0; mm <= 120; mm += 10) {
        const int y = qRound(mm * pxPerMmY);
        const bool major = (mm % 20 == 0);
        const int halfLen = qRound((major ? 10.0 : 5.0) * pxPerMmX);
        p.setPen(QPen(lineCol, major ? thick : thin));
        p.drawLine(cx - halfLen, y, cx + halfLen, y);
        if (major) {
            const QString lbl = QString::number(mm);
            // Ust/alt kenarlarda etiket disari taşmasın diye dikey konum
            // gorunur alana sıkıştırılır (0 ve 120 mm cizgileri tam
            // kenarda oldugundan).
            const int labelY = qBound(fontPx, y + fontPx / 3, H - 4);
            p.setPen(lineCol);
            p.drawText(cx + halfLen + 4, labelY, lbl);
        }
    }

    // ---- Yatay mm cetveli, merkezden +-60 mm, 10/20 mm araliklarla ----
    for (int mm = -60; mm <= 60; mm += 10) {
        if (mm == 0) continue;
        const int x = cx + qRound(mm * pxPerMmX);
        const bool major = (mm % 20 == 0);
        const int halfLen = qRound((major ? 10.0 : 5.0) * pxPerMmY);
        p.setPen(QPen(lineCol, major ? thick : thin));
        p.drawLine(x, cy - halfLen, x, cy + halfLen);
    }

    // ---- Torpido dusus kapisi: tup ici genislik 55 mm, cam ust
    // kenarinda (y=0) merkeze ortalanmis kose ayraclari. Tupun kendisi
    // bu noktanin 76 mm ustunde (goruntu disinda) oldugundan, torpido
    // goruntude ilk kez bu genislikte belirir. ----
    const int gateHalfW = qRound((55.0 / 2.0) * pxPerMmX);
    const int gateH = qRound(14.0 * pxPerMmY);
    p.setPen(QPen(gateCol, thick));
    p.drawLine(cx - gateHalfW, 0, cx - gateHalfW, gateH);
    p.drawLine(cx - gateHalfW, gateH, cx - gateHalfW + gateH / 3, gateH);
    p.drawLine(cx + gateHalfW, 0, cx + gateHalfW, gateH);
    p.drawLine(cx + gateHalfW, gateH, cx + gateHalfW - gateH / 3, gateH);

    const QString gateLbl = "TORPIDO 55mm  (tup +76mm yukarda)";
    const int gateLblW = fm.horizontalAdvance(gateLbl);
    p.setPen(gateCol);
    p.drawText(cx - gateLblW / 2, gateH + fontPx + 2, gateLbl);

    // ---- Merkez nokta (kamera optik merkezi) ----
    p.setPen(Qt::NoPen);
    p.setBrush(lineCol);
    const int dotR = qMax(2, H / 200);
    p.drawEllipse(QPoint(cx, cy), dotR, dotR);
}

void RovGUI::draw_rec_badge(QImage &img) {
    if (img.isNull()) return;
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing);
    const int H = img.height();
    const int fontPx = qMax(11, H / 22);
    QFont f("Monospace");
    f.setPixelSize(fontPx);
    f.setBold(true);
    p.setFont(f);
    QFontMetrics fm(f);
    const int pad = fontPx / 2;
    const int dotR = fontPx / 2;
    const QString txt = "REC";
    const int boxW = dotR * 2 + pad + fm.horizontalAdvance(txt) + pad * 2;
    const int x = img.width() - boxW;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 120));
    p.drawRect(x, 0, boxW, fontPx + pad * 2);
    p.setBrush(QColor(255, 40, 40));
    p.drawEllipse(QPoint(x + pad + dotR, (fontPx + pad * 2) / 2), dotR, dotR);
    p.setPen(QColor(255, 40, 40));
    p.drawText(x + pad + dotR * 2 + pad, pad + fontPx - 3, txt);
}

// ==================== Snapshot / Video kaydı ====================
QString RovGUI::media_dir() {
    QString dir = QDir::homePath() + "/rov_media";
    QDir().mkpath(dir);
    return dir;
}

void RovGUI::on_ana_snapshot() {
    if (ana_last_frame.isNull()) {
        log_message("[ANA] Snapshot alınamadı - henüz kamera görüntüsü yok.", 0);
        return;
    }
    QString path = media_dir() + "/ana_" +
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".png";
    if (ana_last_frame.save(path, "PNG")) {
        log_message("[ANA] Snapshot kaydedildi: " + path, 0);
    } else {
        log_message("[ANA] Snapshot kaydedilemedi: " + path, 0);
    }
}

void RovGUI::on_mini_snapshot() {
    if (mini_last_frame.isNull()) {
        log_message("[MİNİ] Snapshot alınamadı - henüz kamera görüntüsü yok.", 1);
        return;
    }
    QString path = media_dir() + "/mini_" +
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".png";
    if (mini_last_frame.save(path, "PNG")) {
        log_message("[MİNİ] Snapshot kaydedildi: " + path, 1);
    } else {
        log_message("[MİNİ] Snapshot kaydedilemedi: " + path, 1);
    }
}

void RovGUI::toggle_ana_record() {
    RovPanel *A = ui.anaRovPanel;
    if (ana_recorder.isOpen()) {
        int frames = ana_recorder.frameCount();
        ana_recorder.close();
        A->btn_record->setText("⏺ Kayıt");
        A->btn_record->setStyleSheet("");
        log_message(QString("[ANA] Video kaydı durduruldu (%1 kare).").arg(frames), 0);
        return;
    }
    if (ana_last_frame.isNull()) {
        log_message("[ANA] Kayıt başlatılamadı - henüz kamera görüntüsü yok.", 0);
        return;
    }
    QString path = media_dir() + "/ana_" +
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".avi";
    int fps = ana_cam_fps > 0 ? ana_cam_fps : 20;
    if (!ana_recorder.open(path, ana_last_frame.width(), ana_last_frame.height(), fps)) {
        log_message("[ANA] Kayıt dosyası açılamadı: " + path, 0);
        return;
    }
    A->btn_record->setText("⏹ Durdur");
    A->btn_record->setStyleSheet("background-color:#dc2626;color:white;font-weight:bold;border-radius:4px;");
    log_message("[ANA] Video kaydı başladı: " + path, 0);
}

void RovGUI::toggle_mini_record() {
    RovPanel *M = ui.miniRovPanel;
    if (mini_recorder.isOpen()) {
        int frames = mini_recorder.frameCount();
        mini_recorder.close();
        M->btn_record->setText("⏺ Kayıt");
        M->btn_record->setStyleSheet("");
        log_message(QString("[MİNİ] Video kaydı durduruldu (%1 kare).").arg(frames), 1);
        return;
    }
    if (mini_last_frame.isNull()) {
        log_message("[MİNİ] Kayıt başlatılamadı - henüz kamera görüntüsü yok.", 1);
        return;
    }
    QString path = media_dir() + "/mini_" +
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".avi";
    int fps = mini_cam_fps > 0 ? mini_cam_fps : 20;
    if (!mini_recorder.open(path, mini_last_frame.width(), mini_last_frame.height(), fps)) {
        log_message("[MİNİ] Kayıt dosyası açılamadı: " + path, 1);
        return;
    }
    M->btn_record->setText("⏹ Durdur");
    M->btn_record->setStyleSheet("background-color:#dc2626;color:white;font-weight:bold;border-radius:4px;");
    log_message("[MİNİ] Video kaydı başladı: " + path, 1);
}

void RovGUI::on_ana_jpeg(const QByteArray &jpeg) {
    if (ana_recorder.isOpen()) ana_recorder.addFrame(jpeg);
}

void RovGUI::on_mini_jpeg(const QByteArray &jpeg) {
    if (mini_recorder.isOpen()) mini_recorder.addFrame(jpeg);
}

// ==================== Ping ====================
void RovGUI::read_cam_ping() {
    if (!cam_ping_proc) return;
    QByteArray data = cam_ping_proc->readAllStandardOutput();
    QString text = QString::fromUtf8(data);
    QRegExp re("time=([0-9.]+) ms");
    if (re.indexIn(text) >= 0) {
        ui.miniRovPanel->lbl_cam_ping->setText("Ping: " + re.cap(1) + " ms");
    }
}

void RovGUI::read_anarov_ping() {
    if (!anarov_ping_proc) return;
    QByteArray data = anarov_ping_proc->readAllStandardOutput();
    QString text = QString::fromUtf8(data);
    QRegExp re("time=([0-9.]+) ms");
    if (re.indexIn(text) >= 0) {
        ui.anaRovPanel->lbl_cam_ping->setText("Ping: " + re.cap(1) + " ms");
    }
}

// ==================== Yön Butonları ====================
void RovGUI::ana_dir_pressed() {
    if (!ana_esp_thread) {
        log_message("[ANA] Önce bağlanın!", 0);
        return;
    }
    if (ana_autonomous) {
        log_message("[ANA] Otonom mod aktifken manuel hareket engellendi!", 0);
        return;
    }
    QPushButton *b = qobject_cast<QPushButton*>(sender());
    if (!b) return;
    RovPanel *A = ui.anaRovPanel;
    if      (b == A->btn_forward)    { apply_ana_motor_mix(1, 0, 0, 0);  log_message("[ANA] İleri", 0); }
    else if (b == A->btn_backward)   { apply_ana_motor_mix(-1, 0, 0, 0); log_message("[ANA] Geri", 0); }
    else if (b == A->btn_left)       { apply_ana_motor_mix(0, -1, 0, 0); log_message("[ANA] Sol", 0); }
    else if (b == A->btn_right)      { apply_ana_motor_mix(0, 1, 0, 0);  log_message("[ANA] Sağ", 0); }
    else if (b == A->btn_up)         { apply_ana_motor_mix(0, 0, 0, 1);  log_message("[ANA] Z+", 0); }
    else if (b == A->btn_down)       { apply_ana_motor_mix(0, 0, 0, -1); log_message("[ANA] Z-", 0); }
    else if (b == A->btn_turn_left)  { apply_ana_motor_mix(0, 0, -1, 0); log_message("[ANA] Sola Dön", 0); }
    else if (b == A->btn_turn_right) { apply_ana_motor_mix(0, 0, 1, 0);  log_message("[ANA] Sağa Dön", 0); }
}

void RovGUI::ana_dir_released() {
    if (!ana_esp_thread || !ana_armed) return;
    if (ana_autonomous) return;
    QPushButton *b = qobject_cast<QPushButton*>(sender());
    if (!b) return;
    // apply_ana_motor_mix(0,0,0,0) yerine dogrudan BUTTON_STOP_US gonderiliyor -
    // MotorMixer::NEUTRAL_US (joystick/otonom merkezi) ile karistirilmasin diye.
    std::array<int, 8> pulses;
    pulses.fill(BUTTON_STOP_US);
    ana_esp_thread->set_motors(pulses);
    ana_last_pulses = pulses;
    ui.anaRovPanel->motor_diagram->set_motor_pulses(pulses);
}

void RovGUI::mini_dir_pressed() {
    if (!mini_esp_thread) {
        log_message("[MİNİ] Önce bağlanın!", 1);
        return;
    }
    if (!minirov_launched) {
        log_message("[MİNİ] Mini ROV henüz bırakılmadı! Hareket engellendi.", 1);
        return;
    }
    QPushButton *b = qobject_cast<QPushButton*>(sender());
    if (!b) return;
    RovPanel *M = ui.miniRovPanel;
    if      (b == M->btn_forward)    { apply_mini_motor_mix(1, 0, 0, 0);  log_message("[MİNİ] İleri", 1); }
    else if (b == M->btn_backward)   { apply_mini_motor_mix(-1, 0, 0, 0); log_message("[MİNİ] Geri", 1); }
    else if (b == M->btn_left)       { apply_mini_motor_mix(0, -1, 0, 0); log_message("[MİNİ] Sol", 1); }
    else if (b == M->btn_right)      { apply_mini_motor_mix(0, 1, 0, 0);  log_message("[MİNİ] Sağ", 1); }
    else if (b == M->btn_up)         { apply_mini_motor_mix(0, 0, 0, 1);  log_message("[MİNİ] Z+", 1); }
    else if (b == M->btn_down)       { apply_mini_motor_mix(0, 0, 0, -1); log_message("[MİNİ] Z-", 1); }
    else if (b == M->btn_turn_left)  { apply_mini_motor_mix(0, 0, -1, 0); log_message("[MİNİ] Sola Dön", 1); }
    else if (b == M->btn_turn_right) { apply_mini_motor_mix(0, 0, 1, 0);  log_message("[MİNİ] Sağa Dön", 1); }
}

void RovGUI::mini_dir_released() {
    if (!mini_esp_thread) return;
    if (!minirov_launched) return;
    QPushButton *b = qobject_cast<QPushButton*>(sender());
    if (!b) return;
    // apply_mini_motor_mix(0,0,0,0) yerine dogrudan BUTTON_STOP_US gonderiliyor -
    // MotorMixer::NEUTRAL_US (joystick merkezi) ile karistirilmasin diye.
    std::array<int, 8> pulses;
    pulses.fill(BUTTON_STOP_US);
    mini_esp_thread->set_motors(pulses);
    mini_last_pulses = pulses;
    ui.miniRovPanel->motor_diagram->set_motor_pulses(pulses);
}

// ==================== Kumanda kopma watchdog'u ====================
// Kol koptugunda heartbeat (send_motor_heartbeat) son komutu tekrarlamaya
// devam ettigi icin ESP32'nin 500ms failsafe'i HIC devreye girmez - stick
// ileri basiliyken kol koparsa ROV surekli ileri giderdi. Bu yuzden kopma
// aninda eksenleri sifirlayip motorlari acikca notrluyoruz.
void RovGUI::on_ana_joystick_lost() {
    for (int i = 0; i < 4; ++i) ana_axes_state[i] = 0.0f;
    ana_last_pulses = settings.motorNeutralUs;
    // Otonom mod kola bagimli degil (derinlik sabitleme calismaya devam
    // edebilir); manuel moddaysa motorlari hemen notrle.
    if (!ana_autonomous) {
        apply_ana_motor_mix(0, 0, 0, 0);
    }
    log_message("[ANA] KUMANDA KOPTU - motorlar nötrlendi!", 0);
    show_screen_warning("ANA ROV: Kumanda bağlantısı koptu! Motorlar nötrlendi.");
}

void RovGUI::on_mini_joystick_lost() {
    for (int i = 0; i < 4; ++i) mini_axes_state[i] = 0.0f;
    mini_last_pulses = settings.motorNeutralUs;
    apply_mini_motor_mix(0, 0, 0, 0);
    log_message("[MİNİ] KUMANDA KOPTU - motorlar nötrlendi!", 1);
    show_screen_warning("MİNİ ROV: Kumanda bağlantısı koptu! Motorlar nötrlendi.");
}

// ==================== Hızlı Komutlar ====================
// Space tusu: iki aracta birden acil durdurma. QShortcut, odak hangi
// widget'ta olursa olsun (ApplicationShortcut) tusu yakalar - bu yuzden
// Space artik odaklanmis dugmeleri "tiklamaz", her zaman E-STOP'tur.
void RovGUI::on_global_emergency() {
    on_emergency_ana();
    on_emergency_mini();
    log_message("SPACE ile ACİL DURDURMA tetiklendi (her iki araç).");
}

void RovGUI::on_emergency_ana() {
    apply_ana_motor_mix(0, 0, 0, 0);
    if (ana_esp_thread) ana_esp_thread->disarm();
    log_message("[ANA] ACİL DURDURMA tetiklendi! DISARM gönderildi.", 0);
    show_screen_warning("ANA ROV: ACİL DURDURMA tetiklendi! Motorlar sıfırlandı, DISARM gönderildi.");
}

void RovGUI::on_emergency_mini() {
    apply_mini_motor_mix(0, 0, 0, 0);
    if (mini_esp_thread) mini_esp_thread->disarm();
    log_message("[MİNİ] ACİL DURDURMA tetiklendi! DISARM gönderildi.", 1);
    show_screen_warning("MİNİ ROV: ACİL DURDURMA tetiklendi! Motorlar sıfırlandı, DISARM gönderildi.");
}

void RovGUI::on_stabilize_ana() {
    if (!ana_esp_thread) {
        log_message("[ANA] Önce bağlanın!", 0);
        return;
    }
    if (!ana_armed) {
        ana_esp_thread->arm();
        log_message("[ANA] ARM komutu gönderildi, ESC hazırlanıyor...", 0);
    } else {
        ana_esp_thread->disarm();
        log_message("[ANA] DISARM komutu gönderildi.", 0);
    }
}

void RovGUI::on_stabilize_mini() {
    if (!mini_esp_thread) {
        log_message("[MİNİ] Önce bağlanın!", 1);
        return;
    }
    if (!mini_armed) {
        mini_esp_thread->arm();
        log_message("[MİNİ] ARM komutu gönderildi, ESC hazırlanıyor...", 1);
    } else {
        mini_esp_thread->disarm();
        log_message("[MİNİ] DISARM komutu gönderildi.", 1);
    }
}

void RovGUI::on_autonomous_ana() {
    if (!ana_cam_connected) {
        log_message("[ANA] Kamera bağlı değil, otonom moda geçilemez!", 0);
        show_screen_warning("Kamera bağlı değil! Otonom moda geçilemez.");
        return;
    }
    ana_autonomous = true;
    set_led(ui.anaRovPanel->led_autonomous, true);
    set_led(ui.anaRovPanel->led_manual, false);

    // Otonom moda gecince roll/pitch sabitleme otomatik acilir ve mevcut
    // derinlik "hedef" olarak kilitlenir - update_ana_depth() her yeni okumada
    // bu hedeften sapmayi dikey itkiye duzeltme olarak ekleyip ROV'u ne yukari
    // ne asagi kacirmadan oldugu derinlikte tutmaya calisir.
    ana_stabilize = true;
    if (ui.anaRovPanel->lbl_stabilize_status) {
        ui.anaRovPanel->lbl_stabilize_status->setText("Açık");
        ui.anaRovPanel->lbl_stabilize_status->setStyleSheet("color:#2ecc71;font-weight:bold;font-size:14px;");
    }
    ana_depth_target = ana_current_depth;

    // Otonom moda gecerken elde kalan son manuel komutu notrle: yoksa heartbeat
    // (bkz. send_motor_heartbeat) eski manuel hareketi otonom modda da göndermeye
    // devam eder. Surge/yaw otonom modda hep 0'dir; dikey eksen bir sonraki
    // derinlik okumasinda depth-hold tarafindan devralinacak.
    apply_ana_motor_mix(0, 0, 0, 0);
    log_message(QString("[ANA] Otonom moda geçildi - sabitleme açıldı, hedef derinlik: %1m")
                     .arg(ana_depth_target, 0, 'f', 2), 0);
}

void RovGUI::on_manual_ana() {
    ana_autonomous = false;
    set_led(ui.anaRovPanel->led_manual, true);
    set_led(ui.anaRovPanel->led_autonomous, false);
    // Otonom moddan cikarken kalan otonom itkiyi (ornegin derinlik-sabitleme
    // duzeltmesi) sifirla - aksi halde kumanda o an notrde olsa bile motorlar
    // bir sonraki gercek eksen hareketine kadar eski otonom komutta kalirdi.
    apply_ana_motor_mix(0, 0, 0, 0);
    log_message("[ANA] Manuel moda geçildi", 0);
}

void RovGUI::toggle_stabilize_mode_ana() {
    ana_stabilize = !ana_stabilize;
    QLabel *lbl = ui.anaRovPanel->lbl_stabilize_status;
    if (ana_stabilize) {
        if (lbl) {
            lbl->setText("Açık");
            lbl->setStyleSheet("color:#2ecc71;font-weight:bold;font-size:14px;");
        }
        log_message("[ANA] Sabitleme modu AÇILDI (IMU destekli dengeleme).", 0);
        show_screen_warning("Sabitleme modu açıldı: IMU destekli dengeleme aktif.");
    } else {
        if (lbl) {
            lbl->setText("Kapalı");
            lbl->setStyleSheet("color:#c0392b;font-weight:bold;font-size:14px;");
        }
        log_message("[ANA] Sabitleme modu KAPANDI (%100 manuel).", 0);
        show_screen_warning("Sabitleme modu kapandı: %100 manuel kontrol.");
    }

    // Mod degisir degismez mevcut eksen durumuyla mixi yeniden hesaplayip
    // gonder: aksi halde bir sonraki eksen/buton olayina kadar eski moddan
    // kalma (ornegin sabitleme duzeltmesi icermis) darbeler heartbeat
    // tarafindan degismeden tekrar tekrar gonderilmeye devam eder.
    if (ana_esp_thread && !ana_autonomous) {
        apply_ana_motor_mix(-ana_axes_state[1], ana_axes_state[0],
                             ana_axes_state[2], -ana_axes_state[3]);
    }
}

void RovGUI::on_minirov_launch() {
    minirov_launched = true;
    set_led(ui.anaRovPanel->led_minirov, true);
    log_message("[ANA] MiniROV Bırakıldı! (not: ayrı bırakma donanımı bu ESP32 kartında tanımlı değil, sadece arayüz durumu güncellendi)", 0);
}

// Gercek ates alma isini (kalan sayi, 10sn bekleme) artik ESP32 yapiyor
// (bkz. AnaRovBeyin.ino fireTorpedo()) - burada sadece komut gonderiyoruz;
// sonuc (kalan sayi veya ERR:TORPEDOEMPTY/COOLDOWN) update_ana_torpedo()/
// update_ana_status() uzerinden geri doner.
void RovGUI::on_torpedo_fire(int index) {
    if (!ana_esp_thread) {
        log_message("[ANA] Önce bağlanın!", 0);
        return;
    }
    ana_torpedo_fired[index] = true;
    QLabel *led = index == 0 ? ui.anaRovPanel->led_torpedo1
                 : index == 1 ? ui.anaRovPanel->led_torpedo2
                              : ui.anaRovPanel->led_torpedo3;
    set_led(led, false);
    ana_esp_thread->torpedo(index);
    log_message(QString("[ANA] Torpido %1 fırlatma komutu gönderildi.").arg(index + 1), 0);
}

void RovGUI::on_torpedo_reverse(int index) {
    if (!ana_esp_thread) {
        log_message("[ANA] Önce bağlanın!", 0);
        return;
    }
    ana_esp_thread->torpedo_reverse(index);
    log_message(QString("[ANA] Torpido %1 geri komutu gönderildi.").arg(index + 1), 0);
}

void RovGUI::on_lamp_on_ana() {
    ana_lamp_on = true;
    set_led(ui.anaRovPanel->led_lamp_on, true);
    set_led(ui.anaRovPanel->led_lamp_off, false);
    ui.anaRovPanel->lbl_lamp_status->setText("Açık");
    ui.anaRovPanel->lbl_lamp_status->setStyleSheet("color:#2ecc71;font-weight:bold;font-size:14px;");
    log_message("[ANA] Lamba Açıldı (not: ayrı lamba çıkışı bu ESP32 kartında tanımlı değil, sadece arayüz durumu)", 0);
}

void RovGUI::on_lamp_off_ana() {
    ana_lamp_on = false;
    set_led(ui.anaRovPanel->led_lamp_on, false);
    set_led(ui.anaRovPanel->led_lamp_off, true);
    ui.anaRovPanel->lbl_lamp_status->setText("Kapalı");
    ui.anaRovPanel->lbl_lamp_status->setStyleSheet("color:#c0392b;font-weight:bold;font-size:14px;");
    log_message("[ANA] Lamba Kapatıldı", 0);
}

void RovGUI::on_lamp_on_mini() {
    mini_lamp_on = true;
    set_led(ui.miniRovPanel->led_lamp_on, true);
    set_led(ui.miniRovPanel->led_lamp_off, false);
    ui.miniRovPanel->lbl_lamp_status->setText("Açık");
    ui.miniRovPanel->lbl_lamp_status->setStyleSheet("color:#2ecc71;font-weight:bold;font-size:14px;");
    log_message("[MİNİ] Lamba Açıldı (not: ayrı lamba çıkışı bu ESP32 kartında tanımlı değil, sadece arayüz durumu)", 1);
}

void RovGUI::on_lamp_off_mini() {
    mini_lamp_on = false;
    set_led(ui.miniRovPanel->led_lamp_on, false);
    set_led(ui.miniRovPanel->led_lamp_off, true);
    ui.miniRovPanel->lbl_lamp_status->setText("Kapalı");
    ui.miniRovPanel->lbl_lamp_status->setStyleSheet("color:#c0392b;font-weight:bold;font-size:14px;");
    log_message("[MİNİ] Lamba Kapatıldı", 1);
}

// ==================== Yardımcılar ====================
void RovGUI::reset_labels(RovPanel *panel) {
    panel->lbl_roll->setText("---");
    panel->lbl_pitch->setText("---");
    panel->lbl_yaw->setText("---");
    panel->lbl_alt->setText("---");
    panel->lbl_heading->setText("---");
    panel->lbl_speed->setText("---");
    panel->lbl_voltage->setText("---");
    panel->lbl_battery->setText("---");
    if (panel->lbl_humidity) panel->lbl_humidity->setText("---");
    if (panel->lbl_dht_temp) panel->lbl_dht_temp->setText("---");
    panel->attitude_indicator->set_attitude(0, 0);
    panel->bar_alt->setValue(0);
}

void RovGUI::set_led(QLabel *led, bool on) {
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
    RovPanel *panel = nullptr;
    if (led == ui.anaRovPanel->led_autonomous || led == ui.anaRovPanel->led_manual ||
        led == ui.anaRovPanel->led_minirov ||
        led == ui.anaRovPanel->led_torpedo1 || led == ui.anaRovPanel->led_torpedo2 ||
        led == ui.anaRovPanel->led_torpedo3 ||
        led == ui.anaRovPanel->led_lamp_on || led == ui.anaRovPanel->led_lamp_off) {
        panel = ui.anaRovPanel;
    } else if (led == ui.miniRovPanel->led_autonomous || led == ui.miniRovPanel->led_manual ||
               led == ui.miniRovPanel->led_minirov ||
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
        } else if (led == panel->led_torpedo1 && panel->btn_torpedo1) {
            panel->btn_torpedo1->setStyleSheet(on ? activeStyle : inactiveStyle);
        } else if (led == panel->led_torpedo2 && panel->btn_torpedo2) {
            panel->btn_torpedo2->setStyleSheet(on ? activeStyle : inactiveStyle);
        } else if (led == panel->led_torpedo3 && panel->btn_torpedo3) {
            panel->btn_torpedo3->setStyleSheet(on ? activeStyle : inactiveStyle);
        } else if (led == panel->led_lamp_on) {
            panel->btn_lamp_on->setStyleSheet(on ? lampOnActiveStyle : inactiveStyle);
        } else if (led == panel->led_lamp_off) {
            panel->btn_lamp_off->setStyleSheet(on ? lampOffActiveStyle : inactiveStyle);
        }
    }
}

void RovGUI::toggle_theme() {
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
