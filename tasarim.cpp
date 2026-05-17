#include "tasarim.h"
#include <QPainterPath>
#include <QFont>
#include <QMainWindow>

static QLabel* create_data_label() {
    QLabel *lbl = new QLabel("---");
    lbl->setFont(QFont("Arial", 16, QFont::Bold));
    lbl->setStyleSheet("color: #2c3e50;");
    lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return lbl;
}

AttitudeIndicator::AttitudeIndicator(QWidget *parent) : QWidget(parent), roll(0.0), pitch(0.0) {
    setMinimumSize(250, 250);
}

void AttitudeIndicator::set_attitude(float r, float p) {
    roll = r;
    pitch = p;
    update();
}

void AttitudeIndicator::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int side = std::min(w, h);

    painter.translate(w / 2, h / 2);
    painter.scale(side / 250.0, side / 250.0);

    // Draw outer circle
    painter.setPen(QPen(QColor(30, 30, 30), 4));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(-120, -120, 240, 240);

    QPainterPath path;
    path.addEllipse(-118, -118, 236, 236);
    painter.setClipPath(path);

    painter.save();
    painter.rotate(-roll);
    
    float pitch_pixels = pitch * 3.0;
    painter.translate(0, pitch_pixels);

    // Sky
    painter.setBrush(QColor(74, 144, 226));
    painter.setPen(Qt::NoPen);
    painter.drawRect(-250, -500, 500, 500);

    // Ground
    painter.setBrush(QColor(139, 69, 19));
    painter.setPen(Qt::NoPen);
    painter.drawRect(-250, 0, 500, 500);

    painter.setPen(QPen(Qt::white, 2));
    painter.drawLine(-250, 0, 250, 0);

    QFont font("Arial", 10);
    painter.setFont(font);

    for (int p = -90; p <= 90; p += 10) {
        if (p == 0) continue;
        float y = -p * 3.0;
        int w_line = (p % 20 == 0) ? 40 : 20;
        painter.drawLine(-w_line, static_cast<int>(y), w_line, static_cast<int>(y));
        
        QString p_str = QString::number(std::abs(p));
        if (p > 0) {
            painter.drawText(-w_line - 25, static_cast<int>(y) + 5, p_str);
            painter.drawText(w_line + 5, static_cast<int>(y) + 5, p_str);
        } else {
            painter.drawText(-w_line - 30, static_cast<int>(y) + 5, p_str);
            painter.drawText(w_line + 5, static_cast<int>(y) + 5, p_str);
        }
    }
    painter.restore();

    // Aircraft Symbol
    painter.setPen(QPen(QColor(255, 140, 0), 3));
    painter.drawLine(-60, 0, -20, 0);
    painter.drawLine(-20, 0, -20, 10);
    painter.drawLine(20, 0, 60, 0);
    painter.drawLine(20, 0, 20, 10);

    QPolygonF triangle;
    triangle << QPointF(0, -10) << QPointF(-10, 5) << QPointF(10, 5);
    painter.setBrush(QColor(255, 140, 0));
    painter.drawPolygon(triangle);

    // Roll pointer
    painter.setPen(QPen(Qt::yellow, 2));
    painter.setBrush(Qt::yellow);
    QPolygonF roll_pointer;
    roll_pointer << QPointF(0, -118) << QPointF(-8, -105) << QPointF(8, -105);
    painter.drawPolygon(roll_pointer);
}

PixhawkPanel::PixhawkPanel(const QString &title, QWidget *parent) : QWidget(parent) {
    main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);

    main_group = new QGroupBox(title);
    QVBoxLayout *group_layout = new QVBoxLayout(main_group);

    // Connection Bar
    conn_layout = new QHBoxLayout();
    port_combo = new QComboBox();
    port_combo->addItems({"/dev/ttyACM0", "/dev/ttyACM1", "/dev/ttyUSB0", "/dev/ttyUSB1"});
    baud_combo = new QComboBox();
    baud_combo->addItems({"115200", "57600", "921600"});
    btn_connect = new QPushButton("Bağlan");
    lbl_status = new QLabel("Durum: Bekleniyor...");
    lbl_status->setStyleSheet("color: gray; font-weight: bold;");

    conn_layout->addWidget(new QLabel("Port:"));
    conn_layout->addWidget(port_combo);
    conn_layout->addWidget(new QLabel("Baudrate:"));
    conn_layout->addWidget(baud_combo);
    conn_layout->addWidget(btn_connect);
    conn_layout->addWidget(lbl_status);
    conn_layout->addStretch();

    // Telemetry
    data_layout = new QGridLayout();

    // Attitude
    group_att = new QGroupBox("Yönelim (Attitude)");
    layout_att = new QHBoxLayout();
    text_att_layout = new QGridLayout();
    lbl_roll = create_data_label();
    lbl_pitch = create_data_label();
    lbl_yaw = create_data_label();

    text_att_layout->addWidget(new QLabel("Roll:"), 0, 0);
    text_att_layout->addWidget(lbl_roll, 0, 1);
    text_att_layout->addWidget(new QLabel("Pitch:"), 1, 0);
    text_att_layout->addWidget(lbl_pitch, 1, 1);
    text_att_layout->addWidget(new QLabel("Yaw:"), 2, 0);
    text_att_layout->addWidget(lbl_yaw, 2, 1);

    attitude_indicator = new AttitudeIndicator();
    attitude_indicator->setFixedSize(200, 200);
    
    // İrtifa (Altitude) Barı Yapay Ufuğun yanına dikey olarak eklendi
    bar_roll = new QProgressBar();
    bar_roll->setRange(-90, 90);
    bar_roll->setOrientation(Qt::Vertical);
    bar_roll->setFormat("%v°");
    bar_roll->setTextVisible(true);
    bar_roll->setFixedSize(30, 180);
    bar_roll->setStyleSheet(
        "QProgressBar { border: 1px solid #bdc3c7; border-radius: 4px; text-align: center; background-color: #ecf0f1; font-weight: bold; font-size: 11px; }"
        "QProgressBar::chunk { background-color: #3498db; }"
    );

    layout_att->addLayout(text_att_layout);
    layout_att->addWidget(attitude_indicator, 1, Qt::AlignCenter);
    layout_att->addWidget(bar_roll, 0, Qt::AlignLeft | Qt::AlignVCenter);
    group_att->setLayout(layout_att);

    // VFR HUD
    group_vfr = new QGroupBox("Seyrüsefer (VFR HUD)");
    layout_vfr = new QGridLayout();
    lbl_alt = create_data_label(); // Segfault engellendi (null pointer düzeltildi)
    lbl_heading = create_data_label();
    lbl_speed = create_data_label();

    layout_vfr->addWidget(new QLabel("İrtifa (m):"), 0, 0);
    layout_vfr->addWidget(lbl_alt, 0, 1);
    layout_vfr->addWidget(new QLabel("Pusula Yönü:"), 1, 0);
    layout_vfr->addWidget(lbl_heading, 1, 1);
    layout_vfr->addWidget(new QLabel("Hız (m/s):"), 2, 0);
    layout_vfr->addWidget(lbl_speed, 2, 1);
    group_vfr->setLayout(layout_vfr);

    // System Status
    group_sys = new QGroupBox("Sistem & Sensör");
    layout_sys = new QGridLayout();
    lbl_voltage = create_data_label();
    lbl_battery = create_data_label();
    lbl_servo_status = create_data_label();
    lbl_servo_status->setText("Kapalı");

    layout_sys->addWidget(new QLabel("Batarya Voltaj:"), 0, 0);
    layout_sys->addWidget(lbl_voltage, 0, 1);
    layout_sys->addWidget(new QLabel("Batarya %:"), 1, 0);
    layout_sys->addWidget(lbl_battery, 1, 1);
    layout_sys->addWidget(new QLabel("AUX 1 Servo Durumu:"), 2, 0);
    layout_sys->addWidget(lbl_servo_status, 2, 1);
    group_sys->setLayout(layout_sys);

    data_layout->addWidget(group_att, 0, 0, 2, 1);
    data_layout->addWidget(group_vfr, 0, 1);
    data_layout->addWidget(group_sys, 1, 1);

    // ==========================================
    // KAMERA PANELİ (PixhawkPanel içine eklendi)
    // ==========================================
    group_cam = new QGroupBox("Kamera Akışı");
    layout_cam = new QHBoxLayout();
    cam_stats_layout = new QVBoxLayout();
    
    lbl_cam_fps = new QLabel("FPS: --");
    lbl_cam_fps->setFont(QFont("Arial", 12, QFont::Bold));
    lbl_cam_fps->setStyleSheet("color: #2980b9;");
    lbl_cam_bitrate = new QLabel("Veri Hızı: -- KB/s");
    lbl_cam_bitrate->setFont(QFont("Arial", 12, QFont::Bold));
    lbl_cam_bitrate->setStyleSheet("color: #27ae60;");
    lbl_cam_res = new QLabel("Çözünürlük: --x--");
    lbl_cam_res->setFont(QFont("Arial", 10));
    lbl_cam_res->setStyleSheet("color: #7f8c8d;");
    lbl_cam_ping = new QLabel("Ping: -- ms");
    lbl_cam_ping->setFont(QFont("Arial", 10, QFont::Bold));
    lbl_cam_ping->setStyleSheet("color: #d35400;");

    cam_stats_layout->addWidget(lbl_cam_fps);
    cam_stats_layout->addWidget(lbl_cam_bitrate);
    cam_stats_layout->addWidget(lbl_cam_res);
    cam_stats_layout->addWidget(lbl_cam_ping);
    cam_stats_layout->addStretch();
    layout_cam->addLayout(cam_stats_layout, 1);

    lbl_cam_stream = new QLabel("Kamera Bekleniyor...");
    lbl_cam_stream->setAlignment(Qt::AlignCenter);
    lbl_cam_stream->setStyleSheet("background-color: #1a1a1a; color: #7f8c8d; border-radius: 5px;"); 
    lbl_cam_stream->setMinimumSize(320, 240);
    layout_cam->addWidget(lbl_cam_stream, 4);
    group_cam->setLayout(layout_cam);

    // ==========================================
    // HIZLI KOMUTLAR PANELİ (PixhawkPanel içine eklendi)
    // ==========================================
    group_commands = new QGroupBox("Hızlı Komutlar");
    layout_commands = new QHBoxLayout();
    
    QVBoxLayout *action_layout = new QVBoxLayout();
    btn_emergency = new QPushButton("ACİL DURDURMA");
    btn_emergency->setStyleSheet("QPushButton { background-color: #c0392b; color: white; font-weight: bold; font-size: 14px; padding: 10px; border-radius: 5px; } "
                                 "QPushButton:pressed { background-color: #922b21; }");
    
    btn_start = new QPushButton("Start");
    btn_start->setStyleSheet("QPushButton { background-color: #27ae60; color: white; font-weight: bold; padding: 10px; border-radius: 5px; } "
                             "QPushButton:pressed { background-color: #1e8449; }");
                             
    QString btn_style = "QPushButton { background-color: #2980b9; color: white; font-weight: bold; padding: 10px; border-radius: 5px; } "
                        "QPushButton:pressed { background-color: #1a5276; }";
                        
    QHBoxLayout *other_actions = new QHBoxLayout();
    btn_stabilize = new QPushButton("Sabitle");
    btn_stabilize->setStyleSheet(btn_style);
    other_actions->addWidget(btn_stabilize);
    other_actions->addWidget(btn_start);

    btn_torpedo = new QPushButton("Torpido Fırlat");
    btn_minirov_launch = new QPushButton("MiniROV Fırlat");
    btn_torpedo->setStyleSheet(btn_style);
    btn_minirov_launch->setStyleSheet(btn_style);

    QHBoxLayout *ana_actions = new QHBoxLayout();
    ana_actions->addWidget(btn_torpedo);
    ana_actions->addWidget(btn_minirov_launch);
    
    if (!title.contains("ANA ROV")) {
        btn_torpedo->hide();
        btn_minirov_launch->hide();
    }

    action_layout->addWidget(btn_emergency);
    action_layout->addLayout(other_actions);
    if (title.contains("ANA ROV")) {
        action_layout->addLayout(ana_actions);
    }
    
    layout_directions = new QGridLayout();
    btn_up = new QPushButton("Z+");
    btn_down = new QPushButton("Z-");
    btn_left = new QPushButton("Sol");
    btn_right = new QPushButton("Sağ");
    btn_forward = new QPushButton("İleri");
    btn_backward = new QPushButton("Geri");
    
    QString dir_style = "QPushButton { background-color: #34495e; color: white; font-weight: bold; padding: 10px; border-radius: 5px; } "
                        "QPushButton:pressed { background-color: #2c3e50; }";
    btn_up->setStyleSheet(dir_style);
    btn_down->setStyleSheet(dir_style);
    btn_left->setStyleSheet(dir_style);
    btn_right->setStyleSheet(dir_style);
    btn_forward->setStyleSheet(dir_style);
    btn_backward->setStyleSheet(dir_style);
    
    layout_directions->addWidget(btn_forward, 0, 1);
    layout_directions->addWidget(btn_left, 1, 0);
    layout_directions->addWidget(btn_backward, 1, 1);
    layout_directions->addWidget(btn_right, 1, 2);
    layout_directions->addWidget(btn_up, 0, 3);
    layout_directions->addWidget(btn_down, 1, 3);
    
    layout_commands->addLayout(action_layout, 1);
    layout_commands->addStretch(1);
    layout_commands->addLayout(layout_directions, 1);
    group_commands->setLayout(layout_commands);

    // ==========================================
    // KUMANDA (GAMEPAD) PANELİ
    // ==========================================
    group_pad = new QGroupBox(title + " Kumanda Durumu");
    layout_pad = new QVBoxLayout();
    
    QHBoxLayout *joy_conn_layout = new QHBoxLayout();
    joy_combo = new QComboBox();
    joy_combo->addItems({"/dev/input/js0", "/dev/input/js1", "/dev/input/js2", "/dev/input/js3"});
    joy_combo->setStyleSheet("QComboBox { border: 1px solid #bdc3c7; border-radius: 4px; padding: 4px; }");
    
    btn_joy_connect = new QPushButton("Bağlan");
    btn_joy_connect->setStyleSheet(
        "QPushButton { background-color: #2980b9; color: white; border-radius: 4px; padding: 4px 15px; font-weight: bold; }"
        "QPushButton:hover { background-color: #3498db; }"
    );
    
    joy_conn_layout->addWidget(joy_combo);
    joy_conn_layout->addWidget(btn_joy_connect);

    lbl_pad_status = new QLabel("Durum: Bekleniyor...");
    lbl_pad_status->setStyleSheet("color: gray;");
    lbl_pad_axes = new QLabel("Sol Stick: X: 0.00  Y: 0.00");
    lbl_pad_buttons = new QLabel("Basılan Tuş: Yok");
    lbl_pad_buttons->setFont(QFont("Arial", 11, QFont::Bold));
    lbl_pad_buttons->setStyleSheet("color: #e67e22;");
    lbl_pad_buttons->setWordWrap(true);
    lbl_pad_buttons->setMinimumHeight(40); // Butonlara basıldığında boyutu oynatmasını engelle
    
    layout_pad->addLayout(joy_conn_layout);
    layout_pad->addWidget(lbl_pad_status);
    layout_pad->addWidget(lbl_pad_axes);
    layout_pad->addWidget(lbl_pad_buttons);
    group_pad->setLayout(layout_pad);
    group_pad->setMinimumHeight(160); // Kumanda grubunun genel yüksekliğini sabitle

    // ==========================================
    // LOG PANELİ (PixhawkPanel içine eklendi)
    // ==========================================
    terminal_log = new QTextEdit();
    terminal_log->setReadOnly(true);
    terminal_log->setStyleSheet("background-color: black; color: #00ff00; font-family: monospace; font-size: 11px;");
    terminal_log->setMinimumHeight(150);

    group_layout->addLayout(conn_layout);
    group_layout->addLayout(data_layout);
    group_layout->addWidget(group_cam);
    group_layout->addWidget(group_commands);
    group_layout->addWidget(group_pad);
    group_layout->addWidget(terminal_log);
    group_layout->addStretch();
    main_layout->addWidget(main_group);
}

void Ui_MainWindow::setupUi(QWidget *MainWindow) {
    MainWindow->setWindowTitle("SuGaca Kontrol Merkezi");
    MainWindow->setMinimumSize(1200, 800);

    main_widget = new QWidget(MainWindow);
    main_layout = new QVBoxLayout(main_widget);
    
    QMainWindow *qMainWindow = qobject_cast<QMainWindow*>(MainWindow);
    if (qMainWindow) {
        qMainWindow->setCentralWidget(main_widget);
    }

    QLabel *lbl_title = new QLabel("SuGaca");
    lbl_title->setAlignment(Qt::AlignCenter);
    lbl_title->setFont(QFont("Arial", 24, QFont::Bold));
    lbl_title->setStyleSheet("color: #2980b9; margin-bottom: 10px;");
    main_layout->addWidget(lbl_title);

    panels_layout = new QHBoxLayout();
    anaRovPanel = new PixhawkPanel("ANA ROV");
    miniRovPanel = new PixhawkPanel("MİNİ ROV");

    // 2 Sütunlu Ana Yerleşim (Sol: Ana ROV, Sağ: Mini ROV)
    panels_layout->addWidget(anaRovPanel, 1);
    panels_layout->addWidget(miniRovPanel, 1);
    
    main_layout->addLayout(panels_layout);

    apply_styles(MainWindow);
}

void Ui_MainWindow::apply_styles(QWidget *MainWindow) {
    MainWindow->setStyleSheet(
        "QGroupBox {"
        "    font-weight: bold;"
        "    border: 1px solid #bdc3c7;"
        "    border-radius: 5px;"
        "    margin-top: 10px;"
        "    padding-top: 15px;"
        "}"
        "QGroupBox::title {"
        "    subcontrol-origin: margin;"
        "    left: 10px;"
        "    padding: 0 3px 0 3px;"
        "    color: #2980b9;"
        "}"
    );
}
