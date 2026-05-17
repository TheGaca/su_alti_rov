#ifndef YAZILIM_H
#define YAZILIM_H

#include <QMainWindow>
#include <QSet>
#include <QMap>
#include <QFile>
#include <QTextStream>
#include "tasarim.h"
#include "algoritma.h"
#include <QProcess>

class PixhawkGUI : public QMainWindow {
    Q_OBJECT
public:
    explicit PixhawkGUI(QWidget *parent = nullptr);
    ~PixhawkGUI();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // Ana ROV
    void toggle_ana_connection();
    void start_ana_connection();
    void stop_ana_connection();
    void update_ana_attitude(float roll, float pitch, float yaw);
    void update_ana_vfr_hud(float alt, float heading, float speed);
    void update_ana_battery(float voltage, int remaining);
    void update_ana_status(const QString &msg);

    // Mini ROV
    void toggle_mini_connection();
    void start_mini_connection();
    void stop_mini_connection();
    void update_mini_attitude(float roll, float pitch, float yaw);
    void update_mini_vfr_hud(float alt, float heading, float speed);
    void update_mini_battery(float voltage, int remaining);
    void update_mini_status(const QString &msg);
    
    // Ortak
    void start_ana_joystick(QString port);
    void start_mini_joystick(QString port);
    void start_camera();
    void start_anarov();

    void update_ana_joy_status(const QString &msg);
    void update_ana_joy_button(int btn_id, int state);
    void update_ana_joy_axis(int axis_id, float value);

    void update_mini_joy_status(const QString &msg);
    void update_mini_joy_button(int btn_id, int state);
    void update_mini_joy_axis(int axis_id, float value);

    void update_camera_frame(const QImage &q_img);
    void update_camera_status(const QString &msg);
    void update_camera_stats(int fps, float kbps, int width, int height);

    void update_anarov_frame(const QImage &q_img);
    void update_anarov_status(const QString &msg);
    void update_anarov_stats(int fps, float kbps, int width, int height);

    void read_cam_ping();
    void read_anarov_ping();

public:
    void log_message(const QString &msg, int target = -1); // 0: Ana, 1: Mini, -1: Her ikisi

private:
    void reset_labels(PixhawkPanel *panel);
    void toggle_servo();
    void set_servo_pwm(int servo_no, int pwm);

    Ui_MainWindow ui;
    
    MAVLinkThread *ana_mav_thread;
    MAVLinkThread *mini_mav_thread;
    JoystickThread *ana_joy_thread;
    JoystickThread *mini_joy_thread;
    CameraThread *cam_thread;
    AnaRovThread *anarov_thread;
    
    QMap<int, QString> button_map;
    float ana_axes_state[2];
    float mini_axes_state[2];
    QSet<QString> ana_pressed_buttons;
    QSet<QString> mini_pressed_buttons;

    bool servo_open;
    int servo_pin;

    QProcess *cam_ping_proc;
    QProcess *anarov_ping_proc;

    QFile *log_file;
    QTextStream *log_stream;
};

#endif // YAZILIM_H
