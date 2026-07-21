#ifndef PIXHAWKGUI_H
#define PIXHAWKGUI_H

#include <QMainWindow>
#include <QSet>
#include <QMap>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <array>

#include "Ui_MainWindow.hpp"
#include "EspRovThread.hpp"
#include "JoystickThread.hpp"
#include "CameraThread.hpp"
#include "AnaRovThread.hpp"

class PixhawkGUI : public QMainWindow {
    Q_OBJECT
public:
    explicit PixhawkGUI(QWidget *parent = nullptr);
    ~PixhawkGUI();

    void log_message(const QString &msg, int target = -1); // 0: Ana, 1: Mini, -1: Her ikisi
    void show_screen_warning(const QString &msg); // Ekrani engellemeyen, kendiliginden kapanan uyari

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // Ana ROV baglanti (ESP32 seri port)
    void toggle_ana_connection();
    void update_ana_status(const QString &msg);
    void update_ana_armed(bool armed);
    void update_ana_attitude(float roll, float pitch);

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

private:
    void connect_signals();
    void start_camera_threads();
    void reset_labels(PixhawkPanel *panel);
    void set_led(QLabel *led, bool on);

    // ESP32 8 motor karisimi: surge (ileri+), lateral (sag+), yaw (saga don+),
    // vertical (yukari+) -> 8 motorun darbe genisligi (us). Bkz. MotorDiagramWidget
    // ve foto/ dizinindeki motor semasi. Isaretler ilk tahmindir; bir motor ters
    // donerse compute_motor_mix() icindeki ilgili agirligi ters cevirmek yeterlidir.
    static std::array<int, 8> compute_motor_mix(float surge, float lateral, float yaw, float vertical,
                                                 float rollCorr = 0.0f, float pitchCorr = 0.0f);
    void apply_ana_motor_mix(float surge, float lateral, float yaw, float vertical);
    void apply_mini_motor_mix(float surge, float lateral, float yaw, float vertical);

    Ui_MainWindow ui;

    // Thread'ler
    EspRovThread *ana_esp_thread;
    EspRovThread *mini_esp_thread;
    JoystickThread *ana_joy_thread;
    JoystickThread *mini_joy_thread;
    CameraThread *cam_thread;       // MiniROV kamera (MJPEG)
    AnaRovThread *anarov_thread;    // AnaROV kamera (WebSocket)

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

    // IMU (ESP32 uzerindeki MPU-6050) ile dengeleme (stabilize) modu: acikken roll/pitch
    // sapmasi dikey itki motorlarina otomatik fark olarak eklenir; kapaliyken
    // %100 manuel (hicbir IMU duzeltmesi uygulanmaz).
    bool ana_stabilize;
    float ana_roll;
    float ana_pitch;

    // Ping islemleri
    QProcess *cam_ping_proc;
    QProcess *anarov_ping_proc;

    // Log
    QFile *log_file;
    QTextStream *log_stream;
};

#endif // PIXHAWKGUI_H
