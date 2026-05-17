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
#include <QColorDialog>
#include <QColor>

// Suni ufuk (attitude indicator) - yuvarlak, mavi/kahverengi yarım küre + sarı üst üçgen + turuncu uçak
// Color picker button widget
class ColorPickerButton : public QPushButton {
    Q_OBJECT
public:
    explicit ColorPickerButton(QWidget *parent = nullptr);
    void setColor(const QColor &c);
signals:
    void colorChosen(const QColor &c);
private slots:
    void on_clicked();
private:
    QColor m_color;
    void refreshStyle();
};
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

// Tek bir ROV (Ana ya da Mini) için tüm panel
class PixhawkPanel : public QWidget {
    Q_OBJECT
public:
    explicit PixhawkPanel(const QString &title, QWidget *parent = nullptr);
    explicit PixhawkPanel(const QString &title, bool mini, QWidget *parent = nullptr);
    // Note: mini flag indicates Mini ROV panel
    bool isMini = false;


    // Üst bağlantı barı
    QGroupBox *main_group;
    QVBoxLayout *main_layout;
    QHBoxLayout *conn_layout;
    QComboBox *port_combo;
    QComboBox *baud_combo;
    QPushButton *btn_connect;
    QLabel *lbl_status;

    // Yönelim (Attitude)
    QGroupBox *group_att;
    QHBoxLayout *layout_att;
    QGridLayout *text_att_layout;
    QLabel *lbl_roll;
    QLabel *lbl_pitch;
    QLabel *lbl_yaw;
    AttitudeIndicator *attitude_indicator;
    QLabel *lbl_alt_title;       // "İrtifa (m):"
    QProgressBar *bar_alt;       // dikey irtifa barı

    // Seyrüsefer (VFR HUD)
    QGroupBox *group_vfr;
    QGridLayout *layout_vfr;
    QLabel *lbl_alt;
    QLabel *lbl_heading;
    QLabel *lbl_speed;

    // Sistem & Sensör
    QGroupBox *group_sys;
    QGridLayout *layout_sys;
    QLabel *lbl_voltage;
    QLabel *lbl_battery;
    QLabel *lbl_servo_status;
    QLabel *lbl_lamp_status;

    // Kamera Akışı (içinde stats + video + yön butonları + Sabitle)
    QGroupBox *group_cam;
    QHBoxLayout *layout_cam;
    QVBoxLayout *cam_stats_layout;
    QLabel *lbl_cam_fps;
    QLabel *lbl_cam_bitrate;
    QLabel *lbl_cam_res;
    QLabel *lbl_cam_ping;
    QLabel *lbl_cam_stream;

    // Yön butonları (Kamera grubunun SAĞINDA)
    QWidget *dir_widget;
    QGridLayout *layout_directions;
    QPushButton *btn_turn_left;   // Sola Dön
    QPushButton *btn_turn_right;  // Sağa Dön
    QPushButton *btn_forward;     // İleri
    QPushButton *btn_backward;    // Geri
    QPushButton *btn_left;        // Sol
    QPushButton *btn_right;       // Sağ
    QPushButton *btn_up;          // Z+
    QPushButton *btn_down;        // Z-
    QPushButton *btn_stabilize;   // Sabitle (büyük mavi)

    // Hızlı Komutlar (gizli)
    QGroupBox *group_commands;
    QHBoxLayout *layout_commands;
    QPushButton *btn_emergency;     // ACİL DURDURMA
    QPushButton *btn_autonomous;    // Otonom
    QPushButton *btn_manual;        // Manuel
    QPushButton *btn_minirov_launch;// MiniROV Bırak
    QPushButton *btn_torpedo;       // Torpido Fırlat
    QLabel *led_autonomous;
    QLabel *led_manual;
    QLabel *led_minirov;
    QLabel *led_torpedo;
    QLabel *lbl_color_title;
    QLabel *color_box;              // Renk seçici mor kutu
    QPushButton *color_btn;         // Color picker button
    QPushButton *btn_lamp_on;
    QPushButton *btn_lamp_off;
    QLabel *led_lamp_on;
    QLabel *led_lamp_off;

    // Kumanda (Gamepad) Durumu
    QGroupBox *group_pad;
    QVBoxLayout *layout_pad;
    QComboBox *joy_combo;
    QPushButton *btn_joy_connect;
    QLabel *lbl_pad_status;
    QLabel *lbl_pad_axes;
    QLabel *lbl_pad_buttons;

    // Terminal (siyah arka plan, açık mavi yazı)
    QTextEdit *terminal_log;
};

class Ui_MainWindow {
public:
    QWidget *main_widget;
    QVBoxLayout *main_layout;

    QHBoxLayout *panels_layout;
    PixhawkPanel *anaRovPanel;
    PixhawkPanel *miniRovPanel;
    QPushButton *btn_theme;

    void setupUi(QWidget *MainWindow);
    void apply_styles(QWidget *MainWindow, bool dark = false);
};

#endif // TASARIM_H