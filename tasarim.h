#ifndef TASARIM_H
#define TASARIM_H

#include <QWidget>
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QPolygonF>
#include <QTextEdit>
#include <QProgressBar>

class AttitudeIndicator : public QWidget {
    Q_OBJECT
public:
    explicit AttitudeIndicator(QWidget *parent = nullptr);
    void set_attitude(float roll, float pitch);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    float roll;
    float pitch;
};

class PixhawkPanel : public QWidget {
    Q_OBJECT
public:
    explicit PixhawkPanel(const QString &title, QWidget *parent = nullptr);

    QGroupBox *main_group;
    QVBoxLayout *main_layout;

    QHBoxLayout *conn_layout;
    QComboBox *port_combo;
    QComboBox *baud_combo;
    QPushButton *btn_connect;
    QLabel *lbl_status;

    QGridLayout *data_layout;

    QGroupBox *group_att;
    QHBoxLayout *layout_att;
    QGridLayout *text_att_layout;
    QLabel *lbl_roll;
    QLabel *lbl_pitch;
    QLabel *lbl_yaw;
    AttitudeIndicator *attitude_indicator;
    QProgressBar *bar_roll;

    QGroupBox *group_vfr;
    QGridLayout *layout_vfr;
    QLabel *lbl_alt;
    QLabel *lbl_heading;
    QLabel *lbl_speed;

    QGroupBox *group_sys;
    QGridLayout *layout_sys;
    QLabel *lbl_voltage;
    QLabel *lbl_battery;
    QLabel *lbl_servo_status;

    // Camera Components
    QGroupBox *group_cam;
    QHBoxLayout *layout_cam;
    QVBoxLayout *cam_stats_layout;
    QLabel *lbl_cam_fps;
    QLabel *lbl_cam_bitrate;
    QLabel *lbl_cam_res;
    QLabel *lbl_cam_ping;
    QLabel *lbl_cam_stream;

    // Fast Commands
    QGroupBox *group_commands;
    QHBoxLayout *layout_commands;
    QPushButton *btn_emergency;
    QPushButton *btn_stabilize;
    QPushButton *btn_start;
    QPushButton *btn_torpedo;
    QPushButton *btn_minirov_launch;
    
    QGridLayout *layout_directions;
    QPushButton *btn_forward;
    QPushButton *btn_backward;
    QPushButton *btn_left;
    QPushButton *btn_right;
    QPushButton *btn_up;
    QPushButton *btn_down;
    
    // Kumanda (Gamepad) Durumu
    QGroupBox *group_pad;
    QVBoxLayout *layout_pad;
    QComboBox *joy_combo;
    QPushButton *btn_joy_connect;
    QLabel *lbl_pad_status;
    QLabel *lbl_pad_axes;
    QLabel *lbl_pad_buttons;

    QTextEdit *terminal_log;
};

class Ui_MainWindow {
public:
    QWidget *main_widget;
    QVBoxLayout *main_layout;

    QHBoxLayout *panels_layout;
    PixhawkPanel *anaRovPanel;
    PixhawkPanel *miniRovPanel;

    void setupUi(QWidget *MainWindow);
    void apply_styles(QWidget *MainWindow);
};

#endif // TASARIM_H
