#ifndef ROVGUI_H
#define ROVGUI_H

#include <QMainWindow>
#include <QSet>
#include <QMap>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QTimer>
#include <array>

#include "Ui_MainWindow.hpp"
#include "EspRovThread.hpp"
#include "JoystickThread.hpp"
#include "CameraThread.hpp"
#include "RovSettings.hpp"
#include "AviMjpegWriter.hpp"

class RovGUI : public QMainWindow {
    Q_OBJECT
public:
    explicit RovGUI(QWidget *parent = nullptr);
    ~RovGUI();

    void log_message(const QString &msg, int target = -1); // 0: Ana, 1: Mini, -1: Her ikisi
    void show_screen_warning(const QString &msg); // Ekrani engellemeyen, kendiliginden kapanan uyari

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // Ana ROV baglanti (ESP32 seri port)
    void toggle_ana_connection();
    void update_ana_status(const QString &msg);
    void update_ana_armed(bool armed);
    void update_ana_attitude(float roll, float pitch, float yaw);
    void update_ana_depth(float meters, float vertical_speed_ms);
    void update_ana_nem(float humidity_pct, float temperature_c);
    void update_ana_torpedo(int remaining);

    // Mini ROV baglanti (ESP32 seri port)
    void toggle_mini_connection();
    void update_mini_status(const QString &msg);
    void update_mini_armed(bool armed);

    // Joystick
    void toggle_ana_joystick();
    void toggle_mini_joystick();
    void update_ana_joy_status(const QString &msg);
    void update_ana_joy_button(int btn_id, int state);
    void update_ana_joy_axis(int axis_id, float value);
    void update_mini_joy_status(const QString &msg);
    void update_mini_joy_button(int btn_id, int state);
    void update_mini_joy_axis(int axis_id, float value);

    // Kamera + AnaRov video
    void update_camera_frame(const QImage &img);
    void update_camera_status(const QString &msg);
    void update_camera_stats(int fps, float kbps, int w, int h);
    void update_anarov_frame(const QImage &img);
    void update_anarov_status(const QString &msg);
    void update_anarov_stats(int fps, float kbps, int w, int h);

    // Ping
    void read_cam_ping();
    void read_anarov_ping();

    // Yon butonlari - basildikca motor mixi gonder, birakinca notrle
    void ana_dir_pressed();
    void ana_dir_released();
    void mini_dir_pressed();
    void mini_dir_released();

    // ESP32 tarafindaki FAILSAFE_MS (500ms) suresi icinde yeni komut gelmezse
    // motorlar notrlenir. Kumanda/tus basili tutulurken eksen ya da buton olayi
    // tekrar tetiklenmedigi surece (ornegin stick sabit bir konumda tutulunca)
    // yeni komut gitmez; bu da "bir kere calisip duruyor" sorununa yol acar.
    // Bu heartbeat, basili tutulan yon oldugu surece son komutu periyodik olarak
    // tekrar gonderip failsafe'i tetiklenmeden once tazeler.
    void send_motor_heartbeat();

    // Hizli komutlar
    void on_emergency_ana();
    void on_emergency_mini();
    void on_stabilize_ana();  // ARM/DISARM toggle
    void on_stabilize_mini(); // ARM/DISARM toggle
    void on_autonomous_ana();
    void on_manual_ana();
    void on_minirov_launch();
    void on_torpedo_fire();
    void on_lamp_on_ana();
    void on_lamp_off_ana();
    void on_lamp_on_mini();
    void on_lamp_off_mini();
    void toggle_theme();
    void toggle_stabilize_mode_ana(); // Kumandadan IMU destekli dengeleme modu ac/kapa

    // Ayarlar penceresini acar (IP/port + kontrol kazanclari, bkz. RovSettings)
    void open_settings();

    // Anlik goruntu (HUD'lu PNG) ve video kaydi (ham MJPEG-AVI) - dosyalar
    // ~/rov_media/ altina zaman damgali adlarla yazilir.
    void on_ana_snapshot();
    void on_mini_snapshot();
    void toggle_ana_record();
    void toggle_mini_record();

    // CameraThread'den gelen ham JPEG kareleri - kayit acikken AVI'ye yazilir
    void on_ana_jpeg(const QByteArray &jpeg);
    void on_mini_jpeg(const QByteArray &jpeg);

    // Kumanda (joystick) cihazi koptugunda cagrilir - motorlari NOTRLER.
    // Bu olmadan heartbeat, kol koptugu andaki son komutu (ornegin tam ileri)
    // sonsuza dek tekrarlar ve ESP32'nin 500ms failsafe'i hic tetiklenmezdi.
    void on_ana_joystick_lost();
    void on_mini_joystick_lost();

    // Space tusuyla iki aracta birden acil durdurma (motor notrle + DISARM)
    void on_global_emergency();

private:
    void connect_signals();
    void start_camera_threads();
    void start_ping_processes();
    void reset_labels(RovPanel *panel);
    void set_led(QLabel *led, bool on);

    // Ana ROV videosunun uzerine telemetri bindirme (HUD): derinlik, pusula,
    // roll/pitch, ARM/mod/sabitleme durumu, merkez artisi, REC gostergesi.
    // Goruntunun kendisine cizilir; snapshot'ta da gorunur.
    void draw_ana_hud(QImage &img);
    // Kayit acikken sag ust koseye kirmizi REC rozeti (mini panelde HUD yok)
    void draw_rec_badge(QImage &img);
    static QString media_dir(); // ~/rov_media (yoksa olusturur)

    // Motor karisimi hesabi MotorMixer::compute()'ta (include/MotorMixer.hpp,
    // Qt'siz, birim testli); asagidakiler sonucu thread'e gonderip arayuzu
    // (motor diyagrami) gunceller.
    void apply_ana_motor_mix(float surge, float lateral, float yaw, float vertical);
    void apply_mini_motor_mix(float surge, float lateral, float yaw, float vertical);

    // Otonom moddaki motor karisimini tek bir yerden hesaplar: mevcut
    // derinlik-hold ve roll/pitch stabilizasyonuyla (IMU, Motor ESP'den)
    // birlestirir. ATT:/DEPTH: satirlarindan HANGISI gelirse gelsin (iki ayri
    // kaynaktan, farkli hizlarda), son bilinen degerlerle bu fonksiyon cagrilir
    // - boylece iki yerde ayni mantik tekrar edilmez ve tek bir M: komutu
    // (Motor ESP'ye) uretilir, cakisan/birbirini gecersiz kilan iki ayri komut
    // kaynagi olmaz.
    void recompute_ana_autonomous_mix();

    Ui_MainWindow ui;

    // Kullanicinin degistirebildigi kalici ayarlar (IP'ler, kazanclar) -
    // bkz. RovSettings.hpp ve open_settings()
    RovSettings settings;

    // Thread'ler
    EspRovThread *ana_esp_thread;
    EspRovThread *mini_esp_thread;
    JoystickThread *ana_joy_thread;
    JoystickThread *mini_joy_thread;
    CameraThread *cam_thread;       // MiniROV kamera (MJPEG)
    CameraThread *anarov_thread;    // AnaROV kamera (MJPEG @ AnaRovKamera.ino)

    // Joystick durum: [0]=Sol Stick X, [1]=Sol Stick Y, [2]=Sağ Stick X, [3]=Sağ Stick Y
    QMap<int, QString> button_map;
    float ana_axes_state[4];
    float mini_axes_state[4];
    QSet<QString> ana_pressed_buttons;
    QSet<QString> mini_pressed_buttons;

    // Servo/motor durumlari
    bool ana_lamp_on;
    bool mini_lamp_on;
    bool ana_autonomous;
    bool torpedo_ready;
    bool minirov_launched;
    bool dark_mode;
    bool ana_armed;
    bool mini_armed;
    bool ana_cam_connected;

    // IMU (ESP32 uzerindeki BNO055) ile dengeleme (stabilize) modu: acikken roll/pitch
    // sapmasi dikey itki motorlarina otomatik fark olarak eklenir; kapaliyken
    // %100 manuel (hicbir IMU duzeltmesi uygulanmaz).
    bool ana_stabilize;
    float ana_roll;
    float ana_pitch;
    float ana_yaw; // BNO055'in manyetometreli pusulasindan; su an sadece
                    // gosterge/log icin - motor karisimina henuz dahil edilmiyor
                    // (yaw-hold ileride istenirse buradan baslanir).

    // Otonom moda gecince otomatik acilan irtifa (derinlik) sabitleme: hedef
    // derinlik, otonoma gecildigi andaki mevcut derinlige kilitlenir; sonraki
    // her derinlik okumasinda bu hedeften sapma dikey itkiye duzeltme olarak
    // eklenir (bkz. update_ana_depth()). ana_current_depth manuel modda da
    // guncellenir (hedefi yakalamak icin), duzeltme ise sadece ana_autonomous
    // acikken uygulanir.
    float ana_current_depth;
    float ana_depth_target;

    // Ping islemleri
    QProcess *cam_ping_proc;
    QProcess *anarov_ping_proc;

    // Son cozulmus kareler (snapshot ve kayit boyutlari icin) + kayitcilar.
    // ana_last_frame HUD islenmis halidir (snapshot'ta telemetri gorunsun);
    // videoya ise CameraThread'in ham JPEG'leri yazilir (kara kutu kaydi).
    QImage ana_last_frame;
    QImage mini_last_frame;
    AviMjpegWriter ana_recorder;
    AviMjpegWriter mini_recorder;
    int ana_cam_fps = 0;  // son olculen FPS - kayit dosyasinin oynatma hizi icin
    int mini_cam_fps = 0;

    // Kumanda/tus basili tutulurken ESP32 failsafe'ini (500ms) tazelemek icin
    // son gonderilen motor darbelerini tutar ve periyodik olarak yeniden gonderir.
    QTimer *motor_heartbeat_timer;
    std::array<int, 8> ana_last_pulses;
    std::array<int, 8> mini_last_pulses;

    // Log
    QFile *log_file;
    QTextStream *log_stream;
};

#endif // ROVGUI_H
