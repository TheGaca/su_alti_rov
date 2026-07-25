#ifndef ROVPANEL_H
#define ROVPANEL_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QProgressBar>
#include <QResizeEvent>

#include "AttitudeIndicator.hpp"
#include "ColorPickerButton.hpp"
#include "MotorDiagramWidget.hpp"

// Tek bir ROV (Ana ya da Mini) icin tum panel
class RovPanel : public QWidget {
    Q_OBJECT
public:
    explicit RovPanel(const QString &title, QWidget *parent = nullptr);
    explicit RovPanel(const QString &title, bool mini, QWidget *parent = nullptr);
    // Note: mini flag indicates Mini ROV panel
    bool isMini = false;


    // Ust baglanti bari
    QGroupBox *main_group;
    QVBoxLayout *main_layout;
    QHBoxLayout *conn_layout;
    QComboBox *port_combo;
    QComboBox *baud_combo;
    QPushButton *btn_connect;
    QLabel *lbl_status;

    // Yonelim (Attitude) - Ana ROV'da MPU-6050 IMU'dan roll/pitch gelir (bkz.
    // EspRovThread ATT: satiri). Yaw bu ivmeolcerle olculemez, hep "---" kalir.
    // Mini ROV'da IMU yok, bu alanlar hep "---" olarak kalir.
    QGroupBox *group_att;
    QHBoxLayout *layout_att;
    QGridLayout *text_att_layout;
    QLabel *lbl_roll;
    QLabel *lbl_pitch;
    QLabel *lbl_yaw;
    AttitudeIndicator *attitude_indicator;
    QLabel *lbl_alt_title;       // "Irtifa (m):"
    QProgressBar *bar_alt;       // dikey irtifa bari

    // Seyrusefer (VFR HUD)
    QGroupBox *group_vfr;
    QGridLayout *layout_vfr;
    QLabel *lbl_alt;
    QLabel *lbl_heading;
    QLabel *lbl_speed;

    // Sistem & Sensor
    QGroupBox *group_sys;
    QGridLayout *layout_sys;
    QLabel *lbl_voltage;
    QLabel *lbl_battery;
    QLabel *lbl_servo_status;
    QLabel *lbl_lamp_status;
    QLabel *lbl_stabilize_status; // Ana ROV'da sabitleme (IMU) modu; Mini'de nullptr
    QLabel *lbl_humidity;    // DHT11 nem (%) - Ana ROV'da; Mini'de nullptr
    QLabel *lbl_dht_temp;    // DHT11 sicaklik (C) - Ana ROV'da; Mini'de nullptr

    // Kamera Akisi (icinde stats + video + yon butonlari + Arm/Disarm)
    QGroupBox *group_cam;
    QHBoxLayout *layout_cam;
    QVBoxLayout *cam_stats_layout;
    QLabel *lbl_cam_fps;
    QLabel *lbl_cam_bitrate;
    QLabel *lbl_cam_res;
    QLabel *lbl_cam_ping;
    QLabel *lbl_cam_stream;

    // Yon butonlari (Kamera grubunun SAGINDA)
    QWidget *dir_widget;
    QGridLayout *layout_directions;
    QPushButton *btn_turn_left;   // Sola Don
    QPushButton *btn_turn_right;  // Saga Don
    QPushButton *btn_forward;     // Ileri
    QPushButton *btn_backward;    // Geri
    QPushButton *btn_left;        // Sol
    QPushButton *btn_right;       // Sag
    QPushButton *btn_up;          // Z+
    QPushButton *btn_down;        // Z-
    QPushButton *btn_stabilize;   // ARM Et / DISARM Et (eskiden "Sabitle")

    // Motorlarin canli durumunu gosteren diyagram (8 motor, bkz. MotorDiagramWidget)
    MotorDiagramWidget *motor_diagram;

    // Hizli Komutlar (gizli)
    QGroupBox *group_commands;
    QHBoxLayout *layout_commands;
    QPushButton *btn_emergency;     // ACIL DURDURMA
    QPushButton *btn_autonomous;    // Otonom
    QPushButton *btn_manual;        // Manuel
    QPushButton *btn_minirov_launch;// MiniROV Birak
    QPushButton *btn_torpedo;       // Torpido Firlat
    QLabel *led_autonomous;
    QLabel *led_manual;
    QLabel *led_minirov;
    QLabel *led_torpedo;
    QLabel *lbl_color_title;
    QLabel *color_box;              // Renk secici mor kutu
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

    // Terminal (siyah arka plan, acik mavi yazi)
    QTextEdit *terminal_log;

    // Mini ROV henuz Ethernet'e gecmedi (IP bilgisi bekleniyor) - gecici
    // olarak panelin tamamini kaplayan, tiklamalari ENGELLEMEYEN (mevcut
    // seri baglanti calismaya devam etsin diye) yari saydam "GELECEK" katmani.
    // Sadece Mini panelde olusturulur (isMini true), Ana panelde nullptr kalir.
    QLabel *coming_soon_overlay = nullptr;

protected:
    void resizeEvent(QResizeEvent *event) override;
};

#endif // ROVPANEL_H
