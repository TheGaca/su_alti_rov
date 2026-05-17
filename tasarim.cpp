#include "tasarim.h"
#include <QPaintEvent>
#include <QPainterPath>
#include <QFont>
#include <QSerialPortInfo>
#include <QDir>
#include <QFileInfoList>
#include <QColorDialog>
#include <cmath>

// ==================== AttitudeIndicator (BÜYÜTÜLDÜ) ====================
AttitudeIndicator::AttitudeIndicator(QWidget *parent)
    : QWidget(parent), roll(0.0f), pitch(0.0f) {
    setMinimumSize(260, 260);   // 180 → 260
    setMaximumSize(280, 280);
}

void AttitudeIndicator::set_attitude(float r, float p) {
    roll = r;
    pitch = p;
    update();
}

void AttitudeIndicator::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int side = qMin(width(), height());
    QPointF center(width() / 2.0, height() / 2.0);
    qreal radius = side / 2.0 - 6;

    QPainterPath clipPath;
    clipPath.addEllipse(center, radius, radius);
    painter.setClipPath(clipPath);

    painter.save();
    painter.translate(center);
    painter.rotate(-roll);

    qreal pitchOffset = pitch * (radius / 45.0);

    // Gökyüzü
    QRectF skyRect(-radius * 2, -radius * 2 + pitchOffset, radius * 4, radius * 2);
    painter.fillRect(skyRect, QColor(94, 168, 218));
    // Zemin
    QRectF groundRect(-radius * 2, pitchOffset, radius * 4, radius * 2);
    painter.fillRect(groundRect, QColor(133, 80, 40));
    // Ufuk çizgisi
    painter.setPen(QPen(Qt::white, 2.5));
    painter.drawLine(QPointF(-radius * 2, pitchOffset), QPointF(radius * 2, pitchOffset));

    // Pitch işaretleri
    painter.setPen(QPen(Qt::white, 2));
    QFont f("Sans", 10, QFont::Bold);
    painter.setFont(f);
    for (int p = -30; p <= 30; p += 10) {
        if (p == 0) continue;
        qreal y = pitchOffset - p * (radius / 45.0);
        qreal lineLen = (p % 20 == 0) ? radius * 0.50 : radius * 0.34;
        painter.drawLine(QPointF(-lineLen / 2, y), QPointF(lineLen / 2, y));
        QString txt = QString::number(std::abs(p));
        painter.drawText(QPointF(-lineLen / 2 - 28, y + 5), txt);
        painter.drawText(QPointF(lineLen / 2 + 8, y + 5), txt);
    }

    painter.restore();

    // Sarı üst üçgen
    painter.setClipping(false);
    painter.setBrush(QColor(255, 215, 0));
    painter.setPen(Qt::NoPen);
    QPolygonF topTri;
    topTri << QPointF(center.x(), center.y() - radius + 6)
           << QPointF(center.x() - 13, center.y() - radius + 24)
           << QPointF(center.x() + 13, center.y() - radius + 24);
    painter.drawPolygon(topTri);

    // Turuncu uçak (daha büyük)
    painter.setBrush(QColor(255, 140, 0));
    painter.setPen(QPen(Qt::black, 1.5));
    QPolygonF plane;
    plane << QPointF(center.x() - 30, center.y())
          << QPointF(center.x() - 8, center.y())
          << QPointF(center.x(), center.y() + 10)
          << QPointF(center.x() + 8, center.y())
          << QPointF(center.x() + 30, center.y())
          << QPointF(center.x() + 30, center.y() + 5)
          << QPointF(center.x() + 8, center.y() + 5)
          << QPointF(center.x() + 8, center.y() + 16)
          << QPointF(center.x() - 8, center.y() + 16)
          << QPointF(center.x() - 8, center.y() + 5)
          << QPointF(center.x() - 30, center.y() + 5);
    painter.drawPolygon(plane);

    // Dış çerçeve
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(40, 40, 40), 3));
    painter.drawEllipse(center, radius, radius);
}

// ==================== ColorPickerButton ====================
ColorPickerButton::ColorPickerButton(QWidget *parent)
    : QPushButton(parent), m_color(QColor("#9C27B0")) {
    setMinimumHeight(34);
    setText("Renk Seç");
    setCursor(Qt::PointingHandCursor);
    refreshStyle();
    connect(this, &QPushButton::clicked, this, &ColorPickerButton::on_clicked);
}

void ColorPickerButton::setColor(const QColor &c) {
    m_color = c;
    refreshStyle();
}

void ColorPickerButton::on_clicked() {
    QColor c = QColorDialog::getColor(m_color, this, "Renk Seç");
    if (c.isValid()) {
        m_color = c;
        refreshStyle();
        emit colorChosen(m_color);
    }
}

void ColorPickerButton::refreshStyle() {
    // Renk koyuysa yazı beyaz, açıksa siyah
    int luma = (m_color.red() * 299 + m_color.green() * 587 + m_color.blue() * 114) / 1000;
    QString txtColor = (luma < 128) ? "#ffffff" : "#000000";
    setStyleSheet(QString(
        "QPushButton {"
        "  background-color: %1;"
        "  color: %2;"
        "  border: 2px solid #333;"
        "  border-radius: 4px;"
        "  font-weight: bold;"
        "  padding: 4px 12px;"
        "}"
        "QPushButton:hover { border: 2px solid #000; }"
    ).arg(m_color.name(), txtColor));
    setText(QString("Renk: %1").arg(m_color.name().toUpper()));
}

// ==================== PixhawkPanel ====================
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
        port_combo->addItem(info.systemLocation());
    }
    if (port_combo->count() == 0) port_combo->addItem("/dev/ttyACM0");

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
    if (!isMini) {
        conn_layout->addWidget(btn_connect);
    }
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
    QLabel *ls3 = new QLabel("AUX 1 Servo:");
    QLabel *ls4 = new QLabel("Lamba Durumu:");
    lbl_voltage = new QLabel("---");
    lbl_battery = new QLabel("---");
    lbl_servo_status = new QLabel("Kapalı");
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
    rightCol->addWidget(group_sys);

    topRow->addLayout(rightCol, 2);
    main_layout->addLayout(topRow);

    // ====== Kamera Akışı (sol stats büyük + video + sağ yön butonları büyük + Sabitle) ======
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

    // Sağ: BÜYÜK yön butonları + Sabitle
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

    btn_stabilize = new QPushButton("Sabitle");
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
        // Create the connect button in the Quick Commands row for Mini ROV
        QVBoxLayout *v = new QVBoxLayout();
        v->setSpacing(6);
        QHBoxLayout *ledRow = new QHBoxLayout();
        ledRow->setSpacing(6);
        ledRow->addStretch();
        QLabel *redLed = makeLed(false, true); // Connected starts false (Red LED active)
        QLabel *grnLed = makeLed(true, false);
        ledRow->addWidget(redLed);
        ledRow->addWidget(grnLed);
        ledRow->addStretch();
        v->addLayout(ledRow);
        
        btn_connect->setText("Mini Rov\nBağlantısı");
        btn_connect->setMinimumSize(140, 42);
        v->addWidget(btn_connect);
        
        grnLed->setProperty("redPartner", QVariant::fromValue<void*>(redLed));
        grnLed->setProperty("isGreen", true);
        led_autonomous = grnLed; // We use led_autonomous to store the connection status LED!
        
        layout_commands->addLayout(v);
        
        btn_autonomous = nullptr;
        btn_manual = nullptr;
        btn_minirov_launch = nullptr;
        btn_torpedo = nullptr;
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
    lbl_pad_axes = new QLabel("Sol Stick: X: 0.00  Y: 0.00");
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

// ==================== Ui_MainWindow ====================
void Ui_MainWindow::setupUi(QWidget *MainWindow) {
    MainWindow->setWindowTitle("Su Altı ROV Kontrol Arayüzü");
    main_widget = MainWindow;

    main_layout = new QVBoxLayout(MainWindow);
    main_layout->setContentsMargins(6, 6, 6, 6);
    main_layout->setSpacing(6);

    QHBoxLayout *title_layout = new QHBoxLayout();
    title_layout->setContentsMargins(10, 2, 10, 2);

    title_layout->addStretch(1);

    QLabel *lbl_title = new QLabel("SuGaca");
    lbl_title->setObjectName("app_main_title");
    lbl_title->setAlignment(Qt::AlignCenter);
    lbl_title->setStyleSheet("font-size: 28px; font-weight: bold; color: #ff69b4; padding: 4px; letter-spacing: 3px; font-family: 'Outfit', 'Inter', sans-serif;");
    title_layout->addWidget(lbl_title);

    title_layout->addStretch(1);

    btn_theme = new QPushButton("🌙 Koyu Mod");
    btn_theme->setObjectName("btn_theme");
    btn_theme->setCursor(Qt::PointingHandCursor);
    btn_theme->setStyleSheet("font-size: 13px; font-weight: bold; background-color: #334155; color: white; border: none; border-radius: 4px; padding: 6px 12px;");
    title_layout->addWidget(btn_theme);

    main_layout->addLayout(title_layout);

    panels_layout = new QHBoxLayout();
    panels_layout->setSpacing(8);

    anaRovPanel  = new PixhawkPanel("ANA ROV",  false);
    miniRovPanel = new PixhawkPanel("MİNİ ROV", true);

    panels_layout->addWidget(anaRovPanel,  1);
    panels_layout->addWidget(miniRovPanel, 1);
    main_layout->addLayout(panels_layout, 1);

    apply_styles(MainWindow, false); // start with light theme
}

void Ui_MainWindow::apply_styles(QWidget *MainWindow, bool dark) {
    QString style;
    if (dark) {
        style = R"(
        QWidget {
            background-color: #0b0f19;
            color: #e2e8f0;
            font-family: 'DejaVu Sans', 'Segoe UI', sans-serif;
            font-size: 13px;
        }
        QGroupBox {
            border: 1px solid #1e293b;
            border-radius: 4px;
            margin-top: 12px;
            padding-top: 6px;
            background-color: #111827;
            font-weight: 600;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 6px;
            background-color: #111827;
            color: #94a3b8;
            font-size: 13px;
        }
        QGroupBox#rov_main_group {
            border: 2px solid #334155;
            background-color: #111827;
        }
        QGroupBox#rov_main_group::title {
            font-size: 15px;
            font-weight: bold;
            color: #f1f5f9;
            background-color: #0b0f19;
            padding: 4px 14px;
            border: 2px solid #334155;
            border-bottom: none;
            border-top-left-radius: 3px;
            border-top-right-radius: 3px;
        }
        QLabel { background: transparent; color: #e2e8f0; }
        QLabel#section_label {
            font-weight: 600;
            color: #94a3b8;
        }
        QLabel#att_title {
            font-size: 15px;
            font-weight: 600;
            color: #94a3b8;
        }
        QLabel#attval {
            font-size: 16px;
            font-weight: bold;
            color: #f8fafc;
        }
        QLabel#hud_val {
            font-size: 14px;
            font-weight: bold;
            color: #cbd5e1;
        }
        QLabel#servo_status {
            font-size: 15px;
            font-weight: bold;
            color: #ef4444;
        }
        QLabel#lamp_status {
            font-size: 15px;
            font-weight: bold;
            color: #ef4444;
        }
        QLabel#cam_fps {
            font-size: 18px;
            font-weight: bold;
            color: #22c55e;
        }
        QLabel#cam_bitrate {
            font-size: 16px;
            font-weight: bold;
            color: #22c55e;
        }
        QLabel#cam_res {
            font-size: 14px;
            color: #94a3b8;
        }
        QLabel#cam_ping {
            font-size: 18px;
            font-weight: bold;
            color: #ec4899;
        }
        QLabel#cam_stream {
            background-color: #000000;
            color: #6b7280;
            border: 1px solid #1e293b;
            border-radius: 3px;
        }
        QLabel#pad_status {
            font-size: 14px;
            color: #ef4444;
            font-weight: bold;
        }
        QLabel#pad_axes {
            font-size: 13px;
            color: #94a3b8;
        }
        QLabel#pad_buttons {
            font-size: 13px;
            color: #f97316;
            font-weight: bold;
        }
        QLabel#lbl_status {
            font-weight: bold;
            color: #cbd5e1;
        }

        QPushButton {
            background-color: #1e293b;
            color: #f1f5f9;
            border: 1px solid #334155;
            padding: 4px 10px;
            border-radius: 3px;
        }
        QPushButton:hover  { background-color: #334155; }
        QPushButton:pressed{ background-color: #0f172a; }
        QPushButton#btn_connect {
            background-color: #1e293b;
            color: #f1f5f9;
            border: 1px solid #334155;
            font-weight: bold;
        }
        QPushButton#dir_btn {
            background-color: #1e293b;
            color: #f1f5f9;
            border: 1px solid #334155;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton#dir_btn:pressed {
            background-color: #3b82f6;
            color: white;
        }
        QPushButton#btn_stabilize {
            background-color: #2563eb;
            color: white;
            font-weight: bold;
            border: 1px solid #1d4ed8;
            border-radius: 4px;
            font-size: 16px;
        }
        QPushButton#btn_stabilize:hover  { background-color: #1d4ed8; }
        QPushButton#btn_stabilize:pressed{ background-color: #1e40af; }
        QPushButton#btn_emergency {
            background-color: #dc2626;
            color: white;
            font-weight: bold;
            font-size: 15px;
            border: 2px solid #991b1b;
            border-radius: 4px;
        }
        QPushButton#btn_emergency:pressed { background-color: #991b1b; }
        QPushButton#cmd_btn {
            background-color: #334155;
            color: white;
            border: 1px solid #1e293b;
            font-weight: bold;
            font-size: 13px;
        }
        QPushButton#cmd_btn:hover { background-color: #475569; }
        QPushButton#lamp_on {
            background-color: #f59e0b;
            color: #111111;
            font-weight: bold;
            border: 1px solid #b45309;
        }
        QPushButton#lamp_on:hover { background-color: #fbbf24; }
        QPushButton#lamp_off {
            background-color: #4b5563;
            color: white;
            font-weight: bold;
            border: 1px solid #374151;
        }
        QPushButton#lamp_off:hover { background-color: #6b7280; }

        QLabel#led_on {
            background-color: #22c55e;
            border: 1px solid #14532d;
            border-radius: 7px;
        }
        QLabel#led_off {
            background-color: #ef4444;
            border: 1px solid #7f1d1d;
            border-radius: 7px;
        }

        QComboBox {
            background-color: #1e293b;
            color: #f1f5f9;
            border: 1px solid #334155;
            border-radius: 3px;
            padding: 2px 6px;
        }
        QComboBox::drop-down {
            border-left: 1px solid #334155;
            width: 20px;
        }
        QProgressBar {
            border: 1px solid #334155;
            background-color: #1e293b;
            text-align: center;
        }
        QProgressBar::chunk {
            background-color: #3b82f6;
        }
        QTextEdit#terminal_log {
            background-color: #020617;
            color: #38bdf8;
            font-family: 'DejaVu Sans Mono', 'Courier New', monospace;
            font-size: 12px;
            border: 1px solid #1e293b;
            border-radius: 3px;
        }
        )";
    } else {
        style = R"(
        QWidget {
            background-color: #f2f4f7;
            color: #1f2937;
            font-family: 'DejaVu Sans', 'Segoe UI', sans-serif;
            font-size: 13px;
        }
        QGroupBox {
            border: 1px solid #b0b6be;
            border-radius: 4px;
            margin-top: 12px;
            padding-top: 6px;
            background-color: #ffffff;
            font-weight: 600;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 6px;
            background-color: #ffffff;
            color: #374151;
            font-size: 13px;
        }
        QGroupBox#rov_main_group {
            border: 2px solid #4b5563;
            background-color: #fafbfc;
        }
        QGroupBox#rov_main_group::title {
            font-size: 15px;
            font-weight: bold;
            color: #111827;
            background-color: #ffffff;
            padding: 4px 14px;
            border: 2px solid #4b5563;
            border-bottom: none;
            border-top-left-radius: 3px;
            border-top-right-radius: 3px;
        }
        QLabel { background: transparent; }
        QLabel#section_label {
            font-weight: 600;
            color: #4b5563;
        }
        QLabel#att_title {
            font-size: 15px;
            font-weight: 600;
            color: #374151;
        }
        QLabel#attval {
            font-size: 16px;
            font-weight: bold;
            color: #111827;
        }
        QLabel#hud_val {
            font-size: 14px;
            font-weight: bold;
            color: #111827;
        }
        QLabel#servo_status {
            font-size: 15px;
            font-weight: bold;
            color: #c0392b;
        }
        QLabel#cam_fps {
            font-size: 18px;
            font-weight: bold;
            color: #16a34a;
        }
        QLabel#cam_bitrate {
            font-size: 16px;
            font-weight: bold;
            color: #16a34a;
        }
        QLabel#cam_res {
            font-size: 14px;
            color: #4b5563;
        }
        QLabel#cam_ping {
            font-size: 18px;
            font-weight: bold;
            color: #db2777;
        }
        QLabel#cam_stream {
            background-color: #000;
            color: #6b7280;
            border: 1px solid #374151;
            border-radius: 3px;
        }
        QLabel#pad_status {
            font-size: 14px;
            color: #c0392b;
            font-weight: bold;
        }
        QLabel#pad_axes {
            font-size: 13px;
            color: #1f2937;
        }
        QLabel#pad_buttons {
            font-size: 13px;
            color: #ea580c;
            font-weight: bold;
        }
        QLabel#lbl_status {
            font-weight: bold;
            color: #4b5563;
        }

        QPushButton {
            background-color: #ffffff;
            border: 1px solid #6b7280;
            padding: 4px 10px;
            border-radius: 3px;
        }
        QPushButton:hover  { background-color: #e5e7eb; }
        QPushButton:pressed{ background-color: #d1d5db; }
        QPushButton#btn_connect {
            background-color: #ffffff;
            border: 1px solid #374151;
            font-weight: bold;
        }
        QPushButton#dir_btn {
            background-color: #ffffff;
            border: 1px solid #6b7280;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton#dir_btn:pressed {
            background-color: #1e3a8a;
            color: white;
        }
        QPushButton#btn_stabilize {
            background-color: #2563eb;
            color: white;
            font-weight: bold;
            border: 1px solid #1d4ed8;
            border-radius: 4px;
            font-size: 16px;
        }
        QPushButton#btn_stabilize:hover  { background-color: #1d4ed8; }
        QPushButton#btn_stabilize:pressed{ background-color: #1e40af; }
        QPushButton#btn_emergency {
            background-color: #dc2626;
            color: white;
            font-weight: bold;
            font-size: 15px;
            border: 2px solid #991b1b;
            border-radius: 4px;
        }
        QPushButton#btn_emergency:pressed { background-color: #991b1b; }
        QPushButton#cmd_btn {
            background-color: #334155;
            color: white;
            border: 1px solid #1e293b;
            font-weight: bold;
            font-size: 13px;
        }
        QPushButton#cmd_btn:hover { background-color: #475569; }
        QPushButton#lamp_on {
            background-color: #f59e0b;
            color: #111;
            font-weight: bold;
            border: 1px solid #b45309;
        }
        QPushButton#lamp_on:hover { background-color: #fbbf24; }
        QPushButton#lamp_off {
            background-color: #6b7280;
            color: white;
            font-weight: bold;
            border: 1px solid #374151;
        }
        QPushButton#lamp_off:hover { background-color: #4b5563; }

        QLabel#led_on {
            background-color: #22c55e;
            border: 1px solid #14532d;
            border-radius: 7px;
        }
        QLabel#led_off {
            background-color: #ef4444;
            border: 1px solid #7f1d1d;
            border-radius: 7px;
        }

        QComboBox {
            background-color: white;
            border: 1px solid #6b7280;
            border-radius: 3px;
            padding: 2px 6px;
        }
        QComboBox::drop-down {
            border-left: 1px solid #9ca3af;
            width: 20px;
        }
        QProgressBar {
            border: 1px solid #6b7280;
            background-color: white;
            text-align: center;
        }
        QProgressBar::chunk {
            background-color: #2563eb;
        }
        QTextEdit#terminal_log {
            background-color: #0b1220;
            color: #60a5fa;
            font-family: 'DejaVu Sans Mono', 'Courier New', monospace;
            font-size: 12px;
            border: 1px solid #1f2937;
            border-radius: 3px;
        }
        )";
    }
    MainWindow->setStyleSheet(style);
}