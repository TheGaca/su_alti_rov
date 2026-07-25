#include "RovGUI.hpp"

#include <QDateTime>
#include <QCloseEvent>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QStatusBar>
#include <QRegExp>
#include <QShortcut>
#include <QKeySequence>

namespace {
constexpr int ESP_NEUTRAL_US = 1490;
constexpr int ESP_DELTA_US = 200;

// M1..M4 = on/arka capraz yatay itki, M5..M8 = dikey itki
// (bkz. foto/5a5bd473784e48cfed6daa1d1cb0c4e51b705469.png ve MotorDiagramWidget).
// Isaretler ilk tahmindir: bench testinde bir motor beklenenin tersine donerse
// asagidaki ilgili agirligi (-1 <-> +1) ters cevirmek yeterlidir.
constexpr int W_SURGE[4]   = { +1, +1, +1, +1 };
constexpr int W_LATERAL[4] = { +1, -1, +1, -1 };
constexpr int W_YAW[4]     = { -1, +1, +1, -1 };

// Logitech tipi kumandalarda tipik eksen numaralari. Kumandan farkli davranirsa
// "jstest /dev/input/js0" ile gercek eksen numaralarini gor ve asagidakileri guncelle.
constexpr int AXIS_LEFT_X  = 0; // Sol stick X  -> yanal kayma (Sol/Sağ)
constexpr int AXIS_LEFT_Y  = 1; // Sol stick Y  -> ileri/geri
constexpr int AXIS_RIGHT_X = 3; // Sağ stick X  -> dönüş (yaw)
constexpr int AXIS_RIGHT_Y = 4; // Sağ stick Y  -> yukarı/aşağı (derinlik)

// Sabitleme (stabilize) modu: roll/pitch (derece) / 90 * STAB_GAIN, dikey itki
// motorlarina fark olarak eklenir (-1..1 araliginda sinirlanir). STAB_GAIN=4.0
// ile ~22.5 derece egimde tam duzeltme (doygunluk) uygulanir. Havuzda asiri/az
// tepki gorulurse bu degeri ayarla; yon ters gelirse isaretini cevir.
constexpr float STAB_GAIN = 4.0f;

// Otonom modda irtifa (derinlik) sabitleme: hedef-mevcut derinlik farki (metre)
// * DEPTH_HOLD_GAIN, dikey itki olarak uygulanir (-1..1 sinirli). GAIN=2.0 ile
// ~0.5m sapmada tam duzeltme (doygunluk). Cok yavas/hizli tepki gorulurse ayarla;
// yon ters gelirse isaretini cevir.
constexpr float DEPTH_HOLD_GAIN = 2.0f;

// Otonom modda WegSh (kirmizi serit takibi) izi gorurken uygulanan sabit,
// dusuk ileri itki. Izi kaybedince (visible=false) surge 0'a duser - iz
// gorunmuyorsa korlemesine ilerlemek yerine dur.
constexpr float WEGSH_AUTONOMOUS_SURGE = 0.25f;
}

RovGUI::RovGUI(QWidget *parent)
    : QMainWindow(parent),
      ana_esp_thread(nullptr), mini_esp_thread(nullptr),
      ana_joy_thread(nullptr), mini_joy_thread(nullptr),
      cam_thread(nullptr), anarov_thread(nullptr),
      ana_lamp_on(false), mini_lamp_on(false), ana_autonomous(false), torpedo_ready(true), minirov_launched(false), dark_mode(false),
      ana_armed(false), mini_armed(false), ana_cam_connected(false),
      ana_stabilize(false), ana_roll(0.0f), ana_pitch(0.0f), ana_yaw(0.0f),
      ana_current_depth(0.0f), ana_depth_target(0.0f),
      ana_wegsh_yaw(0.0f), ana_wegsh_visible(false),
      cam_ping_proc(nullptr), anarov_ping_proc(nullptr),
      log_file(nullptr), log_stream(nullptr),
      motor_heartbeat_timer(nullptr)
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    ui.setupUi(central);

    for (int i = 0; i < 4; ++i) {
        ana_axes_state[i] = 0.0f;
        mini_axes_state[i] = 0.0f;
    }
    ana_last_pulses.fill(ESP_NEUTRAL_US);
    mini_last_pulses.fill(ESP_NEUTRAL_US);

    // Uygulama fullscreen açıldığı için pencere çerçevesi/kapatma düğmesi görünmez;
    // Ctrl+Q veya Esc ile çıkış kısayolu (Alt+F4 zaten pencere yöneticisi tarafından desteklenir)
    QShortcut *quitShortcutCtrlQ = new QShortcut(QKeySequence("Ctrl+Q"), this);
    connect(quitShortcutCtrlQ, &QShortcut::activated, this, &QWidget::close);
    QShortcut *quitShortcutEsc = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(quitShortcutEsc, &QShortcut::activated, this, &QWidget::close);

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

    // Kamera thread'lerini başlat
    start_camera_threads();

    // Ping işlemlerini başlat
    cam_ping_proc = new QProcess(this);
    connect(cam_ping_proc, &QProcess::readyReadStandardOutput,
            this, &RovGUI::read_cam_ping);
    cam_ping_proc->start("ping", QStringList() << "-i" << "1" << "192.168.88.2");

    anarov_ping_proc = new QProcess(this);
    connect(anarov_ping_proc, &QProcess::readyReadStandardOutput,
            this, &RovGUI::read_anarov_ping);
    anarov_ping_proc->start("ping", QStringList() << "-i" << "1" << "192.168.2.220");

    // Initialize default states for quick commands on GUI startup
    // Manuel active by default, Otonom inactive
    set_led(ui.anaRovPanel->led_manual, true);
    set_led(ui.anaRovPanel->led_autonomous, false);
    if (ui.anaRovPanel->led_minirov) set_led(ui.anaRovPanel->led_minirov, false);
    if (ui.anaRovPanel->led_torpedo) set_led(ui.anaRovPanel->led_torpedo, false);
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
        log_message("Uygulama Kapatılıyor...");
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
    connect(A->btn_torpedo,        &QPushButton::clicked, this, &RovGUI::on_torpedo_fire);
    connect(A->btn_lamp_on,        &QPushButton::clicked, this, &RovGUI::on_lamp_on_ana);
    connect(A->btn_lamp_off,       &QPushButton::clicked, this, &RovGUI::on_lamp_off_ana);
    connect(M->btn_lamp_on,        &QPushButton::clicked, this, &RovGUI::on_lamp_on_mini);
    connect(M->btn_lamp_off,       &QPushButton::clicked, this, &RovGUI::on_lamp_off_mini);

    // Temizlik / Tema Değişimi
    connect(ui.btn_theme, &QPushButton::clicked, this, &RovGUI::toggle_theme);
}

void RovGUI::start_camera_threads() {
    // MiniROV kamera (MJPEG @ 192.168.88.2/stream)
    cam_thread = new CameraThread("192.168.88.2", this);
    connect(cam_thread, &CameraThread::image_signal,  this, &RovGUI::update_camera_frame);
    connect(cam_thread, &CameraThread::status_signal, this, &RovGUI::update_camera_status);
    connect(cam_thread, &CameraThread::stats_signal,  this, &RovGUI::update_camera_stats);
    cam_thread->start();

    // AnaROV kamera (MJPEG @ 192.168.2.220:81/stream - bkz. AnaRovKamera/AnaRovKamera.ino,
    // Espressif CameraWebServer ornegi: ana sunucu 80'de, stream sunucusu 80+1=81'de acilir)
    anarov_thread = new CameraThread("192.168.2.220:81", this);
    connect(anarov_thread, &CameraThread::image_signal,  this, &RovGUI::update_anarov_frame);
    connect(anarov_thread, &CameraThread::status_signal, this, &RovGUI::update_anarov_status);
    connect(anarov_thread, &CameraThread::stats_signal,  this, &RovGUI::update_anarov_stats);
    anarov_thread->start();
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
std::array<int, 8> RovGUI::compute_motor_mix(float surge, float lateral, float yaw, float vertical,
                                                  float rollCorr, float pitchCorr) {
    std::array<int, 8> m{};
    for (int i = 0; i < 4; ++i) {
        float v = surge * W_SURGE[i] + lateral * W_LATERAL[i] + yaw * W_YAW[i];
        m[i] = qBound(1000, ESP_NEUTRAL_US + static_cast<int>(v * ESP_DELTA_US), 2000);
    }
    // M5=on sag, M6=on sol, M7=arka sag, M8=arka sol (bkz. MotorDiagramWidget).
    // Sabitleme kapaliyken rollCorr/pitchCorr 0 gelir, dördü de ayni deger olur.
    // Pitch negatif (burun asagida) oldugunda on motorlarin ARTMASI, arka
    // motorlarin AZALMASI gerekir (burnu yukari kaldirip duzeltmek icin) -
    // bu yuzden pitchCorr on'a eksi, arkaya arti isaretle ekleniyor.
    float verticalUs[4] = {
        vertical + rollCorr - pitchCorr, // M5 on-sag
        vertical - rollCorr - pitchCorr, // M6 on-sol
        vertical + rollCorr + pitchCorr, // M7 arka-sag
        vertical - rollCorr + pitchCorr, // M8 arka-sol
    };
    for (int i = 0; i < 4; ++i) {
        m[4 + i] = qBound(1000, ESP_NEUTRAL_US + static_cast<int>(verticalUs[i] * ESP_DELTA_US), 2000);
    }
    return m;
}

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
        rollCorr  = qBound(-1.0f, (ana_roll  / 90.0f) * STAB_GAIN, 1.0f);
        pitchCorr = qBound(-1.0f, (ana_pitch / 90.0f) * STAB_GAIN, 1.0f);
    }
    auto pulses = compute_motor_mix(surge, lateral, yaw, vertical, rollCorr, pitchCorr);
    ana_esp_thread->set_motors(pulses);
    ana_last_pulses = pulses;

    // Motor diyagraminda hangi motorlara ne kadar stabilize duzeltmesi
    // gittigini gostermek icin (bkz. compute_motor_mix'teki M5-M8 isaretleri).
    std::array<int, 8> correctionUs{};
    correctionUs[4] = static_cast<int>(( rollCorr - pitchCorr) * ESP_DELTA_US); // M5 on-sag
    correctionUs[5] = static_cast<int>((-rollCorr - pitchCorr) * ESP_DELTA_US); // M6 on-sol
    correctionUs[6] = static_cast<int>(( rollCorr + pitchCorr) * ESP_DELTA_US); // M7 arka-sag
    correctionUs[7] = static_cast<int>((-rollCorr + pitchCorr) * ESP_DELTA_US); // M8 arka-sol
    ui.anaRovPanel->motor_diagram->set_motor_pulses(pulses, correctionUs);
}

void RovGUI::apply_mini_motor_mix(float surge, float lateral, float yaw, float vertical) {
    if (!mini_esp_thread) return;
    auto pulses = compute_motor_mix(surge, lateral, yaw, vertical);
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
    const QString anaRovHost = "192.168.2.220";
    const quint16 anaRovBridgePort = 8888;
    ana_esp_thread = EspRovThread::createTcp(anaRovHost, anaRovBridgePort, this);
    connect(ana_esp_thread, &EspRovThread::status_signal, this, &RovGUI::update_ana_status);
    connect(ana_esp_thread, &EspRovThread::armed_signal,  this, &RovGUI::update_ana_armed);
    connect(ana_esp_thread, &EspRovThread::attitude_signal, this, &RovGUI::update_ana_attitude);
    connect(ana_esp_thread, &EspRovThread::depth_signal, this, &RovGUI::update_ana_depth);
    connect(ana_esp_thread, &EspRovThread::nem_signal, this, &RovGUI::update_ana_nem);
    connect(ana_esp_thread, &EspRovThread::wegsh_signal, this, &RovGUI::update_ana_wegsh);
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
        // basina degisince guncel kalsin diye WegSh/derinlik girdileriyle
        // birlikte yeniden hesaplanip gonderiyoruz.
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
    // - hedef oraya gecerken kilitlenir). Surge/yaw WegSh'ten gelir (bkz.
    // recompute_ana_autonomous_mix()).
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

// ESP32'nin "TORPEDO:kalan" satirina karsilik gelir - hangi torpidonun
// dolu/bos oldugunu operatorun elle takip etmesine gerek kalmasin diye
// "Torpido Durumu" "Hazır" yerine "kalan/5" seklinde gosterilir.
void RovGUI::update_ana_torpedo(int remaining) {
    torpedo_ready = remaining > 0;
    ui.anaRovPanel->lbl_servo_status->setText(QString("%1/5").arg(remaining));
    set_led(ui.anaRovPanel->led_torpedo, torpedo_ready);
}

// Kamera ESP'sinin WegSh (kirmizi serit tespiti) analizinden gelen periyodik
// "WEGSH:yaw,gorunur" verisi - motor komutuna DOGRUDAN degil, sadece son
// bilinen deger olarak kaydedilip recompute_ana_autonomous_mix() araciligiyla
// islenir (bkz. o fonksiyonun yorumu).
void RovGUI::update_ana_wegsh(float yaw, bool visible) {
    ana_wegsh_yaw = yaw;
    ana_wegsh_visible = visible;
    if (ana_autonomous && ana_esp_thread) {
        recompute_ana_autonomous_mix();
    }
}

// Otonom moddaki tek M: komutunu ureten ortak yer: WegSh'in yaw kararini
// (Kamera ESP, goruntu isleme) mevcut derinlik-hold duzeltmesiyle (Motor ESP,
// basinc sensoru) birlestirir. ATT:/DEPTH:/WEGSH: satirlarindan hangisi
// gelirse gelsin, son bilinen degerlerle burasi cagrilir - boylece uc ayri
// guncelleme yerinde ayni mantik tekrarlanmaz ve Motor ESP'ye cakisan iki
// ayri komut kaynagi gitmez (roll/pitch duzeltmesi apply_ana_motor_mix()
// icinde, ana_stabilize acikken, ayrica ekleniyor).
void RovGUI::recompute_ana_autonomous_mix() {
    if (!ana_autonomous || !ana_esp_thread) return;

    float surge = ana_wegsh_visible ? WEGSH_AUTONOMOUS_SURGE : 0.0f;
    float yaw = ana_wegsh_visible ? ana_wegsh_yaw : 0.0f;

    float depthErr = ana_depth_target - ana_current_depth; // pozitif: hedeften daha sigda, asagi itki gerekir
    float depthCorr = qBound(-1.0f, depthErr * DEPTH_HOLD_GAIN, 1.0f);

    apply_ana_motor_mix(surge, 0.0f, yaw, -depthCorr);
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

    // Buton bazlı aksiyon: A=lamba aç, B=lamba kapat, Y=torpido, X=miniROV bırak,
    // LB=sabitleme modu hızlı aç/kapa, RB=otonom, Back=manuel, Start=ARM/DISARM,
    // L3=acil durdurma
    if (state && ana_esp_thread) {
        if (name == "A")          on_lamp_on_ana();
        else if (name == "B")     on_lamp_off_ana();
        else if (name == "Y")     on_torpedo_fire();
        else if (name == "X")     on_minirov_launch();
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
    QLabel *lbl = ui.miniRovPanel->lbl_cam_stream;
    lbl->setPixmap(QPixmap::fromImage(img).scaled(lbl->size(),
                                                  Qt::KeepAspectRatio,
                                                  Qt::SmoothTransformation));
}

void RovGUI::update_camera_status(const QString &msg) {
    log_message("[CAM-MİNİ] " + msg, 1);
}

void RovGUI::update_camera_stats(int fps, float kbps, int w, int h) {
    RovPanel *M = ui.miniRovPanel;
    M->lbl_cam_fps->setText(QString("FPS: %1").arg(fps));
    M->lbl_cam_bitrate->setText(QString("Veri Hızı: %1 KB/s").arg(kbps, 0, 'f', 1));
    M->lbl_cam_res->setText(QString("Çözünürlük: %1x%2").arg(w).arg(h));
}

void RovGUI::update_anarov_frame(const QImage &img) {
    QLabel *lbl = ui.anaRovPanel->lbl_cam_stream;
    lbl->setPixmap(QPixmap::fromImage(img).scaled(lbl->size(),
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
    RovPanel *A = ui.anaRovPanel;
    A->lbl_cam_fps->setText(QString("FPS: %1").arg(fps));
    A->lbl_cam_bitrate->setText(QString("Veri Hızı: %1 KB/s").arg(kbps, 0, 'f', 1));
    A->lbl_cam_res->setText(QString("Çözünürlük: %1x%2").arg(w).arg(h));
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
    if (!ana_esp_thread) return;
    if (ana_autonomous) return;
    QPushButton *b = qobject_cast<QPushButton*>(sender());
    if (!b) return;
    apply_ana_motor_mix(0, 0, 0, 0);
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
    apply_mini_motor_mix(0, 0, 0, 0);
}

// ==================== Hızlı Komutlar ====================
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
    // devam eder. WegSh henuz taze bir "gorunur" verisi vermediyse surge/yaw
    // guvenli varsayilan olan 0'dir (bkz. recompute_ana_autonomous_mix());
    // dikey eksen bir sonraki derinlik okumasinda depth-hold tarafindan,
    // yon ise bir sonraki WEGSH: satirinda WegSh tarafindan devralinacak.
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

// Gercek ates alma isini (sirali secim, kalan sayi, 10sn bekleme) artik ESP32
// yapiyor (bkz. AnaRovBeyin.ino fireTorpedo()) - burada sadece komut
// gonderiyoruz; sonuc (kalan sayi veya ERR:TORPEDOEMPTY/COOLDOWN)
// update_ana_torpedo()/update_ana_status() uzerinden geri doner.
void RovGUI::on_torpedo_fire() {
    if (!ana_esp_thread) {
        log_message("[ANA] Önce bağlanın!", 0);
        return;
    }
    ana_esp_thread->torpedo();
    log_message("[ANA] Torpido fırlatma komutu gönderildi.", 0);
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
