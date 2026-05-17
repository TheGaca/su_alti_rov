#ifndef YAZILIM_H
#define YAZILIM_H

#include <QMainWindow>
#include <QSet>
#include <QMap>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include "tasarim.h"
#include "algoritma.h"

class PixhawkGUI : public QMainWindow {
    Q_OBJECT
public:
    explicit PixhawkGUI(QWidget *parent = nullptr);
    ~PixhawkGUI();

    void log_message(const QString &msg, int target = -1); // 0: Ana, 1: Mini, -1: Her ikisi

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // Ana ROV bağlantı
    void toggle_ana_connection();
    void update_ana_attitude(float roll, float pitch, float yaw);
    void update_ana_vfr_hud(float alt, float heading, float speed);
    void update_ana_battery(float voltage, int remaining);
    void update_ana_status(const QString &msg);

    // Mini ROV bağlantı
    void toggle_mini_connection();
    void update_mini_attitude(float roll, float pitch, float yaw);
    void update_mini_vfr_hud(float alt, float heading, float speed);
    void update_mini_battery(float voltage, int remaining);
    void update_mini_status(const QString &msg);

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

    // Yön butonları - basıldıkça PWM gönder, bırakınca nötrle
    void ana_dir_pressed();
    void ana_dir_released();
    void mini_dir_pressed();
    void mini_dir_released();

    // Hızlı komutlar
    void on_emergency_ana();
    void on_emergency_mini();
    void on_stabilize_ana();
    void on_stabilize_mini();
    void on_autonomous_ana();
    void on_manual_ana();
    void on_minirov_launch();
    void on_torpedo_fire();
    void on_lamp_on_ana();
    void on_lamp_off_ana();
    void on_lamp_on_mini();
    void on_lamp_off_mini();
    void toggle_theme();

private:
    void connect_signals();
    void start_camera_threads();
    void reset_labels(PixhawkPanel *panel);
    void set_led(QLabel *led, bool on);

    Ui_MainWindow ui;

    // Thread'ler
    MAVLinkThread *ana_mav_thread;
    MAVLinkThread *mini_mav_thread;
    JoystickThread *ana_joy_thread;
    JoystickThread *mini_joy_thread;
    CameraThread *cam_thread;       // MiniROV kamera (MJPEG)
    AnaRovThread *anarov_thread;    // AnaROV kamera (WebSocket)

    // Joystick durum
    QMap<int, QString> button_map;
    float ana_axes_state[2];
    float mini_axes_state[2];
    QSet<QString> ana_pressed_buttons;
    QSet<QString> mini_pressed_buttons;

    // Servo durumları
    bool ana_lamp_on;
    bool mini_lamp_on;
    bool ana_autonomous;
    bool torpedo_ready;
    bool minirov_launched;
    bool dark_mode;

    // Ping işlemleri
    QProcess *cam_ping_proc;
    QProcess *anarov_ping_proc;

    // Log
    QFile *log_file;
    QTextStream *log_stream;
};

#endif // YAZILIM_H