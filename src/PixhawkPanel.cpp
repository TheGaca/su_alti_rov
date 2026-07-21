#include "PixhawkPanel.hpp"

#include <QSerialPortInfo>
#include <QDir>
#include <QFileInfoList>

PixhawkPanel::PixhawkPanel(const QString &title, bool mini, QWidget *parent)
    : QWidget(parent), isMini(mini) {

    main_group = new QGroupBox(title, this);
    main_group->setObjectName("rov_main_group");

    main_layout = new QVBoxLayout(main_group);
    main_layout->setSpacing(8);
    main_layout->setContentsMargins(8, 20, 8, 8);

    // ====== Üst bağlantı barı ======
    conn_layout = new QHBoxLayout();
    conn_layout->setSpacing(6);
    QLabel *lbl_port = new QLabel("Port:");
    lbl_port->setObjectName("section_label");
    port_combo = new QComboBox();
    port_combo->setMinimumWidth(150);
    port_combo->setMinimumHeight(28);
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
        QString portName = info.systemLocation();
        if (portName.contains("ttyUSB") || portName.contains("ttyACM") || info.vendorIdentifier() != 0) {
            port_combo->addItem(portName);
        }
    }
    if (port_combo->count() == 0) port_combo->addItem("/dev/ttyUSB0");

    QLabel *lbl_baud = new QLabel("Baudrate:");
    lbl_baud->setObjectName("section_label");
    baud_combo = new QComboBox();
    baud_combo->setMinimumHeight(28);
    baud_combo->addItems({"9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"});
    baud_combo->setCurrentText("115200");

    btn_connect = new QPushButton("Bağlan");
    btn_connect->setObjectName("btn_connect");
    btn_connect->setMinimumSize(90, 30);

    lbl_status = new QLabel("Durum: Bekleniyor...");
    lbl_status->setObjectName("lbl_status");

    conn_layout->addWidget(lbl_port);
    conn_layout->addWidget(port_combo);
    conn_layout->addWidget(lbl_baud);
    conn_layout->addWidget(baud_combo);
    conn_layout->addWidget(btn_connect);
    conn_layout->addWidget(lbl_status, 1);
    main_layout->addLayout(conn_layout);

    // ====== Üst sıra: Yönelim (sol, geniş) | VFR+Sys (sağ kolon) ======
    QHBoxLayout *topRow = new QHBoxLayout();
    topRow->setSpacing(8);

    group_att = new QGroupBox("Yönelim (Attitude)");
    layout_att = new QHBoxLayout(group_att);
    layout_att->setSpacing(12);
    layout_att->setContentsMargins(10, 18, 10, 10);

    // Sol: Roll/Pitch/Yaw (büyük font)
    text_att_layout = new QGridLayout();
    text_att_layout->setHorizontalSpacing(10);
    text_att_layout->setVerticalSpacing(18);
    QLabel *lr = new QLabel("Roll:");
    QLabel *lp = new QLabel("Pitch:");
    QLabel *ly = new QLabel("Yaw:");
    lr->setObjectName("att_title");
    lp->setObjectName("att_title");
    ly->setObjectName("att_title");
    lbl_roll = new QLabel("---");
    lbl_pitch = new QLabel("---");
    lbl_yaw = new QLabel("---");
    lbl_roll->setObjectName("attval");
    lbl_pitch->setObjectName("attval");
    lbl_yaw->setObjectName("attval");
    lbl_roll->setMinimumWidth(60);
    lbl_pitch->setMinimumWidth(60);
    lbl_yaw->setMinimumWidth(60);
    text_att_layout->addWidget(lr, 0, 0);
    text_att_layout->addWidget(lbl_roll, 0, 1);
    text_att_layout->addWidget(lp, 1, 0);
    text_att_layout->addWidget(lbl_pitch, 1, 1);
    text_att_layout->addWidget(ly, 2, 0);
    text_att_layout->addWidget(lbl_yaw, 2, 1);
    layout_att->addLayout(text_att_layout);

    // Orta: BÜYÜK attitude indicator
    attitude_indicator = new AttitudeIndicator();
    layout_att->addWidget(attitude_indicator, 0, Qt::AlignCenter);

    // Sağ: İrtifa dikey barı
    QVBoxLayout *altCol = new QVBoxLayout();
    lbl_alt_title = new QLabel("İrtifa (m)");
    lbl_alt_title->setAlignment(Qt::AlignCenter);
    lbl_alt_title->setObjectName("section_label");
    bar_alt = new QProgressBar();
    bar_alt->setOrientation(Qt::Vertical);
    bar_alt->setRange(0, 100);
    bar_alt->setValue(15);
    bar_alt->setTextVisible(false);
    bar_alt->setMinimumWidth(34);
    bar_alt->setMaximumWidth(40);
    bar_alt->setMinimumHeight(220);
    altCol->addWidget(lbl_alt_title);
    altCol->addWidget(bar_alt, 1, Qt::AlignHCenter);
    layout_att->addLayout(altCol);

    topRow->addWidget(group_att, 3);

    // Sağ kolon: VFR + Sistem
    QVBoxLayout *rightCol = new QVBoxLayout();
    rightCol->setSpacing(8);

    group_vfr = new QGroupBox("Seyrüsefer (VFR HUD)");
    layout_vfr = new QGridLayout(group_vfr);
    layout_vfr->setVerticalSpacing(14);
    layout_vfr->setContentsMargins(10, 18, 10, 10);
    QLabel *li1 = new QLabel("İrtifa (m):");
    QLabel *li2 = new QLabel("Pusula Yönü:");
    QLabel *li3 = new QLabel("Hız (m/s):");
    lbl_alt = new QLabel("---");
    lbl_heading = new QLabel("---");
    lbl_speed = new QLabel("---");
    for (QLabel *l : {lbl_alt, lbl_heading, lbl_speed}) {
        l->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        l->setObjectName("hud_val");
        l->setMinimumWidth(80);
    }
    layout_vfr->addWidget(li1, 0, 0); layout_vfr->addWidget(lbl_alt, 0, 1);
    layout_vfr->addWidget(li2, 1, 0); layout_vfr->addWidget(lbl_heading, 1, 1);
    layout_vfr->addWidget(li3, 2, 0); layout_vfr->addWidget(lbl_speed, 2, 1);
    rightCol->addWidget(group_vfr);

    group_sys = new QGroupBox("Sistem & Sensör");
    layout_sys = new QGridLayout(group_sys);
    layout_sys->setVerticalSpacing(14);
    layout_sys->setContentsMargins(10, 18, 10, 10);
    QLabel *ls1 = new QLabel("Batarya Voltaj:");
    QLabel *ls2 = new QLabel("Batarya %:");
    QLabel *ls3 = new QLabel(isMini ? "AUX 1 Servo:" : "Torpido Durumu:");
    QLabel *ls4 = new QLabel("Lamba Durumu:");
    lbl_voltage = new QLabel("---");
    lbl_battery = new QLabel("---");
    lbl_servo_status = new QLabel(isMini ? "Kapalı" : "Hazır");
    lbl_lamp_status = new QLabel("Kapalı");
    lbl_voltage->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lbl_battery->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lbl_servo_status->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lbl_lamp_status->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    lbl_voltage->setObjectName("hud_val");
    lbl_battery->setObjectName("hud_val");
    lbl_servo_status->setObjectName("servo_status");
    lbl_lamp_status->setObjectName("lamp_status");

    lbl_servo_status->setStyleSheet("color:#c0392b;font-weight:bold;font-size:14px;");
    lbl_lamp_status->setStyleSheet("color:#c0392b;font-weight:bold;font-size:14px;");

    layout_sys->addWidget(ls1, 0, 0); layout_sys->addWidget(lbl_voltage, 0, 1);
    layout_sys->addWidget(ls2, 1, 0); layout_sys->addWidget(lbl_battery, 1, 1);
    layout_sys->addWidget(ls3, 2, 0); layout_sys->addWidget(lbl_servo_status, 2, 1);
    layout_sys->addWidget(ls4, 3, 0); layout_sys->addWidget(lbl_lamp_status, 3, 1);

    if (!isMini) {
        QLabel *ls5 = new QLabel("Sabitleme Modu:");
        lbl_stabilize_status = new QLabel("Kapalı");
        lbl_stabilize_status->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        lbl_stabilize_status->setObjectName("lamp_status");
        lbl_stabilize_status->setStyleSheet("color:#c0392b;font-weight:bold;font-size:14px;");
        layout_sys->addWidget(ls5, 4, 0); layout_sys->addWidget(lbl_stabilize_status, 4, 1);
    } else {
        lbl_stabilize_status = nullptr;
    }
    rightCol->addWidget(group_sys);

    topRow->addLayout(rightCol, 2);
    main_layout->addLayout(topRow);

    // ====== Kamera Akışı (sol stats büyük + video + sağ yön butonları büyük + Arm/Disarm) ======
    group_cam = new QGroupBox("Kamera Akışı");
    layout_cam = new QHBoxLayout(group_cam);
    layout_cam->setSpacing(10);
    layout_cam->setContentsMargins(10, 18, 10, 10);

    // Sol: BÜYÜK stats
    cam_stats_layout = new QVBoxLayout();
    cam_stats_layout->setSpacing(10);
    lbl_cam_fps = new QLabel("FPS: 0");
    lbl_cam_bitrate = new QLabel("Veri Hızı: 0 KB/s");
    lbl_cam_res = new QLabel("Çözünürlük: ---");
    lbl_cam_ping = new QLabel("Ping: --- ms");
    lbl_cam_fps->setObjectName("cam_fps");
    lbl_cam_bitrate->setObjectName("cam_bitrate");
    lbl_cam_res->setObjectName("cam_res");
    lbl_cam_ping->setObjectName("cam_ping");
    cam_stats_layout->addWidget(lbl_cam_fps);
    cam_stats_layout->addWidget(lbl_cam_bitrate);
    cam_stats_layout->addWidget(lbl_cam_res);
    cam_stats_layout->addWidget(lbl_cam_ping);
    cam_stats_layout->addStretch();
    layout_cam->addLayout(cam_stats_layout);

    // Orta: video
    lbl_cam_stream = new QLabel();
    lbl_cam_stream->setMinimumSize(380, 260);
    lbl_cam_stream->setAlignment(Qt::AlignCenter);
    lbl_cam_stream->setObjectName("cam_stream");
    lbl_cam_stream->setText("Kamera Bekleniyor...");
    layout_cam->addWidget(lbl_cam_stream, 1);

    // Sağ: BÜYÜK yön butonları + motor diyagramı + ARM/DISARM
    dir_widget = new QWidget();
    QVBoxLayout *dirRoot = new QVBoxLayout(dir_widget);
    dirRoot->setContentsMargins(0, 0, 0, 0);
    dirRoot->setSpacing(8);

    QHBoxLayout *dirRow = new QHBoxLayout();
    dirRow->setSpacing(12);
    dirRow->setContentsMargins(0, 0, 0, 0);

    btn_turn_left  = new QPushButton("Sola\nDön");
    btn_turn_right = new QPushButton("Sağa\nDön");
    btn_forward    = new QPushButton("İleri");
    btn_backward   = new QPushButton("Geri");
    btn_left       = new QPushButton("Sol");
    btn_right      = new QPushButton("Sağ");
    btn_up         = new QPushButton("Z+");
    btn_down       = new QPushButton("Z-");

    for (QPushButton *b : {btn_turn_left, btn_turn_right, btn_forward, btn_backward,
                            btn_left, btn_right, btn_up, btn_down}) {
        b->setObjectName("dir_btn");
        b->setFixedSize(70, 48); // Perfect uniform size so they align beautifully
    }

    // Left Column (Turn) - vertically centered
    QVBoxLayout *turnCol = new QVBoxLayout();
    turnCol->setSpacing(6);
    turnCol->setContentsMargins(0, 0, 0, 0);
    turnCol->addStretch();
    turnCol->addWidget(btn_turn_left);
    turnCol->addWidget(btn_turn_right);
    turnCol->addStretch();
    dirRow->addLayout(turnCol);

    // Middle D-Pad
    layout_directions = new QGridLayout();
    layout_directions->setSpacing(6);
    layout_directions->setContentsMargins(0, 0, 0, 0);
    layout_directions->addWidget(btn_forward,    0, 1, Qt::AlignCenter);
    layout_directions->addWidget(btn_left,       1, 0, Qt::AlignCenter);
    layout_directions->addWidget(btn_right,      1, 2, Qt::AlignCenter);
    layout_directions->addWidget(btn_backward,   2, 1, Qt::AlignCenter);
    dirRow->addLayout(layout_directions);

    // Right Column (Depth) - vertically centered
    QVBoxLayout *depthCol = new QVBoxLayout();
    depthCol->setSpacing(6);
    depthCol->setContentsMargins(0, 0, 0, 0);
    depthCol->addStretch();
    depthCol->addWidget(btn_up);
    depthCol->addWidget(btn_down);
    depthCol->addStretch();
    dirRow->addLayout(depthCol);

    dirRoot->addLayout(dirRow);

    // Motorlarin canli durumunu gosteren diyagram (bkz. foto/ dizinindeki motor semasi)
    motor_diagram = new MotorDiagramWidget();
    dirRoot->addWidget(motor_diagram, 0, Qt::AlignCenter);

    btn_stabilize = new QPushButton("ARM Et");
    btn_stabilize->setObjectName("btn_stabilize");
    btn_stabilize->setMinimumHeight(50);  // büyütüldü
    dirRoot->addWidget(btn_stabilize);
    dirRoot->addStretch();

    layout_cam->addWidget(dir_widget);
    main_layout->addWidget(group_cam);

    // ====== Hızlı Komutlar ======
    group_commands = new QGroupBox("Hızlı Komutlar");
    layout_commands = new QHBoxLayout(group_commands);
    layout_commands->setSpacing(8);
    layout_commands->setContentsMargins(10, 18, 10, 10);

    btn_emergency = new QPushButton("ACİL\nDURDURMA");
    btn_emergency->setObjectName("btn_emergency");
    btn_emergency->setMinimumSize(130, 78);
    layout_commands->addWidget(btn_emergency);

    auto makeLed = [](bool isGreen, bool isActive) {
        QLabel *l = new QLabel();
        l->setFixedSize(14, 14);
        if (isGreen) {
            l->setStyleSheet(isActive ? "background-color: #2ecc71; border: 2px solid #27ae60; border-radius: 7px;"
                                      : "background-color: #152e1a; border: 2px solid #0d1f11; border-radius: 7px;");
        } else {
            l->setStyleSheet(isActive ? "background-color: #e74c3c; border: 2px solid #c0392b; border-radius: 7px;"
                                      : "background-color: #2e1515; border: 2px solid #1f0d0d; border-radius: 7px;");
        }
        return l;
    };

    auto addCmd = [&](QPushButton *&btn, const QString &text, QLabel *&ledOut) {
        QVBoxLayout *v = new QVBoxLayout();
        v->setSpacing(6);
        QHBoxLayout *ledRow = new QHBoxLayout();
        ledRow->setSpacing(6);
        ledRow->addStretch();
        QLabel *redLed = makeLed(false, true); // Active Red by default
        QLabel *grnLed = makeLed(true, false); // Inactive Green by default
        ledRow->addWidget(redLed);
        ledRow->addWidget(grnLed);
        ledRow->addStretch();
        v->addLayout(ledRow);

        btn = new QPushButton(text);
        btn->setObjectName("cmd_btn");
        btn->setMinimumSize(110, 42);
        v->addWidget(btn);

        grnLed->setProperty("redPartner", QVariant::fromValue<void*>(redLed));
        grnLed->setProperty("isGreen", true);

        ledOut = grnLed;
        layout_commands->addLayout(v);
    };

    if (!isMini) {
        addCmd(btn_autonomous, "Otonom", led_autonomous);
        addCmd(btn_manual,     "Manuel", led_manual);
        addCmd(btn_minirov_launch, "MiniROV\nBırak", led_minirov);
        addCmd(btn_torpedo,        "Torpido\nFırlat", led_torpedo);
    } else {
        btn_autonomous = nullptr;
        btn_manual = nullptr;
        btn_minirov_launch = nullptr;
        btn_torpedo = nullptr;
        led_autonomous = nullptr;
        led_manual = nullptr;
        led_minirov = nullptr;
        led_torpedo = nullptr;
    }

    // Symmetrical Right Side Column for Renk + Lamba
    QVBoxLayout *lampColorCol = new QVBoxLayout();
    lampColorCol->setSpacing(8);

    // Renk Yatay Satırı
    QHBoxLayout *colorRow = new QHBoxLayout();
    colorRow->setSpacing(8);
    lbl_color_title = new QLabel("Renk");
    lbl_color_title->setObjectName("section_label");
    color_btn = new ColorPickerButton();
    color_btn->setMinimumSize(100, 36);
    color_btn->setMaximumWidth(120);
    colorRow->addWidget(lbl_color_title);
    colorRow->addWidget(color_btn);
    lampColorCol->addLayout(colorRow);

    // Lamba Bölümü
    QGroupBox *lampGroup = new QGroupBox("Lamba");
    lampGroup->setObjectName("sub_group");
    QVBoxLayout *lampLayout = new QVBoxLayout(lampGroup);
    lampLayout->setSpacing(6);
    lampLayout->setContentsMargins(6, 12, 6, 6);

    QHBoxLayout *lampBody = new QHBoxLayout();
    lampBody->setSpacing(8);

    // Lamba Aç Sütunu
    QVBoxLayout *lampOnCol = new QVBoxLayout();
    lampOnCol->setSpacing(4);
    QHBoxLayout *ledOnRow = new QHBoxLayout();
    ledOnRow->addStretch();
    led_lamp_on = makeLed(true, false); // Green LED, initially inactive
    led_lamp_on->setProperty("isGreen", true);
    ledOnRow->addWidget(led_lamp_on);
    ledOnRow->addStretch();
    lampOnCol->addLayout(ledOnRow);
    btn_lamp_on = new QPushButton("Aç");
    btn_lamp_on->setObjectName("lamp_on");
    btn_lamp_on->setMinimumSize(60, 38);
    lampOnCol->addWidget(btn_lamp_on);

    // Lamba Kapa Sütunu
    QVBoxLayout *lampOffCol = new QVBoxLayout();
    lampOffCol->setSpacing(4);
    QHBoxLayout *ledOffRow = new QHBoxLayout();
    ledOffRow->addStretch();
    led_lamp_off = makeLed(false, true); // Red LED, initially active (Closed)
    led_lamp_off->setProperty("isGreen", false);
    ledOffRow->addWidget(led_lamp_off);
    ledOffRow->addStretch();
    lampOffCol->addLayout(ledOffRow);
    btn_lamp_off = new QPushButton("Kapa");
    btn_lamp_off->setObjectName("lamp_off");
    btn_lamp_off->setMinimumSize(60, 38);
    lampOffCol->addWidget(btn_lamp_off);

    lampBody->addLayout(lampOnCol);
    lampBody->addLayout(lampOffCol);
    lampLayout->addLayout(lampBody);

    lampColorCol->addWidget(lampGroup);
    layout_commands->addLayout(lampColorCol);

    layout_commands->addStretch();
    main_layout->addWidget(group_commands);

    // ====== Kumanda Durumu ======
    group_pad = new QGroupBox("Kumanda Durumu");
    layout_pad = new QVBoxLayout(group_pad);
    layout_pad->setSpacing(6);
    layout_pad->setContentsMargins(10, 18, 10, 10);

    QHBoxLayout *joyRow = new QHBoxLayout();
    joy_combo = new QComboBox();
    joy_combo->setMinimumHeight(28);
    QDir devInput("/dev/input");
    QFileInfoList js_list = devInput.entryInfoList(QStringList() << "js*", QDir::System);
    for (const QFileInfo &fi : js_list) joy_combo->addItem(fi.absoluteFilePath());
    if (joy_combo->count() == 0) {
        joy_combo->addItem("/dev/input/js0");
        joy_combo->addItem("/dev/input/js1");
    }
    btn_joy_connect = new QPushButton("Kol Bağla");
    btn_joy_connect->setMinimumHeight(30);
    joyRow->addWidget(joy_combo, 1);
    joyRow->addWidget(btn_joy_connect);
    layout_pad->addLayout(joyRow);

    lbl_pad_status = new QLabel("Durum: Kol Bağlı Değil");
    lbl_pad_status->setObjectName("pad_status");
    lbl_pad_axes = new QLabel("Sol Stick: X:0.00 Y:0.00  Sağ Stick: X:0.00 Y:0.00");
    lbl_pad_axes->setObjectName("pad_axes");
    lbl_pad_buttons = new QLabel("Basılan Tuş: Yok");
    lbl_pad_buttons->setObjectName("pad_buttons");
    layout_pad->addWidget(lbl_pad_status);
    layout_pad->addWidget(lbl_pad_axes);
    layout_pad->addWidget(lbl_pad_buttons);

    main_layout->addWidget(group_pad);

    // ====== Terminal ======
    terminal_log = new QTextEdit();
    terminal_log->setReadOnly(true);
    terminal_log->setObjectName("terminal_log");
    terminal_log->setMinimumHeight(110);
    terminal_log->setMaximumHeight(140);
    main_layout->addWidget(terminal_log);

    QVBoxLayout *rootL = new QVBoxLayout(this);
    rootL->setContentsMargins(0, 0, 0, 0);
    rootL->addWidget(main_group);
}
